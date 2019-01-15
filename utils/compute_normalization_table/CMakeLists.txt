cmake_minimum_required(VERSION 2.8)

project(tl_compute_normalization_table)

include_directories(.)
include_directories(../../src)
add_executable(compute_normalization_table compute_normalization_table.c pcg_basic.c)

project(tl_write_normalization_table_images)
add_executable(write_image write_image.c)
