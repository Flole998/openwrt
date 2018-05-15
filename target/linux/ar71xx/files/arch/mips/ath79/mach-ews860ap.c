/*
 *  EnGenius EWS860AP/ENH1750EXT board support
 *
 *  Copyright (c) 2018 Robert Marko <robimarko@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/pci.h>
#include <linux/phy.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/ar8216_platform.h>
#include <asm/mach-ath79/ar71xx_regs.h>
#include <linux/platform_data/phy-at803x.h>

#include "common.h"
#include "pci.h"
#include "dev-ap9x-pci.h"
#include "dev-gpio-buttons.h"
#include "dev-eth.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"

#define EWS860AP_GPIO_LED_WLAN2		14
#define EWS860AP_GPIO_LED_WLAN5		15

#define EWS860AP_GPIO_BTN_RESET		21
#define EWS860AP_KEYS_POLL_INTERVAL	20
#define EWS860AP_KEYS_DEBOUNCE_INTERVAL	(3 * EWS860AP_KEYS_POLL_INTERVAL)

#define EWS860AP_WMAC_CALDATA_OFFSET	0x1000

static struct gpio_led ews860ap_leds_gpio[] __initdata = {
	{
		.name		= "ews860:green:wlan-2g",
		.gpio		= EWS860AP_GPIO_LED_WLAN2,
		.active_low	= 1,
	},
	{
		.name		= "ews860:green:wlan-5g",
		.gpio		= EWS860AP_GPIO_LED_WLAN5,
		.active_low	= 1,
	},
};

static struct gpio_keys_button ews860ap_gpio_keys[] __initdata = {
	{
		.desc		= "Reset button",
		.type		= EV_KEY,
		.code		= KEY_RESTART,
		.debounce_interval = EWS860AP_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= EWS860AP_GPIO_BTN_RESET,
		.active_low	= 1,
	},
};

static void __init ews860ap_setup(void)
{
	u8 *art = (u8 *) KSEG1ADDR(0x1fff0000);

	ath79_register_m25p80(NULL);

	ath79_gpio_function_enable(QCA955X_GPIO_FUNC_JTAG_DISABLE);

	ath79_register_gpio_keys_polled(-1, EWS860AP_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(ews860ap_gpio_keys),
					ews860ap_gpio_keys);

	ath79_register_leds_gpio(-1, ARRAY_SIZE(ews860ap_leds_gpio),
				ews860ap_leds_gpio);

	ath79_register_mdio(0, 0x0);

	ath79_register_wmac(art + EWS860AP_WMAC_CALDATA_OFFSET, NULL);

	ath79_setup_qca955x_eth_cfg(QCA955X_ETH_CFG_RGMII_EN);

	/* GMAC0 is connected to an AR8035 PHY */
 	ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_RGMII;
 	ath79_eth0_data.speed = SPEED_1000;
 	ath79_eth0_data.duplex = DUPLEX_FULL;
	ath79_eth0_data.phy_mask = BIT(1);
	ath79_eth0_data.mii_bus_dev = &ath79_mdio0_device.dev;
	ath79_eth0_pll_data.pll_1000 = 0xa6000000;
	ath79_eth0_pll_data.pll_100 = 0xa0000101;
	ath79_eth0_pll_data.pll_10 = 0x80001313;
	ath79_register_eth(0);

	ath79_init_mac(ath79_eth0_data.mac_addr, art, 0);

	/* GMAC1 is connected to an AR8033 switch */
 	ath79_eth1_data.phy_if_mode = PHY_INTERFACE_MODE_SGMII;
 	ath79_eth1_data.speed = SPEED_1000;
 	ath79_eth1_data.duplex = DUPLEX_FULL;
	ath79_eth1_data.phy_mask = BIT(2);
	ath79_eth1_data.mii_bus_dev = &ath79_mdio0_device.dev;
	ath79_register_eth(1);

	ath79_init_mac(ath79_eth1_data.mac_addr, art, 1);

	ath79_register_pci();

}

MIPS_MACHINE(ATH79_MACH_EWS860AP, "EWS860AP",
	"EnGenius EWS860AP", ews860ap_setup);

MIPS_MACHINE(ATH79_MACH_ENH1750EXT, "ENH1750EXT",
	"EnGenius ENH1750EXT", ews860ap_setup);
