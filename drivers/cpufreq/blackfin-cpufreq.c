#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <asm/blackfin.h>
#include <asm/time.h>
#include <asm/dpmc.h>

static struct cpufreq_frequency_table bfin_freq_table[] = {
	{
		.frequency = CPUFREQ_TABLE_END,
		.index = 0,
	},
	{
		.frequency = CPUFREQ_TABLE_END,
		.index = 1,
	},
	{
		.frequency = CPUFREQ_TABLE_END,
		.index = 2,
	},
	{
		.frequency = CPUFREQ_TABLE_END,
		.index = 0,
	},
};

static struct bfin_dpm_state {
	unsigned int csel;  
	unsigned int tscale;  
} dpm_state_table[3];

#if defined(CONFIG_CYCLES_CLOCKSOURCE)
 
unsigned long long __bfin_cycles_off;
unsigned int __bfin_cycles_mod;
#endif

static void __init bfin_init_tables(unsigned long cclk, unsigned long sclk)
{

	unsigned long csel, min_cclk;
	int index;

#if ANOMALY_05000273 || ANOMALY_05000274 || \
	(!(defined(CONFIG_BF54x) || defined(CONFIG_BF60x)) \
	&& defined(CONFIG_BFIN_EXTMEM_DCACHEABLE))
	min_cclk = sclk * 2;
#else
	min_cclk = sclk;
#endif

#ifndef CONFIG_BF60x
	csel = ((bfin_read_PLL_DIV() & CSEL) >> 4);
#else
	csel = bfin_read32(CGU0_DIV) & 0x1F;
#endif

	for (index = 0;  (cclk >> index) >= min_cclk && csel <= 3 && index < 3; index++, csel++) {
		bfin_freq_table[index].frequency = cclk >> index;
#ifndef CONFIG_BF60x
		dpm_state_table[index].csel = csel << 4;  
#else
		dpm_state_table[index].csel = csel;
#endif
		dpm_state_table[index].tscale =  (TIME_SCALE >> index) - 1;

		pr_debug("cpufreq: freq:%d csel:0x%x tscale:%d\n",
						 bfin_freq_table[index].frequency,
						 dpm_state_table[index].csel,
						 dpm_state_table[index].tscale);
	}
	return;
}

static void bfin_adjust_core_timer(void *info)
{
	unsigned int tscale;
	unsigned int index = *(unsigned int *)info;

	tscale = dpm_state_table[index].tscale;
	bfin_write_TSCALE(tscale);
	return;
}

static unsigned int bfin_getfreq_khz(unsigned int cpu)
{
	 
	return get_cclk() / 1000;
}

#ifdef CONFIG_BF60x
unsigned long cpu_set_cclk(int cpu, unsigned long new)
{
	struct clk *clk;
	int ret;

	clk = clk_get(NULL, "CCLK");
	if (IS_ERR(clk))
		return -ENODEV;

	ret = clk_set_rate(clk, new);
	clk_put(clk);
	return ret;
}
#endif

static int bfin_target(struct cpufreq_policy *policy,
			unsigned int target_freq, unsigned int relation)
{
#ifndef CONFIG_BF60x
	unsigned int plldiv;
#endif
	unsigned int index;
	unsigned long cclk_hz;
	struct cpufreq_freqs freqs;
	static unsigned long lpj_ref;
	static unsigned int  lpj_ref_freq;
	int ret = 0;

#if defined(CONFIG_CYCLES_CLOCKSOURCE)
	cycles_t cycles;
#endif

	if (cpufreq_frequency_table_target(policy, bfin_freq_table, target_freq,
				relation, &index))
		return -EINVAL;

	cclk_hz = bfin_freq_table[index].frequency;

	freqs.old = bfin_getfreq_khz(0);
	freqs.new = cclk_hz;

	pr_debug("cpufreq: changing cclk to %lu; target = %u, oldfreq = %u\n",
			cclk_hz, target_freq, freqs.old);

	cpufreq_notify_transition(policy, &freqs, CPUFREQ_PRECHANGE);
#ifndef CONFIG_BF60x
	plldiv = (bfin_read_PLL_DIV() & SSEL) | dpm_state_table[index].csel;
	bfin_write_PLL_DIV(plldiv);
#else
	ret = cpu_set_cclk(policy->cpu, freqs.new * 1000);
	if (ret != 0) {
		WARN_ONCE(ret, "cpufreq set freq failed %d\n", ret);
		return ret;
	}
#endif
	on_each_cpu(bfin_adjust_core_timer, &index, 1);
#if defined(CONFIG_CYCLES_CLOCKSOURCE)
	cycles = get_cycles();
	SSYNC();
	cycles += 10;  
	__bfin_cycles_off += (cycles << __bfin_cycles_mod) - (cycles << index);
	__bfin_cycles_mod = index;
#endif
	if (!lpj_ref_freq) {
		lpj_ref = loops_per_jiffy;
		lpj_ref_freq = freqs.old;
	}
	if (freqs.new != freqs.old) {
		loops_per_jiffy = cpufreq_scale(lpj_ref,
				lpj_ref_freq, freqs.new);
	}

	cpufreq_notify_transition(policy, &freqs, CPUFREQ_POSTCHANGE);

	pr_debug("cpufreq: done\n");
	return ret;
}

static int bfin_verify_speed(struct cpufreq_policy *policy)
{
	return cpufreq_frequency_table_verify(policy, bfin_freq_table);
}

static int __bfin_cpu_init(struct cpufreq_policy *policy)
{

	unsigned long cclk, sclk;

	cclk = get_cclk() / 1000;
	sclk = get_sclk() / 1000;

	if (policy->cpu == CPUFREQ_CPU)
		bfin_init_tables(cclk, sclk);

	policy->cpuinfo.transition_latency = 50000;  

	policy->cur = cclk;
	cpufreq_frequency_table_get_attr(bfin_freq_table, policy->cpu);
	return cpufreq_frequency_table_cpuinfo(policy, bfin_freq_table);
}

static struct freq_attr *bfin_freq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver bfin_driver = {
	.verify = bfin_verify_speed,
	.target = bfin_target,
	.get = bfin_getfreq_khz,
	.init = __bfin_cpu_init,
	.name = "bfin cpufreq",
#if defined(MY_ABC_HERE)
	 
#else  
	.owner = THIS_MODULE,
#endif  
	.attr = bfin_freq_attr,
};

static int __init bfin_cpu_init(void)
{
	return cpufreq_register_driver(&bfin_driver);
}

static void __exit bfin_cpu_exit(void)
{
	cpufreq_unregister_driver(&bfin_driver);
}

MODULE_AUTHOR("Michael Hennerich <hennerich@blackfin.uclinux.org>");
MODULE_DESCRIPTION("cpufreq driver for Blackfin");
MODULE_LICENSE("GPL");

module_init(bfin_cpu_init);
module_exit(bfin_cpu_exit);
