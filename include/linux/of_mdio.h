#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __LINUX_OF_MDIO_H
#define __LINUX_OF_MDIO_H

#include <linux/phy.h>
#include <linux/of.h>

#ifdef CONFIG_OF
extern int of_mdiobus_register(struct mii_bus *mdio, struct device_node *np);
extern struct phy_device *of_phy_find_device(struct device_node *phy_np);
extern struct phy_device *of_phy_connect(struct net_device *dev,
					 struct device_node *phy_np,
					 void (*hndlr)(struct net_device *),
					 u32 flags, phy_interface_t iface);
extern struct phy_device *of_phy_connect_fixed_link(struct net_device *dev,
					 void (*hndlr)(struct net_device *),
					 phy_interface_t iface);

extern struct mii_bus *of_mdio_find_bus(struct device_node *mdio_np);

#else  
static inline int of_mdiobus_register(struct mii_bus *mdio, struct device_node *np)
{
	return -ENOSYS;
}

static inline struct phy_device *of_phy_find_device(struct device_node *phy_np)
{
	return NULL;
}

static inline struct phy_device *of_phy_connect(struct net_device *dev,
						struct device_node *phy_np,
						void (*hndlr)(struct net_device *),
						u32 flags, phy_interface_t iface)
{
	return NULL;
}

static inline struct phy_device *of_phy_connect_fixed_link(struct net_device *dev,
							   void (*hndlr)(struct net_device *),
							   phy_interface_t iface)
{
	return NULL;
}

static inline struct mii_bus *of_mdio_find_bus(struct device_node *mdio_np)
{
	return NULL;
}
#endif  

#if defined(MY_ABC_HERE)
#if defined(CONFIG_OF) && defined(CONFIG_FIXED_PHY)
extern int of_phy_register_fixed_link(struct device_node *np);
#else
static inline int of_phy_register_fixed_link(struct device_node *np)
{
	return -ENOSYS;
}
#endif
#endif  

#endif  
