/*
 * gcmmu.h
 *
 * Copyright (C) 2010-2011 Vivante Corporation.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef GCMMU_H
#define GCMMU_H

#include <linux/gccore.h>
#include "gcmain.h"

#define MMU_ENABLE 1

#ifndef GC_DUMP_MMU
#	define GC_DUMP_MMU 0
#endif

/*
 * Master table can be configured in 1KB mode with 256 maximum entries
 * or 4KB mode with 1024 maximum entries.
 *
 * Address bit allocation.
 * +------+------+--------+
 * | MTLB | STLB | Offset |
 * +------+------+--------+
 *
 * # of address bits | # of address bits | Page | Addressable | Total
 *          /        |          /        | size |  per one    | addressable
 * # of MTLB entries | # of STLB entries |      | MTLB entry  |
 * ------------------+-------------------+------+-------------+------------
 *       8 / 256     |      4 / 16       |  1MB |             |
 *                   |      8 / 256      | 64KB |     16MB    |    4GB
 *                   |     12 / 4096     |  4KB |             |
 * ------------------+-------------------+------+-------------+------------
 *      10 / 1024    |      6 / 64       | 64KB |             |
 *                   |     10 / 1024     |  4KB |      4MB    |    4GB
 */

/* Page size. */
#define MMU_PAGE_SIZE		4096

/* Master table definitions. */
#define MMU_MTLB_BITS		8
#define MMU_MTLB_ENTRY_NUM	(1 << MMU_MTLB_BITS)
#define MMU_MTLB_SIZE		(MMU_MTLB_ENTRY_NUM << 2)
#define MMU_MTLB_SHIFT		(32 - MMU_MTLB_BITS)
#define MMU_MTLB_MASK		(((1U << MMU_MTLB_BITS) - 1) << MMU_MTLB_SHIFT)

#if MMU_MTLB_BITS == 8
#	define MMU_MTLB_MODE	GCREG_MMU_CONFIGURATION_MODE_MODE1_K
#elif MMU_MTLB_BITS == 10
#	define MMU_MTLB_MODE	GCREG_MMU_CONFIGURATION_MODE_MODE4_K
#else
#	error Invalid MMU_MTLB_BITS.
#endif

#define MMU_MTLB_PRESENT_MASK	0x00000001
#define MMU_MTLB_EXCEPTION_MASK	0x00000002
#define MMU_MTLB_PAGE_SIZE_MASK	0x0000000C
#define MMU_MTLB_SLAVE_MASK	0xFFFFFFC0

#define MMU_MTLB_PRESENT	0x00000001
#define MMU_MTLB_EXCEPTION	0x00000002
#define MMU_MTLB_4K_PAGE	0x00000000

#define MMU_MTLB_ENTRY_VACANT	MMU_MTLB_EXCEPTION

/* Slave table definitions. */
#define MMU_STLB_BITS		12
#define MMU_STLB_ENTRY_NUM	(1 << MMU_STLB_BITS)
#define MMU_STLB_SIZE		(MMU_STLB_ENTRY_NUM << 2)
#define MMU_STLB_SHIFT		(32 - (MMU_MTLB_BITS + MMU_STLB_BITS))
#define MMU_STLB_MASK		(((1U << MMU_STLB_BITS) - 1) << MMU_STLB_SHIFT)

#define MMU_STLB_PRESENT_MASK	0x00000001
#define MMU_STLB_EXCEPTION_MASK	0x00000002
#define MMU_STLB_WRITEABLE_MASK	0x00000004
#define MMU_STLB_ADDRESS_MASK	0xFFFFF000

#define MMU_STLB_PRESENT	0x00000001
#define MMU_STLB_EXCEPTION	0x00000002
#define MMU_STLB_WRITEABLE	0x00000004

#define MMU_STLB_ENTRY_VACANT	MMU_STLB_EXCEPTION

/* Page offset definitions. */
#define MMU_OFFSET_BITS		(32 - MMU_MTLB_BITS - MMU_STLB_BITS)
#define MMU_OFFSET_MASK		((1U << MMU_OFFSET_BITS) - 1)

#define MMU_SAFE_ZONE_SIZE	64

/*
 * This structure defines two lists; one that defines a list of vacant
 * arenas ready to map and the other a list of already mapped arenas.
 */
struct mmu2darena {
	/* Master table index. */
	u32 mtlb;

	/* Slave table index. */
	u32 stlb;

	/* Number of pages. */
	u32 count;

	/* Mapped virtual pointer. */
	u32 address;

	/* Client's virtual pointer. */
	void *logical;

	/* Size of the mapped buffer. */
	unsigned int size;

	/* Page descriptor array. */
	struct page **pages;

	/* Next arena. */
	struct mmu2darena *next;
};

struct mmu2darenablock {
	struct mmu2darenablock *next;
};

/* Private internal structure shared between contexts. */
struct mmu2dprivate {
	/* Reference count. */
	int refcount;

	/* MMU enabled. */
	int enabled;

	/* Safe zone allocation. */
	struct gcpage safezone;

	/* Available page allocation arenas. */
	struct mmu2darenablock *arena_blocks;
	struct mmu2darena *arena_recs;
};

/* This structure defines a list of allocated slave tables. */
struct mmu2dstlb {
	/* Slave table allocation. */
	struct gcpage pages;

	/* Entries used. */
	u32 count;

	/* Next STLB descriptor. */
	struct mmu2dstlb *next;
};

struct mmu2dstlbblock {
	struct mmu2dstlbblock *next;
};

#define STLB_PREALLOC_SIZE	MMU_PAGE_SIZE
#define STLB_PREALLOC_COUNT \
	((STLB_PREALLOC_SIZE - sizeof(struct mmu2dstlbblock)) \
		/ sizeof(struct mmu2dstlb))

struct mmu2dcontext {
	struct mmu2dprivate *mmu;

	/* Slave table allocation available wrappers. */
	struct mmu2dstlbblock *slave_blocks;
	struct mmu2dstlb *slave_recs;

	/* Master table allocation. */
	struct gcpage master;
	struct mmu2dstlb **slave;

	/* Hardware pointer to the master table. */
	unsigned long physical;

	/* Page mapping tracking. */
	struct mmu2darena *vacant;
	struct mmu2darena *allocated;
};

struct mmu2dphysmem {
	/* Virtual pointer and offset of the first page to map. */
	u32 base;
	u32 offset;

	/* An array of physical addresses of the pages to map. */
	u32 count;
	pte_t *pages;

	/* 0 => system default. */
	int pagesize;
};

/*----------------------------------------------------------------------------*/

enum gcerror mmu2d_create_context(struct mmu2dcontext *ctxt);
enum gcerror mmu2d_destroy_context(struct mmu2dcontext *ctxt);
enum gcerror mmu2d_set_master(struct mmu2dcontext *ctxt);
enum gcerror mmu2d_map(struct mmu2dcontext *ctxt,
	struct mmu2dphysmem *mem, struct mmu2darena **mapped);
enum gcerror mmu2d_unmap(struct mmu2dcontext *ctxt,
	struct mmu2darena *mapped);
int mmu2d_flush(void *logical, u32 address, u32 size);
enum gcerror mmu2d_fixup(struct gcfixup *fixup, unsigned int *data);
void mmu2d_dump(struct mmu2dcontext *ctxt);

#endif
