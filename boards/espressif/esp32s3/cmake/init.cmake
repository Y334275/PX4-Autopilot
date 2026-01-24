set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mtext-section-literals -mlongcalls -Wl,--wrap=bootloader_print_banner")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtext-section-literals -mlongcalls -Wl,--wrap=bootloader_print_banner")

set(PX4_EXTRA_MAKEFILE_STATEMENT "include ${NUTTX_DIR}/arch/xtensa/src/lx7/Toolchain.defs")
set(ESP_HAL_3RDPARTY_REPO esp-hal-3rdparty)
set(CHIP_SERIES ${PX4_CHIP})
set(NUTTX_ARCH_DIR "${NUTTX_DIR}/arch/xtensa/src/chip")
add_compile_options(-mtext-section-literals -mlongcalls -Wl,--wrap=bootloader_print_banner
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/nuttx/${CHIP_SERIES}/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/nuttx/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/bootloader_support/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/efuse/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/efuse/private_include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/efuse/${CHIP_SERIES}/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/efuse/${CHIP_SERIES}/private_include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_adc/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_adc/interface
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_adc/${CHIP_SERIES}/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_common/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_coex/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_event/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_timer/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_timer/private_include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_hw_support/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_hw_support/include/esp_private
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_hw_support/include/soc
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_hw_support/ldo/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_hw_support/mspi_timing_tuning/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_hw_support/mspi_timing_tuning/tuning_scheme_impl/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_hw_support/mspi_timing_tuning/port/${CHIP_SERIES}
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_hw_support/port/${CHIP_SERIES}/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_hw_support/port/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_hw_support/power_supply/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_mm/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_phy/${CHIP_SERIES}/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_phy/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_rom/${CHIP_SERIES}
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_rom
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_rom/${CHIP_SERIES}/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_rom/${CHIP_SERIES}/include/${CHIP_SERIES}
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_rom/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_system/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_system/port/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_system/port/include/private
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_system/port/public_compat
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_timer/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_wifi/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/hal/${CHIP_SERIES}/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/hal/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/hal/platform_port/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/heap/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/log
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/log/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/log/src/log_level/tag_log_level
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/mbedtls/mbedtls/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/newlib/priv_include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/soc/${CHIP_SERIES}/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/soc/${CHIP_SERIES}/register
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/soc/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/xtensa/${CHIP_SERIES}/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/xtensa/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/xtensa/deprecated_include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/bootloader_support/private_include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/bootloader_support/bootloader_flash/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/spi_flash/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/spi_flash/include/spi_flash
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/esp_app_format/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/nuttx/src/components/esp_driver_gpio/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/nuttx/src/components/esp_driver_uart/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/nuttx/include/mbedtls
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/ulp/ulp_common
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/ulp/ulp_common/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/ulp/ulp_riscv/include
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/ulp/ulp_riscv/shared
	-I${NUTTX_ARCH_DIR}/${ESP_HAL_3RDPARTY_REPO}/components/ulp/ulp_riscv/shared/include
	)
