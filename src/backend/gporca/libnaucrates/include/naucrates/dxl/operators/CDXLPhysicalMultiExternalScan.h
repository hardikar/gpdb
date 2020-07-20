//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 Pivotal, Inc.
//
//	@filename:
//		CDXLPhysicalMultiExternalScan.h
//
//	@doc:
//		Class for representing DXL external scan operators
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLPhysicalMultiExternalScan_H
#define GPDXL_CDXLPhysicalMultiExternalScan_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLPhysicalTableScan.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"

namespace gpdxl
{
	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLPhysicalMultiExternalScan
	//
	//	@doc:
	//		Class for representing DXL external scan operators
	//
	//---------------------------------------------------------------------------
	class CDXLPhysicalMultiExternalScan : public CDXLPhysicalTableScan
	{
		private:

			// private copy ctor
			CDXLPhysicalMultiExternalScan(CDXLPhysicalMultiExternalScan&);

		public:
			// ctors
			explicit
			CDXLPhysicalMultiExternalScan(CMemoryPool *mp);

			CDXLPhysicalMultiExternalScan(CMemoryPool *mp, CDXLTableDescr *table_descr);

			// operator type
			virtual
			Edxlopid GetDXLOperator() const;

			// operator name
			virtual
			const CWStringConst *GetOpNameStr() const;

			// conversion function
			static
			CDXLPhysicalMultiExternalScan *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopPhysicalMultiExternalScan == dxl_op->GetDXLOperator());

				return dynamic_cast<CDXLPhysicalMultiExternalScan*>(dxl_op);
			}

	};
}
#endif // !GPDXL_CDXLPhysicalMultiExternalScan_H

// EOF

