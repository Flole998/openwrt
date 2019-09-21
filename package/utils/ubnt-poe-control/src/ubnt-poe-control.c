/*
 * Copyright 2006-2012, Ubiquiti Networks, Inc.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */
/*
 * Ubiquiti POE Switch - driver for power controller.
 */

#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>
#include <linux/delay.h>

#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/random.h>
#include <linux/init.h>
#include <linux/uaccess.h>

#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
//#include <linux/sysdev.h>

#define POE_ON_DELAY 300 /* miliseconds */
//#define USE_AR71XX_GPIO 1

#ifdef USE_AR71XX_GPIO
unsigned int gpio_line_config(unsigned char, unsigned char);
unsigned int gpio_line_set(unsigned char, u32);
#else
#include <linux/gpio.h>
#endif /* USE_AR71XX_GPIO */

#define CLOCK_LINE 16
#define DATA_LINE 15
#define STORE_LINE 14
#define OUTPUT_LINE 7

#define CLOCK_LINE_NAME "ser_clk"
#define DATA_LINE_NAME "ser_data"
#define STORE_LINE_NAME "ser_store"
#define OUTPUT_LINE_NAME "ser_out"

#define DEBUG(...) printk(__VA_ARGS__)
//#define DEBUG(...) /* UBNT default"

/*
	The shift register has 16 bits.
	There are 8 ports.
	24V or 48V can be enabled for each port.

	SET_0 - port0_48V
	SET_1 - port0_24V
	SET_2 - port1_48V
	SET_3 - port1_24V
	...
	SET_14 - port7_48V
	SET_15 - port7_24V

*/

static int register_value = 0;
static int enable = 1;

static void set_shift_register(int value)
{
	int i;

	DEBUG(" register = %08x\n", register_value);

#ifdef USE_AR71XX_GPIO
	/* reset to defaults - just in case */
	gpio_line_set(STORE_LINE, 0);
	gpio_line_set(CLOCK_LINE, 0);
	for (i = 15; i >= 0; i--) {
		int bit;

		bit = (register_value >> i) & 1;
		/* set data bit */
		gpio_line_set(DATA_LINE, bit);

		/* toggle clock */
		gpio_line_set(CLOCK_LINE, 1);
		gpio_line_set(CLOCK_LINE, 0);
	}
	/* toggle STORE - change the port state */
	gpio_line_set(STORE_LINE, 1);
	gpio_line_set(STORE_LINE, 0);
#else
	/* reset to defaults - just in case */
	gpio_direction_output(STORE_LINE, 0);
	gpio_direction_output(CLOCK_LINE, 0);
	for (i = 15; i >= 0; i--) {
		int bit;

		bit = (register_value >> i) & 1;
		/* set data bit */
		gpio_direction_output(DATA_LINE, bit);

		/* toggle clock */
		gpio_direction_output(CLOCK_LINE, 1);
		gpio_direction_output(CLOCK_LINE, 0);
	}
	/* toggle STORE - change the port state */
	gpio_direction_output(STORE_LINE, 1);
	gpio_direction_output(STORE_LINE, 0);
#endif
}

static void set_port(int port, int is_24v, int is_48v)
{
	int set_bits, port_mask;

	/* cut to legal values - 0 or 1 */
	is_24v &= 1;
	is_48v &= 1;

	DEBUG("enable port #%d, 24v:%d, 48v:%d\n", port, is_24v, is_48v);

	port_mask = ~(0x3 << (port << 1));

	set_bits = (is_24v ? 2 : 0) << (port << 1);
	set_bits |= (is_48v ? 1 : 0) << (port << 1);

	/* Enable PoE in sequence one port at the time. Ticket #4130 */
	if (set_bits && (register_value & ~port_mask) != set_bits)
		msleep(POE_ON_DELAY);

	register_value &= port_mask;
	register_value |= set_bits;
	/* register value ready to be shifted out to register */

	set_shift_register(register_value);
}

static int alloc_gpios(void)
{
	int ret = 0;

#ifdef USE_AR71XX_GPIO
	/* set direction to output */
	gpio_line_config(OUTPUT_LINE, 1);
	gpio_line_set(OUTPUT_LINE, 0);
	gpio_line_config(CLOCK_LINE, 1);
	gpio_line_set(CLOCK_LINE, 0);
	gpio_line_config(DATA_LINE, 1);
	gpio_line_set(DATA_LINE, 0);
	gpio_line_config(STORE_LINE, 1);
	gpio_line_set(STORE_LINE, 0);
#else
	/* allocate GPIOs */
	ret = gpio_request(OUTPUT_LINE, OUTPUT_LINE_NAME);
	if (ret) {
		printk("GPIO%d request failed!\n", OUTPUT_LINE);
		return ret;
	}

	ret = gpio_request(CLOCK_LINE, CLOCK_LINE_NAME);
	if (ret) {
		printk("GPIO%d request failed!\n", CLOCK_LINE);
		return ret;
	}

	ret = gpio_request(DATA_LINE, DATA_LINE_NAME);
	if (ret) {
		gpio_free(CLOCK_LINE);
		printk("GPIO%d request failed!\n", DATA_LINE);
		return ret;
	}

	ret = gpio_request(STORE_LINE, STORE_LINE_NAME);
	if (ret) {
		gpio_free(CLOCK_LINE);
		gpio_free(DATA_LINE);
		printk("GPIO%d request failed!\n", STORE_LINE);
		return ret;
	}

	/* everything set to output */
	gpio_direction_output(OUTPUT_LINE, 0);
	gpio_direction_output(CLOCK_LINE, 0);
	gpio_direction_output(DATA_LINE, 0);
	gpio_direction_output(STORE_LINE, 0);
#endif
	set_shift_register(0x0);

	/* A high on ToughSwitch GPIO_11 
	 enables Pin13 G# "output enable" on shift register
	 */
#ifdef USE_AR71XX_GPIO
	gpio_line_config(OUTPUT_LINE, 1);
	gpio_line_set(OUTPUT_LINE, 1);
#else
	gpio_direction_output(OUTPUT_LINE, 1);
#endif

	return ret;
}

struct poe_port_attribute{
        struct device_attribute port_attr;
        int index;
};

#define to_poe_port_attr(_port_attr) \
        container_of(_port_attr, struct poe_port_attribute, port_attr)

#define PORT_ATTR(_name, _mode, _show, _store, _index)        \
        { .port_attr = __ATTR(_name, _mode, _show, _store),      \
          .index = _index }

#define POE_PORT_ATTR(_name, _mode, _show, _store, _index) \
struct poe_port_attribute poe_port_attr_##_name          \
        = PORT_ATTR(_name, _mode, _show, _store, _index)

static ssize_t poe_show_name(struct device *dev, struct device_attribute *attr,
                       char *buf)
 {
        char *str = buf;
        str += sprintf(str, "Ubiquiti POE Switch controller\n");
        return str - buf;
}

static ssize_t poe_show_port(struct device *dev, struct device_attribute *attr,
                       char *buf)
 {
        char *str = buf;
		int port_no = to_poe_port_attr(attr)->index;
		int port_mask, is_24v = 0, is_48v = 0;

		port_mask = (3 << (port_no << 1));

		if (((register_value & port_mask) >> (port_no << 1)) & 1)
			is_48v = 1;
		if (((register_value & port_mask) >> (port_no << 1)) & 2)
			is_24v = 1;

        str += sprintf(str, "%s\n", is_48v ? "48V" : (is_24v ? "24V" : "Off"));
        return str - buf;
}

static ssize_t poe_store_port(struct device *dev, struct device_attribute *attr,
                          const char *buf, size_t count)
{
		int port_no = to_poe_port_attr(attr)->index;
		int enable_24v = 0, enable_48v = 0;
		int voltage = simple_strtoul(buf, NULL, 10); /* XXX check for terminator? */

		if (voltage == 24)
			enable_24v = 1;

		if (voltage == 48)
			enable_48v = 1;

		DEBUG("Setting port #%d to %s\n", port_no, enable_24v ? "24V" : enable_48v ? "48V" : "Off");

		set_port(port_no, enable_24v, enable_48v);

		return count;
}

static DEVICE_ATTR(name, 0444, poe_show_name, NULL);

static ssize_t show_raw_register(struct device *dev, struct device_attribute *attr,
                       char *buf)
{
        char *str = buf;
        str += sprintf(str, "0x%x\n", register_value);
        return str - buf;
}

static ssize_t store_raw_register(struct device *dev, struct device_attribute *attr,
                          const char *buf, size_t count)
{
		/* Warning - only hexadecimal value accepted! */
		int value = simple_strtoul(buf, NULL, 16); /* XXX check for terminator? */

		register_value = value;
		set_shift_register(register_value);

		return count;
}

static ssize_t show_enable_register(struct device *dev, struct device_attribute *attr,
                       char *buf)
{
        char *str = buf;
	str += sprintf(str, "%s\n", (enable ? "On" : "Off"));
        return str - buf;
}

static ssize_t store_enable_register(struct device *dev, struct device_attribute *attr,
                          const char *buf, size_t count)
{
    	int value = simple_strtoul(buf, NULL, 10);

	enable = (value ? 1 : 0);
	DEBUG("Setting output enable to %s\n", (enable ? "On" : "Off"));
#ifdef USE_AR71XX_GPIO
	gpio_line_config(OUTPUT_LINE, 1);
	gpio_line_set(OUTPUT_LINE, enable);
#else
	gpio_direction_output(OUTPUT_LINE, enable);
#endif
	return count;
}

static DEVICE_ATTR(raw_value, 0644, show_raw_register, store_raw_register);
static DEVICE_ATTR(enable, 0644, show_enable_register, store_enable_register);

static POE_PORT_ATTR(port_0, 0644, poe_show_port, poe_store_port, 0);
static POE_PORT_ATTR(port_1, 0644, poe_show_port, poe_store_port, 1);
static POE_PORT_ATTR(port_2, 0644, poe_show_port, poe_store_port, 2);
static POE_PORT_ATTR(port_3, 0644, poe_show_port, poe_store_port, 3);
static POE_PORT_ATTR(port_4, 0644, poe_show_port, poe_store_port, 4);
static POE_PORT_ATTR(port_5, 0644, poe_show_port, poe_store_port, 5);
static POE_PORT_ATTR(port_6, 0644, poe_show_port, poe_store_port, 6);
static POE_PORT_ATTR(port_7, 0644, poe_show_port, poe_store_port, 7);

static struct attribute *poe_attrs[] = {
		&dev_attr_name.attr,
		&dev_attr_raw_value.attr,
		&dev_attr_enable.attr,
		&poe_port_attr_port_0.port_attr.attr,
		&poe_port_attr_port_1.port_attr.attr,
		&poe_port_attr_port_2.port_attr.attr,
		&poe_port_attr_port_3.port_attr.attr,
		&poe_port_attr_port_4.port_attr.attr,
		&poe_port_attr_port_5.port_attr.attr,
		&poe_port_attr_port_6.port_attr.attr,
		&poe_port_attr_port_7.port_attr.attr,
		NULL,
};
static struct attribute_group poe_attr_grp = {.attrs = poe_attrs };

static struct device *f_dev;

static void dummy_release(struct device *dev)
{
}

static void register_sysfs(void)
{
	int ret;

	f_dev = kzalloc(sizeof(*f_dev), GFP_KERNEL);
    dev_set_name(f_dev, "%s", "ubnt-poe");
    f_dev->parent = NULL;
	f_dev->release = dummy_release;
    dev_set_drvdata(f_dev, NULL);
    dev_set_uevent_suppress(f_dev, 1);
	ret = device_register(f_dev);
	if (ret)
		printk("failed to register device driver!\n");

	if (sysfs_create_group(&f_dev->kobj, &poe_attr_grp)) {
		printk("failed to create sysfs\n");
	}
}

int init_module(void)
{

	register_sysfs();

	if (alloc_gpios()) {
		printk("gpio alloc failed!\n");

		/* cleanup */
		sysfs_remove_group(&f_dev->kobj, &poe_attr_grp);
		device_unregister(f_dev);
		kfree(f_dev);

		return -ENOMEM;
	}

	/* debug only - enable port0_24v */
	//set_port(0, 1, 0); /* port,24v,48v */

	return 0;
}

void cleanup_module(void)
{
#ifndef USE_AR71XX_GPIO
	gpio_free(CLOCK_LINE);
	gpio_free(DATA_LINE);
	gpio_free(STORE_LINE);
#endif
	sysfs_remove_group(&f_dev->kobj, &poe_attr_grp);
	device_unregister(f_dev);
	kfree(f_dev);
}

MODULE_LICENSE("GPL");
