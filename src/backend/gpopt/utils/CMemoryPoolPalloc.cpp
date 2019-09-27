//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2019 Pivotal, Inc.
//
//	@filename:
//		CMemoryPoolPalloc.cpp
//
//	@doc:
//		CMemoryPool implementation that uses PostgreSQL memory
//		contexts.
//
//---------------------------------------------------------------------------

extern "C" {
#include "postgres.h"
#include "utils/memutils.h"
}

#include "gpos/memory/CMemoryPool.h"

#include "gpopt/utils/CMemoryPoolPalloc.h"

using namespace gpos;

// ctor
CMemoryPoolPalloc::CMemoryPoolPalloc()
	: m_cxt(NULL)
{
	m_cxt = AllocSetContextCreate(OptimizerMemoryContext,
								  "GPORCA memory pool",
								  ALLOCSET_DEFAULT_MINSIZE,
								  ALLOCSET_DEFAULT_INITSIZE,
								  ALLOCSET_DEFAULT_MAXSIZE);
}

void *
CMemoryPoolPalloc::NewImpl
	(
	const ULONG bytes,
	const CHAR *,
	const ULONG,
	CMemoryPool::EAllocationType eat
	)
{
	// if it's a singleton allocation, allocate requested memory
	if (CMemoryPool::EatSingleton == eat)
	{
		return MemoryContextAlloc(m_cxt, bytes);
	}
	// if it's an array allocation, allocate header + requested memory
	else
	{
		ULONG alloc_size = GPOS_MEM_ALIGNED_STRUCT_SIZE(SArrayAllocHeader) + GPOS_MEM_ALIGNED_SIZE(bytes);

		void *ptr = MemoryContextAlloc(m_cxt, alloc_size);
		if (NULL == ptr)
		{
			return NULL;
		}

		SArrayAllocHeader *header = static_cast<SArrayAllocHeader*>(ptr);

		header->m_user_size = bytes;
		return header+1;
	}
}

void
CMemoryPoolPalloc::DeleteImpl(void *ptr, CMemoryPool::EAllocationType eat)
{
	if (CMemoryPool::EatSingleton == eat)
	{
		pfree(ptr);
	}
	else
	{
		SArrayAllocHeader *header = static_cast<SArrayAllocHeader*>(ptr) - 1;

		pfree(header);
	}
}

// Prepare the memory pool to be deleted
void
CMemoryPoolPalloc::TearDown()
{
	MemoryContextDelete(m_cxt);
}

// Total allocated size including management overheads
ULLONG
CMemoryPoolPalloc::TotalAllocatedSize() const
{
	return MemoryContextGetCurrentSpace(m_cxt);
}

// get user requested size of array allocation. Note: this is ONLY called for arrays
ULONG
CMemoryPoolPalloc::UserSizeOfAlloc(const void *ptr)
{
	GPOS_ASSERT(ptr != NULL);
	const SArrayAllocHeader *header = static_cast<const SArrayAllocHeader*>(ptr) - 1;

	return header->m_user_size;
}


// EOF
