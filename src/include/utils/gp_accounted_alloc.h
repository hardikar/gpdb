/*-------------------------------------------------------------------------
 *
 * gp_alloc.h
 *	  This file contains declarations for an allocator that works with vmem quota.
 *
 * Copyright (c) 2016, Pivotal Inc.
 */
#ifndef GP_ACCOUNTED_ALLOC_H
#define GP_ACCOUNTED_ALLOC_H

#include "nodes/nodes.h"
#include "memaccounting.h"

extern bool
MemoryAccounting_Allocate(struct MemoryAccount* memoryAccount, Size allocatedSize);
extern bool
MemoryAccounting_Free(MemoryAccount* memoryAccount, uint16 memoryAccountGeneration, Size allocatedSize);



#ifdef USE_ASSERT_CHECKING
#define GP_ALLOC_DEBUG
#endif

#ifdef GP_ALLOC_DEBUG

#define ACCOUNTED_ALLOC_HEADER_CHECKSUM 0xacacacacac

typedef int64 AccountedAllocHeaderChecksumType;

#define AccountedAllocPtr_VerifyChecksum(ptr) \
	Assert(AccountedAllocPtr_GetChecksum(ptr) == ACCOUNTED_ALLOC_HEADER_CHECKSUM)

#else

#define AccountedAllocPtr_VerifyChecksum(ptr)

#endif

typedef struct AccountedAllocHeader
{
#ifdef GP_ALLOC_DEBUG
	AccountedAllocHeaderChecksumType checksum;
#endif
	struct MemoryAccount *memoryAccount;
	uint16 memoryAccountGeneration;
} AccountedAllocHeader;

extern void *gp_accounted_malloc(int64 sz);
extern void *gp_accounted_realloc(void *ptr, int64 newsz);
extern void gp_accounted_free(void *ptr);

static inline
void *AccountedAllocPtrToUserPtr(AccountedAllocHeader *ptr)
{
	return (void *)(((char*)ptr) + sizeof(AccountedAllocHeader));
}

static inline
AccountedAllocHeader *UserPtrToAccountedAllocPtr(void *ptr)
{
	return (AccountedAllocHeader *)(((char*)ptr) - sizeof(AccountedAllocHeader));
}

#ifdef GP_ALLOC_DEBUG
static inline
AccountedAllocHeaderChecksumType AccountedAllocPtr_GetChecksum(AccountedAllocHeader *ptr)
{
	return ptr->checksum;
}

static inline
void AccountedAllocPtr_SetChecksum(AccountedAllocHeader *ptr)
{
	ptr->checksum = ACCOUNTED_ALLOC_HEADER_CHECKSUM;
}
#endif   /* GP_ALLOC_DEBUG */

static inline
struct MemoryAccount* AccountedAllocPtr_GetMemoryAccount(AccountedAllocHeader *ptr)
{
	AccountedAllocPtr_VerifyChecksum(ptr);
	return ptr->memoryAccount;
}

static inline
uint16 AccountedAllocPtr_GetMemoryAccountGeneration(AccountedAllocHeader *ptr)
{
	AccountedAllocPtr_VerifyChecksum(ptr);
	return ptr->memoryAccountGeneration;
}

static inline
struct MemoryAccount* AccountedAllocPtr_SetMemoryAccount(AccountedAllocHeader *ptr, struct MemoryAccount* memoryAccount)
{
	return ptr->memoryAccount = memoryAccount;
}

static inline
uint16 AccountedAllocPtr_SetMemoryAccountGeneration(AccountedAllocHeader *ptr, uint16 memoryAccountGeneration)
{
	return ptr->memoryAccountGeneration = memoryAccountGeneration;
}

static inline
void AccountedAllocPtr_Initialize(AccountedAllocHeader *ptr)
{
	AccountedAllocPtr_SetMemoryAccount(ptr, ActiveMemoryAccount);
	AccountedAllocPtr_SetMemoryAccountGeneration(ptr, MemoryAccountingCurrentGeneration);
#ifdef GP_ALLOC_DEBUG
	AccountedAllocPtr_SetChecksum(ptr);
#endif
}

#endif   /* GP_ACCOUNTED_ALLOC_H */
