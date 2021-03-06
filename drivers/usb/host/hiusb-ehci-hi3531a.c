#include <linux/init.h>
#include <linux/timer.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/spinlock.h>
#include <asm/byteorder.h>
#include <linux/io.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <mach/hardware.h>

#define IO_REG_USB2_CTRL	__io_address(CRG_REG_BASE + REG_USB2_CTRL)
#define USB2_BUS_CKEN		(1 << 0)
#define USB2_OHCI48M_CKEN	(1 << 1)
#define USB2_OHCI12M_CKEN	(1 << 2)
#define USB2_HST_PHY_CKEN	(1 << 4)
#define USB2_UTMI0_CKEN		(1 << 5)
#define USB2_BUS_SRST_REQ	(1 << 12)
#define USB2_UTMI0_SRST_REQ	(1 << 13)
#define USB2_HST_PHY_SYST_REQ	(1 << 16)

#define IO_REG_USB2_PHY0	__io_address(CRG_REG_BASE + REG_USB2_PHY0)
#define USB_PHY0_REF_CKEN	(1 << 0)
#define USB_PHY0_SRST_REQ	(1 << 8)
#define USB_PHY0_SRST_TREQ	(1 << 9)
#define USB_PHY0_REFCLK_SEL	(1 << 16)

#define IO_REG_USB2_CTRL0	__io_address(MISC_REG_BASE + REG_USB2_CTRL0)
#define WORDINTERFACE		(1 << 0)
#define SS_BURST4_EN		(1 << 7)
#define SS_BURST8_EN		(1 << 8)
#define SS_BURST16_EN		(1 << 9)

#define IO_REG_USB2_CTRL1	__io_address(MISC_REG_BASE + REG_USB2_CTRL1)
/* write(0x1 << 5) 0x6 to addr 0x4 */
#define CONFIG_CLK		((0x1 << 5) | (0x6 << 0) | (0x4 << 8))

static atomic_t dev_open_cnt = {
	.counter = 0,
};

void hiusb_start_hcd(void)
{
	if (atomic_add_return(1, &dev_open_cnt) == 1) {
		int reg;

		/* reset enable */
		reg = readl(IO_REG_USB2_CTRL);
		reg |= (USB2_BUS_SRST_REQ
			| USB2_UTMI0_SRST_REQ
			| USB2_HST_PHY_SYST_REQ);

		writel(reg, IO_REG_USB2_CTRL);
		udelay(200);

		reg = readl(IO_REG_USB2_PHY0);
		reg |= (USB_PHY0_SRST_REQ
			| USB_PHY0_SRST_TREQ);
		writel(reg, IO_REG_USB2_PHY0);
		udelay(200);

		reg = readl(IO_REG_USB2_CTRL0);
		reg &= ~(WORDINTERFACE);	/* 8bit */
		reg &= ~(SS_BURST16_EN);	/* 16 bit burst disable */
		writel(reg, IO_REG_USB2_CTRL0);
		udelay(100);

		/* for ssk usb storage ok */
		msleep(20);

		/* open ref clock */
		reg = readl(IO_REG_USB2_PHY0);
		reg |= (USB_PHY0_REF_CKEN);
		writel(reg, IO_REG_USB2_PHY0);
		udelay(100);

		/* cancel power on reset */
		reg = readl(IO_REG_USB2_PHY0);
		reg &= ~(USB_PHY0_SRST_REQ);
		writel(reg , IO_REG_USB2_PHY0);
		udelay(300);

		/* config clock */
		writel(0x0, IO_REG_USB2_CTRL1);
		mdelay(200);
		reg = readl(IO_REG_USB2_CTRL1);
		reg |= CONFIG_CLK;
		writel(reg, IO_REG_USB2_CTRL1);
		udelay(100);

		/* writel(0x0, IO_REG_USB2_CTRL1); */
		/* mdelay(2); */

		/* config u2 eye diagram */
		/* close HS pre-emphasis */
		writel(0x0, IO_REG_USB2_CTRL1);
		udelay(10);
		writel(0x1820, IO_REG_USB2_CTRL1);
		udelay(100);

		/* cancel port reset */
		reg = readl(IO_REG_USB2_PHY0);
		reg &= ~(USB_PHY0_SRST_TREQ);
		writel(reg, IO_REG_USB2_PHY0);
		udelay(300);

		/* cancel control reset */
		reg = readl(IO_REG_USB2_CTRL);
		reg &= ~(USB2_BUS_SRST_REQ
			| USB2_UTMI0_SRST_REQ
			| USB2_HST_PHY_SYST_REQ);

		reg |= (USB2_BUS_CKEN
			| USB2_OHCI48M_CKEN
			| USB2_OHCI12M_CKEN
			| USB2_HST_PHY_CKEN
			| USB2_UTMI0_CKEN);
		writel(reg, IO_REG_USB2_CTRL);
		udelay(200);
	}

	return;
}
EXPORT_SYMBOL(hiusb_start_hcd);

void hiusb_stop_hcd(void)
{
	if (atomic_sub_return(1, &dev_open_cnt) == 0) {
		int reg;

		reg = readl(IO_REG_USB2_PHY0);
		reg |= (USB_PHY0_SRST_REQ
			| USB_PHY0_SRST_TREQ);
		writel(reg, IO_REG_USB2_PHY0);
		udelay(100);

		/* close clock */
		reg = readl(IO_REG_USB2_PHY0);
		reg &= ~(USB_PHY0_REFCLK_SEL
			| USB_PHY0_REF_CKEN);
		writel(reg, IO_REG_USB2_PHY0);
		udelay(300);

		/* close clock */
		reg = readl(IO_REG_USB2_CTRL);
		reg &= ~(USB2_BUS_CKEN
			| USB2_OHCI48M_CKEN
			| USB2_OHCI12M_CKEN
			| USB2_HST_PHY_CKEN
			| USB2_UTMI0_CKEN);
		writel(reg, IO_REG_USB2_CTRL);
		udelay(200);
	}

}
EXPORT_SYMBOL(hiusb_stop_hcd);
