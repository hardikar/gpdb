//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 Pivotal, Inc.
//
//	@filename:
//		CDXLLogicalMultiExternalGet.cpp
//
//	@doc:
//		Implementation of DXL logical external get operator
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLLogicalMultiExternalGet.h"
#include "naucrates/dxl/xml/dxltokens.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalMultiExternalGet::CDXLLogicalMultiExternalGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLLogicalMultiExternalGet::CDXLLogicalMultiExternalGet
	(
	CMemoryPool *mp,
	CDXLTableDescr *table_descr
	)
	:
	CDXLLogicalGet(mp, table_descr)
{}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalMultiExternalGet::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalMultiExternalGet::GetDXLOperator() const
{
	return EdxlopLogicalMultiExternalGet;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalMultiExternalGet::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalMultiExternalGet::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenLogicalMultiExternalGet);
}

// EOF
