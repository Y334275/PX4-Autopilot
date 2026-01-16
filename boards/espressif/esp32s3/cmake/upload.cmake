# TODO support other port
set(FW_BIN ${PX4_BOARD_VENDOR}_${PX4_BOARD_MODEL}_${PX4_BOARD_LABEL}.bin)
if (DEFINED ENV{ESP_PORT})
	set(ESP_PORT $ENV{ESP_PORT})
else()
	set(ESP_PORT /dev/ttyACM0)
endif()


set(ESP_BOOTLOADER ${PX4_BOARD_DIR}/bootloader/bootloader-esp32s3.bin)
set(ESP_PARTITION_TABLE ${PX4_BOARD_DIR}/bootloader/partition-table-esp32s3.bin)
set(ESP_BIN ${PX4_BINARY_DIR}/${PX4_CONFIG}_merged.bin)
set(PX4_BINARY_OUTPUT ${PX4_BINARY_DIR}/${PX4_CONFIG}.bin)
add_custom_command(OUTPUT ${ESP_BIN}
	COMMAND esptool.py --chip esp32s3 merge_bin -o ${ESP_BIN} -fs ${FLASH_SIZE} -fm ${FLASH_MODE} -ff ${FLASH_FREQ} 0x0 ${ESP_BOOTLOADER} 0x8000 ${ESP_PARTITION_TABLE} 0x10000 ${PX4_BINARY_OUTPUT}
	COMMAND du ${ESP_BIN} -h
	DEPENDS ${fw_package}
	COMMENT "Merging binaries for esp32s3"
	VERBATIM
	USES_TERMINAL
	WORKING_DIRECTORY ${PX4_BINARY_DIR}
)
add_custom_target(esp_flash
	COMMAND esptool.py -p ${ESP_PORT} -b 921600 --before default_reset --after hard_reset --chip esp32s3 write_flash -fs ${FLASH_SIZE} -fm ${FLASH_MODE} -ff ${FLASH_FREQ} 0x0 ${ESP_BIN}
	DEPENDS ${ESP_BIN}
	COMMENT "Uploading to esp32s3"
	VERBATIM
	USES_TERMINAL
	WORKING_DIRECTORY ${PX4_BINARY_DIR}
)
