#include <stdlib.h>
#include <Arduino.h>
#include <NeoSWSerial.h>

#define RS485_RXEN_PIN 9
#define INTERBIT_DELAY_US 400

#define DEBUG_DUMP_MSG
extern NeoSWSerial swserial;

void qs_halt(uint8_t addr);
void qs_poll(uint8_t addr);
void qs_move(uint8_t addr);
void qs_enable_stepdir(uint8_t addr);