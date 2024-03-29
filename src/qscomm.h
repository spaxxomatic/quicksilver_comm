#include <stdlib.h>
#include <Arduino.h>
#include <NeoSWSerial.h>

#define RS485_RXEN_PIN 8 //D8 is PB0
#define INTERBIT_DELAY_US 400

//#define DEBUG_DUMP_MSG
extern NeoSWSerial swserial;

void send_servo_msg(byte* msg, uint8_t len);
void qs_halt(uint8_t addr);
void qs_poll(uint8_t addr);
void qs_move_rel_velocitybased(uint8_t addr);
void qs_move_abs_timebased(uint8_t addr, long position, long acc_time, long total_time);
void qs_enable_stepdir(uint8_t addr);