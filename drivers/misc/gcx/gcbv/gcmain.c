/*
 * gcmain.c
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

#include <linux/gcx.h>
#include <linux/gcbv.h>
#include "gcmain.h"
#include "gcbv-priv.h"

#ifndef GC_DUMP
#	define GC_DUMP 0
#endif

#if GC_DUMP
#	define GC_PRINT gcdump
#else
#	define GC_PRINT(...)
#endif

/*******************************************************************************
 * Convert floating point in 0..1 range to an 8-bit value in range 0..255.
 */

union gcfp {
	struct {
		unsigned int mantissa:23;
		unsigned int exponent:8;
		unsigned int sign:1;
	} comp;

	float value;
};

unsigned char gcfp2norm8(float value)
{
	union gcfp gcfp;
	int exponent;
	unsigned int mantissa;
	int shift;

	/* Get access to components. */
	gcfp.value = value;

	/* Clamp negatives. */
	if (gcfp.comp.sign)
		return 0;

	/* Get unbiased exponent. */
	exponent = (int) gcfp.comp.exponent - 127;

	/* Clamp if too large. */
	if (exponent >= 0)
		return 255;

	/* Clamp if too small. */
	if (exponent < -8)
		return 0;

	/* Determine the shift value. */
	shift = (23 - 8) - exponent;

	/* Compute the mantissa. */
	mantissa = (gcfp.comp.mantissa | 0x00800000) >> shift;

	/* Normalize. */
	mantissa = (mantissa * 255) >> 8;

	return (unsigned char) mantissa;
}

/*******************************************************************************
 * Device init/cleanup.
 */

static int __init mod_init(void)
{
	GC_PRINT(GC_INFO_MSG "\n", __func__, __LINE__);
	bv_init();
	gcbv_assign();
	return 0;
}

static void __exit mod_exit(void)
{
	GC_PRINT(GC_INFO_MSG "\n", __func__, __LINE__);
	gcbv_clear();
	bv_exit();
}

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("www.vivantecorp.com");
MODULE_AUTHOR("www.ti.com");
module_init(mod_init);
module_exit(mod_exit);
