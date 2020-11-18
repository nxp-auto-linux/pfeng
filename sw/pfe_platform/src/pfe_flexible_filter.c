/* =========================================================================
 *
 *  Copyright (c) 2020 Imagination Technologies Limited
 *  Copyright 2018-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
#include "pfe_cfg.h"
#include "oal.h"
#include "pfe_ct.h"
#include "pfe_class.h"

/**
* @brief Initializes the module
*/
/* Magic function - QNX runtime linker fails to find the rest of the functions if this
   one is not called from somewhere in the pfe_platform */
void pfe_flexible_filter_init(void)
{
	;
}

/**
 * @brief Configures the Flexible Filter
 * @param[in] class The classifier instance
 * @param[in] dmem_addr Address of the  flexible parser table to be used as filter. Value 0 to disable the filter.
 * @return Either EOK or error code.
 */
errno_t pfe_flexible_filter_set(pfe_class_t *class, const uint32_t dmem_addr)
{
	pfe_ct_class_mmap_t mmap;
	errno_t ret = EOK;
    uint32_t ff_addr;
    uint32_t ff = dmem_addr;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

    /* Get the memory map */
	/* All PEs share the same memory map therefore we can read
	   arbitrary one (in this case 0U) */
	ret = pfe_class_get_mmap(class, 0U, &mmap);
	if(EOK == ret)
	{
        /* Get the flexible filter address */
        ff_addr = oal_ntohl(mmap.flexible_filter);
        /* Write new address of flexible filter */
        ret = pfe_class_write_dmem(class, -1, (void *)(addr_t)ff_addr, &ff, sizeof(pfe_ct_flexible_filter_t));
    }
    return ret;
}
