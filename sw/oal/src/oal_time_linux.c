/* =========================================================================
 *  Copyright 2018-2021,2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_OAL_TIME
 * @{
 * 
 * @file		oal_time_linux.c
 * @brief		The oal_time module source file (Linux).
 * @details		This file contains Linux-specific time management implementation.
 *
 */

#include <linux/delay.h>
#include <linux/ktime.h>
#include "pfe_cfg.h"

/*
   See in kernel documentation "Documentation/timers/timers-howto.txt"
   for more detailed info
*/

#define MSEC 1000U

void oal_time_usleep(uint32_t usec)
{
	if(usec <= 10U)
		/* less then 10 usec = use udelay (busywait!) */
		udelay(usec);
	else if(usec <= (10U * MSEC))
		usleep_range(usec < 100U ? 0U : (usec - 100U), usec + 50U);
	else
		/* more then 10ms = use msleep_int */
		(void)msleep_interruptible(usec / MSEC);
}

void oal_time_msleep(uint32_t msec)
{
	oal_time_usleep(msec * MSEC);
}

void oal_time_udelay(uint32_t usec)
{
	udelay(usec);
}

void oal_time_mdelay(uint32_t msec)
{
	mdelay(msec);
}

uint64_t oal_time_gettime(void)
{
	ktime_t time;
	
	time = ktime_get();
	
	return (uint64_t)ktime_to_ms(time);
}

/** @}*/
