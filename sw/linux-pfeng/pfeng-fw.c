/*
 * Copyright 2018-2020 NXP
 *
 * SPDX-License-Identifier:     BSD OR GPL-2.0
 *
 */

#include <linux/firmware.h>

#include "pfe_cfg.h"
#include "pfeng.h"

#ifdef OPT_FW_EMBED
#include "fw-s32g-class.h"
#include "fw-s32g-util.h"
#endif

static int pfeng_fw_load_file(struct device *dev, const char *name, void **data, u32 *len)
{
	int ret;
	const struct firmware *entry;

	ret = request_firmware(&entry, name, dev);
	if(ret < 0) {
		dev_err(dev, "Firmware not available: %s\n", name);
		return ret;
	}

	if(!entry->size) {
		dev_err(dev, "Firmware file is empty: %s\n", name);
		goto end;
	}

	*data = kmalloc(entry->size, GFP_KERNEL);
	if(IS_ERR(*data)) {
		dev_err(dev, "Failed to alloc fw data memory\n");
		ret = IS_ERR(*data);
		goto end;
	}
	*len = entry->size;
	memcpy(*data, (char *)entry->data, entry->size);

end:
	release_firmware(entry);

	return ret;
}

int pfeng_fw_load(struct pfeng_priv *priv, const char *class_name, const char *util_name)
{
	struct device *dev = &priv->pdev->dev;
	pfe_fw_t *fw;
	int ret;
	bool enable_util = priv->cfg->enable_util;

	fw = kzalloc(sizeof(*fw), GFP_KERNEL);
	if(IS_ERR(fw)) {
		dev_err(dev, "Failed to alloc fw memory\n");
		return -ENOMEM;
	}

	priv->cfg->fw = fw;

#ifdef OPT_FW_EMBED
	fw->class_data = __fw_class_s32g_elf_bin;
	fw->class_size = __fw_class_s32g_elf_len;
	fw->util_data = __fw_util_s32g_elf_bin;
	fw->util_size = __fw_util_s32g_elf_len;
#else
	/* load CLASS fw */
	ret = pfeng_fw_load_file(dev, class_name, &fw->class_data, &fw->class_size);
	if (ret)
		goto err;

	/* load UTIL fw */
	if (enable_util) {
		ret = pfeng_fw_load_file(dev, util_name, &fw->util_data, &fw->util_size);
		if (ret)
			goto err;
	}
#endif

	dev_info(dev, "Firmware: CLASS %s [%d bytes]\n", class_name, fw->class_size);
	if (enable_util)
		dev_info(dev, "Firmware: UTIL %s [%d bytes]\n", util_name, fw->util_size);

end:
	return ret;

err:
	pfeng_fw_free(priv);
	goto end;
}

void pfeng_fw_free(struct pfeng_priv *priv)
{
	pfe_fw_t *fw = priv->cfg->fw;

#ifndef OPT_FW_EMBED
	if(fw->class_data) {
		kfree(fw->class_data);
		fw->class_data = NULL;
	}

	if(fw->util_data) {
		kfree(fw->util_data);
		fw->util_data = NULL;
	}
#endif

	priv->cfg->fw = NULL;

	kfree(fw);
}
