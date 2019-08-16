/* =========================================================================
 *  Copyright 2018-2019 NXP
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ========================================================================= */

/**
 * @addtogroup	dxgrPFE_PLATFORM
 * @{
 * 
 * @defgroup    dxgr_PFE_GPI GPI
 * @brief		The Generic Packet Interface
 * @details     This is the software representation of the GPI block.
 * 
 * @addtogroup  dxgr_PFE_GPI
 * @{
 * 
 * @file		pfe_gpi.h
 * @brief		The GPI module header file.
 * @details		This file contains GPI-related API.
 *
 */

#ifndef PUBLIC_PFE_GPI_H_
#define PUBLIC_PFE_GPI_H_

typedef struct __pfe_gpi_tag pfe_gpi_t;

typedef struct
{
	uint32_t alloc_retry_cycles;		/* Number of system clock cycles, the state machine has to wait before retrying in case the buffers are full at the buffer manager */
	uint32_t gpi_tmlf_txthres;		/* */
	uint32_t gpi_dtx_aseq_len;		/* */
	bool_t emac_1588_ts_en;		/* If TRUE then the 1588 time-stamping is enabled */
} pfe_gpi_cfg_t;

pfe_gpi_t *pfe_gpi_create(void *cbus_base_va, void *gpi_base, pfe_gpi_cfg_t *cfg);
void pfe_gpi_enable(pfe_gpi_t *gpi);
void pfe_gpi_reset(pfe_gpi_t *gpi);
void pfe_gpi_disable(pfe_gpi_t *gpi);
uint32_t pfe_gpi_get_text_statistics(pfe_gpi_t *gpi, char_t *buf, uint32_t buf_len, uint8_t verb_level);
void pfe_gpi_destroy(pfe_gpi_t *gpi);

#endif /* PUBLIC_PFE_GPI_H_ */

/** @}*/
/** @}*/
