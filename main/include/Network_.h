#pragma once

#include <Arduino.h>

uint16_t networkGetConsumerTypes();

// Return the count of consumers (TCP, NTRIP Client, etc) that are enabled
// and set consumerTypes bitfield
// From this number we can decide if the network (WiFi, ethernet, cellular, etc) needs to be started
// ESP-NOW is not considered a network consumer and is blended during wifiConnect()
uint8_t networkConsumers();

uint8_t networkConsumers(uint16_t *consumerTypes);