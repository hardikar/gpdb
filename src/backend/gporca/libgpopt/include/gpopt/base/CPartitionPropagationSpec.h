//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPartitionPropagationSpec.h
//
//	@doc:
//		Partition Propagation spec in required properties
//---------------------------------------------------------------------------
#ifndef GPOPT_CPartitionPropagationSpec_H
#define GPOPT_CPartitionPropagationSpec_H

#include "gpos/base.h"

#include "gpopt/base/CPropSpec.h"
#include "gpopt/base/CPartFilterMap.h"
#include "gpopt/base/CPartIndexMap.h"
#include "gpos/common/CHashMap.h"
#include "gpos/common/CRefCount.h"


namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CPartitionPropagationSpec
//
//	@doc:
//		Partition Propagation specification
//
//---------------------------------------------------------------------------
class CPartitionPropagationSpec : public CPropSpec
{
private:
	// unresolved partitions map
	CPartIndexMap *m_ppim;

	// filter expressions indexed by the part index id
	CPartFilterMap *m_ppfm;

	// split the partition elimination predicates over the various levels
	// as well as the residual predicate
	void SplitPartPredicates(CMemoryPool *mp, CExpression *pexprScalar,
							 CColRef2dArray *pdrgpdrgpcrKeys,
							 UlongToExprMap *phmulexprEqFilter,
							 UlongToExprMap *phmulexprFilter,
							 CExpression **ppexprResidual);

	// return a residual filter given an array of predicates and a bitset
	// indicating which predicates have already been used
	CExpression *PexprResidualFilter(CMemoryPool *mp,
									 CExpressionArray *pdrgpexpr,
									 CBitSet *pbsUsed);

	// return an array of predicates on the given partitioning key given
	// an array of predicates on all keys
	CExpressionArray *PdrgpexprPredicatesOnKey(CMemoryPool *mp,
											   CExpressionArray *pdrgpexpr,
											   CColRef *colref,
											   CColRefSet *pcrsKeys,
											   CBitSet **ppbs);

	// return a colrefset containing all the part keys
	CColRefSet *PcrsKeys(CMemoryPool *mp, CColRef2dArray *pdrgpdrgpcrKeys);

	// return the filter expression for the given Scan Id
	CExpression *PexprFilter(CMemoryPool *mp, ULONG scan_id);

public:
	CPartitionPropagationSpec(const CPartitionPropagationSpec &) = delete;

	// ctor
	CPartitionPropagationSpec(CPartIndexMap *ppim, CPartFilterMap *ppfm);

	// dtor
	~CPartitionPropagationSpec() override;

	// accessor of part index map
	CPartIndexMap *
	Ppim() const
	{
		return m_ppim;
	}

	// accessor of part filter map
	CPartFilterMap *
	Ppfm() const
	{
		return m_ppfm;
	}

	// append enforcers to dynamic array for the given plan properties
	void AppendEnforcers(CMemoryPool *mp, CExpressionHandle &exprhdl,
						 CReqdPropPlan *prpp, CExpressionArray *pdrgpexpr,
						 CExpression *pexpr) override;

	// hash function
	ULONG HashValue() const override;

	// extract columns used by the rewindability spec
	CColRefSet *
	PcrsUsed(CMemoryPool *mp) const override
	{
		// return an empty set
		return GPOS_NEW(mp) CColRefSet(mp);
	}

	// property type
	EPropSpecType
	Epst() const override
	{
		return EpstDistribution;
	}

	// equality function
	BOOL Matches(const CPartitionPropagationSpec *ppps) const;

	// is partition propagation required
	BOOL
	FPartPropagationReqd() const
	{
		return m_ppim->FContainsUnresolvedZeroPropagators();
	}


	// print
	IOstream &OsPrint(IOstream &os) const override;

};	// class CPartitionPropagationSpec

}  // namespace gpopt

#endif	// !GPOPT_CPartitionPropagationSpec_H

// EOF
