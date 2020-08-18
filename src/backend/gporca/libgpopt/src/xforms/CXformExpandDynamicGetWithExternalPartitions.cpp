//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformExpandDynamicGetWithExternalPartitions.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpopt/xforms/CXformExpandDynamicGetWithExternalPartitions.h"

#include "gpopt/operators/CLogicalDynamicGet.h"
#include "gpopt/operators/CLogicalUnionAll.h"
#include "gpopt/operators/CLogicalMultiExternalGet.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/xforms/CXformUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformExpandDynamicGetWithExternalPartitions::CXformExpandDynamicGetWithExternalPartitions
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformExpandDynamicGetWithExternalPartitions::CXformExpandDynamicGetWithExternalPartitions
	(
	CMemoryPool *mp
	)
	:
	CXformExploration
		(
		 // pattern
		GPOS_NEW(mp) CExpression
				(
				mp,
				GPOS_NEW(mp) CLogicalDynamicGet(mp)
				)
		)
{}


//---------------------------------------------------------------------------
//	@function:
//		CXformExpandDynamicGetWithExternalPartitions::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformExpandDynamicGetWithExternalPartitions::Transform
	(
	CXformContext *pxfctxt,
	CXformResult *pxfres,
	CExpression *pexpr
	)
	const
{
	GPOS_ASSERT(NULL != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMDAccessor *mda = COptCtxt::PoctxtFromTLS()->Pmda();

	CLogicalDynamicGet *popGet = CLogicalDynamicGet::PopConvert(pexpr->Pop());
	CTableDescriptor *ptabdesc = popGet->Ptabdesc();
	const IMDRelation *relation = mda->RetrieveRel(ptabdesc->MDId());

	if (!relation->HasExternalPartitions() || popGet->IsPartial())
	{
		// no external partitions or already a partial dynamic get;
		// do not try to split further
		return;
	}

	CMemoryPool *mp = pxfctxt->Pmp();

	// Iterate over all the External Scans to determine partial scan contraints
	CColRef2dArray *pdrgpdrgpcrPartKeys = popGet->PdrgpdrgpcrPart();
	CColRefArray *pdrgpcrOutput = popGet->PdrgpcrOutput();

	// capture constraints of all external and remaining (non-external) scans in
	// ppartcnstrCovered & ppartcnstrRest respectively
	CPartConstraint *ppartcnstrCovered = NULL;
	CPartConstraint *ppartcnstrRest = NULL;

	IMdIdArray *external_part_mdids = relation->GetExternalPartitions();
	for (ULONG ul = 0; ul < external_part_mdids->Size(); ul++)
	{
		// FIXME: Implement static partition selection here

		IMDId *extpart_mdid = (*external_part_mdids)[ul];
		const IMDRelation *extpart = mda->RetrieveRel(extpart_mdid);

		GPOS_ASSERT(NULL != extpart->MDPartConstraint());
		CPartConstraint *ppartcnstr =
			CUtils::PpartcnstrFromMDPartCnstr(mp, mda, pdrgpdrgpcrPartKeys,
											  extpart->MDPartConstraint(),
											  pdrgpcrOutput);
		GPOS_ASSERT(NULL != ppartcnstr);

		CPartConstraint *ppartcnstrNewlyCovered =
			CXformUtils::PpartcnstrDisjunction(mp, ppartcnstrCovered, ppartcnstr);

		if (NULL == ppartcnstrNewlyCovered)
		{
			// FIXME: Can this happen here?
			ppartcnstr->Release();
			continue;
		}
		CRefCount::SafeRelease(ppartcnstrCovered);
		ppartcnstrCovered = ppartcnstrNewlyCovered;
	}
	CPartConstraint *ppartcnstrRel = CUtils::PpartcnstrFromMDPartCnstr(mp, mda,
																	   popGet->PdrgpdrgpcrPart(),
																	   relation->MDPartConstraint(),
																	   popGet->PdrgpcrOutput());
	ppartcnstrRest = ppartcnstrRel->PpartcnstrRemaining(mp, ppartcnstrCovered);

	// Create new partial DynamicGet node (copying from the previous node)

	// FIXME: is this the best way to copy an expr?
	// Also this is similar to CXformSelect2PartialDynamicIndexGet::PexprSelectOverDynamicGet()

	// use dummy colref_mapping to preserve the same columns
	CName *pnameDG = GPOS_NEW(mp) CName(mp, popGet->Name());
	CLogicalDynamicGet *popPartialDynamicGet =
		GPOS_NEW(mp) CLogicalDynamicGet(mp, pnameDG, popGet->Ptabdesc(),
										popGet->ScanId(),
										popGet->PdrgpcrOutput(),
										popGet->PdrgpdrgpcrPart(),
										COptCtxt::PoctxtFromTLS()->UlPartIndexNextVal(),
										true, /* is_partial */
										ppartcnstrRest,
										ppartcnstrRel);

	CExpression *pexprPartialDynamicGet = GPOS_NEW(mp) CExpression (mp, popPartialDynamicGet);


	// Create new MultiExternalGet node capturing all the external scans
	CName *pnameMEG = GPOS_NEW(mp) CName(mp, popGet->Name());
	CColRefArray *pdrgpcrNew = CUtils::PdrgpcrCopy(mp, popGet->PdrgpcrOutput());
	ptabdesc->AddRef();
	CLogicalMultiExternalGet *popMultiExternalGet =
		GPOS_NEW(mp) CLogicalMultiExternalGet(mp, pnameMEG,
											  ptabdesc,
											  popGet->ScanId(),
											  pdrgpcrNew);
	popMultiExternalGet->SetSecondaryScanId(COptCtxt::PoctxtFromTLS()->UlPartIndexNextVal());
	popMultiExternalGet->SetPartial();
	CExpression *pexprMultiExternalGet = GPOS_NEW(mp) CExpression(mp, popMultiExternalGet);

	CColRef2dArray *pdrgpdrgpcrInput = GPOS_NEW(mp) CColRef2dArray(mp);

	popPartialDynamicGet->PdrgpcrOutput()->AddRef();
	pdrgpdrgpcrInput->Append(popPartialDynamicGet->PdrgpcrOutput());
	popMultiExternalGet->PdrgpcrOutput()->AddRef();
	pdrgpdrgpcrInput->Append(popMultiExternalGet->PdrgpcrOutput());

	CExpressionArray *pdrgpexprInput = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprInput->Append(pexprPartialDynamicGet);
	pdrgpexprInput->Append(pexprMultiExternalGet);

	popGet->PdrgpcrOutput()->AddRef();
	CExpression *pexprResult =
		GPOS_NEW(mp) CExpression(mp,
								 GPOS_NEW(mp) CLogicalUnionAll(mp,
															   popGet->PdrgpcrOutput(),
															   pdrgpdrgpcrInput,
															   popGet->ScanId()),
								 pdrgpexprInput);

	// add alternative to transformation result
	pxfres->Add(pexprResult);
}


// EOF

