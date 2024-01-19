/* =========================================================================
 *  Copyright 2018-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef __OAL_TYPES_LINUX__
#define __OAL_TYPES_LINUX__

#include <linux/kernel.h>
#include <linux/types.h>

#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/netdevice.h>

/* get device from oal_mm_linux */
extern struct device *oal_mm_get_dev(void);
/* msg_verbosity from pfeng driver M/S */
extern int msg_verbosity;

#define __STR_HELPER(x) #x
#define __STR(x) __STR_HELPER(x)

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define NXP_LOG_ENABLED

#define NXP_LOG_RAW_WARNING(format, ...) { \
	struct device *dev = oal_mm_get_dev(); \
	if (dev) { \
		if (msg_verbosity >= 7) { \
			dev_warn(dev, "[%s:"__STR(__LINE__)"] " format, __FILENAME__, ##__VA_ARGS__); \
		} else { \
			dev_warn(dev, format, ##__VA_ARGS__); \
		} \
	} \
}

#define NXP_LOG_RAW_ERROR(format, ...) { \
	struct device *dev = oal_mm_get_dev(); \
	if (dev) { \
		if (msg_verbosity >= 7) { \
			dev_err(dev, "ERR: [%s:"__STR(__LINE__)"] " format, __FILENAME__, ##__VA_ARGS__); \
		} else { \
			dev_err(dev, "ERR: " format, ##__VA_ARGS__); \
		} \
	} \
}

#define NXP_LOG_RAW_INFO(format, ...) { \
	struct device *dev = oal_mm_get_dev(); \
	if (dev) { \
		if (msg_verbosity >= 7) { \
			dev_info(dev, "[%s:"__STR(__LINE__)"] " format, __FILENAME__, ##__VA_ARGS__); \
		} else { \
			dev_info(dev, format, ##__VA_ARGS__); \
		} \
	} \
}

#define NXP_LOG_RAW_DEBUG(format, ...) { \
	struct device *dev = oal_mm_get_dev(); \
	if (dev) { \
		if (msg_verbosity >= 7) { \
			dev_dbg(dev, "[%s:"__STR(__LINE__)"] " format, __FILENAME__, ##__VA_ARGS__); \
		} else { \
			dev_dbg(dev, format, ##__VA_ARGS__); \
		} \
	} \
}

#define HM_MSG_RAW_DEV_ERR(dev, format, ...) dev_err(dev, format, ##__VA_ARGS__)
#define HM_MSG_RAW_DEV_WARN(dev, format, ...) dev_warn(dev, format, ##__VA_ARGS__)
#define HM_MSG_RAW_DEV_INFO(dev, format, ...) dev_info(dev, format, ##__VA_ARGS__)
#define HM_MSG_RAW_DEV_DBG(dev, format, ...) dev_dbg(dev, format, ##__VA_ARGS__)

#define HM_MSG_RAW_NETDEV_ERR(netdev, format, ...) netdev_err(netdev, format, ##__VA_ARGS__)
#define HM_MSG_RAW_NETDEV_WARN(netdev, format, ...) netdev_warn(netdev, format, ##__VA_ARGS__)
#define HM_MSG_RAW_NETDEV_INFO(netdev, format, ...) netdev_info(netdev, format, ##__VA_ARGS__)
#define HM_MSG_RAW_NETDEV_DBG(netdev, format, ...) netdev_dbg(netdev, format, ##__VA_ARGS__)

/* To be used from Healt Monitor pfe_hm_report() function */
#define NXP_LOG_HM_WARNING(format, ...) { \
	struct device *dev = oal_mm_get_dev(); \
	if (dev) { \
		dev_warn(dev, format, ##__VA_ARGS__); \
	} \
}

#define NXP_LOG_HM_ERROR(format, ...) { \
	struct device *dev = oal_mm_get_dev(); \
	if (dev) { \
		dev_err(dev, "ERR: " format, ##__VA_ARGS__); \
	} \
}

#define NXP_LOG_HM_INFO(format, ...) { \
	struct device *dev = oal_mm_get_dev(); \
	if (dev) { \
		dev_info(dev, format, ##__VA_ARGS__); \
	} \
}

#define HM_MSG_HM_DEV_ERR(dev, format, ...) { \
	if (dev) { \
		dev_err(dev, "ERR: " format, ##__VA_ARGS__); \
	} \
}

#define HM_MSG_HM_DEV_WARN(dev, format, ...) { \
	if (dev) { \
		dev_warn(dev, format, ##__VA_ARGS__); \
	} \
}

#define HM_MSG_HM_DEV_INFO(dev, format, ...) { \
	if (dev) { \
		dev_info(dev, format, ##__VA_ARGS__); \
	} \
}

#define HM_MSG_HM_NETDEV_ERR(dev, format, ...) { \
	if (dev) { \
		netdev_err(dev, "ERR: " format, ##__VA_ARGS__); \
	} \
}

#define HM_MSG_HM_NETDEV_WARN(dev, format, ...) { \
	if (dev) { \
		netdev_warn(dev, format, ##__VA_ARGS__); \
	} \
}

#define HM_MSG_HM_NETDEV_INFO(dev, format, ...) { \
	if (dev) { \
		netdev_info(dev, format, ##__VA_ARGS__); \
	} \
}

#if defined(PFE_CFG_TARGET_ARCH_i386)
typedef unsigned int addr_t;
#define PRINT64 "l"
#define PRINTADDR_T "x"
#elif defined(PFE_CFG_TARGET_ARCH_x86_64) || defined(PFE_CFG_TARGET_ARCH_aarch64)

#define MAX_ADDR_T_VAL UINT_MAX

typedef unsigned long long addr_t;
#define PRINT64 "ll"
#define PRINTADDR_T "llx"
#else
#error Unsupported or no platform defined
#endif

typedef int errno_t;
typedef bool bool_t;
typedef char char_t;
typedef int int_t; /* For use within printf like functions that require "int" regardless its size */
typedef unsigned int uint_t; /* For use within printf like functions */

typedef enum {
	NXP_LOG_TYPE_PFE,
	NXP_LOG_TYPE_DEV,
	NXP_LOG_TYPE_NETDEV
} pfe_hm_log_type_t;

typedef struct {
	pfe_hm_log_type_t log_type;
	union {
		struct device *log_dev;
		struct net_device *log_netdev;
	};
} pfe_hm_log_t;

#ifndef EOK
#define EOK 0
#endif

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */
#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

#ifndef NULL_ADDR
#define NULL_ADDR ((addr_t)0U)
#endif /* NULL_ADDR */

#define oal_htons(x)	htons(x)
#define oal_ntohs(x)	ntohs(x)
#define oal_htonl(x)	htonl(x)
#define oal_ntohl(x)	ntohl(x)

#ifndef ENOTSUP
#define ENOTSUP		EOPNOTSUPP
#endif

#include "pfe_hm.h"
#define NXP_LOG_ERROR(format, ...) pfe_hm_report_error(HM_SRC_DRIVER, HM_EVT_RUNTIME, format, ##__VA_ARGS__)
#define NXP_LOG_WARNING(format, ...) NXP_LOG_RAW_WARNING(format, ##__VA_ARGS__)
#define NXP_LOG_INFO(format, ...) NXP_LOG_RAW_INFO(format, ##__VA_ARGS__)
#define NXP_LOG_DEBUG(format, ...) NXP_LOG_RAW_DEBUG(format, ##__VA_ARGS__)

#define HM_MSG_DEV_ERR(dev, format, ...) pfe_hm_report_dev_error(HM_SRC_DRIVER, HM_EVT_RUNTIME, dev, format, ##__VA_ARGS__)
#define HM_MSG_DEV_WARN(dev, format, ...) HM_MSG_RAW_DEV_WARN(dev, format, ##__VA_ARGS__)
#define HM_MSG_DEV_INFO(dev, format, ...) HM_MSG_RAW_DEV_INFO(dev, format, ##__VA_ARGS__)
#define HM_MSG_DEV_DBG(dev, format, ...) HM_MSG_RAW_DEV_DBG(dev, format, ##__VA_ARGS__)

#define HM_MSG_NETDEV_ERR(netdev, format, ...) pfe_hm_report_netdev_error(HM_SRC_DRIVER, HM_EVT_RUNTIME, netdev, format, ##__VA_ARGS__)
#define HM_MSG_NETDEV_WARN(netdev, format, ...) HM_MSG_RAW_NETDEV_WARN(netdev, format, ##__VA_ARGS__)
#define HM_MSG_NETDEV_INFO(netdev, format, ...) HM_MSG_RAW_NETDEV_INFO(netdev, format, ##__VA_ARGS__)
#define HM_MSG_NETDEV_DBG(netdev, format, ...) HM_MSG_RAW_NETDEV_DBG(netdev, format, ##__VA_ARGS__)

#endif
