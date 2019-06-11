/*
 * uart.h
 *
 *  Created on: 24.11.2011
 *      Author: Frank Link
 */

#ifndef UART_H_
#define UART_H_

#include <stdint.h>

typedef union _DMX_CRC
{
	uint8_t a;
	uint8_t b;
	uint8_t c;
	uint8_t d;
	uint32_t crc32;
} _dmx_crc;

typedef struct _DMX_TRANSCEIVE
{
	uint8_t startAdress;
	uint8_t frameLength;
	uint8_t h;
	uint8_t s;
	uint8_t v;
	uint8_t power;
	uint8_t fadingSpeed;
	_dmx_crc crc32;
} _dmx_transceive;

typedef struct _DMX_RECEIVE
{
	uint8_t startAdress;
	_dmx_crc crc32;
} _dmx_receive;

extern void uart_init(void);
extern void uart_putc (uint16_t ch);
extern void uart_puts (const char * s);
extern void uart_puts_p(const char *s );
extern uint8_t uart_hit( void );
extern uint8_t uart_ready( void );

extern uint16_t uart_getc(void);
#define uart_puts_P(__s) uart_puts_p(PSTR(__s))

extern void setTransmitMode( void );
extern void setReceiveMode( void );

#endif 
