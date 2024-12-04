cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(p2)

idf_component_register(
    SRCS "main.c"                  # Tệp mã nguồn chính
    INCLUDE_DIRS "."               # Thư mục chứa tệp header
    REQUIRES bt                    # Thành phần Bluetooth
)