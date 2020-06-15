/*-------------------------------------------------------------------------
 *
 * nodeDynamicSeqscan.c
 *	  Support routines for scanning one or more relations that are
 *	  determined at run time. The relations could be Heap, AppendOnly Row,
 *	  AppendOnly Columnar.
 *
 * DynamicExternalScan node scans each relation one after the other. For each
 * relation, it opens the table, scans the tuple, and returns relevant tuples.
 *
 * Portions Copyright (c) 2012 - present, EMC/Greenplum
 * Portions Copyright (c) 2012-Present Pivotal Software, Inc.
 *
 *
 * IDENTIFICATION
 *	    src/backend/executor/nodeDynamicExternalScan.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "executor/executor.h"
#include "executor/instrument.h"
#include "nodes/execnodes.h"
#include "executor/execDynamicScan.h"
#include "executor/nodeDynamicExternalscan.h"
#include "executor/nodeExternalscan.h"
#include "utils/hsearch.h"
#include "parser/parsetree.h"
#include "utils/memutils.h"
#include "cdb/cdbpartition.h"
#include "cdb/cdbvars.h"
#include "cdb/partitionselection.h"

static void CleanupOnePartition(DynamicExternalScanState *node);

DynamicExternalScanState *
ExecInitDynamicExternalScan(DynamicExternalScan *node, EState *estate, int eflags)
{
	DynamicExternalScanState *state;
	Oid			reloid;

	Assert((eflags & (EXEC_FLAG_BACKWARD | EXEC_FLAG_MARK)) == 0);

	state = makeNode(DynamicExternalScanState);
	state->eflags = eflags;
	state->ss.ps.plan = (Plan *) node;
	state->ss.ps.state = estate;

	state->scan_state = SCAN_INIT;

	/* Initialize child expressions. This is needed to find subplans. */
	state->ss.ps.qual = (List *)
		ExecInitExpr((Expr *) node->externalscan.scan.plan.qual,
					 (PlanState *) state);
	state->ss.ps.targetlist = (List *)
		ExecInitExpr((Expr *) node->externalscan.scan.plan.targetlist,
					 (PlanState *) state);

	/* Initialize result tuple type. */
	ExecInitResultTupleSlot(estate, &state->ss.ps);
	ExecAssignResultTypeFromTL(&state->ss.ps);

	reloid = getrelid(node->externalscan.scan.scanrelid, estate->es_range_table);
	Assert(OidIsValid(reloid));

	state->firstPartition = true;

	/* lastRelOid is used to remap varattno for heterogeneous partitions */
	state->lastRelOid = reloid;

	state->scanrelid = node->externalscan.scan.scanrelid;

	/*
	 * This context will be reset per-partition to free up per-partition
	 * qual and targetlist allocations
	 */
	state->partitionMemoryContext = AllocSetContextCreate(CurrentMemoryContext,
									 "DynamicExternalScanPerPartition",
									 ALLOCSET_DEFAULT_MINSIZE,
									 ALLOCSET_DEFAULT_INITSIZE,
									 ALLOCSET_DEFAULT_MAXSIZE);

	return state;
}

/*
 * initNextTableToScan
 *   Find the next table to scan and initiate the scan if the previous table
 * is finished.
 *
 * If scanning on the current table is not finished, or a new table is found,
 * this function returns true.
 * If no more table is found, this function returns false.
 */
static bool
initNextTableToScan(DynamicExternalScanState *node)
{
	ScanState  *scanState = (ScanState *) node;
	DynamicExternalScan *plan = (DynamicExternalScan *) scanState->ps.plan;
	EState	   *estate = scanState->ps.state;
	Relation	lastScannedRel;
	TupleDesc	partTupDesc;
	TupleDesc	lastTupDesc;
	AttrNumber *attMap;
	Oid		   *pid;
	Relation	currentRelation;

	pid = hash_seq_search(&node->pidStatus);
	if (pid == NULL)
	{
		node->shouldCallHashSeqTerm = false;
		return false;
	}

	/* Collect number of partitions scanned in EXPLAIN ANALYZE */
	if (NULL != scanState->ps.instrument)
	{
		Instrumentation *instr = scanState->ps.instrument;
		instr->numPartScanned++;
	}

	CleanupOnePartition(node);

	currentRelation = scanState->ss_currentRelation = heap_open(*pid, AccessShareLock);
	lastScannedRel = heap_open(node->lastRelOid, AccessShareLock);
	lastTupDesc = RelationGetDescr(lastScannedRel);
	partTupDesc = RelationGetDescr(scanState->ss_currentRelation);
	attMap = varattnos_map(lastTupDesc, partTupDesc);
	heap_close(lastScannedRel, AccessShareLock);

	/* If attribute remapping is not necessary, then do not change the varattno */
	if (attMap)
	{
		change_varattnos_of_a_varno((Node*)scanState->ps.plan->qual, attMap, node->scanrelid);
		change_varattnos_of_a_varno((Node*)scanState->ps.plan->targetlist, attMap, node->scanrelid);

		/*
		 * Now that the varattno mapping has been changed, change the relation that
		 * the new varnos correspond to
		 */
		node->lastRelOid = *pid;
	}

	/*
	 * For the very first partition, the targetlist of planstate is set to null. So, we must
	 * initialize quals and targetlist, regardless of remapping requirements. For later
	 * partitions, we only initialize quals and targetlist if a column re-mapping is necessary.
	 */
	if (attMap || node->firstPartition)
	{
		node->firstPartition = false;
		MemoryContextReset(node->partitionMemoryContext);
		MemoryContext oldCxt = MemoryContextSwitchTo(node->partitionMemoryContext);

		/* Initialize child expressions */
		scanState->ps.qual = (List *) ExecInitExpr((Expr *) scanState->ps.plan->qual,
												   (PlanState *) scanState);
		scanState->ps.targetlist = (List *) ExecInitExpr((Expr *) scanState->ps.plan->targetlist,
														 (PlanState *) scanState);

		MemoryContextSwitchTo(oldCxt);
	}

	if (attMap)
		pfree(attMap);

	DynamicScan_SetTableOid(&node->ss, *pid);
	node->externalScanState = ExecInitExternalScanForPartition(&plan->externalscan, estate, node->eflags,
													 currentRelation);
	return true;
}

/*
 * setPidIndex
 *   Set the pid index for the given dynamic table.
 */
static void
setPidIndex(DynamicExternalScanState *node)
{
	Assert(node->pidIndex == NULL);

	ScanState *scanState = (ScanState *)node;
	EState *estate = scanState->ps.state;
	DynamicExternalScan *plan = (DynamicExternalScan *) scanState->ps.plan;
	Assert(estate->dynamicTableScanInfo != NULL);

	/*
	 * Ensure that the dynahash exists even if the partition selector
	 * didn't choose any partition for current scan node [MPP-24169].
	 */
	InsertPidIntoDynamicTableScanInfo(estate, plan->partIndex,
									  InvalidOid, InvalidPartitionSelectorId);

	Assert(NULL != estate->dynamicTableScanInfo->pidIndexes);
	Assert(estate->dynamicTableScanInfo->numScans >= plan->partIndex);
	node->pidIndex = estate->dynamicTableScanInfo->pidIndexes[plan->partIndex - 1];
	Assert(node->pidIndex != NULL);
}

TupleTableSlot *
ExecDynamicExternalScan(DynamicExternalScanState *node)
{
	TupleTableSlot *slot = NULL;

	/*
	 * If this is called the first time, find the pid index that contains all unique
	 * partition pids for this node to scan.
	 */
	if (node->pidIndex == NULL)
	{
		setPidIndex(node);
		Assert(node->pidIndex != NULL);

		hash_seq_init(&node->pidStatus, node->pidIndex);
		node->shouldCallHashSeqTerm = true;
	}

	/*
	 * Scan the table to find next tuple to return. If the current table
	 * is finished, close it and open the next table for scan.
	 */
	for (;;)
	{
		if (!node->externalScanState)
		{
			/* No partition open. Open the next one, if any. */
			if (!initNextTableToScan(node))
				break;
		}

		slot = ExecExternalScan(node->externalScanState);

		if (!TupIsNull(slot))
			break;

		/* No more tuples from this partition. Move to next one. */
		CleanupOnePartition(node);
	}

	return slot;
}

/*
 * CleanupOnePartition
 *		Cleans up a partition's relation and releases all locks.
 */
static void
CleanupOnePartition(DynamicExternalScanState *scanState)
{
	Assert(NULL != scanState);

	if (scanState->externalScanState)
	{
		ExecEndExternalScan(scanState->externalScanState);
		scanState->externalScanState = NULL;
	}
	if ((scanState->scan_state & SCAN_SCAN) != 0)
	{
		Assert(scanState->ss.ss_currentRelation != NULL);
		ExecCloseScanRelation(scanState->ss.ss_currentRelation);
		scanState->ss.ss_currentRelation = NULL;
	}
}

/*
 * DynamicExternalScanEndCurrentScan
 *		Cleans up any ongoing scan.
 */
static void
DynamicExternalScanEndCurrentScan(DynamicExternalScanState *node)
{
	CleanupOnePartition(node);

	if (node->shouldCallHashSeqTerm)
	{
		hash_seq_term(&node->pidStatus);
		node->shouldCallHashSeqTerm = false;
	}
}

/*
 * ExecEndDynamicExternalScan
 *		Ends the scanning of this DynamicExternalScanNode and frees
 *		up all the resources.
 */
void
ExecEndDynamicExternalScan(DynamicExternalScanState *node)
{
	DynamicExternalScanEndCurrentScan(node);

	ExecClearTuple(node->ss.ps.ps_ResultTupleSlot);

	EndPlanStateGpmonPkt(&node->ss.ps);
}

/*
 * ExecReScanDynamicExternalScan
 *		Prepares the internal states for a rescan.
 */
void
ExecReScanDynamicExternalScan(DynamicExternalScanState *node)
{
	DynamicExternalScanEndCurrentScan(node);

	/* Force reloading the partition hash table */
	node->pidIndex = NULL;
}
