/*
 * Copyright (c) 2023-2025 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

/**
 * @brief Trackball mode behavior defines.
 */

/* Mode constants for the trackball_mode behavior */
#define TB_TOG 0 /* Toggle between move and scroll modes */
#define TB_SCR 1 /* Set to scroll mode */
#define TB_MOV 2 /* Set to move mode */

/* LED color constants */
#define TB_LED_OFF 0
#define TB_LED_RED 1
#define TB_LED_GREEN 2
#define TB_LED_BLUE 3
#define TB_LED_YELLOW 4
#define TB_LED_CYAN 5
#define TB_LED_MAGENTA 6
#define TB_LED_WHITE 7