//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2019 Pivotal, Inc.
//
//	@filename:
//		CMemoryPoolPallocManager.h
//
//	@doc:
//		MemoryPoolManager implementation that creates
//		CMemoryPoolPalloc memory pools
//
//---------------------------------------------------------------------------

#ifndef GPDXL_CMemoryPoolPallocManager_H
#define GPDXL_CMemoryPoolPallocManager_H

#include "gpos/base.h"

#include "gpos/memory/CMemoryPoolManager.h"

namespace gpos
{
	// memory pool manager that uses GPDB memory contexts
	class CMemoryPoolPallocManager : public CMemoryPoolManager
	{
		private:

			INT m_t;

			// private no copy ctor
			CMemoryPoolPallocManager(const CMemoryPoolPallocManager&);

		public:

			// ctor
			CMemoryPoolPallocManager(CMemoryPool *internal);

			virtual CMemoryPool *NewMemoryPool();

			virtual ULONG SizeOfAlloc(const void* ptr) override;
	};
}

#endif // !GPDXL_CMemoryPoolPallocManager_H

// EOF
