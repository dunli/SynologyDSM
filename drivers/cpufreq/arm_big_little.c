#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/export.h>
#include <linux/of_platform.h>
#if defined(MY_ABC_HERE)
#include <linux/pm_opp.h>
#else  
#include <linux/opp.h>
#endif  
#include <linux/slab.h>
#include <linux/topology.h>
#include <linux/types.h>

#include "arm_big_little.h"

#define MAX_CLUSTERS	2

static struct cpufreq_arm_bL_ops *arm_bL_ops;
static struct clk *clk[MAX_CLUSTERS];
static struct cpufreq_frequency_table *freq_table[MAX_CLUSTERS];
static atomic_t cluster_usage[MAX_CLUSTERS] = {ATOMIC_INIT(0), ATOMIC_INIT(0)};

static unsigned int bL_cpufreq_get(unsigned int cpu)
{
	u32 cur_cluster = cpu_to_cluster(cpu);

	return clk_get_rate(clk[cur_cluster]) / 1000;
}

static int bL_cpufreq_verify_policy(struct cpufreq_policy *policy)
{
	u32 cur_cluster = cpu_to_cluster(policy->cpu);

	return cpufreq_frequency_table_verify(policy, freq_table[cur_cluster]);
}

static int bL_cpufreq_set_target(struct cpufreq_policy *policy,
		unsigned int target_freq, unsigned int relation)
{
	struct cpufreq_freqs freqs;
	u32 cpu = policy->cpu, freq_tab_idx, cur_cluster;
	int ret = 0;

	cur_cluster = cpu_to_cluster(policy->cpu);

	freqs.old = bL_cpufreq_get(policy->cpu);

	cpufreq_frequency_table_target(policy, freq_table[cur_cluster],
			target_freq, relation, &freq_tab_idx);
	freqs.new = freq_table[cur_cluster][freq_tab_idx].frequency;

	pr_debug("%s: cpu: %d, cluster: %d, oldfreq: %d, target freq: %d, new freq: %d\n",
			__func__, cpu, cur_cluster, freqs.old, target_freq,
			freqs.new);

	if (freqs.old == freqs.new)
		return 0;

	cpufreq_notify_transition(policy, &freqs, CPUFREQ_PRECHANGE);

	ret = clk_set_rate(clk[cur_cluster], freqs.new * 1000);
	if (ret) {
		pr_err("clk_set_rate failed: %d\n", ret);
		return ret;
	}

	policy->cur = freqs.new;

	cpufreq_notify_transition(policy, &freqs, CPUFREQ_POSTCHANGE);

	return ret;
}

static void put_cluster_clk_and_freq_table(struct device *cpu_dev)
{
	u32 cluster = cpu_to_cluster(cpu_dev->id);

	if (!atomic_dec_return(&cluster_usage[cluster])) {
		clk_put(clk[cluster]);
#if defined(MY_ABC_HERE)
		dev_pm_opp_free_cpufreq_table(cpu_dev, &freq_table[cluster]);
#else  
		opp_free_cpufreq_table(cpu_dev, &freq_table[cluster]);
#endif  
		dev_dbg(cpu_dev, "%s: cluster: %d\n", __func__, cluster);
	}
}

static int get_cluster_clk_and_freq_table(struct device *cpu_dev)
{
	u32 cluster = cpu_to_cluster(cpu_dev->id);
	char name[14] = "cpu-cluster.";
	int ret;

	if (atomic_inc_return(&cluster_usage[cluster]) != 1)
		return 0;

	ret = arm_bL_ops->init_opp_table(cpu_dev);
	if (ret) {
		dev_err(cpu_dev, "%s: init_opp_table failed, cpu: %d, err: %d\n",
				__func__, cpu_dev->id, ret);
		goto atomic_dec;
	}

#if defined(MY_ABC_HERE)
	ret = dev_pm_opp_init_cpufreq_table(cpu_dev, &freq_table[cluster]);
#else  
	ret = opp_init_cpufreq_table(cpu_dev, &freq_table[cluster]);
#endif  
	if (ret) {
		dev_err(cpu_dev, "%s: failed to init cpufreq table, cpu: %d, err: %d\n",
				__func__, cpu_dev->id, ret);
		goto atomic_dec;
	}

	name[12] = cluster + '0';
	clk[cluster] = clk_get_sys(name, NULL);
	if (!IS_ERR(clk[cluster])) {
		dev_dbg(cpu_dev, "%s: clk: %p & freq table: %p, cluster: %d\n",
				__func__, clk[cluster], freq_table[cluster],
				cluster);
		return 0;
	}

	dev_err(cpu_dev, "%s: Failed to get clk for cpu: %d, cluster: %d\n",
			__func__, cpu_dev->id, cluster);
	ret = PTR_ERR(clk[cluster]);
#if defined(MY_ABC_HERE)
	dev_pm_opp_free_cpufreq_table(cpu_dev, &freq_table[cluster]);
#else  
	opp_free_cpufreq_table(cpu_dev, &freq_table[cluster]);
#endif  

atomic_dec:
	atomic_dec(&cluster_usage[cluster]);
	dev_err(cpu_dev, "%s: Failed to get data for cluster: %d\n", __func__,
			cluster);
	return ret;
}

static int bL_cpufreq_init(struct cpufreq_policy *policy)
{
	u32 cur_cluster = cpu_to_cluster(policy->cpu);
	struct device *cpu_dev;
	int ret;

	cpu_dev = get_cpu_device(policy->cpu);
	if (!cpu_dev) {
		pr_err("%s: failed to get cpu%d device\n", __func__,
				policy->cpu);
		return -ENODEV;
	}

	ret = get_cluster_clk_and_freq_table(cpu_dev);
	if (ret)
		return ret;

	ret = cpufreq_frequency_table_cpuinfo(policy, freq_table[cur_cluster]);
	if (ret) {
		dev_err(cpu_dev, "CPU %d, cluster: %d invalid freq table\n",
				policy->cpu, cur_cluster);
		put_cluster_clk_and_freq_table(cpu_dev);
		return ret;
	}

	cpufreq_frequency_table_get_attr(freq_table[cur_cluster], policy->cpu);

	if (arm_bL_ops->get_transition_latency)
		policy->cpuinfo.transition_latency =
			arm_bL_ops->get_transition_latency(cpu_dev);
	else
		policy->cpuinfo.transition_latency = CPUFREQ_ETERNAL;

	policy->cur = bL_cpufreq_get(policy->cpu);

	cpumask_copy(policy->cpus, topology_core_cpumask(policy->cpu));

	dev_info(cpu_dev, "%s: CPU %d initialized\n", __func__, policy->cpu);
	return 0;
}

static int bL_cpufreq_exit(struct cpufreq_policy *policy)
{
	struct device *cpu_dev;

	cpu_dev = get_cpu_device(policy->cpu);
	if (!cpu_dev) {
		pr_err("%s: failed to get cpu%d device\n", __func__,
				policy->cpu);
		return -ENODEV;
	}

	put_cluster_clk_and_freq_table(cpu_dev);
	dev_dbg(cpu_dev, "%s: Exited, cpu: %d\n", __func__, policy->cpu);

	return 0;
}

static struct freq_attr *bL_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver bL_cpufreq_driver = {
	.name			= "arm-big-little",
#if defined(MY_ABC_HERE)
	.flags			= CPUFREQ_STICKY |
					CPUFREQ_HAVE_GOVERNOR_PER_POLICY,
#else  
	.flags			= CPUFREQ_STICKY,
#endif  
	.verify			= bL_cpufreq_verify_policy,
	.target			= bL_cpufreq_set_target,
	.get			= bL_cpufreq_get,
	.init			= bL_cpufreq_init,
	.exit			= bL_cpufreq_exit,
#if defined(MY_ABC_HERE)
	 
#else  
	.have_governor_per_policy = true,
#endif  
	.attr			= bL_cpufreq_attr,
};

int bL_cpufreq_register(struct cpufreq_arm_bL_ops *ops)
{
	int ret;

	if (arm_bL_ops) {
		pr_debug("%s: Already registered: %s, exiting\n", __func__,
				arm_bL_ops->name);
		return -EBUSY;
	}

	if (!ops || !strlen(ops->name) || !ops->init_opp_table) {
		pr_err("%s: Invalid arm_bL_ops, exiting\n", __func__);
		return -ENODEV;
	}

	arm_bL_ops = ops;

	ret = cpufreq_register_driver(&bL_cpufreq_driver);
	if (ret) {
		pr_info("%s: Failed registering platform driver: %s, err: %d\n",
				__func__, ops->name, ret);
		arm_bL_ops = NULL;
	} else {
		pr_info("%s: Registered platform driver: %s\n", __func__,
				ops->name);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(bL_cpufreq_register);

void bL_cpufreq_unregister(struct cpufreq_arm_bL_ops *ops)
{
	if (arm_bL_ops != ops) {
		pr_err("%s: Registered with: %s, can't unregister, exiting\n",
				__func__, arm_bL_ops->name);
		return;
	}

	cpufreq_unregister_driver(&bL_cpufreq_driver);
	pr_info("%s: Un-registered platform driver: %s\n", __func__,
			arm_bL_ops->name);
	arm_bL_ops = NULL;
}
EXPORT_SYMBOL_GPL(bL_cpufreq_unregister);
