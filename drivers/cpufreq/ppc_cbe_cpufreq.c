#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/cpufreq.h>
#include <linux/module.h>
#include <linux/of_platform.h>

#include <asm/machdep.h>
#include <asm/prom.h>
#include <asm/cell-regs.h>

#include "ppc_cbe_cpufreq.h"

static DEFINE_MUTEX(cbe_switch_mutex);

static struct cpufreq_frequency_table cbe_freqs[] = {
	{1,	0},
	{2,	0},
	{3,	0},
	{4,	0},
	{5,	0},
	{6,	0},
	{8,	0},
	{10,	0},
	{0,	CPUFREQ_TABLE_END},
};

static int set_pmode(unsigned int cpu, unsigned int slow_mode)
{
	int rc;

	if (cbe_cpufreq_has_pmi)
		rc = cbe_cpufreq_set_pmode_pmi(cpu, slow_mode);
	else
		rc = cbe_cpufreq_set_pmode(cpu, slow_mode);

	pr_debug("register contains slow mode %d\n", cbe_cpufreq_get_pmode(cpu));

	return rc;
}

static int cbe_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	const u32 *max_freqp;
	u32 max_freq;
	int i, cur_pmode;
	struct device_node *cpu;

	cpu = of_get_cpu_node(policy->cpu, NULL);

	if (!cpu)
		return -ENODEV;

	pr_debug("init cpufreq on CPU %d\n", policy->cpu);

	if (!cbe_get_cpu_pmd_regs(policy->cpu) ||
	    !cbe_get_cpu_mic_tm_regs(policy->cpu)) {
		pr_info("invalid CBE regs pointers for cpufreq\n");
		return -EINVAL;
	}

	max_freqp = of_get_property(cpu, "clock-frequency", NULL);

	of_node_put(cpu);

	if (!max_freqp)
		return -EINVAL;

	max_freq = *max_freqp / 1000;

	pr_debug("max clock-frequency is at %u kHz\n", max_freq);
	pr_debug("initializing frequency table\n");

	for (i=0; cbe_freqs[i].frequency!=CPUFREQ_TABLE_END; i++) {
		cbe_freqs[i].frequency = max_freq / cbe_freqs[i].index;
		pr_debug("%d: %d\n", i, cbe_freqs[i].frequency);
	}

	policy->cpuinfo.transition_latency = 25000;

	cur_pmode = cbe_cpufreq_get_pmode(policy->cpu);
	pr_debug("current pmode is at %d\n",cur_pmode);

	policy->cur = cbe_freqs[cur_pmode].frequency;

#ifdef CONFIG_SMP
	cpumask_copy(policy->cpus, cpu_sibling_mask(policy->cpu));
#endif

	cpufreq_frequency_table_get_attr(cbe_freqs, policy->cpu);

	return cpufreq_frequency_table_cpuinfo(policy, cbe_freqs);
}

static int cbe_cpufreq_cpu_exit(struct cpufreq_policy *policy)
{
	cpufreq_frequency_table_put_attr(policy->cpu);
	return 0;
}

static int cbe_cpufreq_verify(struct cpufreq_policy *policy)
{
	return cpufreq_frequency_table_verify(policy, cbe_freqs);
}

static int cbe_cpufreq_target(struct cpufreq_policy *policy,
			      unsigned int target_freq,
			      unsigned int relation)
{
	int rc;
	struct cpufreq_freqs freqs;
	unsigned int cbe_pmode_new;

	cpufreq_frequency_table_target(policy,
				       cbe_freqs,
				       target_freq,
				       relation,
				       &cbe_pmode_new);

	freqs.old = policy->cur;
	freqs.new = cbe_freqs[cbe_pmode_new].frequency;

	mutex_lock(&cbe_switch_mutex);
	cpufreq_notify_transition(policy, &freqs, CPUFREQ_PRECHANGE);

	pr_debug("setting frequency for cpu %d to %d kHz, " \
		 "1/%d of max frequency\n",
		 policy->cpu,
		 cbe_freqs[cbe_pmode_new].frequency,
		 cbe_freqs[cbe_pmode_new].index);

	rc = set_pmode(policy->cpu, cbe_pmode_new);

	cpufreq_notify_transition(policy, &freqs, CPUFREQ_POSTCHANGE);
	mutex_unlock(&cbe_switch_mutex);

	return rc;
}

static struct cpufreq_driver cbe_cpufreq_driver = {
	.verify		= cbe_cpufreq_verify,
	.target		= cbe_cpufreq_target,
	.init		= cbe_cpufreq_cpu_init,
	.exit		= cbe_cpufreq_cpu_exit,
	.name		= "cbe-cpufreq",
#if defined(MY_ABC_HERE)
	 
#else  
	.owner		= THIS_MODULE,
#endif  
	.flags		= CPUFREQ_CONST_LOOPS,
};

static int __init cbe_cpufreq_init(void)
{
	if (!machine_is(cell))
		return -ENODEV;

	return cpufreq_register_driver(&cbe_cpufreq_driver);
}

static void __exit cbe_cpufreq_exit(void)
{
	cpufreq_unregister_driver(&cbe_cpufreq_driver);
}

module_init(cbe_cpufreq_init);
module_exit(cbe_cpufreq_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Christian Krafft <krafft@de.ibm.com>");
