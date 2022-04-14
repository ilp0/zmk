# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT

zephyr_library_named(zmk__drivers__mouse_sensor)
zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)

zephyr_library_sources_ifdef(CONFIG_ZMK_MOUSE mouse_sensor.c)