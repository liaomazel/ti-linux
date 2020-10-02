// SPDX-License-Identifier: GPL-2.0
/*
 * CZ.NIC's Turris Omnia LEDs driver
 *
 * 2020 by Marek Behun <marek.behun@nic.cz>
 */

#include <linux/i2c.h>
#include <linux/led-class-multicolor.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include "leds.h"

#define OMNIA_BOARD_LEDS		12
#define OMNIA_LED_NUM_CHANNELS		3

#define CMD_LED_MODE			3
#define CMD_LED_MODE_LED(l)		((l) & 0x0f)
#define CMD_LED_MODE_USER		0x10

#define CMD_LED_STATE			4
#define CMD_LED_STATE_LED(l)		((l) & 0x0f)
#define CMD_LED_STATE_ON		0x10

#define CMD_LED_COLOR			5
#define CMD_LED_SET_BRIGHTNESS		7
#define CMD_LED_GET_BRIGHTNESS		8

#define OMNIA_CMD			0

#define OMNIA_CMD_LED_COLOR_LED		1
#define OMNIA_CMD_LED_COLOR_R		2
#define OMNIA_CMD_LED_COLOR_G		3
#define OMNIA_CMD_LED_COLOR_B		4
#define OMNIA_CMD_LED_COLOR_LEN		5

struct omnia_led {
	struct led_classdev_mc mc_cdev;
	struct mc_subled subled_info[OMNIA_LED_NUM_CHANNELS];
	int reg;
};

#define to_omnia_led(l)			container_of(l, struct omnia_led, mc_cdev)

struct omnia_leds {
	struct i2c_client *client;
	struct mutex lock;
	struct omnia_led leds[];
};

static int omnia_led_brightness_set_blocking(struct led_classdev *cdev,
					     enum led_brightness brightness)
{
	struct led_classdev_mc *mc_cdev = lcdev_to_mccdev(cdev);
	struct omnia_leds *leds = dev_get_drvdata(cdev->dev->parent);
	struct omnia_led *led = to_omnia_led(mc_cdev);
	u8 buf[OMNIA_CMD_LED_COLOR_LEN], state;
	int ret;

	mutex_lock(&leds->lock);

	led_mc_calc_color_components(&led->mc_cdev, brightness);

	buf[OMNIA_CMD] = CMD_LED_COLOR;
	buf[OMNIA_CMD_LED_COLOR_LED] = led->reg;
	buf[OMNIA_CMD_LED_COLOR_R] = mc_cdev->subled_info[0].brightness;
	buf[OMNIA_CMD_LED_COLOR_G] = mc_cdev->subled_info[1].brightness;
	buf[OMNIA_CMD_LED_COLOR_B] = mc_cdev->subled_info[2].brightness;

	state = CMD_LED_STATE_LED(led->reg);
	if (buf[OMNIA_CMD_LED_COLOR_R] || buf[OMNIA_CMD_LED_COLOR_G] || buf[OMNIA_CMD_LED_COLOR_B])
		state |= CMD_LED_STATE_ON;

	ret = i2c_smbus_write_byte_data(leds->client, CMD_LED_STATE, state);
	if (ret >= 0 && (state & CMD_LED_STATE_ON))
		ret = i2c_master_send(leds->client, buf, 5);

	mutex_unlock(&leds->lock);

	return ret;
}

static int omnia_led_register(struct i2c_client *client, struct omnia_led *led,
			      struct device_node *np)
{
	struct led_init_data init_data = {};
	struct device *dev = &client->dev;
	struct led_classdev *cdev;
	int ret, color;

	ret = of_property_read_u32(np, "reg", &led->reg);
	if (ret || led->reg >= OMNIA_BOARD_LEDS) {
		dev_warn(dev,
			 "Node %pOF: must contain 'reg' property with values between 0 and %i\n",
			 np, OMNIA_BOARD_LEDS - 1);
		return 0;
	}

	ret = of_property_read_u32(np, "color", &color);
	if (ret || color != LED_COLOR_ID_MULTI) {
		dev_warn(dev,
			 "Node %pOF: must contain 'color' property with value LED_COLOR_ID_MULTI\n",
			 np);
		return 0;
	}

	led->subled_info[0].color_index = LED_COLOR_ID_RED;
	led->subled_info[0].channel = 0;
	led->subled_info[1].color_index = LED_COLOR_ID_GREEN;
	led->subled_info[1].channel = 1;
	led->subled_info[2].color_index = LED_COLOR_ID_BLUE;
	led->subled_info[2].channel = 2;

	led->mc_cdev.subled_info = led->subled_info;
	led->mc_cdev.num_colors = OMNIA_LED_NUM_CHANNELS;

	init_data.fwnode = &np->fwnode;

	cdev = &led->mc_cdev.led_cdev;
	cdev->max_brightness = 255;
	cdev->brightness_set_blocking = omnia_led_brightness_set_blocking;

	of_property_read_string(np, "linux,default-trigger", &cdev->default_trigger);

	/* put the LED into software mode */
	ret = i2c_smbus_write_byte_data(client, CMD_LED_MODE,
					CMD_LED_MODE_LED(led->reg) |
					CMD_LED_MODE_USER);
	if (ret < 0) {
		dev_err(dev, "Cannot set LED %pOF to software mode: %i\n", np, ret);
		return ret;
	}

	/* disable the LED */
	ret = i2c_smbus_write_byte_data(client, CMD_LED_STATE, CMD_LED_STATE_LED(led->reg));
	if (ret < 0) {
		dev_err(dev, "Cannot set LED %pOF brightness: %i\n", np, ret);
		return ret;
	}

	ret = devm_led_classdev_multicolor_register_ext(dev, &led->mc_cdev, &init_data);
	if (ret < 0) {
		dev_err(dev, "Cannot register LED %pOF: %i\n", np, ret);
		return ret;
	}

	return 1;
}

/*
 * On the front panel of the Turris Omnia router there is also a button which
 * can be used to control the intensity of all the LEDs at once, so that if they
 * are too bright, user can dim them.
 * The microcontroller cycles between 8 levels of this global brightness (from
 * 100% to 0%), but this setting can have any integer value between 0 and 100.
 * It is therefore convenient to be able to change this setting from software.
 * We expose this setting via a sysfs attribute file called "brightness". This
 * file lives in the device directory of the LED controller, not an individual
 * LED, so it should not confuse users.
 */
static ssize_t brightness_show(struct device *dev, struct device_attribute *a, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct omnia_leds *leds = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&leds->lock);
	ret = i2c_smbus_read_byte_data(client, CMD_LED_GET_BRIGHTNESS);
	mutex_unlock(&leds->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", ret);
}

static ssize_t brightness_store(struct device *dev, struct device_attribute *a, const char *buf,
				size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct omnia_leds *leds = i2c_get_clientdata(client);
	unsigned int brightness;
	int ret;

	if (sscanf(buf, "%u", &brightness) != 1)
		return -EINVAL;

	if (brightness > 100)
		return -EINVAL;

	mutex_lock(&leds->lock);
	ret = i2c_smbus_write_byte_data(client, CMD_LED_SET_BRIGHTNESS, (u8) brightness);
	mutex_unlock(&leds->lock);

	if (ret < 0)
		return ret;

	return count;
}
static DEVICE_ATTR_RW(brightness);

static struct attribute *omnia_led_controller_attrs[] = {
	&dev_attr_brightness.attr,
	NULL,
};
ATTRIBUTE_GROUPS(omnia_led_controller);

static int omnia_leds_probe(struct i2c_client *client,
			    const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct device_node *np = dev->of_node, *child;
	struct omnia_leds *leds;
	struct omnia_led *led;
	int ret, count;

	count = of_get_available_child_count(np);
	if (!count) {
		dev_err(dev, "LEDs are not defined in device tree!\n");
		return -ENODEV;
	} else if (count > OMNIA_BOARD_LEDS) {
		dev_err(dev, "Too many LEDs defined in device tree!\n");
		return -EINVAL;
	}

	leds = devm_kzalloc(dev, struct_size(leds, leds, count), GFP_KERNEL);
	if (!leds)
		return -ENOMEM;

	leds->client = client;
	i2c_set_clientdata(client, leds);

	mutex_init(&leds->lock);

	led = &leds->leds[0];
	for_each_available_child_of_node(np, child) {
		ret = omnia_led_register(client, led, child);
		if (ret < 0)
			return ret;

		led += ret;
	}

	if (devm_device_add_groups(dev, omnia_led_controller_groups))
		dev_warn(dev, "Could not add attribute group!\n");

	return 0;
}

static int omnia_leds_remove(struct i2c_client *client)
{
	u8 buf[OMNIA_CMD_LED_COLOR_LEN];

	/* put all LEDs into default (HW triggered) mode */
	i2c_smbus_write_byte_data(client, CMD_LED_MODE,
				  CMD_LED_MODE_LED(OMNIA_BOARD_LEDS));

	/* set all LEDs color to [255, 255, 255] */
	buf[OMNIA_CMD] = CMD_LED_COLOR;
	buf[OMNIA_CMD_LED_COLOR_LED] = OMNIA_BOARD_LEDS;
	buf[OMNIA_CMD_LED_COLOR_R] = 255;
	buf[OMNIA_CMD_LED_COLOR_G] = 255;
	buf[OMNIA_CMD_LED_COLOR_B] = 255;

	i2c_master_send(client, buf, 5);

	return 0;
}

static const struct of_device_id of_omnia_leds_match[] = {
	{ .compatible = "cznic,turris-omnia-leds", },
	{},
};

static const struct i2c_device_id omnia_id[] = {
	{ "omnia", 0 },
	{ }
};

static struct i2c_driver omnia_leds_driver = {
	.probe		= omnia_leds_probe,
	.remove		= omnia_leds_remove,
	.id_table	= omnia_id,
	.driver		= {
		.name	= "leds-turris-omnia",
		.of_match_table = of_omnia_leds_match,
	},
};

module_i2c_driver(omnia_leds_driver);

MODULE_AUTHOR("Marek Behun <marek.behun@nic.cz>");
MODULE_DESCRIPTION("CZ.NIC's Turris Omnia LEDs");
MODULE_LICENSE("GPL v2");
