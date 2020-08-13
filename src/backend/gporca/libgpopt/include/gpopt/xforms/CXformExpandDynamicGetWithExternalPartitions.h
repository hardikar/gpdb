//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformExpandDynamicGet2DynamicWithExternalPartitions.h
//
//	@doc:
//		Transform DynamicGet to DynamicTableScan
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformExpandDynamicGetWithExternalPartitions_H
#define GPOPT_CXformExpandDynamicGetWithExternalPartitions_H

#include "gpos/base.h"
#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
	using namespace gpos;
	
	//---------------------------------------------------------------------------
	//	@class:
	//		CXformExpandDynamicGetWithExternalPartitions
	//
	//	@doc:
	//		Transform DynamicGet to DynamicTableScan
	//
	//---------------------------------------------------------------------------
	class CXformExpandDynamicGetWithExternalPartitions : public CXformExploration
	{

		private:

			// private copy ctor
			CXformExpandDynamicGetWithExternalPartitions(const CXformExpandDynamicGetWithExternalPartitions &);

		public:
		
			// ctor
			explicit
			CXformExpandDynamicGetWithExternalPartitions(CMemoryPool *mp);

			// dtor
			virtual 
			~CXformExpandDynamicGetWithExternalPartitions() {}

			// ident accessors
			virtual
			EXformId Exfid() const
			{
				return ExfExpandDynamicGetWithExternalPartitions;
			}
			
			// return a string for xform name
			virtual 
			const CHAR *SzId() const
			{
				return "CXformExpandDynamicGetWithExternalPartitions";
			}
			
			// compute xform promise for a given expression handle
			virtual
			EXformPromise Exfp
				(
				CExpressionHandle & // exprhdl
				)
				const
			{
				return CXform::ExfpHigh;
			}

			// actual transform
			void Transform
					(
					CXformContext *pxfctxt,
					CXformResult *pxfres,
					CExpression *pexpr
					) 
					const;
		
	}; // class CXformExpandDynamicGetWithExternalPartitions

}


#endif // !GPOPT_CXformExpandDynamicGetWithExternalPartitions_H

// EOF