//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 Pivotal, Inc.
//
//	@filename:
//		CLogicalMultiExternalGet.h
//
//	@doc:
//		Logical external get operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalMultiExternalGet_H
#define GPOPT_CLogicalMultiExternalGet_H

#include "gpos/base.h"
#include "gpopt/operators/CLogicalGet.h"

namespace gpopt
{

	// fwd declarations
	class CTableDescriptor;
	class CName;
	class CColRefSet;

	//---------------------------------------------------------------------------
	//	@class:
	//		CLogicalMultiExternalGet
	//
	//	@doc:
	//		Logical external get operator
	//
	//---------------------------------------------------------------------------
	class CLogicalMultiExternalGet : public CLogicalGet
	{

		private:

			// private copy ctor
			CLogicalMultiExternalGet(const CLogicalMultiExternalGet &);

		public:

			// ctors
			explicit
			CLogicalMultiExternalGet(CMemoryPool *mp);

			CLogicalMultiExternalGet
				(
				CMemoryPool *mp,
				const CName *pnameAlias,
				CTableDescriptor *ptabdesc
				);

			CLogicalMultiExternalGet
				(
				CMemoryPool *mp,
				const CName *pnameAlias,
				CTableDescriptor *ptabdesc,
				CColRefArray *pdrgpcrOutput
				);

			// ident accessors
			virtual
			EOperatorId Eopid() const
			{
				return EopLogicalMultiExternalGet;
			}

			// return a string for operator name
			virtual
			const CHAR *SzId() const
			{
				return "CLogicalMultiExternalGet";
			}

			// match function
			virtual
			BOOL Matches(COperator *pop) const;

			// return a copy of the operator with remapped columns
			virtual
			COperator *PopCopyWithRemappedColumns(CMemoryPool *mp, UlongToColRefMap *colref_mapping, BOOL must_exist);

			//-------------------------------------------------------------------------------------
			// Required Relational Properties
			//-------------------------------------------------------------------------------------

			// compute required stat columns of the n-th child
			virtual
			CColRefSet *PcrsStat
				(
				CMemoryPool *, // mp,
				CExpressionHandle &, // exprhdl
				CColRefSet *, // pcrsInput
				ULONG // child_index
				)
				const
			{
				GPOS_ASSERT(!"CLogicalMultiExternalGet has no children");
				return NULL;
			}

			//-------------------------------------------------------------------------------------
			// Transformations
			//-------------------------------------------------------------------------------------

			// candidate set of xforms
			virtual
			CXformSet *PxfsCandidates(CMemoryPool *mp) const;

			//-------------------------------------------------------------------------------------
			//-------------------------------------------------------------------------------------
			//-------------------------------------------------------------------------------------

			// conversion function
			static
			CLogicalMultiExternalGet *PopConvert
				(
				COperator *pop
				)
			{
				GPOS_ASSERT(NULL != pop);
				GPOS_ASSERT(EopLogicalMultiExternalGet == pop->Eopid());

				return dynamic_cast<CLogicalMultiExternalGet*>(pop);
			}

	}; // class CLogicalMultiExternalGet
}

#endif // !GPOPT_CLogicalMultiExternalGet_H

// EOF
