#include <stdlib.h>
#include <Arduino.h>

#define MSGSTATE_WAIT_COMMAND 0
#define MSGSTATE_COMMAND 1
#define MSGSTATE_STAT_REQ 2
#define MSGSTATE_CMD_RECEIVED 3
#define MSGSTATE_WAIT_ACK 4
#define MSGSTATE_ACK_TIMEOUT 5

#define ACK_OK 0x01
#define ACK_NOK 0x02
#define ACK_UNKNOWN 0x03
#define ACK_ERR_INVALID_STARTBYTE 0x04

#define ERR_INVALID_COMMAND 0x10
#define ERR_TIMEOUT_WAITING_SERVO_RESPONSE 0x11
#define ERR_BUFFER_OVERFLOW 0x12

volatile char msgstate;
uint8_t payload_len;

//the TX_REQ_PIN connects to a master IRQ and signalizes a trasmit request
//when received, the master should initiate an SPI trasmission and pick the slave's message
//#define TX_REQ_PIN 4 //PD4  
#define TX_REQ_PIN 9 