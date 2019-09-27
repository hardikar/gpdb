//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2019 Pivotal, Inc.
//
//	@filename:
//		CMemoryPoolPalloc.h
//
//	@doc:
//		CMemoryPool implementation that uses PostgreSQL memory
//		contexts.
//
//---------------------------------------------------------------------------

#ifndef GPDXL_CMemoryPoolPalloc_H
#define GPDXL_CMemoryPoolPalloc_H

#include "gpos/base.h"

#include "gpos/memory/CMemoryPool.h"

namespace gpos
{
	// Memory pool that maps to a Postgres MemoryContext.
	class CMemoryPoolPalloc : public CMemoryPool
	{
		private:

			MemoryContext m_cxt;

			struct SArrayAllocHeader
			{
				// FIGGY: this should be removed, but I'm messing up the alignment somewhere if we remove this and run a query in debug build
				ULONG dummy_for_alginment;

				ULONG m_user_size;
			};
		public:

			// ctor
			CMemoryPoolPalloc();

			// allocate memory
			void *NewImpl
				(
				const ULONG bytes,
				const CHAR *file,
				const ULONG line,
				CMemoryPool::EAllocationType eat
				);

			// free memory
			static void DeleteImpl(void *ptr, CMemoryPool::EAllocationType eat);

			// prepare the memory pool to be deleted
			void TearDown();

			// return total allocated size include management overhead
			ULLONG TotalAllocatedSize() const;

			// get user requested size of allocation
			static ULONG UserSizeOfAlloc(const void *ptr);

	};
}

#endif // !GPDXL_CMemoryPoolPalloc_H

// EOF
