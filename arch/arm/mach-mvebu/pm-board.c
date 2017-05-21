#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include "common.h"

#define ARMADA_XP_GP_PIC_NR_GPIOS 3

static void __iomem *gpio_ctrl[ARMADA_XP_GP_PIC_NR_GPIOS];
static void __iomem *gpio_ctrl_addr[ARMADA_XP_GP_PIC_NR_GPIOS];
static int pic_gpios[ARMADA_XP_GP_PIC_NR_GPIOS];
static int pic_raw_gpios[ARMADA_XP_GP_PIC_NR_GPIOS];
static int pic_gpios_num;

static void mvebu_armada_xp_gp_pm_enter(void __iomem *sdram_reg, u32 srcmd)
{
	u32 reg, ackcmd;
	int i;

	for (i = 0; i < pic_gpios_num; i++) {
		reg = readl(gpio_ctrl[i]);
		reg &= ~BIT(pic_raw_gpios[i]);
		if (i == 0)
			reg |= BIT(pic_raw_gpios[0]);
		writel(reg, gpio_ctrl[i]);
	}

	ackcmd = readl(gpio_ctrl[pic_gpios_num-1]);
	for (i = 0; i < pic_gpios_num; i++) {
		if (gpio_ctrl[i] == gpio_ctrl[pic_gpios_num-1])
			ackcmd |= BIT(pic_raw_gpios[i]);
	}

	mdelay(3000);

	asm volatile (
		 
		".balign 32\n\t"

		"str %[srcmd], [%[sdram_reg]]\n\t"

		"mov r1, #50\n\t"
		"1: subs r1, r1, #1\n\t"
		"bne 1b\n\t"

		"str %[ackcmd], [%[gpio_ctrl]]\n\t"

		"b .\n\t"
		: : [srcmd] "r" (srcmd), [sdram_reg] "r" (sdram_reg),
		  [ackcmd] "r" (ackcmd), [gpio_ctrl] "r" (gpio_ctrl[pic_gpios_num-1]) : "r1");
}

#if defined(MY_ABC_HERE)
static int __init mvebu_armada_xp_gp_pm_init(void)
#else  
static int mvebu_armada_xp_gp_pm_init(void)
#endif  
{
	struct device_node *np;
	struct device_node *gpio_ctrl_np;
	int ret = 0, i;

#if defined(MY_ABC_HERE)
	 
#else  
	if (!of_machine_is_compatible("marvell,axp-gp") &&
		!of_machine_is_compatible("marvell,a388-db-gp") &&
		!of_machine_is_compatible("marvell,a385-db-ap"))
		return -ENODEV;
#endif  

	np = of_find_node_by_name(NULL, "pm_pic");
	if (!np)
		return -ENODEV;

	pic_gpios_num = of_gpio_named_count(np, "ctrl-gpios");
	if (pic_gpios_num < 1)
		return -ENODEV;

	for (i = 0; i < pic_gpios_num; i++) {
		char *name;
		struct of_phandle_args args;

		pic_gpios[i] = of_get_named_gpio(np, "ctrl-gpios", i);
		if (pic_gpios[i] < 0) {
			ret = -ENODEV;
			goto out;
		}

		name = kasprintf(GFP_KERNEL, "pic-pin%d", i);
		if (!name) {
			ret = -ENOMEM;
			goto out;
		}

		ret = gpio_request(pic_gpios[i], name);
		if (ret < 0) {
			kfree(name);
			goto out;
		}

		ret = gpio_direction_output(pic_gpios[i], 0);
		if (ret < 0) {
			gpio_free(pic_gpios[i]);
			kfree(name);
			goto out;
		}

		ret = of_parse_phandle_with_fixed_args(np, "ctrl-gpios", 2,
						       i, &args);
		if (ret < 0) {
			gpio_free(pic_gpios[i]);
			kfree(name);
			goto out;
		}

		gpio_ctrl_np = args.np;
		pic_raw_gpios[i] = args.args[0];
		gpio_ctrl_addr[i] = of_get_address(gpio_ctrl_np, 0, NULL, NULL);

		if ((i == 0) || (i > 0 && gpio_ctrl_addr[i] != gpio_ctrl_addr[i-1]))
			gpio_ctrl[i] = of_iomap(gpio_ctrl_np, 0);
		else
			gpio_ctrl[i] = gpio_ctrl[i-1];

		if (!gpio_ctrl[i]) {
			ret = -ENOMEM;
			goto out;
		}
	}

#if defined(MY_ABC_HERE)
	mvebu_pm_suspend_init(mvebu_armada_xp_gp_pm_enter);
#else  
	mvebu_pm_init(mvebu_armada_xp_gp_pm_enter);
#endif  

out:
	of_node_put(np);
	return ret;
}

#if defined(MY_ABC_HERE)
int __init mvebu_armada_xp_gp_pm_register(void)
{
	if (of_machine_is_compatible("marvell,axp-gp") ||
	    of_machine_is_compatible("marvell,a388-db-gp") ||
	    of_machine_is_compatible("marvell,a385-db-ap"))
		mvebu_pm_register_init(mvebu_armada_xp_gp_pm_init);
	return 0;
}
arch_initcall(mvebu_armada_xp_gp_pm_register);
#else  
late_initcall(mvebu_armada_xp_gp_pm_init);
#endif  
