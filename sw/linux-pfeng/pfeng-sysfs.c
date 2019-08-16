/*
 * Copyright 2018-2019 NXP
 *
 * SPDX-License-Identifier:     BSD OR GPL-2.0
 *
 */

#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/fs.h>

#include "pfeng.h"

#ifdef CONFIG_SYSFS

static struct pfeng_priv *__priv;

static ssize_t pfe_class_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(!__priv)
		return 0;

	return pfe_class_get_text_statistics(__priv->pfe->classifier, buf, PAGE_SIZE, 9);
}

static ssize_t pfe_tmu_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(!__priv)
		return 0;

	return pfe_tmu_get_text_statistics(__priv->pfe->tmu, buf, PAGE_SIZE, 9);
}

static ssize_t pfe_util_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(!__priv)
		return 0;

	return pfe_util_get_text_statistics(__priv->pfe->util, buf, PAGE_SIZE, 9);
}

static ssize_t pfe_bmu_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0, ii;

	if(!__priv)
		return 0;

	for (ii=0; ii<__priv->pfe->bmu_count; ii++) {
		len += scnprintf(buf + len, PAGE_SIZE, "BMU[%d]:\n", ii);
		if((PAGE_SIZE - len) < 8)
			break; /* so small room for debug data */
		len += pfe_bmu_get_text_statistics(__priv->pfe->bmu[ii], buf + len, PAGE_SIZE - len, 9);
	}

	return len;
}

static ssize_t pfe_hif_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(!__priv)
		return 0;

	return pfe_hif_get_text_statistics(__priv->pfe->hif, buf, PAGE_SIZE, 9);
}

static ssize_t pfe_gpi_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0, ii;

	if(!__priv)
		return 0;

	/*	GPI */
	for (ii=0; ii<__priv->pfe->gpi_count; ii++) {
		len += scnprintf(buf + len, PAGE_SIZE, "GPI[%d]:\n", ii);
		if((PAGE_SIZE - len) < 8)
			break; /* so small room for debug data */
		len += pfe_gpi_get_text_statistics(__priv->pfe->gpi[ii], buf, PAGE_SIZE, 9);
	}

	/*	HGPI */
	for (ii=0; ii<__priv->pfe->hgpi_count; ii++) {
		len += scnprintf(buf + len, PAGE_SIZE, "HGPI[%d]:\n", ii);
		if((PAGE_SIZE - len) < 8)
			break; /* so small room for debug data */
		len += pfe_gpi_get_text_statistics(__priv->pfe->hgpi[ii], buf, PAGE_SIZE, 9);
	}

	return len;
}

static ssize_t pfe_emac1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(!__priv) {
		NXP_LOG_ERROR("Failed to reach platform!!!\n");
		return 0;
	}

	if(!test_bit(PFENG_STATE_NAPI_IF0_INDEX, &__priv->state)) {
		NXP_LOG_ERROR("The interface 0 is down\n");
		return 0;
	}

	return pfe_emac_get_text_statistics(__priv->pfe->emac[0], buf, PAGE_SIZE, 9);
}

static ssize_t pfe_emac2_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(!__priv) {
		NXP_LOG_ERROR("Failed to reach platform!!!\n");
		return 0;
	}

	if(!test_bit(PFENG_STATE_NAPI_IF1_INDEX, &__priv->state)) {
		NXP_LOG_ERROR("The interface 1 is down\n");
		return 0;
	}

	return pfe_emac_get_text_statistics(__priv->pfe->emac[1], buf, PAGE_SIZE, 9);
}


static ssize_t pfe_emac3_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(!__priv) {
		NXP_LOG_ERROR("Failed to reach platform!!!\n");
		return 0;
	}

	if(!test_bit(PFENG_STATE_NAPI_IF2_INDEX, &__priv->state)) {
		NXP_LOG_ERROR("The interface 2 is down\n");
		return 0;
	}

	return pfe_emac_get_text_statistics(__priv->pfe->emac[2], buf, PAGE_SIZE, 9);
}

static ssize_t pfe_clrings_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;

	if(!__priv)
		return 0;

	pfe_hif_drv_show_ring_status(__priv->hif, TRUE, FALSE);

	len = scnprintf(buf, PAGE_SIZE, "rx status done\n");

	return len;
}

extern void pfe_hif_chnl_dump_ring(pfe_hif_chnl_t *chnl, bool_t dump_rx, bool_t dump_tx);

static ssize_t pfe_hifring_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;

	if(!__priv)
		return 0;

	pfe_hif_chnl_dump_ring(__priv->channel, TRUE, TRUE);

	len = scnprintf(buf, PAGE_SIZE, "tx status done\n");

	return len;
}

/*
   Debug print interesting only for PFE TLM development.

   Note: Will be REMOVED when TLM gets released.
*/
static ssize_t pfe_ifs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;
	int id;

	if(!__priv)
		return 0;

	/* The sysfs API defines 'buf' size as PAGE_SIZE */

	for(id = 0; id < PFENG_PHY_PORT_NUM; id++) {

		pfe_phy_if_t *phy_if;
		pfe_log_if_t *log_if = pfe_platform_get_log_if_by_id(__priv->pfe, id);
		if(!log_if)
			continue;

		len += pfe_log_if_get_text_statistics(log_if, buf + len, PAGE_SIZE - len, 9);
		if(len >= PAGE_SIZE)
			break;

		phy_if = pfe_log_if_get_parent(log_if);
		if(!phy_if)
			continue;

		len += pfe_phy_if_get_text_statistics(phy_if, buf + len, PAGE_SIZE - len, 9);
		if(len >= PAGE_SIZE)
			break;
	}

	/* hex dump of cf area */
	{
		pfe_ct_pe_mmap_t pfe_pe_mmap;

		len += scnprintf(buf + len, PAGE_SIZE - len, "[CF area, size %ld bytes]\n", sizeof(pfe_pe_mmap));
		if (EOK != pfe_class_get_mmap(__priv->pfe->classifier, 0U, &pfe_pe_mmap))
			len += scnprintf(buf + len, PAGE_SIZE - len, "Error: Could not get memory map\n");
		else {
			int i;

			for(i = 0; i < sizeof(pfe_pe_mmap); i += 16) {
				hex_dump_to_buffer(((void *)&pfe_pe_mmap) + i, sizeof(pfe_pe_mmap), 16, 1, buf + len, PAGE_SIZE - len, true);
				len += strlen(buf + len);
				len += scnprintf(buf + len, PAGE_SIZE - len, "\n");
			}
		}
	}

	return len;
}

static DEVICE_ATTR(class, S_IRUGO, pfe_class_show, NULL);
static DEVICE_ATTR(tmu, S_IRUGO, pfe_tmu_show, NULL);
static DEVICE_ATTR(util, S_IRUGO, pfe_util_show, NULL);
static DEVICE_ATTR(bmu, S_IRUGO, pfe_bmu_show, NULL);
static DEVICE_ATTR(hif, S_IRUGO, pfe_hif_show, NULL);
static DEVICE_ATTR(gpi, S_IRUGO, pfe_gpi_show, NULL);
static DEVICE_ATTR(emac1, S_IRUGO, pfe_emac1_show, NULL);
static DEVICE_ATTR(emac2, S_IRUGO, pfe_emac2_show, NULL);
static DEVICE_ATTR(emac3, S_IRUGO, pfe_emac3_show, NULL);
static DEVICE_ATTR(ifs, S_IRUGO, pfe_ifs_show, NULL);

static DEVICE_ATTR(clrings, S_IRUGO, pfe_clrings_show, NULL);
static DEVICE_ATTR(hifring, S_IRUGO, pfe_hifring_show, NULL);

static struct attribute *pfe_drv_attrs[] = {
	&dev_attr_class.attr,
	&dev_attr_tmu.attr,
	&dev_attr_util.attr,
	&dev_attr_bmu.attr,
	&dev_attr_hif.attr,
	&dev_attr_gpi.attr,
	&dev_attr_emac1.attr,
	&dev_attr_emac2.attr,
	&dev_attr_emac3.attr,
	&dev_attr_ifs.attr,

	&dev_attr_clrings.attr,
	&dev_attr_hifring.attr,

	NULL
};

static const struct attribute_group pfe_drv_group = {
	.name = PFENG_DRIVER_NAME,
	.attrs = pfe_drv_attrs,
};

int pfeng_sysfs_init(struct pfeng_priv *priv)
{
	int ret;

	/* add sysfs entries (kernel_kobj is the global symbol) */
	ret = sysfs_create_group(kernel_kobj, &pfe_drv_group);
	if (ret)
		return ret;

	__priv = priv;

	return 0;
}

void pfeng_sysfs_exit(struct pfeng_priv *priv)
{
	if(__priv != NULL)
		sysfs_remove_group(kernel_kobj, &pfe_drv_group);
}

#else
int pfeng_sysfs_init(struct pfeng_priv *priv) { return 0; }
void pfeng_sysfs_exit(struct pfeng_priv *priv) {}
#endif /* CONFIG_SYSFS */
