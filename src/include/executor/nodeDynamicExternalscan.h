/*-------------------------------------------------------------------------
 *
 * nodeDynamicExternalScan.h
 *
 * Portions Copyright (c) 2012 - present, EMC/Greenplum
 * Portions Copyright (c) 2012-Present Pivotal Software, Inc.
 *
 *
 * IDENTIFICATION
 *	    src/include/executor/nodeDynamicExternalScan.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODEDYNAMICEXTERNALSCAN_H
#define NODEDYNAMICEXTERNALSCAN_H

#include "nodes/execnodes.h"

extern DynamicExternalScanState *ExecInitDynamicExternalScan(DynamicExternalScan *node, EState *estate, int eflags);
extern TupleTableSlot *ExecDynamicExternalScan(DynamicExternalScanState *node);
extern void ExecEndDynamicExternalScan(DynamicExternalScanState *node);
extern void ExecReScanDynamicExternalScan(DynamicExternalScanState *node);

#endif
