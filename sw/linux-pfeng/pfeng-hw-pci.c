/*
 * Copyright 2018-2019 NXP
 *
 * SPDX-License-Identifier:     BSD OR GPL-2.0
 *
 */

#include <linux/pci.h>
#include <linux/dmi.h>

#include "pfeng.h"

/**
 * pfeng_pci_probe
 *
 * @pdev: pci device pointer
 * @id: pointer to table of device id/id's.
 *
 * Description: This probing function gets called for all PCI devices which
 * match the ID table and are not "owned" by other driver yet. This function
 * gets passed a "struct pci_dev *" for each device whose entry in the ID table
 * matches the device. The probe functions returns zero when the driver choose
 * to take "ownership" of the device or an error code(-ve no) otherwise.
 */
static int pfeng_pci_probe(struct pci_dev *pdev,
                const struct pci_device_id *id)
{
	struct pfeng_priv *priv;
	struct pfeng_plat_data *plat;
	struct pfeng_resources res;
	int ret;

	priv = pfeng_mod_init(&pdev->dev);
	if (!priv)
		return -ENOMEM;

	plat = devm_kzalloc(&pdev->dev, sizeof(*plat), GFP_KERNEL);
	if (!plat)
		return -ENOMEM;

	/* Enable pci device */
	ret = pci_enable_device(pdev);
	if (ret) {
		dev_err(&pdev->dev, "pci probe: ERROR: failed to enable device\n");
        goto err_pci_ena;
    }

	ret = pci_request_regions(pdev, PFENG_DRIVER_NAME);
	if (ret) {
		dev_err(&pdev->dev, "pci probe: Failed to get PCI regions\n");
		goto err_pci_req;
	}

	ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
	if(ret) {
		dev_err(&pdev->dev, "pci probe: Couldn't set 32 bit DMA mask\n");
		goto err_pci_dmamask;
	}

	ret = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
	if(ret) {
		dev_err(&pdev->dev, "pci probe: Couldn't set 32 bit DMA\n");
		goto err_pci_dmamask;
	}

	pci_set_master(pdev);

	pfeng_mod_get_setup(&pdev->dev, plat);

	/* Get the base address of device */
	memset(&res, 0, sizeof(res));
	res.addr = pci_resource_start(pdev, 0);
	res.addr_size = pci_resource_len(pdev, 0);

	ret = pci_enable_msi(pdev);
	if(ret) {
		dev_warn(&pdev->dev, "pci probe: Couldn't enable PCI MSI (error: %d), using oldschool PCI IRQ access ...\n", ret);
		res.irq_mode = PFENG_IRQ_MODE_SHARED;
	} else {
		dev_info(&pdev->dev, "MSI enabled\n");
		res.irq_mode = PFENG_IRQ_MODE_PRIVATE;
	}

	/* Get the base irq of device */
	res.irq.hif[0] = pdev->irq;

	ret = pfeng_mod_probe(&pdev->dev, priv, plat, &res);
	if(ret)
		goto err_mod_probe;

end:
	return ret;

err_mod_probe:
	if(res.irq_mode == PFENG_IRQ_MODE_PRIVATE)
		pci_disable_msi(pdev);

err_pci_dmamask:
	pci_release_regions(pdev);

err_pci_req:
	pci_disable_device(pdev);

err_pci_ena:
	pfeng_mod_exit(&pdev->dev);

	goto end;
}


/**
 * pfeng_pci_remove
 *
 * @pdev: platform device pointer
 * Description: this function calls the main to free the net resources
 * and releases the PCI resources.
 */
static void pfeng_pci_remove(struct pci_dev *pdev)
{
	struct pfeng_priv *priv = (struct pfeng_priv *)dev_get_drvdata(&pdev->dev);

	pfeng_mod_exit(&pdev->dev);

	pci_clear_master(pdev);
	if(priv->irq_mode == PFENG_IRQ_MODE_PRIVATE)
		pci_disable_msi(pdev);
	pci_release_regions(pdev);
	pci_disable_device(pdev);

}

/* synthetic ID, no official vendor */
#define PCI_VENDOR_ID_NXP 0x700

#define PFE_FPGA_DEVICE_ID 0x8011

static const struct pci_device_id pfeng_id_table[] = {
	{ PCI_VENDOR_ID_NXP, PFE_FPGA_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
    { 0, }
};

MODULE_DEVICE_TABLE(pci, pfeng_id_table);

static struct pci_driver pfeng_pci_driver = {
	.name = PFENG_DRIVER_NAME,
	.id_table = pfeng_id_table,
	.probe = pfeng_pci_probe,
	.remove = pfeng_pci_remove,
};

module_pci_driver(pfeng_pci_driver);
