#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/timex.h>
#include <linux/io.h>
#include <linux/delay.h>

#include <asm/cpu_device_id.h>
#include <asm/msr.h>
#include <asm/tsc.h>

#if defined CONFIG_ACPI_PROCESSOR || defined CONFIG_ACPI_PROCESSOR_MODULE
#include <linux/acpi.h>
#include <acpi/processor.h>
#endif

#define EPS_BRAND_C7M	0
#define EPS_BRAND_C7	1
#define EPS_BRAND_EDEN	2
#define EPS_BRAND_C3	3
#define EPS_BRAND_C7D	4

struct eps_cpu_data {
	u32 fsb;
#if defined CONFIG_ACPI_PROCESSOR || defined CONFIG_ACPI_PROCESSOR_MODULE
	u32 bios_limit;
#endif
	struct cpufreq_frequency_table freq_table[];
};

static struct eps_cpu_data *eps_cpu[NR_CPUS];

static int freq_failsafe_off;
static int voltage_failsafe_off;
static int set_max_voltage;

#if defined CONFIG_ACPI_PROCESSOR || defined CONFIG_ACPI_PROCESSOR_MODULE
static int ignore_acpi_limit;

static struct acpi_processor_performance *eps_acpi_cpu_perf;

static int eps_acpi_init(void)
{
#if defined(MY_ABC_HERE)
	eps_acpi_cpu_perf = kzalloc(sizeof(*eps_acpi_cpu_perf),
#else  
	eps_acpi_cpu_perf = kzalloc(sizeof(struct acpi_processor_performance),
#endif  
				      GFP_KERNEL);
	if (!eps_acpi_cpu_perf)
		return -ENOMEM;

	if (!zalloc_cpumask_var(&eps_acpi_cpu_perf->shared_cpu_map,
								GFP_KERNEL)) {
		kfree(eps_acpi_cpu_perf);
		eps_acpi_cpu_perf = NULL;
		return -ENOMEM;
	}

	if (acpi_processor_register_performance(eps_acpi_cpu_perf, 0)) {
		free_cpumask_var(eps_acpi_cpu_perf->shared_cpu_map);
		kfree(eps_acpi_cpu_perf);
		eps_acpi_cpu_perf = NULL;
		return -EIO;
	}
	return 0;
}

static int eps_acpi_exit(struct cpufreq_policy *policy)
{
	if (eps_acpi_cpu_perf) {
		acpi_processor_unregister_performance(eps_acpi_cpu_perf, 0);
		free_cpumask_var(eps_acpi_cpu_perf->shared_cpu_map);
		kfree(eps_acpi_cpu_perf);
		eps_acpi_cpu_perf = NULL;
	}
	return 0;
}
#endif

static unsigned int eps_get(unsigned int cpu)
{
	struct eps_cpu_data *centaur;
	u32 lo, hi;

	if (cpu)
		return 0;
	centaur = eps_cpu[cpu];
	if (centaur == NULL)
		return 0;

	rdmsr(MSR_IA32_PERF_STATUS, lo, hi);
	return centaur->fsb * ((lo >> 8) & 0xff);
}

static int eps_set_state(struct eps_cpu_data *centaur,
			 struct cpufreq_policy *policy,
			 u32 dest_state)
{
	struct cpufreq_freqs freqs;
	u32 lo, hi;
	int err = 0;
	int i;

	freqs.old = eps_get(policy->cpu);
	freqs.new = centaur->fsb * ((dest_state >> 8) & 0xff);
	cpufreq_notify_transition(policy, &freqs, CPUFREQ_PRECHANGE);

	rdmsr(MSR_IA32_PERF_STATUS, lo, hi);
	i = 0;
	while (lo & ((1 << 16) | (1 << 17))) {
		udelay(16);
		rdmsr(MSR_IA32_PERF_STATUS, lo, hi);
		i++;
		if (unlikely(i > 64)) {
			err = -ENODEV;
			goto postchange;
		}
	}
	 
	wrmsr(MSR_IA32_PERF_CTL, dest_state & 0xffff, 0);
	 
	i = 0;
	do {
		udelay(16);
		rdmsr(MSR_IA32_PERF_STATUS, lo, hi);
		i++;
		if (unlikely(i > 64)) {
			err = -ENODEV;
			goto postchange;
		}
	} while (lo & ((1 << 16) | (1 << 17)));

postchange:
	rdmsr(MSR_IA32_PERF_STATUS, lo, hi);
	freqs.new = centaur->fsb * ((lo >> 8) & 0xff);

#ifdef DEBUG
	{
	u8 current_multiplier, current_voltage;

	rdmsr(MSR_IA32_PERF_STATUS, lo, hi);
	current_voltage = lo & 0xff;
	printk(KERN_INFO "eps: Current voltage = %dmV\n",
		current_voltage * 16 + 700);
	current_multiplier = (lo >> 8) & 0xff;
	printk(KERN_INFO "eps: Current multiplier = %d\n",
		current_multiplier);
	}
#endif
	cpufreq_notify_transition(policy, &freqs, CPUFREQ_POSTCHANGE);
	return err;
}

static int eps_target(struct cpufreq_policy *policy,
			       unsigned int target_freq,
			       unsigned int relation)
{
	struct eps_cpu_data *centaur;
	unsigned int newstate = 0;
	unsigned int cpu = policy->cpu;
	unsigned int dest_state;
	int ret;

	if (unlikely(eps_cpu[cpu] == NULL))
		return -ENODEV;
	centaur = eps_cpu[cpu];

	if (unlikely(cpufreq_frequency_table_target(policy,
			&eps_cpu[cpu]->freq_table[0],
			target_freq,
			relation,
			&newstate))) {
		return -EINVAL;
	}

	dest_state = centaur->freq_table[newstate].index & 0xffff;
	ret = eps_set_state(centaur, policy, dest_state);
	if (ret)
		printk(KERN_ERR "eps: Timeout!\n");
	return ret;
}

static int eps_verify(struct cpufreq_policy *policy)
{
	return cpufreq_frequency_table_verify(policy,
			&eps_cpu[policy->cpu]->freq_table[0]);
}

static int eps_cpu_init(struct cpufreq_policy *policy)
{
	unsigned int i;
	u32 lo, hi;
	u64 val;
	u8 current_multiplier, current_voltage;
	u8 max_multiplier, max_voltage;
	u8 min_multiplier, min_voltage;
	u8 brand = 0;
	u32 fsb;
	struct eps_cpu_data *centaur;
	struct cpuinfo_x86 *c = &cpu_data(0);
	struct cpufreq_frequency_table *f_table;
	int k, step, voltage;
	int ret;
	int states;
#if defined CONFIG_ACPI_PROCESSOR || defined CONFIG_ACPI_PROCESSOR_MODULE
	unsigned int limit;
#endif

	if (policy->cpu != 0)
		return -ENODEV;

	printk(KERN_INFO "eps: Detected VIA ");

	switch (c->x86_model) {
	case 10:
		rdmsr(0x1153, lo, hi);
		brand = (((lo >> 2) ^ lo) >> 18) & 3;
		printk(KERN_CONT "Model A ");
		break;
	case 13:
		rdmsr(0x1154, lo, hi);
		brand = (((lo >> 4) ^ (lo >> 2))) & 0x000000ff;
		printk(KERN_CONT "Model D ");
		break;
	}

	switch (brand) {
	case EPS_BRAND_C7M:
		printk(KERN_CONT "C7-M\n");
		break;
	case EPS_BRAND_C7:
		printk(KERN_CONT "C7\n");
		break;
	case EPS_BRAND_EDEN:
		printk(KERN_CONT "Eden\n");
		break;
	case EPS_BRAND_C7D:
		printk(KERN_CONT "C7-D\n");
		break;
	case EPS_BRAND_C3:
		printk(KERN_CONT "C3\n");
		return -ENODEV;
		break;
	}
	 
	rdmsrl(MSR_IA32_MISC_ENABLE, val);
	if (!(val & MSR_IA32_MISC_ENABLE_ENHANCED_SPEEDSTEP)) {
		val |= MSR_IA32_MISC_ENABLE_ENHANCED_SPEEDSTEP;
		wrmsrl(MSR_IA32_MISC_ENABLE, val);
		 
		rdmsrl(MSR_IA32_MISC_ENABLE, val);
		if (!(val & MSR_IA32_MISC_ENABLE_ENHANCED_SPEEDSTEP)) {
			printk(KERN_INFO "eps: Can't enable Enhanced PowerSaver\n");
			return -ENODEV;
		}
	}

	rdmsr(MSR_IA32_PERF_STATUS, lo, hi);
	current_voltage = lo & 0xff;
	printk(KERN_INFO "eps: Current voltage = %dmV\n",
			current_voltage * 16 + 700);
	current_multiplier = (lo >> 8) & 0xff;
	printk(KERN_INFO "eps: Current multiplier = %d\n", current_multiplier);

	max_voltage = hi & 0xff;
	printk(KERN_INFO "eps: Highest voltage = %dmV\n",
			max_voltage * 16 + 700);
	max_multiplier = (hi >> 8) & 0xff;
	printk(KERN_INFO "eps: Highest multiplier = %d\n", max_multiplier);
	min_voltage = (hi >> 16) & 0xff;
	printk(KERN_INFO "eps: Lowest voltage = %dmV\n",
			min_voltage * 16 + 700);
	min_multiplier = (hi >> 24) & 0xff;
	printk(KERN_INFO "eps: Lowest multiplier = %d\n", min_multiplier);

	if (current_multiplier == 0 || max_multiplier == 0
	    || min_multiplier == 0)
		return -EINVAL;
	if (current_multiplier > max_multiplier
	    || max_multiplier <= min_multiplier)
		return -EINVAL;
	if (current_voltage > 0x1f || max_voltage > 0x1f)
		return -EINVAL;
	if (max_voltage < min_voltage
	    || current_voltage < min_voltage
	    || current_voltage > max_voltage)
		return -EINVAL;

	if (!freq_failsafe_off && max_multiplier != current_multiplier) {
		printk(KERN_INFO "eps: Your processor is running at different "
			"frequency then its maximum. Aborting.\n");
		printk(KERN_INFO "eps: You can use freq_failsafe_off option "
			"to disable this check.\n");
		return -EINVAL;
	}
	if (!voltage_failsafe_off && max_voltage != current_voltage) {
		printk(KERN_INFO "eps: Your processor is running at different "
			"voltage then its maximum. Aborting.\n");
		printk(KERN_INFO "eps: You can use voltage_failsafe_off "
			"option to disable this check.\n");
		return -EINVAL;
	}

	fsb = cpu_khz / current_multiplier;

#if defined CONFIG_ACPI_PROCESSOR || defined CONFIG_ACPI_PROCESSOR_MODULE
	 
	if (!ignore_acpi_limit && !eps_acpi_init()) {
		if (!acpi_processor_get_bios_limit(policy->cpu, &limit)) {
			printk(KERN_INFO "eps: ACPI limit %u.%uGHz\n",
				limit/1000000,
				(limit%1000000)/10000);
			eps_acpi_exit(policy);
			 
			if (limit && max_multiplier * fsb > limit) {
				printk(KERN_INFO "eps: Aborting.\n");
				return -EINVAL;
			}
		}
	}
#endif

	if (brand == EPS_BRAND_C7M && set_max_voltage) {
		u32 v;

		v = (set_max_voltage - 700) / 16;
		 
		if (v >= min_voltage && v <= max_voltage) {
			printk(KERN_INFO "eps: Setting %dmV as maximum.\n",
				v * 16 + 700);
			max_voltage = v;
		}
	}

	if (brand == EPS_BRAND_C7M)
		states = max_multiplier - min_multiplier + 1;
	else
		states = 2;

#if defined(MY_ABC_HERE)
	centaur = kzalloc(sizeof(*centaur)
#else  
	centaur = kzalloc(sizeof(struct eps_cpu_data)
#endif  
		    + (states + 1) * sizeof(struct cpufreq_frequency_table),
		    GFP_KERNEL);
	if (!centaur)
		return -ENOMEM;
	eps_cpu[0] = centaur;

	centaur->fsb = fsb;
#if defined CONFIG_ACPI_PROCESSOR || defined CONFIG_ACPI_PROCESSOR_MODULE
	centaur->bios_limit = limit;
#endif

	f_table = &centaur->freq_table[0];
	if (brand != EPS_BRAND_C7M) {
		f_table[0].frequency = fsb * min_multiplier;
		f_table[0].index = (min_multiplier << 8) | min_voltage;
		f_table[1].frequency = fsb * max_multiplier;
		f_table[1].index = (max_multiplier << 8) | max_voltage;
		f_table[2].frequency = CPUFREQ_TABLE_END;
	} else {
		k = 0;
		step = ((max_voltage - min_voltage) * 256)
			/ (max_multiplier - min_multiplier);
		for (i = min_multiplier; i <= max_multiplier; i++) {
			voltage = (k * step) / 256 + min_voltage;
			f_table[k].frequency = fsb * i;
			f_table[k].index = (i << 8) | voltage;
			k++;
		}
		f_table[k].frequency = CPUFREQ_TABLE_END;
	}

	policy->cpuinfo.transition_latency = 140000;  
	policy->cur = fsb * current_multiplier;

	ret = cpufreq_frequency_table_cpuinfo(policy, &centaur->freq_table[0]);
	if (ret) {
		kfree(centaur);
		return ret;
	}

	cpufreq_frequency_table_get_attr(&centaur->freq_table[0], policy->cpu);
	return 0;
}

static int eps_cpu_exit(struct cpufreq_policy *policy)
{
	unsigned int cpu = policy->cpu;

#if defined(MY_ABC_HERE)
	 
#else  
	cpufreq_frequency_table_put_attr(policy->cpu);
#endif  
	kfree(eps_cpu[cpu]);
	eps_cpu[cpu] = NULL;
	return 0;
}

static struct freq_attr *eps_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver eps_driver = {
	.verify		= eps_verify,
	.target		= eps_target,
	.init		= eps_cpu_init,
	.exit		= eps_cpu_exit,
	.get		= eps_get,
	.name		= "e_powersaver",
#if defined(MY_ABC_HERE)
	 
#else  
	.owner		= THIS_MODULE,
#endif  
	.attr		= eps_attr,
};

static const struct x86_cpu_id eps_cpu_id[] = {
	{ X86_VENDOR_CENTAUR, 6, X86_MODEL_ANY, X86_FEATURE_EST },
	{}
};
MODULE_DEVICE_TABLE(x86cpu, eps_cpu_id);

static int __init eps_init(void)
{
	if (!x86_match_cpu(eps_cpu_id) || boot_cpu_data.x86_model < 10)
		return -ENODEV;
	if (cpufreq_register_driver(&eps_driver))
		return -EINVAL;
	return 0;
}

static void __exit eps_exit(void)
{
	cpufreq_unregister_driver(&eps_driver);
}

module_param(freq_failsafe_off, int, 0644);
MODULE_PARM_DESC(freq_failsafe_off, "Disable current vs max frequency check");
module_param(voltage_failsafe_off, int, 0644);
MODULE_PARM_DESC(voltage_failsafe_off, "Disable current vs max voltage check");
#if defined CONFIG_ACPI_PROCESSOR || defined CONFIG_ACPI_PROCESSOR_MODULE
module_param(ignore_acpi_limit, int, 0644);
MODULE_PARM_DESC(ignore_acpi_limit, "Don't check ACPI's processor speed limit");
#endif
module_param(set_max_voltage, int, 0644);
MODULE_PARM_DESC(set_max_voltage, "Set maximum CPU voltage (mV) C7-M only");

MODULE_AUTHOR("Rafal Bilski <rafalbilski@interia.pl>");
MODULE_DESCRIPTION("Enhanced PowerSaver driver for VIA C7 CPU's.");
MODULE_LICENSE("GPL");

module_init(eps_init);
module_exit(eps_exit);