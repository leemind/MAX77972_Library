cmake_minimum_required(VERSION 3.16)

idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES "MAX77972_Library"
)
