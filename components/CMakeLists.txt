set(src_dirs
    device
    device/i2c_device
    device/i2c_device/sht40
    device/i2c_device/mp4201
    device/i2c_device/mcp4725
    device/spi_device
    device/spi_device/sd_card
    temperature_monitor
    adjustable_psu
)

set(include_dirs
    device
    device/i2c_device
    device/i2c_device/sht40
    device/i2c_device/mp4201
    device/i2c_device/mcp4725
    device/spi_device
    device/spi_device/sd_card
    temperature_monitor
    adjustable_psu
)

set(required_components
    driver
    esp_timer
    esp_driver_i2c
    esp_driver_spi
    esp_adc
    esp_event
    esp_wifi
    esp_http_server
    spiffs
    nvs_flash
    lwip
    fatfs
)

idf_component_register(
    SRC_DIRS ${src_dirs}
    INCLUDE_DIRS ${include_dirs}
    REQUIRES ${required_components}
)

component_compile_options(
    -ffast-math -O3 -Wno-error=format=-Wno-format
)
