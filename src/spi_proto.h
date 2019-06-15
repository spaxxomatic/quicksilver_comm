#include <stdlib.h>
#include <Arduino.h>

#define MSGSTATE_WAIT_START 0
#define MSGSTATE_COMMAND 1
#define MSGSTATE_STAT_REQ 2
#define MSGSTATE_BUSY_PROCESSING 3

#define ERR_INVALID_COMMAND 0x01
#define ERR_TIMEOUT_WAITING_SERVO_RESPONSE 0x02
volatile char msgstate;
uint8_t payload_len;

#define TX_REQ_PIN PINC6  