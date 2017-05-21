#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/pci.h>
#include <linux/irq.h>
#include <linux/mtd/physmap.h>
#include <linux/mtd/nand.h>
#include <linux/timer.h>
#include <linux/mv643xx_eth.h>
#include <linux/i2c.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/pci.h>
#include <mach/orion5x.h>
#include <linux/platform_data/mtd-orion_nand.h>
#include "common.h"
#include "mpp.h"

#define DB88F5281_NOR_BOOT_BASE		0xf4000000
#define DB88F5281_NOR_BOOT_SIZE		SZ_512K

#define DB88F5281_7SEG_BASE		0xfa000000
#define DB88F5281_7SEG_SIZE		SZ_1K

#define DB88F5281_NOR_BASE		0xfc000000
#define DB88F5281_NOR_SIZE		SZ_32M

#define DB88F5281_NAND_BASE		0xfa800000
#define DB88F5281_NAND_SIZE		SZ_1K

#define DB88F5281_PCI_SLOT0_OFFS		7
#define DB88F5281_PCI_SLOT0_IRQ_PIN		12
#define DB88F5281_PCI_SLOT1_SLOT2_IRQ_PIN	13

static struct physmap_flash_data db88f5281_boot_flash_data = {
	.width		= 1,	 
};

static struct resource db88f5281_boot_flash_resource = {
	.flags		= IORESOURCE_MEM,
	.start		= DB88F5281_NOR_BOOT_BASE,
	.end		= DB88F5281_NOR_BOOT_BASE + DB88F5281_NOR_BOOT_SIZE - 1,
};

static struct platform_device db88f5281_boot_flash = {
	.name		= "physmap-flash",
	.id		= 0,
	.dev		= {
		.platform_data	= &db88f5281_boot_flash_data,
	},
	.num_resources	= 1,
	.resource	= &db88f5281_boot_flash_resource,
};

static struct physmap_flash_data db88f5281_nor_flash_data = {
	.width		= 4,	 
};

static struct resource db88f5281_nor_flash_resource = {
	.flags		= IORESOURCE_MEM,
	.start		= DB88F5281_NOR_BASE,
	.end		= DB88F5281_NOR_BASE + DB88F5281_NOR_SIZE - 1,
};

static struct platform_device db88f5281_nor_flash = {
	.name		= "physmap-flash",
	.id		= 1,
	.dev		= {
		.platform_data	= &db88f5281_nor_flash_data,
	},
	.num_resources	= 1,
	.resource	= &db88f5281_nor_flash_resource,
};

static struct mtd_partition db88f5281_nand_parts[] = {
	{
		.name = "kernel",
		.offset = 0,
		.size = SZ_2M,
	}, {
		.name = "root",
		.offset = SZ_2M,
		.size = (SZ_16M - SZ_2M),
	}, {
		.name = "user",
		.offset = SZ_16M,
		.size = SZ_8M,
	}, {
		.name = "recovery",
		.offset = (SZ_16M + SZ_8M),
		.size = SZ_8M,
	},
};

static struct resource db88f5281_nand_resource = {
	.flags		= IORESOURCE_MEM,
	.start		= DB88F5281_NAND_BASE,
	.end		= DB88F5281_NAND_BASE + DB88F5281_NAND_SIZE - 1,
};

static struct orion_nand_data db88f5281_nand_data = {
	.parts		= db88f5281_nand_parts,
	.nr_parts	= ARRAY_SIZE(db88f5281_nand_parts),
	.cle		= 0,
	.ale		= 1,
	.width		= 8,
};

static struct platform_device db88f5281_nand_flash = {
	.name		= "orion_nand",
	.id		= -1,
	.dev		= {
		.platform_data	= &db88f5281_nand_data,
	},
	.resource	= &db88f5281_nand_resource,
	.num_resources	= 1,
};

static void __iomem *db88f5281_7seg;
static struct timer_list db88f5281_timer;

static void db88f5281_7seg_event(unsigned long data)
{
	static int count = 0;
	writel(0, db88f5281_7seg + (count << 4));
	count = (count + 1) & 7;
	mod_timer(&db88f5281_timer, jiffies + 2 * HZ);
}

static int __init db88f5281_7seg_init(void)
{
	if (machine_is_db88f5281()) {
		db88f5281_7seg = ioremap(DB88F5281_7SEG_BASE,
					DB88F5281_7SEG_SIZE);
		if (!db88f5281_7seg) {
			printk(KERN_ERR "Failed to ioremap db88f5281_7seg\n");
			return -EIO;
		}
		setup_timer(&db88f5281_timer, db88f5281_7seg_event, 0);
		mod_timer(&db88f5281_timer, jiffies + 2 * HZ);
	}

	return 0;
}

__initcall(db88f5281_7seg_init);

void __init db88f5281_pci_preinit(void)
{
	int pin;

	pin = DB88F5281_PCI_SLOT0_IRQ_PIN;
	if (gpio_request(pin, "PCI Int1") == 0) {
		if (gpio_direction_input(pin) == 0) {
			irq_set_irq_type(gpio_to_irq(pin), IRQ_TYPE_LEVEL_LOW);
		} else {
			printk(KERN_ERR "db88f5281_pci_preinit failed to "
					"set_irq_type pin %d\n", pin);
			gpio_free(pin);
		}
	} else {
		printk(KERN_ERR "db88f5281_pci_preinit failed to gpio_request %d\n", pin);
	}

	pin = DB88F5281_PCI_SLOT1_SLOT2_IRQ_PIN;
	if (gpio_request(pin, "PCI Int2") == 0) {
		if (gpio_direction_input(pin) == 0) {
			irq_set_irq_type(gpio_to_irq(pin), IRQ_TYPE_LEVEL_LOW);
		} else {
			printk(KERN_ERR "db88f5281_pci_preinit failed "
					"to set_irq_type pin %d\n", pin);
			gpio_free(pin);
		}
	} else {
		printk(KERN_ERR "db88f5281_pci_preinit failed to gpio_request %d\n", pin);
	}
}

static int __init db88f5281_pci_map_irq(const struct pci_dev *dev, u8 slot,
	u8 pin)
{
	int irq;

	irq = orion5x_pci_map_irq(dev, slot, pin);
	if (irq != -1)
		return irq;

	switch (slot - DB88F5281_PCI_SLOT0_OFFS) {
	case 0:
		return gpio_to_irq(DB88F5281_PCI_SLOT0_IRQ_PIN);
	case 1:
	case 2:
		return gpio_to_irq(DB88F5281_PCI_SLOT1_SLOT2_IRQ_PIN);
	default:
		return -1;
	}
}

static struct hw_pci db88f5281_pci __initdata = {
	.nr_controllers	= 2,
	.preinit	= db88f5281_pci_preinit,
	.setup		= orion5x_pci_sys_setup,
	.scan		= orion5x_pci_sys_scan_bus,
	.map_irq	= db88f5281_pci_map_irq,
};

static int __init db88f5281_pci_init(void)
{
	if (machine_is_db88f5281())
		pci_common_init(&db88f5281_pci);

	return 0;
}

subsys_initcall(db88f5281_pci_init);

static struct mv643xx_eth_platform_data db88f5281_eth_data = {
	.phy_addr	= MV643XX_ETH_PHY_ADDR(8),
};

static struct i2c_board_info __initdata db88f5281_i2c_rtc = {
	I2C_BOARD_INFO("ds1339", 0x68),
};

static unsigned int db88f5281_mpp_modes[] __initdata = {
	MPP0_GPIO,		 
	MPP1_GPIO,		 
	MPP2_PCI_ARB,		 
	MPP3_PCI_ARB,		 
	MPP4_PCI_ARB,		 
	MPP5_PCI_ARB,		 
	MPP6_GPIO,		 
	MPP7_GPIO,		 
	MPP8_GPIO,		 
	MPP9_GPIO,		 
	MPP10_GPIO,		 
	MPP11_GPIO,		 
	MPP12_GPIO,		 
	MPP13_GPIO,		 
	MPP14_NAND,		 
	MPP15_NAND,		 
	MPP16_UART,		 
	MPP17_UART,		 
	MPP18_UART,		 
	MPP19_UART,		 
	0,
};

static void __init db88f5281_init(void)
{
	 
	orion5x_init();

	orion5x_mpp_conf(db88f5281_mpp_modes);
	writel(0, MPP_DEV_CTRL);		 

	orion5x_ehci0_init();
	orion5x_eth_init(&db88f5281_eth_data);
	orion5x_i2c_init();
	orion5x_uart0_init();
	orion5x_uart1_init();

#if defined(MY_ABC_HERE)
	mvebu_mbus_add_window_by_id(ORION_MBUS_DEVBUS_BOOT_TARGET,
				    ORION_MBUS_DEVBUS_BOOT_ATTR,
				    DB88F5281_NOR_BOOT_BASE,
				    DB88F5281_NOR_BOOT_SIZE);
	platform_device_register(&db88f5281_boot_flash);

	mvebu_mbus_add_window_by_id(ORION_MBUS_DEVBUS_TARGET(0),
				    ORION_MBUS_DEVBUS_ATTR(0),
				    DB88F5281_7SEG_BASE,
				    DB88F5281_7SEG_SIZE);

	mvebu_mbus_add_window_by_id(ORION_MBUS_DEVBUS_TARGET(1),
				    ORION_MBUS_DEVBUS_ATTR(1),
				    DB88F5281_NOR_BASE,
				    DB88F5281_NOR_SIZE);
	platform_device_register(&db88f5281_nor_flash);

	mvebu_mbus_add_window_by_id(ORION_MBUS_DEVBUS_TARGET(2),
				    ORION_MBUS_DEVBUS_ATTR(2),
				    DB88F5281_NAND_BASE,
				    DB88F5281_NAND_SIZE);
#else  
	mvebu_mbus_add_window("devbus-boot", DB88F5281_NOR_BOOT_BASE,
			      DB88F5281_NOR_BOOT_SIZE);
	platform_device_register(&db88f5281_boot_flash);

	mvebu_mbus_add_window("devbus-cs0", DB88F5281_7SEG_BASE,
			      DB88F5281_7SEG_SIZE);

	mvebu_mbus_add_window("devbus-cs1", DB88F5281_NOR_BASE,
			      DB88F5281_NOR_SIZE);
	platform_device_register(&db88f5281_nor_flash);

	mvebu_mbus_add_window("devbus-cs2", DB88F5281_NAND_BASE,
			      DB88F5281_NAND_SIZE);
#endif  
	platform_device_register(&db88f5281_nand_flash);

	i2c_register_board_info(0, &db88f5281_i2c_rtc, 1);
}

MACHINE_START(DB88F5281, "Marvell Orion-2 Development Board")
	 
	.atag_offset	= 0x100,
	.init_machine	= db88f5281_init,
	.map_io		= orion5x_map_io,
	.init_early	= orion5x_init_early,
	.init_irq	= orion5x_init_irq,
	.init_time	= orion5x_timer_init,
	.restart	= orion5x_restart,
MACHINE_END