//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPartitionPropagationSpec.cpp
//
//	@doc:
//		Specification of partition propagation requirements
//---------------------------------------------------------------------------

#include "gpopt/base/CPartitionPropagationSpec.h"

#include "gpos/memory/CAutoMemoryPool.h"

#include "gpopt/exception.h"
#include "gpopt/operators/CPhysicalPartitionSelector.h"
#include "gpopt/operators/CPredicateUtils.h"

using namespace gpos;
using namespace gpopt;

INT
CPartitionPropagationSpec::SPartPropSpecInfo::CmpFunc(const void *val1,
													  const void *val2)
{
	const SPartPropSpecInfo *info1 = *(const SPartPropSpecInfo **) val1;
	const SPartPropSpecInfo *info2 = *(const SPartPropSpecInfo **) val2;

	return info1->m_scan_id - info2->m_scan_id;
}

BOOL
CPartitionPropagationSpec::SPartPropSpecInfo::Equals(
	const SPartPropSpecInfo *other) const
{
	return m_scan_id == other->m_scan_id && m_type == other->m_type &&
		   m_root_rel_mdid->Equals(other->m_root_rel_mdid) &&
		   m_selector_ids->Equals(other->m_selector_ids);
}

BOOL
CPartitionPropagationSpec::SPartPropSpecInfo::FSatisfies(
	const SPartPropSpecInfo *other) const
{
	return m_scan_id == other->m_scan_id && m_type == other->m_type &&
		   m_root_rel_mdid->Equals(other->m_root_rel_mdid);
}

CPartitionPropagationSpec::CPartitionPropagationSpec(CMemoryPool *mp)
{
	m_part_prop_spec_infos = GPOS_NEW(mp) SPartPropSpecInfoArray(mp);
}

// dtor
CPartitionPropagationSpec::~CPartitionPropagationSpec()
{
	m_part_prop_spec_infos->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CPartitionPropagationSpec::Matches
//
//	@doc:
//		Check whether two partition propagation specs are equal
//
//---------------------------------------------------------------------------
BOOL
CPartitionPropagationSpec::Equals(const CPartitionPropagationSpec *pps) const
{
	GPOS_ASSERT(m_part_prop_spec_infos->IsSorted(SPartPropSpecInfo::CmpFunc));

	if ((m_part_prop_spec_infos == nullptr) &&
		(pps->m_part_prop_spec_infos == nullptr))
	{
		return true;
	}

	if ((m_part_prop_spec_infos == nullptr) ^
		(pps->m_part_prop_spec_infos == nullptr))
	{
		return false;
	}

	GPOS_ASSERT(m_part_prop_spec_infos != nullptr &&
				pps->m_part_prop_spec_infos != nullptr);
	GPOS_ASSERT(m_part_prop_spec_infos->IsSorted(SPartPropSpecInfo::CmpFunc));
	GPOS_ASSERT(
		pps->m_part_prop_spec_infos->IsSorted(SPartPropSpecInfo::CmpFunc));

	if (m_part_prop_spec_infos->Size() != pps->m_part_prop_spec_infos->Size())
	{
		return false;
	}

	ULONG size = m_part_prop_spec_infos->Size();
	for (ULONG ul = 0; ul < size; ++ul)
	{
		if (!(*m_part_prop_spec_infos)[ul]->Equals(
				(*pps->m_part_prop_spec_infos)[ul]))
		{
			return false;
		}
	}

	return true;
}

CPartitionPropagationSpec::SPartPropSpecInfo *
CPartitionPropagationSpec::FindPartPropSpecInfo(INT scan_id) const
{
	for (ULONG ut = 0; ut < m_part_prop_spec_infos->Size(); ut++)
	{
		SPartPropSpecInfo *part_info = (*m_part_prop_spec_infos)[ut];

		if (part_info->m_scan_id == scan_id)
		{
			return part_info;
		}
	}

	return nullptr;
}

void
CPartitionPropagationSpec::Insert(INT scan_id, EPartPropSpecInfoType type,
								  IMDId *rool_rel_mdid)
{
	CMemoryPool *mp = COptCtxt::PoctxtFromTLS()->Pmp();
	rool_rel_mdid->AddRef();
	SPartPropSpecInfo *info =
		GPOS_NEW(mp) SPartPropSpecInfo(scan_id, type, rool_rel_mdid);

	// NB: This is appended blindly !
	m_part_prop_spec_infos->Append(info);
	m_part_prop_spec_infos->Sort();
}

BOOL
CPartitionPropagationSpec::FSatisfies(
	const CPartitionPropagationSpec *pps) const
{
	if (pps->m_part_prop_spec_infos == nullptr)
	{
		return true;
	}

	for (ULONG ul = 0; ul < pps->m_part_prop_spec_infos->Size(); ul++)
	{
		SPartPropSpecInfo *reqd_info = (*pps->m_part_prop_spec_infos)[ul];
		SPartPropSpecInfo *found_info =
			FindPartPropSpecInfo(reqd_info->m_scan_id);

		if (found_info == nullptr || !found_info->FSatisfies(reqd_info))
		{
			return false;
		}
	}
	return true;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartitionPropagationSpec::AppendEnforcers
//
//	@doc:
//		Add required enforcers to dynamic array
//
//---------------------------------------------------------------------------
void
CPartitionPropagationSpec::AppendEnforcers(CMemoryPool *, CExpressionHandle &,
										   CReqdPropPlan *
#ifdef GPOS_DEBUG

#endif	// GPOS_DEBUG
										   ,
										   CExpressionArray *, CExpression *)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CPartitionPropagationSpec::PexprFilter
//
//	@doc:
//		Return the filter expression for the given Scan Id
//
//---------------------------------------------------------------------------
//CExpression *
//CPartitionPropagationSpec::PexprFilter(CMemoryPool *mp, ULONG scan_id)
//{
//	CExpression *pexprScalar = m_ppfm->Pexpr(scan_id);
//	GPOS_ASSERT(NULL != pexprScalar);
//
//	if (CUtils::FScalarIdent(pexprScalar))
//	{
//		// condition of the form "pkey": translate into pkey = true
//		pexprScalar->AddRef();
//		pexprScalar = CUtils::PexprScalarEqCmp(
//			mp, pexprScalar,
//			CUtils::PexprScalarConstBool(mp, true /*value*/,
//										 false /*is_null*/));
//	}
//	else if (CPredicateUtils::FNot(pexprScalar) &&
//			 CUtils::FScalarIdent((*pexprScalar)[0]))
//	{
//		// condition of the form "!pkey": translate into pkey = false
//		CExpression *pexprId = (*pexprScalar)[0];
//		pexprId->AddRef();
//
//		pexprScalar = CUtils::PexprScalarEqCmp(
//			mp, pexprId,
//			CUtils::PexprScalarConstBool(mp, false /*value*/,
//										 false /*is_null*/));
//	}
//	else
//	{
//		pexprScalar->AddRef();
//	}
//
//	return pexprScalar;
//}

//---------------------------------------------------------------------------
//	@function:
//		CPartitionPropagationSpec::PcrsKeys
//
//	@doc:
//		Return a colrefset containing all the part keys
//
//---------------------------------------------------------------------------
//CColRefSet *
//CPartitionPropagationSpec::PcrsKeys(CMemoryPool *mp,
//									CColRef2dArray *pdrgpdrgpcrKeys)
//{
//	CColRefSet *pcrs = GPOS_NEW(mp) CColRefSet(mp);
//
//	const ULONG ulLevels = pdrgpdrgpcrKeys->Size();
//	for (ULONG ul = 0; ul < ulLevels; ul++)
//	{
//		CColRef *colref = CUtils::PcrExtractPartKey(pdrgpdrgpcrKeys, ul);
//		pcrs->Include(colref);
//	}
//
//	return pcrs;
//}


//---------------------------------------------------------------------------
//	@function:
//		CPartitionPropagationSpec::OsPrint
//
//	@doc:
//		Print function
//
//---------------------------------------------------------------------------
IOstream &
CPartitionPropagationSpec::OsPrint(IOstream &os) const
{
	if (nullptr == m_part_prop_spec_infos ||
		m_part_prop_spec_infos->Size() == 0)
	{
		os << "<empty>";
		return os;
	}

	ULONG size = m_part_prop_spec_infos->Size();
	for (ULONG ul = 0; ul < size; ++ul)
	{
		SPartPropSpecInfo *part_info = (*m_part_prop_spec_infos)[ul];

		switch (part_info->m_type)
		{
			case EpptConsumer:
				os << "consumer";
				break;
			case EpptPropagator:
				os << "propagator";
			default:
				break;
		}

		os << "<" << part_info->m_scan_id << ">";
		os << "(";
		part_info->m_selector_ids->OsPrint(os);
		os << ")";

		if (ul < (size - 1))
		{
			os << ", ";
		}
	}
	return os;
}


// EOF
