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

	// create/extract components for alternative
//	CName *pname = GPOS_NEW(mp) CName(mp, popGet->Name());
	CColRef2dArray *pdrgpdrgpcrPart = popGet->PdrgpdrgpcrPart();

	CColRefArray *pdrgpcrGet = popGet->PdrgpcrOutput();
	GPOS_ASSERT(NULL != pdrgpcrGet);
	pdrgpcrGet->AddRef();

	pdrgpcrGet->AddRef();
	ptabdesc->AddRef();
	// popGet->Ppartcnstr()->AddRef();
	// popGet->PpartcnstrRel()->AddRef();

	// FIXME: is this the best way to copy an expr?
	// This is eerily similar to CXformSelect2PartialDynamicIndexGet::PexprSelectOverDynamicGet()
	UlongToColRefMap *colref_mapping = GPOS_NEW(mp) UlongToColRefMap(mp);
	CLogicalDynamicGet *popPartialDynamicGet =
		(CLogicalDynamicGet *) popGet->PopCopyWithRemappedColumns(mp, colref_mapping, false /*must_exist*/);
	colref_mapping->Release();

	popPartialDynamicGet->SetSecondaryScanId(COptCtxt::PoctxtFromTLS()->UlPartIndexNextVal());

	// FIXME: Get the correct part contraints for the DTS & External Scans
	CPartConstraint *ppartcnstrPartialDynamicGet =
		CUtils::PpartcnstrFromMDPartCnstr(mp, mda, pdrgpdrgpcrPart, relation->MDPartConstraint(), pdrgpcrGet);
	// popGet->Ppartcnstr()->PpartcnstrCopyWithRemappedColumns(mp, colref_mapping, false /*must_exist*/);
	popPartialDynamicGet->SetPartConstraint(ppartcnstrPartialDynamicGet);
	popPartialDynamicGet->SetPartial();

	// create alternative expression
	CExpression *pexprAlt = GPOS_NEW(mp) CExpression (mp, popPartialDynamicGet);
	IMdIdArray *external_partoids = relation->GetExternalPartitions();

	CColRef2dArray *pdrgpdrgpcrInput = GPOS_NEW(mp) CColRef2dArray(mp);
	CExpressionArray *pdrgpexprInput = GPOS_NEW(mp) CExpressionArray(mp);

	pdrgpdrgpcrInput->Append(pdrgpcrGet);
	pdrgpexprInput->Append(pexprAlt);

	for (ULONG ul = 0; ul < external_partoids->Size(); ul++)
	{
		// FIXME: Implement static partition selection here
	}


	CName *pnameMEG = GPOS_NEW(mp) CName(mp, popGet->Name());

	CLogicalMultiExternalGet *pop =
		GPOS_NEW(mp) CLogicalMultiExternalGet(mp, pnameMEG, popGet->Ptabdesc());
	CExpression *pexprMultiExternalGet = GPOS_NEW(mp) CExpression(mp, pop);

	CColRefArray *pdrgpcrNew = pop->PdrgpcrOutput();
	pdrgpcrNew->AddRef();
	pdrgpdrgpcrInput->Append(pdrgpcrNew);
	pdrgpexprInput->Append(pexprMultiExternalGet);

	CExpression *pexprResult =
		GPOS_NEW(mp) CExpression(mp,
								 GPOS_NEW(mp) CLogicalUnionAll(mp, pdrgpcrGet, pdrgpdrgpcrInput, popGet->ScanId()),
								 pdrgpexprInput);

	// add alternative to transformation result
	pxfres->Add(pexprResult);
}


// EOF

