idf_component_register(SRCS "BleSerial.cpp" "BleSerialServer.cpp" "BleBatteryService.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES arduino-esp32 soc freertos)
