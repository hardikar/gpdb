//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 Pivotal, Inc.
//
//	@filename:
//		CPhysicalMultiExternalScan.h
//
//	@doc:
//		External scan operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalMultiExternalScan_H
#define GPOPT_CPhysicalMultiExternalScan_H

#include "gpos/base.h"
#include "gpopt/operators/CPhysicalDynamicScan.h"

namespace gpopt
{

	//---------------------------------------------------------------------------
	//	@class:
	//		CPhysicalMultiExternalScan
	//
	//	@doc:
	//		External scan operator
	//
	//---------------------------------------------------------------------------
	class CPhysicalMultiExternalScan : public CPhysicalDynamicScan
	{

		private:

			// private copy ctor
			CPhysicalMultiExternalScan(const CPhysicalMultiExternalScan&);

		public:

			// ctor
			CPhysicalMultiExternalScan
			(
			CMemoryPool *mp,
			BOOL is_partial,
			CTableDescriptor *ptabdesc,
			ULONG ulOriginOpId,
			const CName *pnameAlias,
			ULONG scan_id,
			CColRefArray *pdrgpcrOutput,
			CColRef2dArray *pdrgpdrgpcrParts,
			ULONG ulSecondaryScanId,
			CPartConstraint *ppartcnstr,
			CPartConstraint *ppartcnstrRel
			);

			// ident accessors
			virtual
			EOperatorId Eopid() const
			{
				return EopPhysicalMultiExternalScan;
			}

			// return a string for operator name
			virtual
			const CHAR *SzId() const
			{
				return "CPhysicalMultiExternalScan";
			}

			// match function
			virtual
			BOOL Matches(COperator *) const;

					// statistics derivation during costing
			virtual
			IStatistics *PstatsDerive
				(
				CMemoryPool *, // mp
				CExpressionHandle &, // exprhdl
				CReqdPropPlan *, // prpplan
				IStatisticsArray * //stats_ctxt
				)
				const
			{
				GPOS_ASSERT(!"stats derivation during costing for table scan is invalid");

				return NULL;
			}

			//-------------------------------------------------------------------------------------
			// Derived Plan Properties
			//-------------------------------------------------------------------------------------

			// derive rewindability
			virtual
			CRewindabilitySpec *PrsDerive
				(
				CMemoryPool *mp,
				CExpressionHandle & // exprhdl
				)
				const
			{
				// external tables are neither rewindable nor rescannable
				return GPOS_NEW(mp) CRewindabilitySpec(CRewindabilitySpec::ErtNone, CRewindabilitySpec::EmhtNoMotion);
			}

			//-------------------------------------------------------------------------------------
			// Enforced Properties
			//-------------------------------------------------------------------------------------

			// return rewindability property enforcing type for this operator
			virtual
			CEnfdProp::EPropEnforcingType EpetRewindability
				(
				CExpressionHandle &exprhdl,
				const CEnfdRewindability *per
				)
				const;
        
			//-------------------------------------------------------------------------------------
			//-------------------------------------------------------------------------------------
			//-------------------------------------------------------------------------------------

			// conversion function
			static
			CPhysicalMultiExternalScan *PopConvert
				(
				COperator *pop
				)
			{
				GPOS_ASSERT(NULL != pop);
				GPOS_ASSERT(EopPhysicalMultiExternalScan == pop->Eopid());

				return reinterpret_cast<CPhysicalMultiExternalScan*>(pop);
			}

	}; // class CPhysicalMultiExternalScan

}

#endif // !GPOPT_CPhysicalMultiExternalScan_H

// EOF
