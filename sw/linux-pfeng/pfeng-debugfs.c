/*
 * Copyright 2018,2020 NXP
 *
 * SPDX-License-Identifier: BSD OR GPL-2.0
 *
 */

#include <linux/debugfs.h>
#include <linux/slab.h>

#include "pfe_cfg.h"
#include "pfeng.h"

#if defined(CONFIG_DEBUG_FS)

/* enabled hif channels */
static u32 hif_chnl_mask = 0;
static u32 *msg_verbosity_ptr;

#define DEBUGFS_BUF_SIZE 4096

#define CREATE_DEBUGFS_ENTRY_TYPE(ename)					\
static int fn_##ename##_debug_show(struct seq_file *seq, void *v)		\
{										\
	pfe_##ename##_t *var_##ename = seq->private;				\
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
CREATE_DEBUGFS_ENTRY_TYPE(hif);
CREATE_DEBUGFS_ENTRY_TYPE(emac);
CREATE_DEBUGFS_ENTRY_TYPE(class);
CREATE_DEBUGFS_ENTRY_TYPE(bmu);
CREATE_DEBUGFS_ENTRY_TYPE(gpi);
CREATE_DEBUGFS_ENTRY_TYPE(tmu);
CREATE_DEBUGFS_ENTRY_TYPE(util);
#endif
CREATE_DEBUGFS_ENTRY_TYPE(hif_chnl);

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

#define ADD_DEBUGFS_CHNL_XSTATS_ENTRY(name)					\
	scnprintf(fname, sizeof(fname), "%s", TOSTRING(name));			\
	debugfs_create_u64(fname, 0444, ndev->chnl_sc.dentry, &ndev->xstats.name);

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
	ADD_DEBUGFS_ENTRY("class", class, priv->dbgfs, priv->pfe->classifier, &dsav);
	ADD_DEBUGFS_ENTRY("hif", hif, priv->dbgfs, priv->pfe->hif, &dsav);
	ADD_DEBUGFS_ENTRY("bmu1", bmu, priv->dbgfs, priv->pfe->bmu[0], &dsav);
	ADD_DEBUGFS_ENTRY("bmu2", bmu, priv->dbgfs, priv->pfe->bmu[1], &dsav);
	ADD_DEBUGFS_ENTRY("egpi1", gpi, priv->dbgfs, priv->pfe->gpi[0], &dsav);
	ADD_DEBUGFS_ENTRY("egpi2", gpi, priv->dbgfs, priv->pfe->gpi[1], &dsav);
	ADD_DEBUGFS_ENTRY("egpi3", gpi, priv->dbgfs, priv->pfe->gpi[2], &dsav);
	ADD_DEBUGFS_ENTRY("tmu", tmu, priv->dbgfs, priv->pfe->tmu, &dsav);
	ADD_DEBUGFS_ENTRY("util", util, priv->dbgfs, priv->pfe->util, &dsav);
	ADD_DEBUGFS_ENTRY("emac0", emac, priv->dbgfs, priv->pfe->emac[0], &dsav);
	ADD_DEBUGFS_ENTRY("emac1", emac, priv->dbgfs, priv->pfe->emac[1], &dsav);
	ADD_DEBUGFS_ENTRY("emac2", emac, priv->dbgfs, priv->pfe->emac[2], &dsav);
#endif

	return 0;
}

int pfeng_debugfs_add_hif_chnl(struct pfeng_priv *priv, struct pfeng_ndev *ndev)
{
	struct device *dev = &priv->pdev->dev;
	char fname[32];
	struct dentry *dsav = NULL;

	if (!priv->dbgfs)
		return -ENODEV;

	if (ndev->eth->hif_chnl_sc >= HIF_CFG_MAX_CHANNELS)
		return -EINVAL;

	if (ndev->chnl_sc.dentry)
		/* already created */
		return 0;

	/* create subdirectory 'chn%d' */
	scnprintf(fname, sizeof(fname), "chnl%d", ndev->eth->hif_chnl_sc);
	ndev->chnl_sc.dentry = debugfs_create_dir(fname, priv->dbgfs);
	if (!ndev->chnl_sc.dentry) {
		dev_err(dev, "debugfs create directory chnl%d failed\n", ndev->eth->hif_chnl_sc);
		return -EINVAL;
	}

	/* remember new chnl number */
	hif_chnl_mask |= 1 << ndev->eth->hif_chnl_sc;

	/* add members to the subdirectory */
	ADD_DEBUGFS_ENTRY("rings", hif_chnl, ndev->chnl_sc.dentry, ndev->chnl_sc.priv, &dsav);

	ADD_DEBUGFS_CHNL_XSTATS_ENTRY(napi_poll);
	ADD_DEBUGFS_CHNL_XSTATS_ENTRY(napi_poll_onrun);
	ADD_DEBUGFS_CHNL_XSTATS_ENTRY(napi_poll_resched);
	ADD_DEBUGFS_CHNL_XSTATS_ENTRY(napi_poll_completed);
	ADD_DEBUGFS_CHNL_XSTATS_ENTRY(napi_poll_rx);
	ADD_DEBUGFS_CHNL_XSTATS_ENTRY(txconf_loop);
	ADD_DEBUGFS_CHNL_XSTATS_ENTRY(txconf);
	ADD_DEBUGFS_CHNL_XSTATS_ENTRY(tx_busy);
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
        ADD_DEBUGFS_CHNL_XSTATS_ENTRY(ihc_rx);
        ADD_DEBUGFS_CHNL_XSTATS_ENTRY(ihc_tx);
#endif

	return 0;
}

void pfeng_debugfs_remove(struct pfeng_priv *priv)
{
	if (priv->dbgfs)
		debugfs_remove_recursive(priv->dbgfs);
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
