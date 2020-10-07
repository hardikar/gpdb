/*--------------------------------------------------------------------------
 *
 * cdbpartition.c
 *	  Provides utility routines to support sharding via partitioning
 *	  within Greenplum Database.
 *
 *    Many items are just extensions of tablecmds.c.
 *
 * Portions Copyright (c) 2005-2010, Greenplum inc
 * Portions Copyright (c) 2012-Present Pivotal Software, Inc.
 *
 *
 * IDENTIFICATION
 *	    src/backend/cdb/cdbpartition.c
 *
 *--------------------------------------------------------------------------
 */
#include "postgres.h"

#include "cdb/cdbpartition.h"
#include "access/table.h"
#include "partitioning/partdesc.h"
#include "utils/rel.h"
#include "utils/relcache.h"

List *
rel_get_leaf_partitions(Oid rel_oid)
{
	List *leaf_parts = NIL;
	List *process_parts = NIL;

	int i;
	Relation relation;
	PartitionDesc partdesc;
	// FIXME: What is the correct lockmode to use here
	LOCKMODE lockmode = AccessShareLock;

	// Initialize process list with root partition oid
	process_parts = list_make1_oid(rel_oid);

	while (process_parts)
	{
		Oid oid = linitial_oid(process_parts);
		process_parts = list_delete_first(process_parts);

		Assert(oid != InvalidOid);

		relation = table_open(oid, lockmode);
		if (relation->rd_rel->relkind != RELKIND_PARTITIONED_TABLE)
		{
			table_close(relation, lockmode);
			continue;
		}

		partdesc = relation->rd_partdesc;
		Assert(partdesc);

		if (partdesc->nparts == 0)
		{
			table_close(relation, lockmode);
			continue;
		}

		for (i = 0; i < partdesc->nparts; ++i)
		{
			if (partdesc->is_leaf[i])
			{
				leaf_parts = lappend_oid(leaf_parts, partdesc->oids[i]);
			}
			else
			{
				process_parts = lappend_oid(process_parts, partdesc->oids[i]);
			}
		}

		table_close(relation, lockmode);
	}

	return leaf_parts;
}
