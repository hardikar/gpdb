/*-------------------------------------------------------------------------
 *
 * partprune.h
 *	  prototypes for partprune.c
 *
 *
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/partitioning/partprune.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARTPRUNE_H
#define PARTPRUNE_H

#include "nodes/execnodes.h"
#include "nodes/pathnodes.h"
#include "partitioning/partdefs.h"

struct PlannerInfo;				/* avoid including pathnodes.h here */
struct RelOptInfo;


/*
 * PartitionPruneContext
 *		Stores information needed at runtime for pruning computations
 *		related to a single partitioned table.
 *
 * strategy			Partition strategy, e.g. LIST, RANGE, HASH.
 * partnatts		Number of columns in the partition key.
 * nparts			Number of partitions in this partitioned table.
 * boundinfo		Partition boundary info for the partitioned table.
 * partcollation	Array of partnatts elements, storing the collations of the
 *					partition key columns.
 * partsupfunc		Array of FmgrInfos for the comparison or hashing functions
 *					associated with the partition keys (partnatts elements).
 *					(This points into the partrel's partition key, typically.)
 * stepcmpfuncs		Array of FmgrInfos for the comparison or hashing function
 *					for each pruning step and partition key.
 * ppccontext		Memory context holding this PartitionPruneContext's
 *					subsidiary data, such as the FmgrInfos.
 * planstate		Points to the parent plan node's PlanState when called
 *					during execution; NULL when called from the planner.
 * exprstates		Array of ExprStates, indexed as per PruneCxtStateIdx; one
 *					for each partition key in each pruning step.  Allocated if
 *					planstate is non-NULL, otherwise NULL.
 */
typedef struct PartitionPruneContext
{
	char		strategy;
	int			partnatts;
	int			nparts;
	PartitionBoundInfo boundinfo;
	Oid		   *partcollation;
	FmgrInfo   *partsupfunc;
	FmgrInfo   *stepcmpfuncs;
	MemoryContext ppccontext;
	PlanState  *planstate;
	ExprState **exprstates;
} PartitionPruneContext;

/*
 * PartClauseTarget
 *		Identifies which qual clauses we can use for generating pruning steps
 */
typedef enum PartClauseTarget
{
	PARTTARGET_PLANNER,			/* want to prune during planning */
	PARTTARGET_INITIAL,			/* want to prune during executor startup */
	PARTTARGET_EXEC				/* want to prune during each plan node scan */
} PartClauseTarget;

/*
 * GeneratePruningStepsContext
 *		Information about the current state of generation of "pruning steps"
 *		for a given set of clauses
 *
 * gen_partprune_steps() initializes and returns an instance of this struct.
 *
 * Note that has_mutable_op, has_mutable_arg, and has_exec_param are set if
 * we found any potentially-useful-for-pruning clause having those properties,
 * whether or not we actually used the clause in the steps list.  This
 * definition allows us to skip the PARTTARGET_EXEC pass in some cases.
 */
typedef struct GeneratePruningStepsContext
{
	/* Copies of input arguments for gen_partprune_steps: */
	RelOptInfo *rel;			/* the partitioned relation */
	PartClauseTarget target;	/* use-case we're generating steps for */

	Relids		available_relids; /* rels whose Vars may be used for pruning */

	/* Result data: */
	List	   *steps;			/* list of PartitionPruneSteps */
	bool		has_mutable_op; /* clauses include any stable operators */
	bool		has_mutable_arg;	/* clauses include any mutable comparison
									 * values, *other than* exec params */
	bool		has_exec_param; /* clauses include any PARAM_EXEC params */
	bool		has_vars;		/* clauses include any Vars from 'available_rels' */
	bool		contradictory;	/* clauses were proven self-contradictory */
	/* Working state: */
	int			next_step_id;
} GeneratePruningStepsContext;

/*
 * PruneCxtStateIdx() computes the correct index into the stepcmpfuncs[],
 * exprstates[] and exprhasexecparam[] arrays for step step_id and
 * partition key column keyno.  (Note: there is code that assumes the
 * entries for a given step are sequential, so this is not chosen freely.)
 */
#define PruneCxtStateIdx(partnatts, step_id, keyno) \
	((partnatts) * (step_id) + (keyno))

extern PartitionPruneInfo *make_partition_pruneinfo(struct PlannerInfo *root,
													struct RelOptInfo *parentrel,
													List *subpaths,
													List *partitioned_rels,
													List *prunequal);
extern PartitionPruneInfo *make_partition_pruneinfo_ext(struct PlannerInfo *root,
														struct RelOptInfo *parentrel,
														List *subpaths, List *partitioned_rels,
														List *prunequal, Bitmapset *available_relids);
extern Bitmapset *prune_append_rel_partitions(struct RelOptInfo *rel);
extern Bitmapset *get_matching_partitions(PartitionPruneContext *context,
										  List *pruning_steps);

extern void gen_partprune_steps(RelOptInfo *rel, List *clauses,
								Relids available_relids,
								PartClauseTarget target,
								GeneratePruningStepsContext *context);

extern List *gen_partprune_steps_orca(Oid reloid, List *clauses, Relids available_relids);

#endif							/* PARTPRUNE_H */
