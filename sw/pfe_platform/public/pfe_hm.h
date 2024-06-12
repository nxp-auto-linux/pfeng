/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2022-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PFE_HM_H
#define PFE_HM_H

#define PFE_HM_DESCRIPTION_MAX_LEN 256
#define PFE_HM_QUEUE_LEN 8

typedef enum {
	HM_INFO = 0,
	HM_WARNING = 1,
	HM_ERROR = 2
} pfe_hm_type_t;

typedef enum {
	HM_EVT_NONE = 0,
	HM_EVT_RUNTIME = 1,

#ifndef PFE_CFG_PFE_SLAVE
	HM_EVT_ECC = 2,

	HM_EVT_WDT_BMU1 = 10,
	HM_EVT_WDT_BMU2 = 11,
	HM_EVT_WDT_CLASS = 12,
	HM_EVT_WDT_EMAC0_GPI = 13,
	HM_EVT_WDT_EMAC1_GPI = 14,
	HM_EVT_WDT_EMAC2_GPI = 15,
	HM_EVT_WDT_HIF_GPI = 16,
	HM_EVT_WDT_HIF_NOCPY = 17,
	HM_EVT_WDT_HIF = 18,
	HM_EVT_WDT_TLITE = 19,
	HM_EVT_WDT_UTIL_PE = 20,
	HM_EVT_WDT_EMAC0_ETGPI = 21,
	HM_EVT_WDT_EMAC1_ETGPI = 22,
	HM_EVT_WDT_EMAC2_ETGPI = 23,
	HM_EVT_WDT_EXT_GPT1 = 24,
	HM_EVT_WDT_EXT_GPT2 = 25,
	HM_EVT_WDT_LMEM = 26,
	HM_EVT_WDT_ROUTE_LMEM = 27,

	HM_EVT_EMAC_ECC_TX_FIFO_CORRECTABLE = 30,
	HM_EVT_EMAC_ECC_TX_FIFO_UNCORRECTABLE = 31,
	HM_EVT_EMAC_ECC_TX_FIFO_ADDRESS = 32,
	HM_EVT_EMAC_ECC_RX_FIFO_CORRECTABLE = 33,
	HM_EVT_EMAC_ECC_RX_FIFO_UNCORRECTABLE = 34,
	HM_EVT_EMAC_ECC_RX_FIFO_ADDRESS = 35,
	HM_EVT_EMAC_APP_TX_PARITY = 36,
	HM_EVT_EMAC_APP_RX_PARITY = 37,
	HM_EVT_EMAC_MTL_PARITY = 38,
	HM_EVT_EMAC_FSM_PARITY = 39,
	HM_EVT_EMAC_FSM_TX_TIMEOUT = 40,
	HM_EVT_EMAC_FSM_RX_TIMEOUT = 41,
	HM_EVT_EMAC_FSM_APP_TIMEOUT = 42,
	HM_EVT_EMAC_FSM_PTP_TIMEOUT = 43,
	HM_EVT_EMAC_MASTER_TIMEOUT = 44,

	HM_EVT_BUS_MASTER1 = 60,
	HM_EVT_BUS_MASTER2 = 61,
	HM_EVT_BUS_MASTER3 = 62,
	HM_EVT_BUS_MASTER4 = 63,
	HM_EVT_BUS_HGPI_READ = 64,
	HM_EVT_BUS_HGPI_WRITE = 65,
	HM_EVT_BUS_EMAC0_READ = 66,
	HM_EVT_BUS_EMAC0_WRITE = 67,
	HM_EVT_BUS_EMAC1_READ = 68,
	HM_EVT_BUS_EMAC1_WRITE = 69,
	HM_EVT_BUS_EMAC2_READ = 70,
	HM_EVT_BUS_EMAC2_WRITE = 71,
	HM_EVT_BUS_CLASS_READ = 72,
	HM_EVT_BUS_CLASS_WRITE = 73,
	HM_EVT_BUS_HIF_NOCPY_READ = 74,
	HM_EVT_BUS_HIF_NOCPY_WRITE = 75,
	HM_EVT_BUS_TMU = 76,
	HM_EVT_BUS_FET = 77,
	HM_EVT_BUS_UTIL_PE_READ = 78,
	HM_EVT_BUS_UTIL_PE_WRITE = 79,

	HM_EVT_PARITY_MASTER1 = 100,
	HM_EVT_PARITY_MASTER2 = 101,
	HM_EVT_PARITY_MASTER3 = 102,
	HM_EVT_PARITY_MASTER4 = 103,
	HM_EVT_PARITY_EMAC_CBUS = 104,
	HM_EVT_PARITY_EMAC_DBUS = 105,
	HM_EVT_PARITY_CLASS_CBUS = 106,
	HM_EVT_PARITY_CLASS_DBUS = 107,
	HM_EVT_PARITY_TMU_CBUS = 108,
	HM_EVT_PARITY_TMU_DBUS = 109,
	HM_EVT_PARITY_HIF_CBUS = 110,
	HM_EVT_PARITY_HIF_DBUS = 111,
	HM_EVT_PARITY_HIF_NOCPY_CBUS = 112,
	HM_EVT_PARITY_HIF_NOCPY_DBUS = 113,
	HM_EVT_PARITY_UPE_CBUS = 114,
	HM_EVT_PARITY_UPE_DBUS = 115,
	HM_EVT_PARITY_HRS_CBUS = 116,
	HM_EVT_PARITY_BRIDGE_CBUS = 117,
	HM_EVT_PARITY_EMAC_SLV = 118,
	HM_EVT_PARITY_BMU1_SLV = 119,
	HM_EVT_PARITY_BMU2_SLV = 120,
	HM_EVT_PARITY_CLASS_SLV = 121,
	HM_EVT_PARITY_HIF_SLV = 122,
	HM_EVT_PARITY_HIF_NOCPY_SLV = 123,
	HM_EVT_PARITY_LMEM_SLV = 124,
	HM_EVT_PARITY_TMU_SLV = 125,
	HM_EVT_PARITY_UPE_SLV = 126,
	HM_EVT_PARITY_WSP_GLOBAL_SLV = 127,
	HM_EVT_PARITY_GPT1_SLV = 128,
	HM_EVT_PARITY_GPT2_SLV = 129,
	HM_EVT_PARITY_ROUTE_LMEM_SLV = 130,

	HM_EVT_FAIL_STOP_PARITY = 140,
	HM_EVT_FAIL_STOP_WATCHDOG = 141,
	HM_EVT_FAIL_STOP_BUS = 142,
	HM_EVT_FAIL_STOP_ECC_MULTIBIT = 143,
	HM_EVT_FAIL_STOP_FW = 144,
	HM_EVT_FAIL_STOP_HOST = 145,

	HM_EVT_FW_FAIL_STOP = 150,
	HM_EVT_HOST_FAIL_STOP = 151,

	HM_EVT_BMU_FULL = 170,
	HM_EVT_BMU_FREE_ERR = 171,
	HM_EVT_BMU_MCAST = 172,
#endif

	HM_EVT_PE_STALL = 180,
	HM_EVT_PE_EXCEPTION = 181,
	HM_EVT_PE_ERROR = 182,

	HM_EVT_HIF_ERR = 190,
	HM_EVT_HIF_TX_FIFO = 191,
	HM_EVT_HIF_RX_FIFO = 192,
} pfe_hm_evt_t;

typedef enum {
	HM_SRC_UNKNOWN = 0,
	HM_SRC_DRIVER = 1,
	HM_SRC_WDT = 2,
	HM_SRC_EMAC0 = 3,
	HM_SRC_EMAC1 = 4,
	HM_SRC_EMAC2 = 5,
	HM_SRC_BUS = 6,
	HM_SRC_PARITY = 7,
	HM_SRC_FAIL_STOP = 8,
	HM_SRC_FW_FAIL_STOP = 9,
	HM_SRC_HOST_FAIL_STOP = 10,
	HM_SRC_ECC = 11,
	HM_SRC_PE_CLASS = 12,
	HM_SRC_PE_UTIL = 13,
	HM_SRC_PE_TMU = 14,
	HM_SRC_HIF = 15,
	HM_SRC_BMU = 16,
} pfe_hm_src_t;

typedef struct {
	pfe_hm_type_t type;
	pfe_hm_src_t src;
	pfe_hm_evt_t id;
#ifdef NXP_LOG_ENABLED
	char descr[PFE_HM_DESCRIPTION_MAX_LEN];
#endif /* NXP_LOG_ENABLED */
} pfe_hm_item_t;

typedef void (* pfe_hm_cb_t)(pfe_hm_item_t *item);

errno_t pfe_hm_init(void);
errno_t pfe_hm_destroy(void);
void pfe_hm_report(pfe_hm_src_t src, pfe_hm_type_t type, pfe_hm_evt_t id, pfe_hm_log_t hm_log,
		const char *format, ...);
errno_t pfe_hm_get(pfe_hm_item_t *item);
const char *pfe_hm_get_event_str(pfe_hm_evt_t id);
const char *pfe_hm_get_src_str(pfe_hm_src_t src);
bool_t pfe_hm_register_event_cb(pfe_hm_cb_t cb);

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define pfe_hm_report_info(src, id, format, ...) \
	pfe_hm_report((src), HM_INFO, (id), (pfe_hm_log_t){.log_type = NXP_LOG_TYPE_PFE}, "[%s:%d] " format, __FILENAME__, __LINE__, ##__VA_ARGS__)
#define pfe_hm_report_warning(src, id, format, ...) \
	pfe_hm_report((src), HM_WARNING, (id), (pfe_hm_log_t){.log_type = NXP_LOG_TYPE_PFE}, "[%s:%d] " format, __FILENAME__, __LINE__, ##__VA_ARGS__)
#define pfe_hm_report_error(src, id, format, ...) \
	pfe_hm_report((src), HM_ERROR, (id), (pfe_hm_log_t){.log_type = NXP_LOG_TYPE_PFE}, "[%s:%d] " format, __FILENAME__, __LINE__, ##__VA_ARGS__)

#define pfe_hm_report_dev_info(src, id, dev, format, ...) \
	pfe_hm_report((src), HM_INFO, (id), (pfe_hm_log_t){.log_type = NXP_LOG_TYPE_DEV, .log_dev = dev}, "[%s:%d] " format, __FILENAME__, __LINE__, ##__VA_ARGS__)
#define pfe_hm_report_dev_warning(src, id, dev, format, ...) \
	pfe_hm_report((src), HM_WARNING, (id), (pfe_hm_log_t){.log_type = NXP_LOG_TYPE_DEV, .log_dev = dev}, "[%s:%d] " format, __FILENAME__, __LINE__, ##__VA_ARGS__)
#define pfe_hm_report_dev_error(src, id, dev, format, ...) \
	pfe_hm_report((src), HM_ERROR, (id), (pfe_hm_log_t){.log_type = NXP_LOG_TYPE_DEV, .log_dev = dev}, "[%s:%d] " format, __FILENAME__, __LINE__, ##__VA_ARGS__)

#define pfe_hm_report_netdev_info(src, id, dev, format, ...) \
	pfe_hm_report((src), HM_INFO, (id), (pfe_hm_log_t){.log_type = NXP_LOG_TYPE_NETDEV, .log_netdev = dev}, "[%s:%d] " format, __FILENAME__, __LINE__, ##__VA_ARGS__)
#define pfe_hm_report_netdev_warning(src, id, dev, format, ...) \
	pfe_hm_report((src), HM_WARNING, (id), (pfe_hm_log_t){.log_type = NXP_LOG_TYPE_NETDEV, .log_netdev = dev}, "[%s:%d] " format, __FILENAME__, __LINE__, ##__VA_ARGS__)
#define pfe_hm_report_netdev_error(src, id, dev, format, ...) \
	pfe_hm_report((src), HM_ERROR, (id), (pfe_hm_log_t){.log_type = NXP_LOG_TYPE_NETDEV, .log_netdev = dev}, "[%s:%d] " format, __FILENAME__, __LINE__, ##__VA_ARGS__)

#endif /* PFE_HM_H */
