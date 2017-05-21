/*
 * arch/arm/mach-mvebu/include/mach/coherency.h
 *
 *
 * Coherency fabric (Aurora) support for Armada 370 and XP platforms.
 *
 * Copyright (C) 2012 Marvell
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __MACH_370_XP_COHERENCY_H
#define __MACH_370_XP_COHERENCY_H

#if defined(CONFIG_SYNO_LSP_ARMADA)
extern unsigned long coherency_phys_base;
extern bool coherency_hard_mode;

#define COHERENCY_FABRIC_HARD_MODE() coherency_hard_mode
int set_cpu_coherent(void);

int coherency_init(void);
int coherency_available(void);
#else /* CONFIG_SYNO_LSP_ARMADA */
#ifdef CONFIG_SMP
int coherency_get_cpu_count(void);
#endif

int set_cpu_coherent(int cpu_id, int smp_group_id);
int coherency_available(void);
int coherency_init(void);
#endif /* CONFIG_SYNO_LSP_ARMADA */

#endif	/* __MACH_370_XP_COHERENCY_H */
