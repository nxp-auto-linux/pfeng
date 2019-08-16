/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:     BSD OR GPL-2.0
 *
 */

#include <linux/firmware.h>

#include "pfeng.h"

#ifdef OPT_FW_EMBED
#include "fw-s32g-class.h"
#endif

int pfeng_fw_load(struct pfeng_priv *priv, char *name)
{
	const struct firmware *fw_entry;
	pfe_fw_t *fw;
	int ret;

	fw = kzalloc(sizeof(*fw), GFP_KERNEL);
	if(IS_ERR(fw)) {
		dev_err(priv->device,
                   "%s: Failed to alloc fw memory\n", __func__);
		return -ENOMEM;
	}

#ifdef OPT_FW_EMBED
	fw->class_data = __fw_class_s32g_elf_bin;
	fw->class_size = __fw_class_s32g_elf_len;
#else
	ret = request_firmware(&fw_entry, name, priv->device);
	if(ret < 0) {
		dev_err(priv->device, "Firmware not available: %s\n", name);
		goto err;
	}
	if(!fw_entry->size) {
		dev_err(priv->device, "Firmware file is empty: %s\n", name);
		goto err;
	}
	fw->class_size = fw_entry->size;
	fw->class_data = kmalloc(fw_entry->size, GFP_KERNEL);
	if(IS_ERR(fw->class_data)) {
		dev_err(priv->device, "Failed to alloc fw data memory\n");
		ret = IS_ERR(fw->class_data);
		goto err;
	}
	memcpy(fw->class_data, (char *)fw_entry->data, fw_entry->size);

	release_firmware(fw_entry);
#endif

	priv->fw = fw;

	dev_info(priv->device, "Firmware: %s [size: %d bytes]\n", name, fw->class_size);

end:
	return ret;

err:
	kfree(fw);
	goto end;

}

void pfeng_fw_free(struct pfeng_priv *priv)
{
	pfe_fw_t *fw = priv->fw;

#ifndef OPT_FW_EMBED
	if(fw->class_data) {
		kfree(fw->class_data);
		fw->class_data = NULL;
	}
#endif

	kfree(fw);
}
