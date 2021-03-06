#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef _ASM_PGTABLE_2LEVEL_HWDEF_H
#define _ASM_PGTABLE_2LEVEL_HWDEF_H

#define PMD_TYPE_MASK		(_AT(pmdval_t, 3) << 0)
#define PMD_TYPE_FAULT		(_AT(pmdval_t, 0) << 0)
#define PMD_TYPE_TABLE		(_AT(pmdval_t, 1) << 0)
#define PMD_TYPE_SECT		(_AT(pmdval_t, 2) << 0)
#define PMD_BIT4		(_AT(pmdval_t, 1) << 4)
#define PMD_DOMAIN(x)		(_AT(pmdval_t, (x)) << 5)
#define PMD_PROTECTION		(_AT(pmdval_t, 1) << 9)		 
 
#define PMD_SECT_BUFFERABLE	(_AT(pmdval_t, 1) << 2)
#define PMD_SECT_CACHEABLE	(_AT(pmdval_t, 1) << 3)
#define PMD_SECT_XN		(_AT(pmdval_t, 1) << 4)		 
#define PMD_SECT_AP_WRITE	(_AT(pmdval_t, 1) << 10)
#define PMD_SECT_AP_READ	(_AT(pmdval_t, 1) << 11)
#define PMD_SECT_TEX(x)		(_AT(pmdval_t, (x)) << 12)	 
#define PMD_SECT_APX		(_AT(pmdval_t, 1) << 15)	 
#define PMD_SECT_S		(_AT(pmdval_t, 1) << 16)	 
#define PMD_SECT_nG		(_AT(pmdval_t, 1) << 17)	 
#define PMD_SECT_SUPER		(_AT(pmdval_t, 1) << 18)	 
#define PMD_SECT_AF		(_AT(pmdval_t, 0))

#define PMD_SECT_UNCACHED	(_AT(pmdval_t, 0))
#define PMD_SECT_BUFFERED	(PMD_SECT_BUFFERABLE)
#define PMD_SECT_WT		(PMD_SECT_CACHEABLE)
#define PMD_SECT_WB		(PMD_SECT_CACHEABLE | PMD_SECT_BUFFERABLE)
#define PMD_SECT_MINICACHE	(PMD_SECT_TEX(1) | PMD_SECT_CACHEABLE)
#define PMD_SECT_WBWA		(PMD_SECT_TEX(1) | PMD_SECT_CACHEABLE | PMD_SECT_BUFFERABLE)
#define PMD_SECT_NONSHARED_DEV	(PMD_SECT_TEX(2))

#define PTE_TYPE_MASK		(_AT(pteval_t, 3) << 0)
#define PTE_TYPE_FAULT		(_AT(pteval_t, 0) << 0)
#define PTE_TYPE_LARGE		(_AT(pteval_t, 1) << 0)
#define PTE_TYPE_SMALL		(_AT(pteval_t, 2) << 0)
#define PTE_TYPE_EXT		(_AT(pteval_t, 3) << 0)		 
#define PTE_BUFFERABLE		(_AT(pteval_t, 1) << 2)
#define PTE_CACHEABLE		(_AT(pteval_t, 1) << 3)

#if defined(MY_ABC_HERE) && defined(CONFIG_MV_64KB_MMU_PAGE_SIZE_SUPPORT)
#define PTE_EXT_XN		(_AT(pteval_t, 1) << 15)	 
#else
#define PTE_EXT_XN		(_AT(pteval_t, 1) << 0)		 
#endif
#define PTE_EXT_AP_MASK		(_AT(pteval_t, 3) << 4)
#define PTE_EXT_AP0		(_AT(pteval_t, 1) << 4)
#define PTE_EXT_AP1		(_AT(pteval_t, 2) << 4)
#define PTE_EXT_AP_UNO_SRO	(_AT(pteval_t, 0) << 4)
#define PTE_EXT_AP_UNO_SRW	(PTE_EXT_AP0)
#define PTE_EXT_AP_URO_SRW	(PTE_EXT_AP1)
#define PTE_EXT_AP_URW_SRW	(PTE_EXT_AP1|PTE_EXT_AP0)
#if defined(MY_ABC_HERE) && defined(CONFIG_MV_64KB_MMU_PAGE_SIZE_SUPPORT)
#define PTE_EXT_TEX(x)		(_AT(pteval_t, (x)) << 12)	 
#else
#define PTE_EXT_TEX(x)		(_AT(pteval_t, (x)) << 6)	 
#endif
#define PTE_EXT_APX		(_AT(pteval_t, 1) << 9)		 
#define PTE_EXT_COHERENT	(_AT(pteval_t, 1) << 9)		 
#define PTE_EXT_SHARED		(_AT(pteval_t, 1) << 10)	 
#define PTE_EXT_NG		(_AT(pteval_t, 1) << 11)	 

#define PTE_SMALL_AP_MASK	(_AT(pteval_t, 0xff) << 4)
#define PTE_SMALL_AP_UNO_SRO	(_AT(pteval_t, 0x00) << 4)
#define PTE_SMALL_AP_UNO_SRW	(_AT(pteval_t, 0x55) << 4)
#define PTE_SMALL_AP_URO_SRW	(_AT(pteval_t, 0xaa) << 4)
#define PTE_SMALL_AP_URW_SRW	(_AT(pteval_t, 0xff) << 4)

#define PHYS_MASK		(~0UL)

#endif
