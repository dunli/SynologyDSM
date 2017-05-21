#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef _DRIVERS_MMC_SDHCI_PLTFM_H
#define _DRIVERS_MMC_SDHCI_PLTFM_H

#include <linux/clk.h>
#include <linux/platform_device.h>
#include "sdhci.h"

struct sdhci_pltfm_data {
	const struct sdhci_ops *ops;
	unsigned int quirks;
#if defined (MY_DEF_HERE)
	unsigned int quirks2;
#endif  
};

struct sdhci_pltfm_host {
	struct clk *clk;
	void *priv;  

	unsigned int clock;
	u16 xfer_mode_shadow;
};

#ifdef CONFIG_MMC_SDHCI_BIG_ENDIAN_32BIT_BYTE_SWAPPER
 
static inline u32 sdhci_be32bs_readl(struct sdhci_host *host, int reg)
{
	return in_be32(host->ioaddr + reg);
}

static inline u16 sdhci_be32bs_readw(struct sdhci_host *host, int reg)
{
	return in_be16(host->ioaddr + (reg ^ 0x2));
}

static inline u8 sdhci_be32bs_readb(struct sdhci_host *host, int reg)
{
	return in_8(host->ioaddr + (reg ^ 0x3));
}

static inline void sdhci_be32bs_writel(struct sdhci_host *host,
				       u32 val, int reg)
{
	out_be32(host->ioaddr + reg, val);
}

static inline void sdhci_be32bs_writew(struct sdhci_host *host,
				       u16 val, int reg)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	int base = reg & ~0x3;
	int shift = (reg & 0x2) * 8;

	switch (reg) {
	case SDHCI_TRANSFER_MODE:
		 
		pltfm_host->xfer_mode_shadow = val;
		return;
	case SDHCI_COMMAND:
		sdhci_be32bs_writel(host,
				    val << 16 | pltfm_host->xfer_mode_shadow,
				    SDHCI_TRANSFER_MODE);
		return;
	}
	clrsetbits_be32(host->ioaddr + base, 0xffff << shift, val << shift);
}

static inline void sdhci_be32bs_writeb(struct sdhci_host *host, u8 val, int reg)
{
	int base = reg & ~0x3;
	int shift = (reg & 0x3) * 8;

	clrsetbits_be32(host->ioaddr + base , 0xff << shift, val << shift);
}
#endif  

extern void sdhci_get_of_property(struct platform_device *pdev);

extern struct sdhci_host *sdhci_pltfm_init(struct platform_device *pdev,
					  const struct sdhci_pltfm_data *pdata);
extern void sdhci_pltfm_free(struct platform_device *pdev);

extern int sdhci_pltfm_register(struct platform_device *pdev,
				const struct sdhci_pltfm_data *pdata);
extern int sdhci_pltfm_unregister(struct platform_device *pdev);

extern unsigned int sdhci_pltfm_clk_get_max_clock(struct sdhci_host *host);

#ifdef CONFIG_PM
extern const struct dev_pm_ops sdhci_pltfm_pmops;
#define SDHCI_PLTFM_PMOPS (&sdhci_pltfm_pmops)
#else
#define SDHCI_PLTFM_PMOPS NULL
#endif

#endif  
