cmake_minimum_required(VERSION 2.8)

project(tl_compute_normalization_table)

include_directories(../../src)
add_executable(compute_normalization_table compute_normalization_table.c)

add_executable(write_image write_image.c)
