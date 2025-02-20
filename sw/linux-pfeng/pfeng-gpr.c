/*
 * Copyright 2023,2025 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/io.h>
#include <linux/phy.h>

#if defined(PFE_CFG_USE_NVMEM)
#include <soc/s32cc/nvmem_common.h>
#endif /* PFE_CFG_USE_NVMEM */

#include "pfeng.h"

/*
 * S32G soc specific addresses
 */
#define GPR_PFE_COH_EN				0x0
#define GPR_PFE_EMACX_INTF_SEL			0x4
#define GPR_PFE_PWR_CTRL			0x20

#define GPR_PFE_COH_EN_UTIL			BIT(5)
#define GPR_PFE_COH_EN_HIF3			BIT(4)
#define GPR_PFE_COH_EN_HIF2			BIT(3)
#define GPR_PFE_COH_EN_HIF1			BIT(2)
#define GPR_PFE_COH_EN_HIF0			BIT(1)
#define GPR_PFE_COH_EN_HIF_0_3_MASK		(GPR_PFE_COH_EN_HIF0 | GPR_PFE_COH_EN_HIF1 | \
						 GPR_PFE_COH_EN_HIF2 | GPR_PFE_COH_EN_HIF3)
#define GPR_PFE_COH_EN_DDR			BIT(0)

#define GPR_PFE_EMAC_N_PWR_DWN(n)		(1 << (3 + (n)))
#define GPR_PFE_EMAC_N_IF(n, i)			((i) << ((n) * 4))
#define GPR_PFE_EMAC_IF_MII			(1)
#define GPR_PFE_EMAC_IF_RMII			(9)
#define GPR_PFE_EMAC_IF_RGMII			(2)
#define GPR_PFE_EMAC_IF_SGMII			(0)

/*
 * GPR:GENCTRL3 is used for PFE H/W IP ready indication, set by Master and read
 * by Slave. Using the higher 16 bits, the lower bits remain untouched for
 * security reasons.
 */
#define GPR_PFE_IP_READY_CTRL_REG		(0x4007CAECU)
#define GPR_PFE_IP_READY_CTRL_REG_LEN		4U
#define GPR_PFE_BIT_IP_READY			16U
#define GPR_PFE_IP_READY			BIT(GPR_PFE_BIT_IP_READY)

static unsigned int xlate_to_s32g_intf(unsigned int n, phy_interface_t intf)
{
	switch (intf) {
	default: /* SGMII is the default */
	case PHY_INTERFACE_MODE_SGMII:
		return GPR_PFE_EMAC_N_IF(n, GPR_PFE_EMAC_IF_SGMII);

	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		return GPR_PFE_EMAC_N_IF(n, GPR_PFE_EMAC_IF_RGMII);

	case PHY_INTERFACE_MODE_RMII:
		return GPR_PFE_EMAC_N_IF(n, GPR_PFE_EMAC_IF_RMII);

	case PHY_INTERFACE_MODE_MII:
		return GPR_PFE_EMAC_N_IF(n, GPR_PFE_EMAC_IF_MII);
	}
}

int pfeng_gpr_check_nvmem_cells(struct device *dev)
{
#if defined(PFE_CFG_USE_NVMEM)
	static const char * const cell_names[] = {
		"pfe_coh_en",
		"pfe_genctrl3",
#if defined(PFE_CFG_PFE_MASTER)
		"pfe_emacs_intf_sel",
		"pfe_pwr_ctrl",
#endif /* PFE_CFG_PFE_MASTER */
	};
	struct nvmem_cell *cell;
	size_t i;

	for (i = 0; i < ARRAY_SIZE(cell_names); i++) {
		cell = nvmem_cell_get(dev, cell_names[i]);
		if (IS_ERR(cell)) {
			HM_MSG_DEV_ERR(dev, "Failed to get NVMEM cell %s\n",
				       cell_names[i]);
			return PTR_ERR(cell);
		}

		nvmem_cell_put(cell);
	}
#endif /* PFE_CFG_USE_NVMEM */
	return 0;
}

#if defined(PFE_CFG_USE_NVMEM)
static int gpr_set_port_coherency_nvmem(struct pfeng_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	int ret = 0;
	u32 val;

	ret = write_nvmem_cell(dev, "pfe_coh_en", GPR_PFE_COH_EN_HIF_0_3_MASK);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to enable port coherency\n");
		return ret;
	}

	ret = read_nvmem_cell(dev, "pfe_coh_en", &val);
	if (ret)
		return ret;

	if ((val & GPR_PFE_COH_EN_HIF_0_3_MASK) == GPR_PFE_COH_EN_HIF_0_3_MASK) {
		HM_MSG_DEV_INFO(dev, "PFE port coherency enabled, mask 0x%x\n", val);
	} else {
		HM_MSG_DEV_ERR(dev, "Failed to enable port coherency (mask 0x%x)\n", val);
		ret = -EINVAL;
	}

	return ret;
}
#else
static int gpr_set_port_coherency_hal(struct pfeng_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	void *syscon;
	int ret = 0;
	u32 val;

	if (!priv->syscon.start) {
		HM_MSG_DEV_ERR(dev, "cannot access GPR, syscon resource is not available, aborting (PFE_COH_EN)\n");
		return -EINVAL;
	}

	syscon = ioremap(priv->syscon.start,
			 priv->syscon.end - priv->syscon.start + 1);
	if (!syscon) {
		HM_MSG_DEV_ERR(dev, "cannot map GPR, aborting (PFE_COH_EN)\n");
		return -EIO;
	}

	val = hal_read32(syscon + GPR_PFE_COH_EN);
	val |= GPR_PFE_COH_EN_HIF_0_3_MASK;
	hal_write32(val, syscon + GPR_PFE_COH_EN);

	val = hal_read32(syscon + GPR_PFE_COH_EN);
	if ((val & GPR_PFE_COH_EN_HIF_0_3_MASK) == GPR_PFE_COH_EN_HIF_0_3_MASK) {
		HM_MSG_DEV_INFO(dev, "PFE port coherency enabled, mask 0x%x\n", val);
	} else {
		HM_MSG_DEV_ERR(dev, "Failed to enable port coherency (mask 0x%x)\n", val);
		ret = -EINVAL;
	}

	iounmap(syscon);

	return ret;
}
#endif /* PFE_CFG_USE_NVMEM */

int pfeng_gpr_set_port_coherency(struct pfeng_priv *priv)
{
#if defined(PFE_CFG_USE_NVMEM)
	return gpr_set_port_coherency_nvmem(priv);
#else
	return gpr_set_port_coherency_hal(priv);
#endif /* PFE_CFG_USE_NVMEM */
}

#if defined(PFE_CFG_USE_NVMEM)
static int gpr_clear_port_coherency_nvmem(struct pfeng_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	int ret = 0;
	u32 val;

	ret = read_nvmem_cell(dev, "pfe_coh_en", &val);
	if (ret)
		return ret;

	if (!(val & GPR_PFE_COH_EN_HIF_0_3_MASK)) {
		HM_MSG_DEV_INFO(dev, "PFE port coherency already cleared\n");
		return 0;
	}

	val &= ~GPR_PFE_COH_EN_HIF_0_3_MASK;
	ret = write_nvmem_cell(dev, "pfe_coh_en", val);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to clear port coherency, mask 0x%x\n", val);
		return ret;
	}

	ret = read_nvmem_cell(dev, "pfe_coh_en", &val);
	if (ret)
		return ret;

	if (val & GPR_PFE_COH_EN_HIF_0_3_MASK) {
		HM_MSG_DEV_ERR(dev, "Failed to clear port coherency, mask 0x%x\n", val);
		return -EINVAL;
	}

	HM_MSG_DEV_INFO(dev, "PFE port coherency cleared\n");

	return 0;
}
#else
static int gpr_clear_port_coherency_hal(struct pfeng_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	void *syscon;
	int ret = 0;
	u32 val;

	if (!priv->syscon.start) {
		HM_MSG_DEV_ERR(dev, "cannot access GPR, syscon resource is not available, aborting (PFE_COH_EN)\n");
		return -EINVAL;
	}

	syscon = ioremap(priv->syscon.start,
			 priv->syscon.end - priv->syscon.start + 1);
	if (!syscon) {
		HM_MSG_DEV_ERR(dev, "cannot map GPR, aborting (PFE_COH_EN)\n");
		return -EIO;
	}

	val = hal_read32(syscon + GPR_PFE_COH_EN);
	if (!(val & GPR_PFE_COH_EN_HIF_0_3_MASK)) {
		HM_MSG_DEV_INFO(dev, "PFE port coherency already cleared\n");
	} else {
		val = hal_read32(syscon + GPR_PFE_COH_EN);
		val &= ~GPR_PFE_COH_EN_HIF_0_3_MASK;
		hal_write32(val, syscon + GPR_PFE_COH_EN);

		val = hal_read32(syscon + GPR_PFE_COH_EN);
		if (val & GPR_PFE_COH_EN_HIF_0_3_MASK) {
			HM_MSG_DEV_ERR(dev, "Failed to clear port coherency, mask 0x%x\n", val);
			ret = -EINVAL;
		} else {
			HM_MSG_DEV_INFO(dev, "PFE port coherency cleared\n");
		}
	}

	iounmap(syscon);

	return ret;
}
#endif /* PFE_CFG_USE_NVMEM */

int pfeng_gpr_clear_port_coherency(struct pfeng_priv *priv)
{
#if defined(PFE_CFG_USE_NVMEM)
	return gpr_clear_port_coherency_nvmem(priv);
#else
	return gpr_clear_port_coherency_hal(priv);
#endif /* PFE_CFG_USE_NVMEM */
}

#if defined(PFE_CFG_USE_NVMEM)
static int gpr_set_emac_interfaces_nvmem(struct pfeng_priv *priv,
					 u32 emacs_intf_sel)
{
	int ret;

	/* set up interfaces */
	ret = write_nvmem_cell(&priv->pdev->dev, "pfe_emacs_intf_sel",
			       emacs_intf_sel);
	if (ret) {
		HM_MSG_DEV_ERR(&priv->pdev->dev, "Failed to set EMACs interfaces\n");
		return ret;
	}

	/* power down and up EMACs */
	ret = write_nvmem_cell(&priv->pdev->dev, "pfe_pwr_ctrl",
			       GPR_PFE_EMAC_N_PWR_DWN(0) |
			       GPR_PFE_EMAC_N_PWR_DWN(1) |
			       GPR_PFE_EMAC_N_PWR_DWN(2));
	if (ret) {
		HM_MSG_DEV_ERR(&priv->pdev->dev, "Failed to power down EMACs\n");
		return ret;
	}
	usleep_range(100, 500);
	ret = write_nvmem_cell(&priv->pdev->dev, "pfe_pwr_ctrl", 0);
	if (ret) {
		HM_MSG_DEV_ERR(&priv->pdev->dev, "Failed to power up EMACs\n");
		return ret;
	}

	return 0;
}
#else
static int gpr_set_emac_interfaces_hal(struct pfeng_priv *priv,
				       u32 emacs_intf_sel)
{
	void *syscon;

	syscon = ioremap(priv->syscon.start,
			 priv->syscon.end - priv->syscon.start + 1);
	if (!syscon) {
		HM_MSG_DEV_ERR(&priv->pdev->dev, "cannot map GPR, aborting (INTF_SEL)\n");
		return -EIO;
	}
	/* set up interfaces */
	hal_write32(emacs_intf_sel, syscon + GPR_PFE_EMACX_INTF_SEL);

	/* power down and up EMACs */
	hal_write32(GPR_PFE_EMAC_N_PWR_DWN(0) |
		    GPR_PFE_EMAC_N_PWR_DWN(1) |
		    GPR_PFE_EMAC_N_PWR_DWN(2),
		    syscon + GPR_PFE_PWR_CTRL);
	usleep_range(100, 500);
	hal_write32(0, syscon + GPR_PFE_PWR_CTRL);

	iounmap(syscon);

	return 0;
}
#endif /* PFE_CFG_USE_NVMEM */

int pfeng_gpr_set_emac_interfaces(struct pfeng_priv *priv)
{
	u32 emacs_intf_sel;
	int ret;

	emacs_intf_sel = xlate_to_s32g_intf(0, priv->emac[0].intf_mode) |
			 xlate_to_s32g_intf(1, priv->emac[1].intf_mode) |
			 xlate_to_s32g_intf(2, priv->emac[2].intf_mode);
#if defined(PFE_CFG_USE_NVMEM)
	ret = gpr_set_emac_interfaces_nvmem(priv, emacs_intf_sel);
#else
	ret = gpr_set_emac_interfaces_hal(priv, emacs_intf_sel);
#endif /* PFE_CFG_USE_NVMEM */
	if (ret)
		return ret;

	HM_MSG_DEV_INFO(&priv->pdev->dev,
			"Interface selected: EMAC0: 0x%x EMAC1: 0x%x EMAC2: 0x%x\n",
			priv->emac[0].intf_mode, priv->emac[1].intf_mode,
			priv->emac[2].intf_mode);

	return 0;
}

#if defined(PFE_CFG_USE_NVMEM)
static int gpr_ip_ready_get_nvmem(struct device *dev, bool *is_on)
{
	int ret;
	u32 val;

	ret = read_nvmem_cell(dev, "pfe_genctrl3", &val);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to read cell \'pfe_genctrl3\'\n");
		return ret;
	}

	*is_on = !!(val);

	return 0;
}
#else
static int gpr_ip_ready_get_hal(struct device *dev, bool *is_on)
{
	u32 *ctrlreg = (u32 *)oal_mm_dev_map((void *)GPR_PFE_IP_READY_CTRL_REG,
					     GPR_PFE_IP_READY_CTRL_REG_LEN);
	u32 val = 0U;

	if (!ctrlreg) {
		HM_MSG_DEV_ERR(dev, "cannot map GPR, aborting (GENCTRL3)\n");
		return -EIO;
	}

	val = hal_read32(ctrlreg);
	val &= GPR_PFE_IP_READY;

	*is_on = val ? true : false;

	(void)oal_mm_dev_unmap(ctrlreg, GPR_PFE_IP_READY_CTRL_REG_LEN);

	return 0;
}
#endif /* PFE_CFG_USE_NVMEM */

int pfeng_gpr_ip_ready_get(struct device *dev, bool *is_on)
{
#if defined(PFE_CFG_USE_NVMEM)
	return gpr_ip_ready_get_nvmem(dev, is_on);
#else
	return gpr_ip_ready_get_hal(dev, is_on);
#endif /* PFE_CFG_USE_NVMEM */
}

#if defined(PFE_CFG_MULTI_INSTANCE_SUPPORT)
#if defined(PFE_CFG_USE_NVMEM)
static int gpr_ip_ready_set_nvmem(struct device *dev, bool on)
{
	int ret;

	ret = write_nvmem_cell(dev, "pfe_genctrl3", on ? 1 : 0);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to write cell \'pfe_genctrl3\'\n");
		return ret;
	}

	return 0;
}
#else
static int gpr_ip_ready_set_hal(struct device *dev, bool on)
{
	u32 *ctrlreg = (u32 *)oal_mm_dev_map((void *)GPR_PFE_IP_READY_CTRL_REG,
					     GPR_PFE_IP_READY_CTRL_REG_LEN);
	u32 val;

	if (!ctrlreg) {
		HM_MSG_DEV_ERR(dev, "cannot map GPR, aborting (GENCTRL3)\n");
		return -EIO;
	}

	val = hal_read32(ctrlreg);
	if (on)
		val |= GPR_PFE_IP_READY;
	else
		val &= ~GPR_PFE_IP_READY;
	hal_write32(val, ctrlreg);

	(void)oal_mm_dev_unmap(ctrlreg, GPR_PFE_IP_READY_CTRL_REG_LEN);

	return 0;
}
#endif /* PFE_CFG_USE_NVMEM */

int pfeng_gpr_ip_ready_set(struct device *dev, bool on)
{
#if defined(PFE_CFG_USE_NVMEM)
	return gpr_ip_ready_set_nvmem(dev, on);
#else
	return gpr_ip_ready_set_hal(dev, on);
#endif /* PFE_CFG_USE_NVMEM */
}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
