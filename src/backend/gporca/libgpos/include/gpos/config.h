//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2016 Greenplum, Inc.
//
//	@filename:
//		config.h
//
//	@doc:
//		Various compile-time options that affect binary
//		compatibility.
//
//---------------------------------------------------------------------------
#ifndef GPOS_config_H
#define GPOS_config_H

#include "pg_config.h"

/* Get this working for now. */
/* Idealy we should generate this files just like pg_config.h is generated */

#ifdef USE_ASSERT_CHECKING
#define GPOS_DEBUG
#else
#undef GPOS_DEBUG
#endif

#define GPOS_x86_64 1
#define GPOS_64BIT 1
#define GPOS_Darwin

/*
// These come from CMAKE_SYSTEM_PROCESSOR
#cmakedefine GPOS_i386 1
#cmakedefine GPOS_i686 1
#cmakedefine GPOS_x86_64 1
#cmakedefine GPOS_sparc 1

// These come from CMAKE_ARCH_BITS (based on CMAKE_SIZEOF_VOID_P)
#cmakedefine GPOS_32BIT 1
#cmakedefine GPOS_64BIT 1

// These come from CMAKE_SYSTEM_NAME
#cmakedefine GPOS_Linux
#cmakedefine GPOS_Darwin
#cmakedefine GPOS_SunOS

// Atomics, detected by cmake/FindAtomics.cmake
#cmakedefine GPOS_GCC_FETCH_ADD_32
#cmakedefine GPOS_GCC_FETCH_ADD_64
#cmakedefine GPOS_GCC_CAS_32
#cmakedefine GPOS_GCC_CAS_64
*/

#endif  // GPOS_config_H
// EOF
