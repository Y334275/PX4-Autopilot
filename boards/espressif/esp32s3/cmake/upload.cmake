# TODO support other port
if (DEFINED ENV{ESP_PORT})
	set(ESP_PORT $ENV{ESP_PORT})
else()
	set(ESP_PORT /dev/ttyACM0)
endif()

set(PX4_BINARY_OUTPUT ${PX4_BINARY_DIR}/${PX4_CONFIG}.bin)

set(FS ${FLASH_SIZE})
if(CONFIG_ESP32S3_FLASH_DETECT)
set(FS detect)
endif()

add_custom_target(esp_flash
	COMMAND esptool.py -p ${ESP_PORT} -b 921600 --before default_reset --after hard_reset --chip esp32s3 write_flash -fs ${FS} -fm dio -ff ${FLASH_FREQ} 0x0 ${PX4_BINARY_OUTPUT}
	DEPENDS ${PX4_BINARY_OUTPUT}
	COMMENT "Uploading to esp32s3"
	VERBATIM
	USES_TERMINAL
	WORKING_DIRECTORY ${PX4_BINARY_DIR}
)
