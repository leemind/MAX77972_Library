cmake_minimum_required(VERSION 3.16)

project(MAX77972_Library)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the library
idf_component_register(
    SRC_DIRS "src"
    INCLUDE_DIRS "include"
    REQUIRES "driver" "esp_driver_i2c" "esp_common" "esp_log" "freertos"
)
