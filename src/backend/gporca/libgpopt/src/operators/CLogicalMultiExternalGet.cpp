//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 Pivotal, Inc.
//
//	@filename:
//		CLogicalMultiExternalGet.cpp
//
//	@doc:
//		Implementation of external get
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/base/COptCtxt.h"

#include "gpopt/operators/CLogicalMultiExternalGet.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/metadata/CName.h"


using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalMultiExternalGet::CLogicalMultiExternalGet
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalMultiExternalGet::CLogicalMultiExternalGet
	(
	CMemoryPool *mp
	)
	:
	CLogicalGet(mp)
{}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalMultiExternalGet::CLogicalMultiExternalGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalMultiExternalGet::CLogicalMultiExternalGet
	(
	CMemoryPool *mp,
	const CName *pnameAlias,
	CTableDescriptor *ptabdesc
	)
	:
	CLogicalGet(mp, pnameAlias, ptabdesc)
{}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalMultiExternalGet::CLogicalMultiExternalGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalMultiExternalGet::CLogicalMultiExternalGet
	(
	CMemoryPool *mp,
	const CName *pnameAlias,
	CTableDescriptor *ptabdesc,
	CColRefArray *pdrgpcrOutput
	)
	:
	CLogicalGet(mp, pnameAlias, ptabdesc, pdrgpcrOutput)
{}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalMultiExternalGet::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CLogicalMultiExternalGet::Matches
	(
	COperator *pop
	)
	const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}
	CLogicalMultiExternalGet *popGet = CLogicalMultiExternalGet::PopConvert(pop);

	return Ptabdesc() == popGet->Ptabdesc() &&
			PdrgpcrOutput()->Equals(popGet->PdrgpcrOutput());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalMultiExternalGet::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator *
CLogicalMultiExternalGet::PopCopyWithRemappedColumns
	(
	CMemoryPool *mp,
	UlongToColRefMap *colref_mapping,
	BOOL must_exist
	)
{
	CColRefArray *pdrgpcrOutput = NULL;
	if (must_exist)
	{
		pdrgpcrOutput = CUtils::PdrgpcrRemapAndCreate(mp, PdrgpcrOutput(), colref_mapping);
	}
	else
	{
		pdrgpcrOutput = CUtils::PdrgpcrRemap(mp, PdrgpcrOutput(), colref_mapping, must_exist);
	}
	CName *pnameAlias = GPOS_NEW(mp) CName(mp, Name());

	CTableDescriptor *ptabdesc = Ptabdesc();
	ptabdesc->AddRef();

	return GPOS_NEW(mp) CLogicalMultiExternalGet(mp, pnameAlias, ptabdesc, pdrgpcrOutput);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalMultiExternalGet::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalMultiExternalGet::PxfsCandidates
	(
	CMemoryPool *mp
	)
	const
{
	CXformSet *xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfExternalGet2ExternalScan);

	return xform_set;
}

// EOF

