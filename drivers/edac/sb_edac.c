#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/edac.h>
#include <linux/mmzone.h>
#include <linux/smp.h>
#include <linux/bitmap.h>
#include <linux/math64.h>
#include <asm/processor.h>
#include <asm/mce.h>

#include "edac_core.h"

static LIST_HEAD(sbridge_edac_list);
static DEFINE_MUTEX(sbridge_edac_lock);
static int probed;

#define SBRIDGE_REVISION    " Ver: 1.0.0 "
#define EDAC_MOD_STR      "sbridge_edac"

#define sbridge_printk(level, fmt, arg...)			\
	edac_printk(level, "sbridge", fmt, ##arg)

#define sbridge_mc_printk(mci, level, fmt, arg...)		\
	edac_mc_chipset_printk(mci, level, "sbridge", fmt, ##arg)

#if defined (MY_DEF_HERE)
#define GET_BITFIELD(v, lo, hi)	\
	(((v) & GENMASK_ULL(hi, lo)) >> (lo))
#else  
#define GET_BITFIELD(v, lo, hi)	\
	(((v) & ((1ULL << ((hi) - (lo) + 1)) - 1) << (lo)) >> (lo))
#endif  

#define PCI_DEVICE_ID_INTEL_SBRIDGE_SAD0	0x3cf4	 
#define PCI_DEVICE_ID_INTEL_SBRIDGE_SAD1	0x3cf6	 
#define PCI_DEVICE_ID_INTEL_SBRIDGE_BR		0x3cf5	 
#define PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_HA0	0x3ca0	 
#define PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_TA	0x3ca8	 
#define PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_RAS	0x3c71	 
#define PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_TAD0	0x3caa	 
#define PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_TAD1	0x3cab	 
#define PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_TAD2	0x3cac	 
#define PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_TAD3	0x3cad	 
#define PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_DDRIO	0x3cb8	 

#define PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_ERR0	0x3c72	 
#define PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_ERR1	0x3c73	 
#define PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_ERR2	0x3c76	 
#define PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_ERR3	0x3c77	 

static const u32 dram_rule[] = {
	0x80, 0x88, 0x90, 0x98, 0xa0,
	0xa8, 0xb0, 0xb8, 0xc0, 0xc8,
};
#define MAX_SAD		ARRAY_SIZE(dram_rule)

#define SAD_LIMIT(reg)		((GET_BITFIELD(reg, 6, 25) << 26) | 0x3ffffff)
#define DRAM_ATTR(reg)		GET_BITFIELD(reg, 2,  3)
#define INTERLEAVE_MODE(reg)	GET_BITFIELD(reg, 1,  1)
#define DRAM_RULE_ENABLE(reg)	GET_BITFIELD(reg, 0,  0)

static char *get_dram_attr(u32 reg)
{
	switch(DRAM_ATTR(reg)) {
		case 0:
			return "DRAM";
		case 1:
			return "MMCFG";
		case 2:
			return "NXM";
		default:
			return "unknown";
	}
}

static const u32 interleave_list[] = {
	0x84, 0x8c, 0x94, 0x9c, 0xa4,
	0xac, 0xb4, 0xbc, 0xc4, 0xcc,
};
#define MAX_INTERLEAVE	ARRAY_SIZE(interleave_list)

#define SAD_PKG0(reg)		GET_BITFIELD(reg, 0, 2)
#define SAD_PKG1(reg)		GET_BITFIELD(reg, 3, 5)
#define SAD_PKG2(reg)		GET_BITFIELD(reg, 8, 10)
#define SAD_PKG3(reg)		GET_BITFIELD(reg, 11, 13)
#define SAD_PKG4(reg)		GET_BITFIELD(reg, 16, 18)
#define SAD_PKG5(reg)		GET_BITFIELD(reg, 19, 21)
#define SAD_PKG6(reg)		GET_BITFIELD(reg, 24, 26)
#define SAD_PKG7(reg)		GET_BITFIELD(reg, 27, 29)

static inline int sad_pkg(u32 reg, int interleave)
{
	switch (interleave) {
	case 0:
		return SAD_PKG0(reg);
	case 1:
		return SAD_PKG1(reg);
	case 2:
		return SAD_PKG2(reg);
	case 3:
		return SAD_PKG3(reg);
	case 4:
		return SAD_PKG4(reg);
	case 5:
		return SAD_PKG5(reg);
	case 6:
		return SAD_PKG6(reg);
	case 7:
		return SAD_PKG7(reg);
	default:
		return -EINVAL;
	}
}

#define TOLM		0x80
#define	TOHM		0x84

#define GET_TOLM(reg)		((GET_BITFIELD(reg, 0,  3) << 28) | 0x3ffffff)
#define GET_TOHM(reg)		((GET_BITFIELD(reg, 0, 20) << 25) | 0x3ffffff)

#define SAD_TARGET	0xf0

#define SOURCE_ID(reg)		GET_BITFIELD(reg, 9, 11)

#define SAD_CONTROL	0xf4

#define NODE_ID(reg)		GET_BITFIELD(reg, 0, 2)

static const u32 tad_dram_rule[] = {
	0x40, 0x44, 0x48, 0x4c,
	0x50, 0x54, 0x58, 0x5c,
	0x60, 0x64, 0x68, 0x6c,
};
#define MAX_TAD	ARRAY_SIZE(tad_dram_rule)

#define TAD_LIMIT(reg)		((GET_BITFIELD(reg, 12, 31) << 26) | 0x3ffffff)
#define TAD_SOCK(reg)		GET_BITFIELD(reg, 10, 11)
#define TAD_CH(reg)		GET_BITFIELD(reg,  8,  9)
#define TAD_TGT3(reg)		GET_BITFIELD(reg,  6,  7)
#define TAD_TGT2(reg)		GET_BITFIELD(reg,  4,  5)
#define TAD_TGT1(reg)		GET_BITFIELD(reg,  2,  3)
#define TAD_TGT0(reg)		GET_BITFIELD(reg,  0,  1)

#define MCMTR			0x7c

#define IS_ECC_ENABLED(mcmtr)		GET_BITFIELD(mcmtr, 2, 2)
#define IS_LOCKSTEP_ENABLED(mcmtr)	GET_BITFIELD(mcmtr, 1, 1)
#define IS_CLOSE_PG(mcmtr)		GET_BITFIELD(mcmtr, 0, 0)

#define RASENABLES		0xac
#define IS_MIRROR_ENABLED(reg)		GET_BITFIELD(reg, 0, 0)

static const int mtr_regs[] = {
	0x80, 0x84, 0x88,
};

#define RANK_DISABLE(mtr)		GET_BITFIELD(mtr, 16, 19)
#define IS_DIMM_PRESENT(mtr)		GET_BITFIELD(mtr, 14, 14)
#define RANK_CNT_BITS(mtr)		GET_BITFIELD(mtr, 12, 13)
#define RANK_WIDTH_BITS(mtr)		GET_BITFIELD(mtr, 2, 4)
#define COL_WIDTH_BITS(mtr)		GET_BITFIELD(mtr, 0, 1)

static const u32 tad_ch_nilv_offset[] = {
	0x90, 0x94, 0x98, 0x9c,
	0xa0, 0xa4, 0xa8, 0xac,
	0xb0, 0xb4, 0xb8, 0xbc,
};
#define CHN_IDX_OFFSET(reg)		GET_BITFIELD(reg, 28, 29)
#define TAD_OFFSET(reg)			(GET_BITFIELD(reg,  6, 25) << 26)

static const u32 rir_way_limit[] = {
	0x108, 0x10c, 0x110, 0x114, 0x118,
};
#define MAX_RIR_RANGES ARRAY_SIZE(rir_way_limit)

#define IS_RIR_VALID(reg)	GET_BITFIELD(reg, 31, 31)
#define RIR_WAY(reg)		GET_BITFIELD(reg, 28, 29)
#define RIR_LIMIT(reg)		((GET_BITFIELD(reg,  1, 10) << 29)| 0x1fffffff)

#define MAX_RIR_WAY	8

static const u32 rir_offset[MAX_RIR_RANGES][MAX_RIR_WAY] = {
	{ 0x120, 0x124, 0x128, 0x12c, 0x130, 0x134, 0x138, 0x13c },
	{ 0x140, 0x144, 0x148, 0x14c, 0x150, 0x154, 0x158, 0x15c },
	{ 0x160, 0x164, 0x168, 0x16c, 0x170, 0x174, 0x178, 0x17c },
	{ 0x180, 0x184, 0x188, 0x18c, 0x190, 0x194, 0x198, 0x19c },
	{ 0x1a0, 0x1a4, 0x1a8, 0x1ac, 0x1b0, 0x1b4, 0x1b8, 0x1bc },
};

#define RIR_RNK_TGT(reg)		GET_BITFIELD(reg, 16, 19)
#define RIR_OFFSET(reg)		GET_BITFIELD(reg,  2, 14)

static const u32 correrrcnt[] = {
	0x104, 0x108, 0x10c, 0x110,
};

#define RANK_ODD_OV(reg)		GET_BITFIELD(reg, 31, 31)
#define RANK_ODD_ERR_CNT(reg)		GET_BITFIELD(reg, 16, 30)
#define RANK_EVEN_OV(reg)		GET_BITFIELD(reg, 15, 15)
#define RANK_EVEN_ERR_CNT(reg)		GET_BITFIELD(reg,  0, 14)

static const u32 correrrthrsld[] = {
	0x11c, 0x120, 0x124, 0x128,
};

#define RANK_ODD_ERR_THRSLD(reg)	GET_BITFIELD(reg, 16, 30)
#define RANK_EVEN_ERR_THRSLD(reg)	GET_BITFIELD(reg,  0, 14)

#define RANK_CFG_A		0x0328

#define IS_RDIMM_ENABLED(reg)		GET_BITFIELD(reg, 11, 11)

#define NUM_CHANNELS		4
#define MAX_DIMMS		3	 
#define CHANNEL_UNSPECIFIED	0xf	 

struct sbridge_info {
	u32	mcmtr;
};

struct sbridge_channel {
	u32		ranks;
	u32		dimms;
};

struct pci_id_descr {
	int			dev;
	int			func;
	int 			dev_id;
	int			optional;
};

struct pci_id_table {
	const struct pci_id_descr	*descr;
	int				n_devs;
};

struct sbridge_dev {
	struct list_head	list;
	u8			bus, mc;
	u8			node_id, source_id;
	struct pci_dev		**pdev;
	int			n_devs;
	struct mem_ctl_info	*mci;
};

struct sbridge_pvt {
	struct pci_dev		*pci_ta, *pci_ddrio, *pci_ras;
	struct pci_dev		*pci_sad0, *pci_sad1, *pci_ha0;
	struct pci_dev		*pci_br;
	struct pci_dev		*pci_tad[NUM_CHANNELS];

	struct sbridge_dev	*sbridge_dev;

	struct sbridge_info	info;
	struct sbridge_channel	channel[NUM_CHANNELS];

	bool			is_mirrored, is_lockstep, is_close_pg;

	struct mce		mce_entry[MCE_LOG_LEN];
	struct mce		mce_outentry[MCE_LOG_LEN];

	unsigned		mce_in, mce_out;

	unsigned		mce_overrun;

	u64			tolm, tohm;
};

#define PCI_DESCR(device, function, device_id, opt)	\
	.dev = (device),				\
	.func = (function),				\
	.dev_id = (device_id),				\
	.optional = opt

static const struct pci_id_descr pci_dev_descr_sbridge[] = {
		 
	{ PCI_DESCR(14, 0, PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_HA0, 0)	},

	{ PCI_DESCR(15, 0, PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_TA, 0)	},
	{ PCI_DESCR(15, 1, PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_RAS, 0)	},
	{ PCI_DESCR(15, 2, PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_TAD0, 0)	},
	{ PCI_DESCR(15, 3, PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_TAD1, 0)	},
	{ PCI_DESCR(15, 4, PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_TAD2, 0)	},
	{ PCI_DESCR(15, 5, PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_TAD3, 0)	},
	{ PCI_DESCR(17, 0, PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_DDRIO, 1)	},

	{ PCI_DESCR(12, 6, PCI_DEVICE_ID_INTEL_SBRIDGE_SAD0, 0)		},
	{ PCI_DESCR(12, 7, PCI_DEVICE_ID_INTEL_SBRIDGE_SAD1, 0)		},

	{ PCI_DESCR(13, 6, PCI_DEVICE_ID_INTEL_SBRIDGE_BR, 0)		},
};

#define PCI_ID_TABLE_ENTRY(A) { .descr=A, .n_devs = ARRAY_SIZE(A) }
static const struct pci_id_table pci_dev_descr_sbridge_table[] = {
	PCI_ID_TABLE_ENTRY(pci_dev_descr_sbridge),
	{0,}			 
};

static DEFINE_PCI_DEVICE_TABLE(sbridge_pci_tbl) = {
	{PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_SBRIDGE_IMC_TA)},
	{0,}			 
};

static inline int numrank(u32 mtr)
{
	int ranks = (1 << RANK_CNT_BITS(mtr));

	if (ranks > 4) {
		edac_dbg(0, "Invalid number of ranks: %d (max = 4) raw value = %x (%04x)\n",
			 ranks, (unsigned int)RANK_CNT_BITS(mtr), mtr);
		return -EINVAL;
	}

	return ranks;
}

static inline int numrow(u32 mtr)
{
	int rows = (RANK_WIDTH_BITS(mtr) + 12);

	if (rows < 13 || rows > 18) {
		edac_dbg(0, "Invalid number of rows: %d (should be between 14 and 17) raw value = %x (%04x)\n",
			 rows, (unsigned int)RANK_WIDTH_BITS(mtr), mtr);
		return -EINVAL;
	}

	return 1 << rows;
}

static inline int numcol(u32 mtr)
{
	int cols = (COL_WIDTH_BITS(mtr) + 10);

	if (cols > 12) {
		edac_dbg(0, "Invalid number of cols: %d (max = 4) raw value = %x (%04x)\n",
			 cols, (unsigned int)COL_WIDTH_BITS(mtr), mtr);
		return -EINVAL;
	}

	return 1 << cols;
}

static struct sbridge_dev *get_sbridge_dev(u8 bus)
{
	struct sbridge_dev *sbridge_dev;

	list_for_each_entry(sbridge_dev, &sbridge_edac_list, list) {
		if (sbridge_dev->bus == bus)
			return sbridge_dev;
	}

	return NULL;
}

static struct sbridge_dev *alloc_sbridge_dev(u8 bus,
					   const struct pci_id_table *table)
{
	struct sbridge_dev *sbridge_dev;

	sbridge_dev = kzalloc(sizeof(*sbridge_dev), GFP_KERNEL);
	if (!sbridge_dev)
		return NULL;

	sbridge_dev->pdev = kzalloc(sizeof(*sbridge_dev->pdev) * table->n_devs,
				   GFP_KERNEL);
	if (!sbridge_dev->pdev) {
		kfree(sbridge_dev);
		return NULL;
	}

	sbridge_dev->bus = bus;
	sbridge_dev->n_devs = table->n_devs;
	list_add_tail(&sbridge_dev->list, &sbridge_edac_list);

	return sbridge_dev;
}

static void free_sbridge_dev(struct sbridge_dev *sbridge_dev)
{
	list_del(&sbridge_dev->list);
	kfree(sbridge_dev->pdev);
	kfree(sbridge_dev);
}

static struct pci_dev *get_pdev_slot_func(u8 bus, unsigned slot,
					  unsigned func)
{
	struct sbridge_dev *sbridge_dev = get_sbridge_dev(bus);
	int i;

	if (!sbridge_dev)
		return NULL;

	for (i = 0; i < sbridge_dev->n_devs; i++) {
		if (!sbridge_dev->pdev[i])
			continue;

		if (PCI_SLOT(sbridge_dev->pdev[i]->devfn) == slot &&
		    PCI_FUNC(sbridge_dev->pdev[i]->devfn) == func) {
			edac_dbg(1, "Associated %02x.%02x.%d with %p\n",
				 bus, slot, func, sbridge_dev->pdev[i]);
			return sbridge_dev->pdev[i];
		}
	}

	return NULL;
}

static int check_if_ecc_is_active(const u8 bus)
{
	struct pci_dev *pdev = NULL;
	u32 mcmtr;

	pdev = get_pdev_slot_func(bus, 15, 0);
	if (!pdev) {
		sbridge_printk(KERN_ERR, "Couldn't find PCI device "
					"%2x.%02d.%d!!!\n",
					bus, 15, 0);
		return -ENODEV;
	}

	pci_read_config_dword(pdev, MCMTR, &mcmtr);
	if (!IS_ECC_ENABLED(mcmtr)) {
		sbridge_printk(KERN_ERR, "ECC is disabled. Aborting\n");
		return -ENODEV;
	}
	return 0;
}

static int get_dimm_config(struct mem_ctl_info *mci)
{
	struct sbridge_pvt *pvt = mci->pvt_info;
	struct dimm_info *dimm;
	unsigned i, j, banks, ranks, rows, cols, npages;
	u64 size;
	u32 reg;
	enum edac_type mode;
	enum mem_type mtype;

	pci_read_config_dword(pvt->pci_br, SAD_TARGET, &reg);
	pvt->sbridge_dev->source_id = SOURCE_ID(reg);

	pci_read_config_dword(pvt->pci_br, SAD_CONTROL, &reg);
	pvt->sbridge_dev->node_id = NODE_ID(reg);
	edac_dbg(0, "mc#%d: Node ID: %d, source ID: %d\n",
		 pvt->sbridge_dev->mc,
		 pvt->sbridge_dev->node_id,
		 pvt->sbridge_dev->source_id);

	pci_read_config_dword(pvt->pci_ras, RASENABLES, &reg);
	if (IS_MIRROR_ENABLED(reg)) {
		edac_dbg(0, "Memory mirror is enabled\n");
		pvt->is_mirrored = true;
	} else {
		edac_dbg(0, "Memory mirror is disabled\n");
		pvt->is_mirrored = false;
	}

	pci_read_config_dword(pvt->pci_ta, MCMTR, &pvt->info.mcmtr);
	if (IS_LOCKSTEP_ENABLED(pvt->info.mcmtr)) {
		edac_dbg(0, "Lockstep is enabled\n");
		mode = EDAC_S8ECD8ED;
		pvt->is_lockstep = true;
	} else {
		edac_dbg(0, "Lockstep is disabled\n");
		mode = EDAC_S4ECD4ED;
		pvt->is_lockstep = false;
	}
	if (IS_CLOSE_PG(pvt->info.mcmtr)) {
		edac_dbg(0, "address map is on closed page mode\n");
		pvt->is_close_pg = true;
	} else {
		edac_dbg(0, "address map is on open page mode\n");
		pvt->is_close_pg = false;
	}

	if (pvt->pci_ddrio) {
		pci_read_config_dword(pvt->pci_ddrio, RANK_CFG_A, &reg);
		if (IS_RDIMM_ENABLED(reg)) {
			 
			edac_dbg(0, "Memory is registered\n");
			mtype = MEM_RDDR3;
		} else {
			edac_dbg(0, "Memory is unregistered\n");
			mtype = MEM_DDR3;
		}
	} else {
		edac_dbg(0, "Cannot determine memory type\n");
		mtype = MEM_UNKNOWN;
	}

	banks = 8;

	for (i = 0; i < NUM_CHANNELS; i++) {
		u32 mtr;

		for (j = 0; j < ARRAY_SIZE(mtr_regs); j++) {
			dimm = EDAC_DIMM_PTR(mci->layers, mci->dimms, mci->n_layers,
				       i, j, 0);
			pci_read_config_dword(pvt->pci_tad[i],
					      mtr_regs[j], &mtr);
			edac_dbg(4, "Channel #%d  MTR%d = %x\n", i, j, mtr);
			if (IS_DIMM_PRESENT(mtr)) {
				pvt->channel[i].dimms++;

				ranks = numrank(mtr);
				rows = numrow(mtr);
				cols = numcol(mtr);

				size = ((u64)rows * cols * banks * ranks) >> (20 - 3);
				npages = MiB_TO_PAGES(size);

				edac_dbg(0, "mc#%d: channel %d, dimm %d, %Ld Mb (%d pages) bank: %d, rank: %d, row: %#x, col: %#x\n",
					 pvt->sbridge_dev->mc, i, j,
					 size, npages,
					 banks, ranks, rows, cols);

				dimm->nr_pages = npages;
				dimm->grain = 32;
				dimm->dtype = (banks == 8) ? DEV_X8 : DEV_X4;
				dimm->mtype = mtype;
				dimm->edac_mode = mode;
				snprintf(dimm->label, sizeof(dimm->label),
					 "CPU_SrcID#%u_Channel#%u_DIMM#%u",
					 pvt->sbridge_dev->source_id, i, j);
			}
		}
	}

	return 0;
}

static void get_memory_layout(const struct mem_ctl_info *mci)
{
	struct sbridge_pvt *pvt = mci->pvt_info;
	int i, j, k, n_sads, n_tads, sad_interl;
	u32 reg;
	u64 limit, prv = 0;
	u64 tmp_mb;
	u32 gb, mb;
	u32 rir_way;

	pci_read_config_dword(pvt->pci_sad1, TOLM,
			      &reg);
	pvt->tolm = GET_TOLM(reg);
	tmp_mb = (1 + pvt->tolm) >> 20;

	gb = div_u64_rem(tmp_mb, 1024, &mb);
	edac_dbg(0, "TOLM: %u.%03u GB (0x%016Lx)\n",
		gb, (mb*1000)/1024, (u64)pvt->tolm);

	pci_read_config_dword(pvt->pci_sad1, TOHM,
			      &reg);
	pvt->tohm = GET_TOHM(reg);
	tmp_mb = (1 + pvt->tohm) >> 20;

	gb = div_u64_rem(tmp_mb, 1024, &mb);
	edac_dbg(0, "TOHM: %u.%03u GB (0x%016Lx)\n",
		gb, (mb*1000)/1024, (u64)pvt->tohm);

	prv = 0;
	for (n_sads = 0; n_sads < MAX_SAD; n_sads++) {
		 
		pci_read_config_dword(pvt->pci_sad0, dram_rule[n_sads],
				      &reg);
		limit = SAD_LIMIT(reg);

		if (!DRAM_RULE_ENABLE(reg))
			continue;

		if (limit <= prv)
			break;

		tmp_mb = (limit + 1) >> 20;
		gb = div_u64_rem(tmp_mb, 1024, &mb);
		edac_dbg(0, "SAD#%d %s up to %u.%03u GB (0x%016Lx) Interleave: %s reg=0x%08x\n",
			 n_sads,
			 get_dram_attr(reg),
			 gb, (mb*1000)/1024,
			 ((u64)tmp_mb) << 20L,
			 INTERLEAVE_MODE(reg) ? "8:6" : "[8:6]XOR[18:16]",
			 reg);
		prv = limit;

		pci_read_config_dword(pvt->pci_sad0, interleave_list[n_sads],
				      &reg);
		sad_interl = sad_pkg(reg, 0);
		for (j = 0; j < 8; j++) {
			if (j > 0 && sad_interl == sad_pkg(reg, j))
				break;

			edac_dbg(0, "SAD#%d, interleave #%d: %d\n",
				 n_sads, j, sad_pkg(reg, j));
		}
	}

	prv = 0;
	for (n_tads = 0; n_tads < MAX_TAD; n_tads++) {
		pci_read_config_dword(pvt->pci_ha0, tad_dram_rule[n_tads],
				      &reg);
		limit = TAD_LIMIT(reg);
		if (limit <= prv)
			break;
		tmp_mb = (limit + 1) >> 20;

		gb = div_u64_rem(tmp_mb, 1024, &mb);
		edac_dbg(0, "TAD#%d: up to %u.%03u GB (0x%016Lx), socket interleave %d, memory interleave %d, TGT: %d, %d, %d, %d, reg=0x%08x\n",
			 n_tads, gb, (mb*1000)/1024,
			 ((u64)tmp_mb) << 20L,
			 (u32)TAD_SOCK(reg),
			 (u32)TAD_CH(reg),
			 (u32)TAD_TGT0(reg),
			 (u32)TAD_TGT1(reg),
			 (u32)TAD_TGT2(reg),
			 (u32)TAD_TGT3(reg),
			 reg);
		prv = limit;
	}

	for (i = 0; i < NUM_CHANNELS; i++) {
		if (!pvt->channel[i].dimms)
			continue;
		for (j = 0; j < n_tads; j++) {
			pci_read_config_dword(pvt->pci_tad[i],
					      tad_ch_nilv_offset[j],
					      &reg);
			tmp_mb = TAD_OFFSET(reg) >> 20;
			gb = div_u64_rem(tmp_mb, 1024, &mb);
			edac_dbg(0, "TAD CH#%d, offset #%d: %u.%03u GB (0x%016Lx), reg=0x%08x\n",
				 i, j,
				 gb, (mb*1000)/1024,
				 ((u64)tmp_mb) << 20L,
				 reg);
		}
	}

	for (i = 0; i < NUM_CHANNELS; i++) {
		if (!pvt->channel[i].dimms)
			continue;
		for (j = 0; j < MAX_RIR_RANGES; j++) {
			pci_read_config_dword(pvt->pci_tad[i],
					      rir_way_limit[j],
					      &reg);

			if (!IS_RIR_VALID(reg))
				continue;

			tmp_mb = RIR_LIMIT(reg) >> 20;
			rir_way = 1 << RIR_WAY(reg);
			gb = div_u64_rem(tmp_mb, 1024, &mb);
			edac_dbg(0, "CH#%d RIR#%d, limit: %u.%03u GB (0x%016Lx), way: %d, reg=0x%08x\n",
				 i, j,
				 gb, (mb*1000)/1024,
				 ((u64)tmp_mb) << 20L,
				 rir_way,
				 reg);

			for (k = 0; k < rir_way; k++) {
				pci_read_config_dword(pvt->pci_tad[i],
						      rir_offset[j][k],
						      &reg);
				tmp_mb = RIR_OFFSET(reg) << 6;

				gb = div_u64_rem(tmp_mb, 1024, &mb);
				edac_dbg(0, "CH#%d RIR#%d INTL#%d, offset %u.%03u GB (0x%016Lx), tgt: %d, reg=0x%08x\n",
					 i, j, k,
					 gb, (mb*1000)/1024,
					 ((u64)tmp_mb) << 20L,
					 (u32)RIR_RNK_TGT(reg),
					 reg);
			}
		}
	}
}

struct mem_ctl_info *get_mci_for_node_id(u8 node_id)
{
	struct sbridge_dev *sbridge_dev;

	list_for_each_entry(sbridge_dev, &sbridge_edac_list, list) {
		if (sbridge_dev->node_id == node_id)
			return sbridge_dev->mci;
	}
	return NULL;
}

static int get_memory_error_data(struct mem_ctl_info *mci,
				 u64 addr,
				 u8 *socket,
				 long *channel_mask,
				 u8 *rank,
				 char **area_type, char *msg)
{
	struct mem_ctl_info	*new_mci;
	struct sbridge_pvt *pvt = mci->pvt_info;
	int 			n_rir, n_sads, n_tads, sad_way, sck_xch;
	int			sad_interl, idx, base_ch;
	int			interleave_mode;
	unsigned		sad_interleave[MAX_INTERLEAVE];
	u32			reg;
	u8			ch_way,sck_way;
	u32			tad_offset;
	u32			rir_way;
	u32			mb, gb;
	u64			ch_addr, offset, limit, prv = 0;

	if ((addr > (u64) pvt->tolm) && (addr < (1LL << 32))) {
		sprintf(msg, "Error at TOLM area, on addr 0x%08Lx", addr);
		return -EINVAL;
	}
	if (addr >= (u64)pvt->tohm) {
		sprintf(msg, "Error at MMIOH area, on addr 0x%016Lx", addr);
		return -EINVAL;
	}

	for (n_sads = 0; n_sads < MAX_SAD; n_sads++) {
		pci_read_config_dword(pvt->pci_sad0, dram_rule[n_sads],
				      &reg);

		if (!DRAM_RULE_ENABLE(reg))
			continue;

		limit = SAD_LIMIT(reg);
		if (limit <= prv) {
			sprintf(msg, "Can't discover the memory socket");
			return -EINVAL;
		}
		if  (addr <= limit)
			break;
		prv = limit;
	}
	if (n_sads == MAX_SAD) {
		sprintf(msg, "Can't discover the memory socket");
		return -EINVAL;
	}
	*area_type = get_dram_attr(reg);
	interleave_mode = INTERLEAVE_MODE(reg);

	pci_read_config_dword(pvt->pci_sad0, interleave_list[n_sads],
			      &reg);
	sad_interl = sad_pkg(reg, 0);
	for (sad_way = 0; sad_way < 8; sad_way++) {
		if (sad_way > 0 && sad_interl == sad_pkg(reg, sad_way))
			break;
		sad_interleave[sad_way] = sad_pkg(reg, sad_way);
		edac_dbg(0, "SAD interleave #%d: %d\n",
			 sad_way, sad_interleave[sad_way]);
	}
	edac_dbg(0, "mc#%d: Error detected on SAD#%d: address 0x%016Lx < 0x%016Lx, Interleave [%d:6]%s\n",
		 pvt->sbridge_dev->mc,
		 n_sads,
		 addr,
		 limit,
		 sad_way + 7,
		 interleave_mode ? "" : "XOR[18:16]");
	if (interleave_mode)
		idx = ((addr >> 6) ^ (addr >> 16)) & 7;
	else
		idx = (addr >> 6) & 7;
	switch (sad_way) {
	case 1:
		idx = 0;
		break;
	case 2:
		idx = idx & 1;
		break;
	case 4:
		idx = idx & 3;
		break;
	case 8:
		break;
	default:
		sprintf(msg, "Can't discover socket interleave");
		return -EINVAL;
	}
	*socket = sad_interleave[idx];
	edac_dbg(0, "SAD interleave index: %d (wayness %d) = CPU socket %d\n",
		 idx, sad_way, *socket);

	new_mci = get_mci_for_node_id(*socket);
	if (!new_mci) {
		sprintf(msg, "Struct for socket #%u wasn't initialized",
			*socket);
		return -EINVAL;
	}
	mci = new_mci;
	pvt = mci->pvt_info;

	prv = 0;
	for (n_tads = 0; n_tads < MAX_TAD; n_tads++) {
		pci_read_config_dword(pvt->pci_ha0, tad_dram_rule[n_tads],
				      &reg);
		limit = TAD_LIMIT(reg);
		if (limit <= prv) {
			sprintf(msg, "Can't discover the memory channel");
			return -EINVAL;
		}
		if  (addr <= limit)
			break;
		prv = limit;
	}
	ch_way = TAD_CH(reg) + 1;
	sck_way = TAD_SOCK(reg) + 1;
	 
	pci_read_config_dword(pvt->pci_tad[0],
				tad_ch_nilv_offset[n_tads],
				&tad_offset);

	if (ch_way == 3)
		idx = addr >> 6;
	else
		idx = addr >> (6 + sck_way);
	idx = idx % ch_way;

	switch (idx) {
	case 0:
		base_ch = TAD_TGT0(reg);
		break;
	case 1:
		base_ch = TAD_TGT1(reg);
		break;
	case 2:
		base_ch = TAD_TGT2(reg);
		break;
	case 3:
		base_ch = TAD_TGT3(reg);
		break;
	default:
		sprintf(msg, "Can't discover the TAD target");
		return -EINVAL;
	}
	*channel_mask = 1 << base_ch;

	if (pvt->is_mirrored) {
		*channel_mask |= 1 << ((base_ch + 2) % 4);
		switch(ch_way) {
		case 2:
		case 4:
			sck_xch = 1 << sck_way * (ch_way >> 1);
			break;
		default:
			sprintf(msg, "Invalid mirror set. Can't decode addr");
			return -EINVAL;
		}
	} else
		sck_xch = (1 << sck_way) * ch_way;

	if (pvt->is_lockstep)
		*channel_mask |= 1 << ((base_ch + 1) % 4);

	offset = TAD_OFFSET(tad_offset);

	edac_dbg(0, "TAD#%d: address 0x%016Lx < 0x%016Lx, socket interleave %d, channel interleave %d (offset 0x%08Lx), index %d, base ch: %d, ch mask: 0x%02lx\n",
		 n_tads,
		 addr,
		 limit,
		 (u32)TAD_SOCK(reg),
		 ch_way,
		 offset,
		 idx,
		 base_ch,
		 *channel_mask);

	if (offset > addr) {
		sprintf(msg, "Can't calculate ch addr: TAD offset 0x%08Lx is too high for addr 0x%08Lx!",
			offset, addr);
		return -EINVAL;
	}
	addr -= offset;
	 
	ch_addr = addr & 0x7f;
	 
	addr >>= 6;
	addr = div_u64(addr, sck_xch);
#if 0
	 
	addr = addr / ch_way;
#endif
	 
	ch_addr |= addr << 6;

	for (n_rir = 0; n_rir < MAX_RIR_RANGES; n_rir++) {
		pci_read_config_dword(pvt->pci_tad[base_ch],
				      rir_way_limit[n_rir],
				      &reg);

		if (!IS_RIR_VALID(reg))
			continue;

		limit = RIR_LIMIT(reg);
		gb = div_u64_rem(limit >> 20, 1024, &mb);
		edac_dbg(0, "RIR#%d, limit: %u.%03u GB (0x%016Lx), way: %d\n",
			 n_rir,
			 gb, (mb*1000)/1024,
			 limit,
			 1 << RIR_WAY(reg));
		if  (ch_addr <= limit)
			break;
	}
	if (n_rir == MAX_RIR_RANGES) {
		sprintf(msg, "Can't discover the memory rank for ch addr 0x%08Lx",
			ch_addr);
		return -EINVAL;
	}
	rir_way = RIR_WAY(reg);
	if (pvt->is_close_pg)
		idx = (ch_addr >> 6);
	else
		idx = (ch_addr >> 13);	 
	idx %= 1 << rir_way;

	pci_read_config_dword(pvt->pci_tad[base_ch],
			      rir_offset[n_rir][idx],
			      &reg);
	*rank = RIR_RNK_TGT(reg);

	edac_dbg(0, "RIR#%d: channel address 0x%08Lx < 0x%08Lx, RIR interleave %d, index %d\n",
		 n_rir,
		 ch_addr,
		 limit,
		 rir_way,
		 idx);

	return 0;
}

static void sbridge_put_devices(struct sbridge_dev *sbridge_dev)
{
	int i;

	edac_dbg(0, "\n");
	for (i = 0; i < sbridge_dev->n_devs; i++) {
		struct pci_dev *pdev = sbridge_dev->pdev[i];
		if (!pdev)
			continue;
		edac_dbg(0, "Removing dev %02x:%02x.%d\n",
			 pdev->bus->number,
			 PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn));
		pci_dev_put(pdev);
	}
}

static void sbridge_put_all_devices(void)
{
	struct sbridge_dev *sbridge_dev, *tmp;

	list_for_each_entry_safe(sbridge_dev, tmp, &sbridge_edac_list, list) {
		sbridge_put_devices(sbridge_dev);
		free_sbridge_dev(sbridge_dev);
	}
}

static int sbridge_get_onedevice(struct pci_dev **prev,
				 u8 *num_mc,
				 const struct pci_id_table *table,
				 const unsigned devno)
{
	struct sbridge_dev *sbridge_dev;
	const struct pci_id_descr *dev_descr = &table->descr[devno];

	struct pci_dev *pdev = NULL;
	u8 bus = 0;

	sbridge_printk(KERN_INFO,
		"Seeking for: dev %02x.%d PCI ID %04x:%04x\n",
		dev_descr->dev, dev_descr->func,
		PCI_VENDOR_ID_INTEL, dev_descr->dev_id);

	pdev = pci_get_device(PCI_VENDOR_ID_INTEL,
			      dev_descr->dev_id, *prev);

	if (!pdev) {
		if (*prev) {
			*prev = pdev;
			return 0;
		}

		if (dev_descr->optional)
			return 0;

		if (devno == 0)
			return -ENODEV;

		sbridge_printk(KERN_INFO,
			"Device not found: dev %02x.%d PCI ID %04x:%04x\n",
			dev_descr->dev, dev_descr->func,
			PCI_VENDOR_ID_INTEL, dev_descr->dev_id);

		return -ENODEV;
	}
	bus = pdev->bus->number;

	sbridge_dev = get_sbridge_dev(bus);
	if (!sbridge_dev) {
		sbridge_dev = alloc_sbridge_dev(bus, table);
		if (!sbridge_dev) {
			pci_dev_put(pdev);
			return -ENOMEM;
		}
		(*num_mc)++;
	}

	if (sbridge_dev->pdev[devno]) {
		sbridge_printk(KERN_ERR,
			"Duplicated device for "
			"dev %02x:%d.%d PCI ID %04x:%04x\n",
			bus, dev_descr->dev, dev_descr->func,
			PCI_VENDOR_ID_INTEL, dev_descr->dev_id);
		pci_dev_put(pdev);
		return -ENODEV;
	}

	sbridge_dev->pdev[devno] = pdev;

	if (unlikely(PCI_SLOT(pdev->devfn) != dev_descr->dev ||
			PCI_FUNC(pdev->devfn) != dev_descr->func)) {
		sbridge_printk(KERN_ERR,
			"Device PCI ID %04x:%04x "
			"has dev %02x:%d.%d instead of dev %02x:%02x.%d\n",
			PCI_VENDOR_ID_INTEL, dev_descr->dev_id,
			bus, PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn),
			bus, dev_descr->dev, dev_descr->func);
		return -ENODEV;
	}

	if (unlikely(pci_enable_device(pdev) < 0)) {
		sbridge_printk(KERN_ERR,
			"Couldn't enable "
			"dev %02x:%d.%d PCI ID %04x:%04x\n",
			bus, dev_descr->dev, dev_descr->func,
			PCI_VENDOR_ID_INTEL, dev_descr->dev_id);
		return -ENODEV;
	}

	edac_dbg(0, "Detected dev %02x:%d.%d PCI ID %04x:%04x\n",
		 bus, dev_descr->dev, dev_descr->func,
		 PCI_VENDOR_ID_INTEL, dev_descr->dev_id);

	pci_dev_get(pdev);

	*prev = pdev;

	return 0;
}

static int sbridge_get_all_devices(u8 *num_mc)
{
	int i, rc;
	struct pci_dev *pdev = NULL;
	const struct pci_id_table *table = pci_dev_descr_sbridge_table;

	while (table && table->descr) {
		for (i = 0; i < table->n_devs; i++) {
			pdev = NULL;
			do {
				rc = sbridge_get_onedevice(&pdev, num_mc,
							   table, i);
				if (rc < 0) {
					if (i == 0) {
						i = table->n_devs;
						break;
					}
					sbridge_put_all_devices();
					return -ENODEV;
				}
			} while (pdev);
		}
		table++;
	}

	return 0;
}

static int mci_bind_devs(struct mem_ctl_info *mci,
			 struct sbridge_dev *sbridge_dev)
{
	struct sbridge_pvt *pvt = mci->pvt_info;
	struct pci_dev *pdev;
	int i, func, slot;

	for (i = 0; i < sbridge_dev->n_devs; i++) {
		pdev = sbridge_dev->pdev[i];
		if (!pdev)
			continue;
		slot = PCI_SLOT(pdev->devfn);
		func = PCI_FUNC(pdev->devfn);
		switch (slot) {
		case 12:
			switch (func) {
			case 6:
				pvt->pci_sad0 = pdev;
				break;
			case 7:
				pvt->pci_sad1 = pdev;
				break;
			default:
				goto error;
			}
			break;
		case 13:
			switch (func) {
			case 6:
				pvt->pci_br = pdev;
				break;
			default:
				goto error;
			}
			break;
		case 14:
			switch (func) {
			case 0:
				pvt->pci_ha0 = pdev;
				break;
			default:
				goto error;
			}
			break;
		case 15:
			switch (func) {
			case 0:
				pvt->pci_ta = pdev;
				break;
			case 1:
				pvt->pci_ras = pdev;
				break;
			case 2:
			case 3:
			case 4:
			case 5:
				pvt->pci_tad[func - 2] = pdev;
				break;
			default:
				goto error;
			}
			break;
		case 17:
			switch (func) {
			case 0:
				pvt->pci_ddrio = pdev;
				break;
			default:
				goto error;
			}
			break;
		default:
			goto error;
		}

		edac_dbg(0, "Associated PCI %02x.%02d.%d with dev = %p\n",
			 sbridge_dev->bus,
			 PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn),
			 pdev);
	}

	if (!pvt->pci_sad0 || !pvt->pci_sad1 || !pvt->pci_ha0 ||
	    !pvt-> pci_tad || !pvt->pci_ras  || !pvt->pci_ta)
		goto enodev;

	for (i = 0; i < NUM_CHANNELS; i++) {
		if (!pvt->pci_tad[i])
			goto enodev;
	}
	return 0;

enodev:
	sbridge_printk(KERN_ERR, "Some needed devices are missing\n");
	return -ENODEV;

error:
	sbridge_printk(KERN_ERR, "Device %d, function %d "
		      "is out of the expected range\n",
		      slot, func);
	return -EINVAL;
}

static void sbridge_mce_output_error(struct mem_ctl_info *mci,
				    const struct mce *m)
{
	struct mem_ctl_info *new_mci;
	struct sbridge_pvt *pvt = mci->pvt_info;
	enum hw_event_mc_err_type tp_event;
	char *type, *optype, msg[256];
	bool ripv = GET_BITFIELD(m->mcgstatus, 0, 0);
	bool overflow = GET_BITFIELD(m->status, 62, 62);
	bool uncorrected_error = GET_BITFIELD(m->status, 61, 61);
	bool recoverable = GET_BITFIELD(m->status, 56, 56);
	u32 core_err_cnt = GET_BITFIELD(m->status, 38, 52);
	u32 mscod = GET_BITFIELD(m->status, 16, 31);
	u32 errcode = GET_BITFIELD(m->status, 0, 15);
	u32 channel = GET_BITFIELD(m->status, 0, 3);
	u32 optypenum = GET_BITFIELD(m->status, 4, 6);
	long channel_mask, first_channel;
	u8  rank, socket;
	int rc, dimm;
	char *area_type = NULL;

	if (uncorrected_error) {
		if (ripv) {
			type = "FATAL";
			tp_event = HW_EVENT_ERR_FATAL;
		} else {
			type = "NON_FATAL";
			tp_event = HW_EVENT_ERR_UNCORRECTED;
		}
	} else {
		type = "CORRECTED";
		tp_event = HW_EVENT_ERR_CORRECTED;
	}

	if (! ((errcode & 0xef80) == 0x80)) {
		optype = "Can't parse: it is not a mem";
	} else {
		switch (optypenum) {
		case 0:
			optype = "generic undef request error";
			break;
		case 1:
			optype = "memory read error";
			break;
		case 2:
			optype = "memory write error";
			break;
		case 3:
			optype = "addr/cmd error";
			break;
		case 4:
			optype = "memory scrubbing error";
			break;
		default:
			optype = "reserved";
			break;
		}
	}

	rc = get_memory_error_data(mci, m->addr, &socket,
				   &channel_mask, &rank, &area_type, msg);
	if (rc < 0)
		goto err_parsing;
	new_mci = get_mci_for_node_id(socket);
	if (!new_mci) {
		strcpy(msg, "Error: socket got corrupted!");
		goto err_parsing;
	}
	mci = new_mci;
	pvt = mci->pvt_info;

	first_channel = find_first_bit(&channel_mask, NUM_CHANNELS);

	if (rank < 4)
		dimm = 0;
	else if (rank < 8)
		dimm = 1;
	else
		dimm = 2;

	snprintf(msg, sizeof(msg),
		 "%s%s area:%s err_code:%04x:%04x socket:%d channel_mask:%ld rank:%d",
		 overflow ? " OVERFLOW" : "",
		 (uncorrected_error && recoverable) ? " recoverable" : "",
		 area_type,
		 mscod, errcode,
		 socket,
		 channel_mask,
		 rank);

	edac_dbg(0, "%s\n", msg);

	if (channel == CHANNEL_UNSPECIFIED)
		channel = -1;

	edac_mc_handle_error(tp_event, mci, core_err_cnt,
			     m->addr >> PAGE_SHIFT, m->addr & ~PAGE_MASK, 0,
			     channel, dimm, -1,
			     optype, msg);
	return;
err_parsing:
	edac_mc_handle_error(tp_event, mci, core_err_cnt, 0, 0, 0,
			     -1, -1, -1,
			     msg, "");

}

static void sbridge_check_error(struct mem_ctl_info *mci)
{
	struct sbridge_pvt *pvt = mci->pvt_info;
	int i;
	unsigned count = 0;
	struct mce *m;

	smp_rmb();
	count = (pvt->mce_out + MCE_LOG_LEN - pvt->mce_in)
		% MCE_LOG_LEN;
	if (!count)
		return;

	m = pvt->mce_outentry;
	if (pvt->mce_in + count > MCE_LOG_LEN) {
		unsigned l = MCE_LOG_LEN - pvt->mce_in;

		memcpy(m, &pvt->mce_entry[pvt->mce_in], sizeof(*m) * l);
		smp_wmb();
		pvt->mce_in = 0;
		count -= l;
		m += l;
	}
	memcpy(m, &pvt->mce_entry[pvt->mce_in], sizeof(*m) * count);
	smp_wmb();
	pvt->mce_in += count;

	smp_rmb();
	if (pvt->mce_overrun) {
		sbridge_printk(KERN_ERR, "Lost %d memory errors\n",
			      pvt->mce_overrun);
		smp_wmb();
		pvt->mce_overrun = 0;
	}

	for (i = 0; i < count; i++)
		sbridge_mce_output_error(mci, &pvt->mce_outentry[i]);
}

static int sbridge_mce_check_error(struct notifier_block *nb, unsigned long val,
				   void *data)
{
	struct mce *mce = (struct mce *)data;
	struct mem_ctl_info *mci;
	struct sbridge_pvt *pvt;

	mci = get_mci_for_node_id(mce->socketid);
	if (!mci)
		return NOTIFY_DONE;
	pvt = mci->pvt_info;

	if ((mce->status & 0xefff) >> 7 != 1)
		return NOTIFY_DONE;

	printk("sbridge: HANDLING MCE MEMORY ERROR\n");

	printk("CPU %d: Machine Check Exception: %Lx Bank %d: %016Lx\n",
	       mce->extcpu, mce->mcgstatus, mce->bank, mce->status);
	printk("TSC %llx ", mce->tsc);
	printk("ADDR %llx ", mce->addr);
	printk("MISC %llx ", mce->misc);

	printk("PROCESSOR %u:%x TIME %llu SOCKET %u APIC %x\n",
		mce->cpuvendor, mce->cpuid, mce->time,
		mce->socketid, mce->apicid);

	if (cpu_data(mce->cpu).phys_proc_id != pvt->sbridge_dev->mc)
		return NOTIFY_DONE;

	smp_rmb();
	if ((pvt->mce_out + 1) % MCE_LOG_LEN == pvt->mce_in) {
		smp_wmb();
		pvt->mce_overrun++;
		return NOTIFY_DONE;
	}

	memcpy(&pvt->mce_entry[pvt->mce_out], mce, sizeof(*mce));
	smp_wmb();
	pvt->mce_out = (pvt->mce_out + 1) % MCE_LOG_LEN;

	if (mce->mcgstatus & 1)
		sbridge_check_error(mci);

	return NOTIFY_STOP;
}

static struct notifier_block sbridge_mce_dec = {
	.notifier_call      = sbridge_mce_check_error,
};

static void sbridge_unregister_mci(struct sbridge_dev *sbridge_dev)
{
	struct mem_ctl_info *mci = sbridge_dev->mci;
	struct sbridge_pvt *pvt;

	if (unlikely(!mci || !mci->pvt_info)) {
		edac_dbg(0, "MC: dev = %p\n", &sbridge_dev->pdev[0]->dev);

		sbridge_printk(KERN_ERR, "Couldn't find mci handler\n");
		return;
	}

	pvt = mci->pvt_info;

	edac_dbg(0, "MC: mci = %p, dev = %p\n",
		 mci, &sbridge_dev->pdev[0]->dev);

	edac_mc_del_mc(mci->pdev);

	edac_dbg(1, "%s: free mci struct\n", mci->ctl_name);
	kfree(mci->ctl_name);
	edac_mc_free(mci);
	sbridge_dev->mci = NULL;
}

static int sbridge_register_mci(struct sbridge_dev *sbridge_dev)
{
	struct mem_ctl_info *mci;
	struct edac_mc_layer layers[2];
	struct sbridge_pvt *pvt;
	int rc;

	rc = check_if_ecc_is_active(sbridge_dev->bus);
	if (unlikely(rc < 0))
		return rc;

	layers[0].type = EDAC_MC_LAYER_CHANNEL;
	layers[0].size = NUM_CHANNELS;
	layers[0].is_virt_csrow = false;
	layers[1].type = EDAC_MC_LAYER_SLOT;
	layers[1].size = MAX_DIMMS;
	layers[1].is_virt_csrow = true;
	mci = edac_mc_alloc(sbridge_dev->mc, ARRAY_SIZE(layers), layers,
			    sizeof(*pvt));

	if (unlikely(!mci))
		return -ENOMEM;

	edac_dbg(0, "MC: mci = %p, dev = %p\n",
		 mci, &sbridge_dev->pdev[0]->dev);

	pvt = mci->pvt_info;
	memset(pvt, 0, sizeof(*pvt));

	pvt->sbridge_dev = sbridge_dev;
	sbridge_dev->mci = mci;

	mci->mtype_cap = MEM_FLAG_DDR3;
	mci->edac_ctl_cap = EDAC_FLAG_NONE;
	mci->edac_cap = EDAC_FLAG_NONE;
	mci->mod_name = "sbridge_edac.c";
	mci->mod_ver = SBRIDGE_REVISION;
	mci->ctl_name = kasprintf(GFP_KERNEL, "Sandy Bridge Socket#%d", mci->mc_idx);
	mci->dev_name = pci_name(sbridge_dev->pdev[0]);
	mci->ctl_page_to_phys = NULL;

	mci->edac_check = sbridge_check_error;

	rc = mci_bind_devs(mci, sbridge_dev);
	if (unlikely(rc < 0))
		goto fail0;

	get_dimm_config(mci);
	get_memory_layout(mci);

	mci->pdev = &sbridge_dev->pdev[0]->dev;

	if (unlikely(edac_mc_add_mc(mci))) {
		edac_dbg(0, "MC: failed edac_mc_add_mc()\n");
		rc = -EINVAL;
		goto fail0;
	}

	return 0;

fail0:
	kfree(mci->ctl_name);
	edac_mc_free(mci);
	sbridge_dev->mci = NULL;
	return rc;
}

static int sbridge_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int rc;
	u8 mc, num_mc = 0;
	struct sbridge_dev *sbridge_dev;

	mutex_lock(&sbridge_edac_lock);

	if (unlikely(probed >= 1)) {
		mutex_unlock(&sbridge_edac_lock);
		return -ENODEV;
	}
	probed++;

	rc = sbridge_get_all_devices(&num_mc);
	if (unlikely(rc < 0))
		goto fail0;
	mc = 0;

	list_for_each_entry(sbridge_dev, &sbridge_edac_list, list) {
		edac_dbg(0, "Registering MC#%d (%d of %d)\n",
			 mc, mc + 1, num_mc);
		sbridge_dev->mc = mc++;
		rc = sbridge_register_mci(sbridge_dev);
		if (unlikely(rc < 0))
			goto fail1;
	}

	sbridge_printk(KERN_INFO, "Driver loaded.\n");

	mutex_unlock(&sbridge_edac_lock);
	return 0;

fail1:
	list_for_each_entry(sbridge_dev, &sbridge_edac_list, list)
		sbridge_unregister_mci(sbridge_dev);

	sbridge_put_all_devices();
fail0:
	mutex_unlock(&sbridge_edac_lock);
	return rc;
}

static void sbridge_remove(struct pci_dev *pdev)
{
	struct sbridge_dev *sbridge_dev;

	edac_dbg(0, "\n");

	mutex_lock(&sbridge_edac_lock);

	if (unlikely(!probed)) {
		mutex_unlock(&sbridge_edac_lock);
		return;
	}

	list_for_each_entry(sbridge_dev, &sbridge_edac_list, list)
		sbridge_unregister_mci(sbridge_dev);

	sbridge_put_all_devices();

	probed--;

	mutex_unlock(&sbridge_edac_lock);
}

MODULE_DEVICE_TABLE(pci, sbridge_pci_tbl);

static struct pci_driver sbridge_driver = {
	.name     = "sbridge_edac",
	.probe    = sbridge_probe,
	.remove   = sbridge_remove,
	.id_table = sbridge_pci_tbl,
};

static int __init sbridge_init(void)
{
	int pci_rc;

	edac_dbg(2, "\n");

	opstate_init();

	pci_rc = pci_register_driver(&sbridge_driver);

	if (pci_rc >= 0) {
		mce_register_decode_chain(&sbridge_mce_dec);
		return 0;
	}

	sbridge_printk(KERN_ERR, "Failed to register device with error %d.\n",
		      pci_rc);

	return pci_rc;
}

static void __exit sbridge_exit(void)
{
	edac_dbg(2, "\n");
	pci_unregister_driver(&sbridge_driver);
	mce_unregister_decode_chain(&sbridge_mce_dec);
}

module_init(sbridge_init);
module_exit(sbridge_exit);

module_param(edac_op_state, int, 0444);
MODULE_PARM_DESC(edac_op_state, "EDAC Error Reporting state: 0=Poll,1=NMI");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mauro Carvalho Chehab <mchehab@redhat.com>");
MODULE_AUTHOR("Red Hat Inc. (http://www.redhat.com)");
MODULE_DESCRIPTION("MC Driver for Intel Sandy Bridge memory controllers - "
		   SBRIDGE_REVISION);
