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

typedef struct AccountedAllocHeader
{
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

static inline
struct MemoryAccount* AccountedAllocPtr_GetMemoryAccount(AccountedAllocHeader *ptr)
{
	return ptr->memoryAccount;
}

static inline
uint16 AccountedAllocPtr_GetMemoryAccountGeneration(AccountedAllocHeader *ptr)
{
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
}

#endif   /* GP_ACCOUNTED_ALLOC_H */
