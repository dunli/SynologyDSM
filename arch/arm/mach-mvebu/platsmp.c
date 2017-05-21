#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/clk.h>
#include <linux/of.h>
#if defined(MY_ABC_HERE)
#include <linux/of_address.h>
#endif  
#include <linux/mbus.h>
#include <asm/cacheflush.h>
#include <asm/smp_plat.h>
#include "common.h"
#include "armada-370-xp.h"
#include "pmsu.h"
#include "coherency.h"

#if defined(MY_ABC_HERE)
#define AXP_BOOTROM_BASE 0xfff00000
#define AXP_BOOTROM_SIZE 0x100000
#endif  

void __init set_secondary_cpus_clock(void)
{
	int thiscpu;
	unsigned long rate;
	struct clk *cpu_clk = NULL;
	struct device_node *np = NULL;

	thiscpu = smp_processor_id();
	for_each_node_by_type(np, "cpu") {
		int err;
		int cpu;

		err = of_property_read_u32(np, "reg", &cpu);
		if (WARN_ON(err))
			return;

		if (cpu == thiscpu) {
			cpu_clk = of_clk_get(np, 0);
			break;
		}
	}
	if (WARN_ON(IS_ERR(cpu_clk)))
		return;
	clk_prepare_enable(cpu_clk);
	rate = clk_get_rate(cpu_clk);

	for_each_node_by_type(np, "cpu") {
		int err;
		int cpu;

		err = of_property_read_u32(np, "reg", &cpu);
		if (WARN_ON(err))
			return;

		if (cpu != thiscpu) {
			cpu_clk = of_clk_get(np, 0);
			clk_set_rate(cpu_clk, rate);
#if defined(MY_ABC_HERE)
			clk_prepare_enable(cpu_clk);
#endif  
		}
	}
}

static void __cpuinit armada_xp_secondary_init(unsigned int cpu)
{
	armada_xp_mpic_smp_cpu_init();
}

static int __cpuinit armada_xp_boot_secondary(unsigned int cpu,
					      struct task_struct *idle)
{
	pr_info("Booting CPU %d\n", cpu);

#if defined(MY_ABC_HERE)
	mvebu_pmsu_set_cpu_boot_addr(cpu, armada_xp_secondary_startup);
	mvebu_boot_cpu(cpu);
#else  
	armada_xp_boot_cpu(cpu, armada_xp_secondary_startup);
#endif  

	return 0;
}

#if defined(MY_ABC_HERE)
static void __init armada_xp_smp_init_cpus(void)
{
	unsigned int ncores = num_possible_cpus();

	if (ncores == 0 || ncores > ARMADA_XP_MAX_CPUS)
		panic("Invalid number of CPUs in DT\n");

	set_smp_cross_call(armada_mpic_send_doorbell);
}

void __init armada_xp_smp_prepare_cpus(unsigned int max_cpus)
{
	struct device_node *node;
	struct resource res;
	int err;

	set_secondary_cpus_clock();
	flush_cache_all();
	set_cpu_coherent();

	node = of_find_compatible_node(NULL, NULL, "marvell,bootrom");
	if (!node)
		panic("Cannot find 'marvell,bootrom' compatible node");

	err = of_address_to_resource(node, 0, &res);
	if (err < 0)
		panic("Cannot get 'bootrom' node address");

	if (res.start != AXP_BOOTROM_BASE ||
	    resource_size(&res) != AXP_BOOTROM_SIZE)
		panic("The address for the BootROM is incorrect");
}
#else  
static void __init armada_xp_smp_init_cpus(void)
{
	unsigned int i, ncores;
	ncores = coherency_get_cpu_count();

	if (ncores > nr_cpu_ids) {
		pr_warn("SMP: %d CPUs physically present. Only %d configured.",
			ncores, nr_cpu_ids);
		pr_warn("Clipping CPU count to %d\n", nr_cpu_ids);
		ncores = nr_cpu_ids;
	}

	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);

	set_smp_cross_call(armada_mpic_send_doorbell);
}

void __init armada_xp_smp_prepare_cpus(unsigned int max_cpus)
{
	set_secondary_cpus_clock();
	flush_cache_all();
	set_cpu_coherent(cpu_logical_map(smp_processor_id()), 0);
	mvebu_mbus_add_window("bootrom", 0xfff00000, SZ_1M);
}
#endif  

struct smp_operations armada_xp_smp_ops __initdata = {
	.smp_init_cpus		= armada_xp_smp_init_cpus,
	.smp_prepare_cpus	= armada_xp_smp_prepare_cpus,
	.smp_secondary_init	= armada_xp_secondary_init,
	.smp_boot_secondary	= armada_xp_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die		= armada_xp_cpu_die,
#endif
};
