//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 Pivotal, Inc.
//
//	@filename:
//		CDXLLogicalMultiExternalGet.h
//
//	@doc:
//		Class for representing DXL logical multi external get operator, for
//		reading from multiple external tables
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLLogicalMultiExternalGet_H
#define GPDXL_CDXLLogicalMultiExternalGet_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLLogicalGet.h"

namespace gpdxl
{
	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLLogicalMultiExternalGet
	//
	//	@doc:
	//		Class for representing DXL logical external get operator
	//
	//---------------------------------------------------------------------------
	class CDXLLogicalMultiExternalGet : public CDXLLogicalGet
	{
		private:

			// private copy ctor
			CDXLLogicalMultiExternalGet(CDXLLogicalMultiExternalGet&);

		public:
			// ctor
			CDXLLogicalMultiExternalGet(CMemoryPool *mp, CDXLTableDescr *table_descr);

			// operator type
			virtual
			Edxlopid GetDXLOperator() const;

			// operator name
			virtual
			const CWStringConst *GetOpNameStr() const;

			// conversion function
			static
			CDXLLogicalMultiExternalGet *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopLogicalMultiExternalGet == dxl_op->GetDXLOperator());

				return dynamic_cast<CDXLLogicalMultiExternalGet*>(dxl_op);
			}

	};
}
#endif // !GPDXL_CDXLLogicalMultiExternalGet_H

// EOF
