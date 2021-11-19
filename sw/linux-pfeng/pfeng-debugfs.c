/*
 * Copyright 2018,2020-2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/debugfs.h>
#include <linux/slab.h>

#include "pfe_cfg.h"
#include "pfeng.h"
#include "fci.h"

#if defined(CONFIG_DEBUG_FS)

static u32 *msg_verbosity_ptr;

#define DEBUGFS_BUF_SIZE 4096

#define CREATE_DEBUGFS_ENTRY_TYPE(ename,var_type)				\
static int fn_##ename##_debug_show(struct seq_file *seq, void *v)		\
{										\
	pfe_##var_type##_t *var_##ename = seq->private;				\
	char *buf;								\
	int ret;								\
										\
	buf = kzalloc(DEBUGFS_BUF_SIZE, GFP_KERNEL);				\
	if (!buf) {								\
		seq_puts(seq, "no memory\n");					\
		return -ENOMEM;							\
	}									\
										\
	ret = pfe_##ename##_get_text_statistics(var_##ename, buf,		\
			DEBUGFS_BUF_SIZE, *msg_verbosity_ptr);			\
										\
	if (ret)								\
		seq_puts(seq, buf);						\
										\
	kfree(buf);								\
										\
	return 0;								\
}										\
										\
static int fn_##ename##_stats_open(struct inode *inode, struct file *file)	\
{										\
	return single_open(file, fn_##ename##_debug_show, inode->i_private);	\
}										\
										\
static const struct file_operations pfeng_##ename##_fops = {			\
	.open		= fn_##ename##_stats_open,				\
	.read		= seq_read,						\
	.llseek		= seq_lseek,						\
	.release	= single_release,					\
};

#ifdef PFE_CFG_PFE_MASTER
CREATE_DEBUGFS_ENTRY_TYPE(emac,emac);
CREATE_DEBUGFS_ENTRY_TYPE(l2br,l2br);
CREATE_DEBUGFS_ENTRY_TYPE(l2br_domain,l2br);
CREATE_DEBUGFS_ENTRY_TYPE(class,class);
CREATE_DEBUGFS_ENTRY_TYPE(bmu,bmu);
CREATE_DEBUGFS_ENTRY_TYPE(gpi,gpi);
CREATE_DEBUGFS_ENTRY_TYPE(tmu,tmu);
CREATE_DEBUGFS_ENTRY_TYPE(util,util);
CREATE_DEBUGFS_ENTRY_TYPE(fp,fp);
#endif
CREATE_DEBUGFS_ENTRY_TYPE(hif_chnl,hif_chnl);

#define ADD_DEBUGFS_ENTRY(ename, etype, parent, epriv, esav)			\
	{									\
		struct dentry *dptr;						\
										\
		if (!(dptr = debugfs_create_file(ename, S_IRUSR, parent,	\
					epriv, &pfeng_##etype##_fops))) {	\
			dev_err(dev, "debugfs file create failed\n");		\
			return -ENOMEM;						\
		} else {							\
			if (*esav)						\
				*esav = dptr;					\
		}								\
	}

int pfeng_debugfs_create(struct pfeng_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
#ifdef PFE_CFG_PFE_MASTER
	struct dentry *dsav = NULL;
#endif

	/* Create debugfs main directory if it doesn't exist yet */
	if (priv->dbgfs)
		return 0;

	msg_verbosity_ptr = &priv->msg_verbosity;

	priv->dbgfs = debugfs_create_dir(PFENG_DRIVER_NAME, NULL);

	if (!priv->dbgfs || IS_ERR(priv->dbgfs)) {
		priv->dbgfs = NULL;
		dev_err(dev, "debugfs create directory failed\n");
		return -ENOMEM;
	}

#ifdef PFE_CFG_PFE_MASTER
	ADD_DEBUGFS_ENTRY("class", class, priv->dbgfs, priv->pfe_platform->classifier, &dsav);
	ADD_DEBUGFS_ENTRY("l2br", l2br, priv->dbgfs, priv->pfe_platform->l2_bridge, &dsav);
	ADD_DEBUGFS_ENTRY("l2br_domain", l2br_domain, priv->dbgfs, priv->pfe_platform->l2_bridge, &dsav);
	ADD_DEBUGFS_ENTRY("bmu1", bmu, priv->dbgfs, priv->pfe_platform->bmu[0], &dsav);
	ADD_DEBUGFS_ENTRY("bmu2", bmu, priv->dbgfs, priv->pfe_platform->bmu[1], &dsav);
	ADD_DEBUGFS_ENTRY("egpi1", gpi, priv->dbgfs, priv->pfe_platform->gpi[0], &dsav);
	ADD_DEBUGFS_ENTRY("egpi2", gpi, priv->dbgfs, priv->pfe_platform->gpi[1], &dsav);
	ADD_DEBUGFS_ENTRY("egpi3", gpi, priv->dbgfs, priv->pfe_platform->gpi[2], &dsav);
	ADD_DEBUGFS_ENTRY("tmu", tmu, priv->dbgfs, priv->pfe_platform->tmu, &dsav);
	ADD_DEBUGFS_ENTRY("util", util, priv->dbgfs, priv->pfe_platform->util, &dsav);
	ADD_DEBUGFS_ENTRY("fp", fp, priv->dbgfs, priv->pfe_platform->classifier, &dsav);
	if (priv->emac[0].enabled)
		ADD_DEBUGFS_ENTRY("emac0", emac, priv->dbgfs, priv->pfe_platform->emac[0], &dsav);
	if (priv->emac[1].enabled)
		ADD_DEBUGFS_ENTRY("emac1", emac, priv->dbgfs, priv->pfe_platform->emac[1], &dsav);
	if (priv->emac[2].enabled)
		ADD_DEBUGFS_ENTRY("emac2", emac, priv->dbgfs, priv->pfe_platform->emac[2], &dsav);
#endif

	return 0;
}

int pfeng_debugfs_add_hif_chnl(struct pfeng_priv *priv, u32 idx)
{
	struct device *dev = &priv->pdev->dev;
	struct pfeng_hif_chnl *chnl = &priv->hif_chnl[idx];
	char fname[32];
	struct dentry *dsav = NULL;

	if (!priv->dbgfs)
		return -ENODEV;

	/* Don't create if particular HIF channel is disabled */
	if (chnl->status == PFENG_HIF_STATUS_DISABLED)
		return -ENODEV;

	/* create subdirectory 'chn%d' */
	scnprintf(fname, sizeof(fname), "hif%d", idx);

	/* add members to the subdirectory */
	ADD_DEBUGFS_ENTRY(fname, hif_chnl, priv->dbgfs, chnl->priv, &dsav);

	return 0;
}

void pfeng_debugfs_remove(struct pfeng_priv *priv)
{
	if (priv->dbgfs) {
		debugfs_remove_recursive(priv->dbgfs);
		priv->dbgfs = NULL;
	}
}

#else
int pfeng_debugfs_create(struct pfeng_priv *priv)
{
	struct device *dev = &priv->pdev->dev;

	dev_info(dev, "debugfs not supported\n");
	return 0;
}

void pfeng_debugfs_remove(struct pfeng_priv *priv) {}
#endif /* CONFIG_DEBUG_FS */
