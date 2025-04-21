# Copyright (c) 2023-2025 The ZMK Contributors
# SPDX-License-Identifier: MIT

# Always include the header directory regardless of CONFIG flag
# This ensures dt-bindings are available during preprocessing
zephyr_include_directories(include)

# Include the driver and behavior subdirectories if their respective Kconfig options are enabled
if(CONFIG_ZMK_TRACKBALL_PIM447)
  zephyr_library_subdirectory(src/drivers/sensor/trackball_pim447)
endif()

if(CONFIG_ZMK_BEHAVIOR_TRACKBALL_MODE)
  zephyr_library_subdirectory(src/behaviors/behavior_trackball_mode)
endif()