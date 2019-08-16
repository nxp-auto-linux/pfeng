/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:     BSD OR GPL-2.0
 *
 */

#include <linux/firmware.h>
#include <linux/debugfs.h>

#include "pfeng.h"

#ifdef CONFIG_DEBUG_FS

int pfeng_debugfs_init(struct pfeng_priv *priv)
{

    /* Create debugfs main directory if it doesn't exist yet */
    if (!priv->dbgfs_dir) {
        priv->dbgfs_dir = debugfs_create_dir(PFENG_DRIVER_NAME, NULL);

        if (!priv->dbgfs_dir || IS_ERR(priv->dbgfs_dir)) {
            pr_err("ERROR %s, debugfs create directory failed\n",
                   PFENG_DRIVER_NAME);

            return -ENOMEM;
        }
    }

    return 0;
}

void pfeng_debugfs_exit(struct pfeng_priv *priv)
{
    debugfs_remove_recursive(priv->dbgfs_dir);
}

#else
int pfeng_debugfs_init(struct pfeng_priv *priv) { return 0; }
void pfeng_debugfs_exit(struct pfeng_priv *priv) {}
#endif /* CONFIG_DEBUG_FS */
