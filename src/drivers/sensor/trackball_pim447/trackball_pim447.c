/*
 * Copyright (c) 2023-2025 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT pimoroni_trackball_pim447

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(trackball_pim447);

/* Register addresses */
#define TRACKBALL_PIM447_REG_LED_RED 0x00
#define TRACKBALL_PIM447_REG_LED_GREEN 0x01
#define TRACKBALL_PIM447_REG_LED_BLUE 0x02
#define TRACKBALL_PIM447_REG_LEFT 0x04
#define TRACKBALL_PIM447_REG_RIGHT 0x05
#define TRACKBALL_PIM447_REG_UP 0x06
#define TRACKBALL_PIM447_REG_DOWN 0x07
#define TRACKBALL_PIM447_REG_SWITCH 0x08
#define TRACKBALL_PIM447_REG_USER_FLASH 0xD0

/* Register ranges */
#define TRACKBALL_PIM447_REG_MIN TRACKBALL_PIM447_REG_LEFT
#define TRACKBALL_PIM447_REG_MAX TRACKBALL_PIM447_REG_SWITCH

/* Data structure */
struct trackball_pim447_data
{
    const struct device *i2c_dev;
    int16_t dx;
    int16_t dy;
    uint8_t button_state;
    bool invert_x;
    bool invert_y;
    uint8_t sensitivity;
    uint8_t move_factor;
    uint8_t scroll_factor;
    uint8_t mode; /* 0=move, 1=scroll */
    uint8_t led_red;
    uint8_t led_green;
    uint8_t led_blue;
};

/* Configuration structure */
struct trackball_pim447_config
{
    struct i2c_dt_spec i2c;
    uint8_t led_red;
    uint8_t led_green;
    uint8_t led_blue;
    bool invert_x;
    bool invert_y;
    uint8_t sensitivity;
    uint8_t move_factor;
    uint8_t scroll_factor;
};

/**
 * @brief Read a register from the trackball
 *
 * @param dev Device instance
 * @param reg Register to read
 * @param value Pointer to store the value
 * @return 0 on success, negative error code otherwise
 */
static int trackball_pim447_read_reg(const struct device *dev, uint8_t reg, uint8_t *value)
{
    const struct trackball_pim447_config *config = dev->config;
    int err = 0;

    err = i2c_write_read_dt(&config->i2c, &reg, sizeof(reg), value, sizeof(*value));
    if (err < 0)
    {
        LOG_ERR("Failed to read register 0x%02x: %d", reg, err);
        return err;
    }

    return 0;
}

/**
 * @brief Write a value to a register in the trackball
 *
 * @param dev Device instance
 * @param reg Register to write
 * @param value Value to write
 * @return 0 on success, negative error code otherwise
 */
static int trackball_pim447_write_reg(const struct device *dev, uint8_t reg, uint8_t value)
{
    const struct trackball_pim447_config *config = dev->config;
    uint8_t buf[2] = {reg, value};
    int err = 0;

    err = i2c_write_dt(&config->i2c, buf, sizeof(buf));
    if (err < 0)
    {
        LOG_ERR("Failed to write register 0x%02x: %d", reg, err);
        return err;
    }

    return 0;
}

/**
 * @brief Read movement data for a single axis
 *
 * @param dev Device instance
 * @param neg_reg Register for negative direction
 * @param pos_reg Register for positive direction
 * @param value Pointer to store the result
 * @return 0 on success, negative error code otherwise
 */
static int trackball_pim447_read_axis(const struct device *dev, uint8_t neg_reg, uint8_t pos_reg,
                                      int16_t *value)
{
    uint8_t neg_val = 0;
    uint8_t pos_val = 0;
    int err = 0;

    err = trackball_pim447_read_reg(dev, neg_reg, &neg_val);
    if (err < 0)
    {
        return err;
    }

    err = trackball_pim447_read_reg(dev, pos_reg, &pos_val);
    if (err < 0)
    {
        return err;
    }

    *value = (int16_t)pos_val - (int16_t)neg_val;
    return 0;
}

/**
 * @brief Set the LED color
 *
 * @param dev Device instance
 * @param red Red component (0-255)
 * @param green Green component (0-255)
 * @param blue Blue component (0-255)
 * @return 0 on success, negative error code otherwise
 */
static int trackball_pim447_set_led(const struct device *dev, uint8_t red, uint8_t green, uint8_t blue)
{
    struct trackball_pim447_data *data = dev->data;
    int err = 0;

    err = trackball_pim447_write_reg(dev, TRACKBALL_PIM447_REG_LED_RED, red);
    if (err < 0)
    {
        return err;
    }

    err = trackball_pim447_write_reg(dev, TRACKBALL_PIM447_REG_LED_GREEN, green);
    if (err < 0)
    {
        return err;
    }

    err = trackball_pim447_write_reg(dev, TRACKBALL_PIM447_REG_LED_BLUE, blue);
    if (err < 0)
    {
        return err;
    }

    // Store the current LED color in our data structure
    data->led_red = red;
    data->led_green = green;
    data->led_blue = blue;

    LOG_DBG("Set LED color to RGB(%d, %d, %d)", red, green, blue);
    return 0;
}

/**
 * @brief Sample data from the trackball
 *
 * @param dev Device instance
 * @param chan The sensor channel to sample
 * @return 0 on success, negative error code otherwise
 */
static int trackball_pim447_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
    struct trackball_pim447_data *data = dev->data;
    int err = 0;

    /* Read X axis (horizontal) movement */
    if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_POS_DX)
    {
        err = trackball_pim447_read_axis(dev,
                                         TRACKBALL_PIM447_REG_LEFT,
                                         TRACKBALL_PIM447_REG_RIGHT,
                                         &data->dx);
        if (err < 0)
        {
            return err;
        }

        /* Apply inversion if configured */
        if (data->invert_x)
        {
            data->dx = -data->dx;
        }

        /* Scale by sensitivity - higher values = more movement */
        data->dx = (data->dx * data->sensitivity) / 64;

        /* Apply move/scroll factor based on mode */
        if (data->mode == 0)
        {
            /* Move mode */
            data->dx = data->dx * data->move_factor;
        }
        else
        {
            /* Scroll mode */
            data->dx = data->dx * data->scroll_factor;
        }
    }

    /* Read Y axis (vertical) movement */
    if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_POS_DY)
    {
        err = trackball_pim447_read_axis(dev,
                                         TRACKBALL_PIM447_REG_UP,
                                         TRACKBALL_PIM447_REG_DOWN,
                                         &data->dy);
        if (err < 0)
        {
            return err;
        }

        /* Apply inversion if configured */
        if (data->invert_y)
        {
            data->dy = -data->dy;
        }

        /* Scale by sensitivity */
        data->dy = (data->dy * data->sensitivity) / 64;

        /* Apply move/scroll factor based on mode */
        if (data->mode == 0)
        {
            /* Move mode */
            data->dy = data->dy * data->move_factor;
        }
        else
        {
            /* Scroll mode */
            data->dy = data->dy * data->scroll_factor;
        }
    }

    /* Read button state */
    if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_PROX)
    {
        uint8_t button = 0;
        err = trackball_pim447_read_reg(dev, TRACKBALL_PIM447_REG_SWITCH, &button);
        if (err < 0)
        {
            return err;
        }

        data->button_state = button;
    }

    return 0;
}

/**
 * @brief Get a channel value from the trackball
 *
 * @param dev Device instance
 * @param chan The sensor channel to get
 * @param val Where to store the value
 * @return 0 on success, negative error code otherwise
 */
static int trackball_pim447_channel_get(const struct device *dev, enum sensor_channel chan,
                                        struct sensor_value *val)
{
    struct trackball_pim447_data *data = dev->data;

    switch (chan)
    {
    case SENSOR_CHAN_POS_DX:
        val->val1 = data->dx;
        val->val2 = 0;
        break;

    case SENSOR_CHAN_POS_DY:
        val->val1 = data->dy;
        val->val2 = 0;
        break;

    case SENSOR_CHAN_PROX:
        val->val1 = data->button_state;
        val->val2 = 0;
        break;

    default:
        return -ENOTSUP;
    }

    return 0;
}

/**
 * @brief Set attributes for the trackball
 *
 * @param dev Device instance
 * @param chan The sensor channel
 * @param attr The attribute to set
 * @param val The value to set
 * @return 0 on success, negative error code otherwise
 */
static int trackball_pim447_attr_set(const struct device *dev, enum sensor_channel chan,
                                     enum sensor_attribute attr, const struct sensor_value *val)
{
    struct trackball_pim447_data *data = dev->data;
    int ret = 0;

    if (chan == SENSOR_CHAN_PROX)
    {
        // For PROX channel, handle special attributes
        switch (attr)
        {
        case SENSOR_ATTR_CONFIGURATION:
            // This is used to configure the LED color
            // Expect an array of 3 values for RGB
            if (val != NULL)
            {
                const struct sensor_value *rgb_vals = val;
                ret = trackball_pim447_set_led(dev,
                                               rgb_vals[0].val1,
                                               rgb_vals[1].val1,
                                               rgb_vals[2].val1);
            }
            break;

        case SENSOR_ATTR_FEATURE_MASK:
            // This is used to set the mode (move=0, scroll=1)
            if (val != NULL)
            {
                data->mode = val->val1 > 0 ? 1 : 0;
                LOG_INF("Trackball mode set to %s", data->mode == 0 ? "MOVE" : "SCROLL");
            }
            break;

        default:
            return -ENOTSUP;
        }
    }
    else
    {
        return -ENOTSUP;
    }

    return ret;
}

/**
 * @brief Initialize the trackball driver
 *
 * @param dev Device instance
 * @return 0 on success, negative error code otherwise
 */
static int trackball_pim447_init(const struct device *dev)
{
    const struct trackball_pim447_config *config = dev->config;
    struct trackball_pim447_data *data = dev->data;
    int err = 0;

    /* Check if I2C is ready */
    if (!device_is_ready(config->i2c.bus))
    {
        LOG_ERR("I2C bus %s not ready", config->i2c.bus->name);
        return -ENODEV;
    }

    /* Store configuration in runtime data */
    data->invert_x = config->invert_x;
    data->invert_y = config->invert_y;
    data->sensitivity = config->sensitivity;
    data->move_factor = config->move_factor;
    data->scroll_factor = config->scroll_factor;
    data->mode = 0; // Default to move mode

    /* Set the initial LED color */
    err = trackball_pim447_set_led(dev, config->led_red, config->led_green, config->led_blue);
    if (err < 0)
    {
        LOG_ERR("Failed to set initial LED color");
        return err;
    }

    LOG_INF("Pimoroni Trackball initialized (addr: 0x%02x)", config->i2c.addr);
    return 0;
}

/* API functions structure */
static const struct sensor_driver_api trackball_pim447_api = {
    .sample_fetch = trackball_pim447_sample_fetch,
    .channel_get = trackball_pim447_channel_get,
    .attr_set = trackball_pim447_attr_set,
};

/* Driver initialization */
#define TRACKBALL_PIM447_INIT(inst)                                                       \
    static struct trackball_pim447_data trackball_pim447_data_##inst;                     \
                                                                                          \
    static const struct trackball_pim447_config trackball_pim447_config_##inst = {        \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                                \
        .led_red = DT_INST_PROP_OR(inst, led_red, 0),                                     \
        .led_green = DT_INST_PROP_OR(inst, led_green, 0),                                 \
        .led_blue = DT_INST_PROP_OR(inst, led_blue, 0),                                   \
        .invert_x = DT_INST_PROP_OR(inst, invert_x, false),                               \
        .invert_y = DT_INST_PROP_OR(inst, invert_y, false),                               \
        .sensitivity = DT_INST_PROP_OR(inst, sensitivity, 64),                            \
        .move_factor = DT_INST_PROP_OR(inst, move_factor, 1),                             \
        .scroll_factor = DT_INST_PROP_OR(inst, scroll_factor, 1),                         \
    };                                                                                    \
                                                                                          \
    DEVICE_DT_INST_DEFINE(inst, trackball_pim447_init, NULL,                              \
                          &trackball_pim447_data_##inst, &trackball_pim447_config_##inst, \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &trackball_pim447_api);

DT_INST_FOREACH_STATUS_OKAY(TRACKBALL_PIM447_INIT)