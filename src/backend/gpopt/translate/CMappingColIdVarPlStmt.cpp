//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CMappingColIdVarPlStmt.cpp
//
//	@doc:
//		Implentation of the functions that provide the mapping between Var, Param
//		and variables of Sub-query to CDXLNode during Query->DXL translation
//
//	@test:
//
//
//---------------------------------------------------------------------------

#include "postgres.h"
#include "nodes/primnodes.h"

#include "gpopt/translate/CMappingColIdVarPlStmt.h"
#include "gpopt/translate/CDXLTranslateContextBaseTable.h"

#include "naucrates/exception.h"
#include "naucrates/md/CMDIdGPDB.h"
#include "naucrates/dxl/operators/CDXLScalarIdent.h"

#include "gpos/base.h"
#include "gpos/common/CAutoP.h"

#include "gpopt/gpdbwrappers.h"

using namespace gpdxl;
using namespace gpos;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CMappingColIdVarPlStmt::CMappingColIdVarPlStmt
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CMappingColIdVarPlStmt::CMappingColIdVarPlStmt
	(
	IMemoryPool *pmp,
	const CDXLTranslateContextBaseTable *pdxltrctxbt,
	DrgPdxltrctx *pdrgpdxltrctx,
	CDXLTranslateContext *pdxltrctxOut,
	CContextDXLToPlStmt *pctxdxltoplstmt
	)
	:
	CMappingColIdVar(pmp),
	m_pdxltrctxbt(pdxltrctxbt),
	m_pdrgpdxltrctx(pdrgpdxltrctx),
	m_pdxltrctxOut(pdxltrctxOut),
	m_pctxdxltoplstmt(pctxdxltoplstmt)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CMappingColIdVarPlStmt::Pctxdxltoplstmt
//
//	@doc:
//		Returns the DXL->PlStmt translation context
//
//---------------------------------------------------------------------------
CContextDXLToPlStmt *
CMappingColIdVarPlStmt::Pctxdxltoplstmt()
{
	return m_pctxdxltoplstmt;
}

//---------------------------------------------------------------------------
//	@function:
//		CMappingColIdVarPlStmt::PpdxltrctxOut
//
//	@doc:
//		Returns the output translation context
//
//---------------------------------------------------------------------------
CDXLTranslateContext *
CMappingColIdVarPlStmt::PpdxltrctxOut()
{
	return m_pdxltrctxOut;
}

//---------------------------------------------------------------------------
//	@function:
//		CMappingColIdVarPlStmt::PparamFromDXLNodeScId
//
//	@doc:
//		Translates a DXL scalar identifier operator into a GPDB Param node
//
//---------------------------------------------------------------------------
Param *
CMappingColIdVarPlStmt::PparamFromDXLNodeScId
	(
	const CDXLScalarIdent *pdxlop
	)
{
	GPOS_ASSERT(NULL != m_pdxltrctxOut);

	Param *pparam = NULL;

	const ULONG ulColId = pdxlop->Pdxlcr()->UlID();
	const CMappingElementColIdParamId *pmecolidparamid = m_pdxltrctxOut->Pmecolidparamid(ulColId);

	if (NULL != pmecolidparamid)
	{
		pparam = MakeNode(Param);
		pparam->paramkind = PARAM_EXEC;
		pparam->paramid = pmecolidparamid->UlParamId();
		pparam->paramtype = CMDIdGPDB::PmdidConvert(pmecolidparamid->PmdidType())->OidObjectId();
		pparam->paramtypmod = pmecolidparamid->ITypeModifier();
	}

	return pparam;
}

//---------------------------------------------------------------------------
//	@function:
//		CMappingColIdVarPlStmt::PvarFromDXLNodeScId
//
//	@doc:
//		Translates a DXL scalar identifier operator into a GPDB Var node
//
//---------------------------------------------------------------------------
Var *
CMappingColIdVarPlStmt::PvarFromDXLNodeScId
	(
	const CDXLScalarIdent *pdxlop
	)
{
	Index idxVarno = 0;
	AttrNumber attno = 0;

	Index idxVarnoold = 0;
	AttrNumber attnoOld = 0;

	const ULONG ulColId = pdxlop->Pdxlcr()->UlID();
	if (NULL != m_pdxltrctxbt)
	{
		// scalar id is used in a base table operator node
		idxVarno = m_pdxltrctxbt->IRel();
		attno = (AttrNumber) m_pdxltrctxbt->IAttnoForColId(ulColId);

		idxVarnoold = idxVarno;
		attnoOld = attno;
	}

	// if lookup has failed in the first step, attempt in all the contexts
	if (0 == attno && NULL != m_pdrgpdxltrctx)
	{
		const ULONG ulContexts = m_pdrgpdxltrctx->UlLength();
		const TargetEntry *pte;

		GPOS_ASSERT(0 != ulContexts);

		for (ULONG ul = 0; ul < ulContexts; ++ul)
		{
			const CDXLTranslateContext *pdxltrctx = (*m_pdrgpdxltrctx)[ul];
			pte = pdxltrctx->Pte(ulColId);

			if (NULL != pte)
			{
				if (0 == ul)
				{  // Left context
					idxVarno = OUTER;
					attno = pte->resno;
				}
				else if (1 == ul)
				{  // Right context
					idxVarno = INNER;
					attno = pte->resno;
				}
				else
				{  // Additional context
					GPOS_ASSERT(IsA(pte->expr, Var));

					Var *pv = (Var*) pte->expr;
					idxVarno = pv->varno;
					attno = pv->varattno;
				}

				break;
			}
		}

		if (NULL == pte)
		{
			return NULL;
		}

		// find the original varno and attno for this column
		if (IsA(pte->expr, Var))
		{
			Var *pv = (Var*) pte->expr;
			idxVarnoold = pv->varnoold;
			attnoOld = pv->varoattno;
		}
		else
		{
			idxVarnoold = idxVarno;
			attnoOld = attno;
		}
	}

	GPOS_ASSERT(attno != 0);

	Var *pvar = gpdb::PvarMakeVar
						(
						idxVarno,
						attno,
						CMDIdGPDB::PmdidConvert(pdxlop->PmdidType())->OidObjectId(),
						pdxlop->ITypeModifier(),
						0	// varlevelsup
						);

	// set varnoold and varoattno since makeVar does not set them properly
	pvar->varnoold = idxVarnoold;
	pvar->varoattno = attnoOld;

	return pvar;
}

// EOF
