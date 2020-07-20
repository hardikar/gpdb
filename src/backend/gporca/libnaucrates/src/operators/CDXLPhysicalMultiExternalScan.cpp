//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 Pivotal, Inc.
//
//	@filename:
//		CDXLPhysicalMultiExternalScan.cpp
//
//	@doc:
//		Implementation of DXL physical external scan operator
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLPhysicalMultiExternalScan.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMultiExternalScan::CDXLPhysicalMultiExternalScan
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLPhysicalMultiExternalScan::CDXLPhysicalMultiExternalScan(CMemoryPool *mp)
	: CDXLPhysicalTableScan(mp)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMultiExternalScan::CDXLPhysicalMultiExternalScan
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLPhysicalMultiExternalScan::CDXLPhysicalMultiExternalScan(
	CMemoryPool *mp, CDXLTableDescr *table_descr)
	: CDXLPhysicalTableScan(mp, table_descr)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMultiExternalScan::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalMultiExternalScan::GetDXLOperator() const
{
	return EdxlopPhysicalMultiExternalScan;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMultiExternalScan::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalMultiExternalScan::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenPhysicalMultiExternalScan);
}

// EOF
