# Copyright (c) 2023-2025 The ZMK Contributors
# SPDX-License-Identifier: MIT

# Always include the header directory regardless of CONFIG flag
# This ensures dt-bindings are available during preprocessing
zephyr_include_directories(include)

if(CONFIG_ZMK_TRACKBALL_PIM447)
  # Add driver and behavior components
  add_subdirectory(src/drivers/sensor/trackball_pim447)
  add_subdirectory(src/behaviors/behavior_trackball_mode)
endif()