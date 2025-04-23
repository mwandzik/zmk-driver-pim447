/*
 * Copyright (c) 2023-2025 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

/**
 * @brief Trackball mode behavior defines.
 * @defgroup trackball_mode_behavior Trackball Mode Behavior
 * @{
 */

/* Trackball Modes (used with default-mode property) */
#define PIM447_MOVE  0
#define PIM447_SCROLL 1

/* Mode Parameters (used as argument to &tb_mode binding) */
#define MOVE_TOGGLE 0 /* Toggle between move and scroll modes */
#define SCROLL_SET  1 /* Set to scroll mode */
#define MOVE_SET    2 /* Set to move mode */


/**
 * @brief Trackball LED color presets
 * @defgroup trackball_led_presets Trackball LED Presets
 * @{
 */
#define LED_OFF     0
#define LED_RED     1
#define LED_GREEN   2
#define LED_BLUE    3
#define LED_YELLOW  4
#define LED_CYAN    5
#define LED_MAGENTA 6
#define LED_WHITE   7
/** @} */

/** @} */