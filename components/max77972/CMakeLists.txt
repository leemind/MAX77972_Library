idf_component_register(
    SRCS "max77972.cpp"
    INCLUDE_DIRS "include"
    REQUIRES "driver" "esp_driver_i2c" "esp_common" "freertos"
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
