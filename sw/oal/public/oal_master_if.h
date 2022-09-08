/* =========================================================================
 *  Copyright 2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * 
 * @file		oal_master_if.h
 * @brief		Master/Slave HIF master interface header file
 * @details		Use this header to include all the OS-dependent HIF master interface transparency
 *
 */

#ifndef OAL_MASTER_IF_H_
#define OAL_MASTER_IF_H_

/*
 * QNX
 *
 */
#ifdef PFE_CFG_TARGET_OS_QNX

#define OAL_PFE_CFG_MASTER_IF PFE_CFG_MASTER_IF

/*
 * LINUX
 *
 */
#elif defined(PFE_CFG_TARGET_OS_LINUX)

/* get_pfeng_pfe_cfg_master_if from pfeng driver M/S */
extern uint32_t get_pfeng_pfe_cfg_master_if(void);
#define OAL_PFE_CFG_MASTER_IF get_pfeng_pfe_cfg_master_if()

/*
 * AUTOSAR
 *
 */
#elif defined(PFE_CFG_TARGET_OS_AUTOSAR)

#define OAL_PFE_CFG_MASTER_IF PFE_CFG_MASTER_IF

/*
 * BARE METAL
 * 
 */
#elif defined(PFE_CFG_TARGET_OS_BARE)

#define OAL_PFE_CFG_MASTER_IF PFE_CFG_MASTER_IF

/*
 * unknown OS
 *
 */
#else
#error "PFE_CFG_TARGET_OS_xx was not set!"
#endif /* PFE_CFG_TARGET_OS_xx */

#endif /* OAL_MASTER_IF_H_ */
