# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
# find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
find_package(Zephyr REQUIRED HINTS /home/luke/zephyrproject/zephyr)

project(gps_compass)

target_sources(app PRIVATE 
    src/main.c
    src/gps_config.c
    src/data_handler.c
    src/command_parser.c
    src/mpu6050_wrapper.c
    src/ht1621.c
    src/hmc5883l.c
)
target_link_libraries(app PUBLIC m)

