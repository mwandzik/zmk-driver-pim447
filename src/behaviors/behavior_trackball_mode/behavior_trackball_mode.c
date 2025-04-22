/*
 * Copyright (c) 2023-2025 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_trackball_mode

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/pointer_event.h>
#include <zmk/endpoints.h> // Needed for device_is_ready check

// Re-define custom attributes locally if not using a shared header
// These MUST match the definitions in the driver
#define PIM447_ATTR_LED_RGB (SENSOR_ATTR_PRIV_START)
#define PIM447_ATTR_MODE (SENSOR_ATTR_PRIV_START + 1)

// Use Kconfig to control logging level
#if CONFIG_ZMK_BEHAVIOR_TRACKBALL_MODE_DEBUG
#define LOG_LEVEL LOG_LEVEL_DBG
#else
#define LOG_LEVEL CONFIG_ZMK_LOG_LEVEL
#endif
LOG_MODULE_REGISTER(behavior_trackball_mode, LOG_LEVEL);

enum trackball_mode
{
    TRACKBALL_MODE_MOVE = 0,
    TRACKBALL_MODE_SCROLL = 1
};

struct behavior_trackball_mode_config
{
    enum trackball_mode default_mode;
    uint8_t led_mode_move;
    uint8_t led_mode_scroll;
};

struct behavior_trackball_mode_data
{
    enum trackball_mode mode;
    const struct device *trackball_dev; // Keep track of the trackball device
};

static int on_trackball_mode_binding_pressed(struct zmk_behavior_binding *binding,
                                             struct zmk_behavior_binding_event event)
{
    const struct device *dev = zmk_behavior_get_binding_device(binding);
    struct behavior_trackball_mode_data *data = dev->data;
    const struct behavior_trackball_mode_config *config = dev->config;

    uint8_t param = binding->param1;
    bool mode_changed = false;

    // Determine the new mode based on the binding parameter
    switch (param)
    {
    case 0: // Toggle mode
        if (data->mode == TRACKBALL_MODE_MOVE)
        {
            data->mode = TRACKBALL_MODE_SCROLL;
        }
        else
        {
            data->mode = TRACKBALL_MODE_MOVE;
        }
        mode_changed = true;
        break;
    case 1: // Set to scroll mode
        if (data->mode != TRACKBALL_MODE_SCROLL)
        {
            data->mode = TRACKBALL_MODE_SCROLL;
            mode_changed = true;
        }
        break;
    case 2: // Set to move mode
        if (data->mode != TRACKBALL_MODE_MOVE)
        {
            data->mode = TRACKBALL_MODE_MOVE;
            mode_changed = true;
        }
        break;
    default:
        LOG_ERR("Unknown trackball mode parameter: %d", param);
        return -ENOTSUP;
    }

    // If mode changed, update driver and LED
    if (mode_changed && data->trackball_dev != NULL && device_is_ready(data->trackball_dev))
    {
        // 1. Tell the driver the new mode
        struct sensor_value mode_val = {.val1 = (data->mode == TRACKBALL_MODE_SCROLL)}; // 0 for move, 1 for scroll
        int ret = sensor_attr_set(data->trackball_dev, SENSOR_CHAN_ALL, PIM447_ATTR_MODE, &mode_val);
        if (ret != 0)
        {
            LOG_ERR("Failed to set trackball driver mode: %d", ret);
        }

        // 2. Set the LED color based on the new mode
        uint8_t led_color = (data->mode == TRACKBALL_MODE_MOVE) ? config->led_mode_move : config->led_mode_scroll;

        // Map preset colors to RGB values
        uint8_t red = 0, green = 0, blue = 0;
        switch (led_color)
        {
        case 0: // OFF
            // All zeros (default)
            break;
        case 1: // RED
            red = 255;
            break;
        case 2: // GREEN
            green = 255;
            break;
        case 3: // BLUE
            blue = 255;
            break;
        case 4: // YELLOW
            red = 255;
            green = 255;
            break;
        case 5: // CYAN
            green = 255;
            blue = 255;
            break;
        case 6: // MAGENTA
            red = 255;
            blue = 255;
            break;
        case 7: // WHITE
            red = 255;
            green = 255;
            blue = 255;
            break;
        default:
            // Invalid color, use default
            break;
        }

        // Use PIM447_ATTR_LED_RGB to set LED color
        struct sensor_value rgb[3] = {
            {.val1 = red, .val2 = 0},
            {.val1 = green, .val2 = 0},
            {.val1 = blue, .val2 = 0}};

        // Channel is ignored by the driver for this custom attribute
        ret = sensor_attr_set(data->trackball_dev, SENSOR_CHAN_ALL, PIM447_ATTR_LED_RGB, rgb);
        if (ret != 0)
        {
            LOG_ERR("Failed to set LED color using custom attribute: %d", ret);
        }
        else
        {
            LOG_DBG("Trackball mode changed to %s, LED color set to %d",
                    data->mode == TRACKBALL_MODE_MOVE ? "MOVE" : "SCROLL",
                    led_color);
        }
    }

    // Report the current mode
    LOG_INF("Trackball mode: %s", data->mode == TRACKBALL_MODE_MOVE ? "MOVE" : "SCROLL");

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_trackball_mode_binding_released(struct zmk_behavior_binding *binding,
                                              struct zmk_behavior_binding_event event)
{
    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_trackball_mode_init(const struct device *dev)
{
    struct behavior_trackball_mode_data *data = dev->data;
    const struct behavior_trackball_mode_config *config = dev->config;

    // Set the initial mode from device tree configuration
    data->mode = config->default_mode;

    // Try to find the trackball device - this behavior depends on the driver via Kconfig
    // so the chosen device should exist if this behavior is enabled.
    data->trackball_dev = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zmk_pointing_device));

    if (data->trackball_dev == NULL)
    {
        LOG_WRN("Trackball device (chosen zmk,pointing-device) not found. LED/Mode control disabled.");
    }
    else if (!device_is_ready(data->trackball_dev))
    {
        LOG_ERR("Trackball device %s is not ready. LED/Mode control disabled.", data->trackball_dev->name);
        data->trackball_dev = NULL; // Mark as unusable
    }
    else
    {
        LOG_INF("Found trackball device: %s. Initializing driver mode and LED.", data->trackball_dev->name);

        // 1. Set initial driver mode
        struct sensor_value mode_val = {.val1 = (data->mode == TRACKBALL_MODE_SCROLL)}; // 0 for move, 1 for scroll
        int ret = sensor_attr_set(data->trackball_dev, SENSOR_CHAN_ALL, PIM447_ATTR_MODE, &mode_val);
        if (ret != 0)
        {
            LOG_ERR("Failed to set initial trackball driver mode: %d", ret);
            // Continue anyway, maybe LED will work
        }

        // 2. Set initial LED color based on the default mode
        uint8_t led_color = (data->mode == TRACKBALL_MODE_MOVE) ? config->led_mode_move : config->led_mode_scroll;

        // Map preset colors to RGB values
        uint8_t red = 0, green = 0, blue = 0;
        switch (led_color)
        {
        case 0: // OFF
            // All zeros (default)
            break;
        case 1: // RED
            red = 255;
            break;
        case 2: // GREEN
            green = 255;
            break;
        case 3: // BLUE
            blue = 255;
            break;
        case 4: // YELLOW
            red = 255;
            green = 255;
            break;
        case 5: // CYAN
            green = 255;
            blue = 255;
            break;
        case 6: // MAGENTA
            red = 255;
            blue = 255;
            break;
        case 7: // WHITE
            red = 255;
            green = 255;
            blue = 255;
            break;
        default:
            // Invalid color, use default
            break;
        }

        // Set the initial LED color using custom attribute
        struct sensor_value rgb[3] = {
            {.val1 = red, .val2 = 0},
            {.val1 = green, .val2 = 0},
            {.val1 = blue, .val2 = 0}};

        // Channel is ignored by the driver for this custom attribute
        ret = sensor_attr_set(data->trackball_dev, SENSOR_CHAN_ALL, PIM447_ATTR_LED_RGB, rgb);
        if (ret != 0)
        {
            LOG_ERR("Failed to set initial LED color using custom attribute: %d", ret);
        }
    }

    LOG_INF("Trackball mode behavior initialized, default mode: %s",
            data->mode == TRACKBALL_MODE_MOVE ? "MOVE" : "SCROLL");

    return 0;
}

// Hook up the behavior handler callbacks
static const struct behavior_driver_api behavior_trackball_mode_driver_api = {
    .binding_pressed = on_trackball_mode_binding_pressed,
    .binding_released = on_trackball_mode_binding_released,
};

// Device instance definition
#define KP_INST(n)                                                                            \
    static struct behavior_trackball_mode_data behavior_trackball_mode_data_##n = {           \
        .mode = TRACKBALL_MODE_MOVE, /* Default mode, overridden by config */                 \
        .trackball_dev = NULL,                                                                \
    };                                                                                        \
                                                                                              \
    static const struct behavior_trackball_mode_config behavior_trackball_mode_config_##n = { \
        .default_mode = DT_INST_ENUM_IDX(n, default_mode),                                    \
        .led_mode_move = DT_INST_PROP_OR(n, led_mode_move, 2),     /* Default Green */        \
        .led_mode_scroll = DT_INST_PROP_OR(n, led_mode_scroll, 3), /* Default Blue */         \
    };                                                                                        \
                                                                                              \
    DEVICE_DT_INST_DEFINE(n, behavior_trackball_mode_init, NULL,                              \
                          &behavior_trackball_mode_data_##n,                                  \
                          &behavior_trackball_mode_config_##n, APPLICATION,                   \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                \
                          &behavior_trackball_mode_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)