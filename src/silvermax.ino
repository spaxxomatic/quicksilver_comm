
#include "qscomm.h"
#include "spi_proto.h"
#include<SPI.h>   
#include <NeoSWSerial.h>

//#include <serial9bit/HardwareSerial.h>

NeoSWSerial swserial( 3, 4 );

int msg[20];

#define ADDR_START 1
uint8_t addr = ADDR_START;
uint8_t i;
void(*cmd_ptr)(uint8_t);  

uint8_t spi_buff_pos;
byte spi_rx_buff[32] ;
byte spi_tx_buff;

void spi_send_stat( byte stat){
  pinMode(TX_REQ_PIN, OUTPUT);
  digitalWrite(TX_REQ_PIN, HIGH);  //signal master to pick the data up
  spi_tx_buff = stat;
  //delayMicroseconds(2);
  digitalWrite(TX_REQ_PIN, LOW);  
  //pinMode(TX_REQ_PIN, OUTPUT);
}

void handleRxChar( uint8_t c ){    
      switch (c){
        case 's': 
          cmd_ptr = qs_enable_stepdir; break;
        case 'p':
          cmd_ptr = qs_poll; break;
        case 'h':
          cmd_ptr = qs_halt; break;          
        case 'm':
          cmd_ptr = qs_move_rel_velocitybased; break;         
        case '+':
          addr++;break;     
        case '-':
          addr--;break; 
        case 'T': //SPI response test
          spi_send_stat('K');
          break;                              
      }   
}
    
#define LED_PIN 13 //arduino pro mini has a led on pin 13
ISR (SPI_STC_vect)
{
  byte c = SPDR;  // grab byte from SPI Data Register
  if (msgstate == MSGSTATE_WAIT_START ) { //status request is indicated when the first byte is 0xFF
    if (c == 0xFF) msgstate = MSGSTATE_STAT_REQ;
    else  msgstate = MSGSTATE_COMMAND;
    spi_buff_pos = 0; //transmission start
    payload_len = 0;
  }
  if (msgstate == MSGSTATE_COMMAND) {// a command comes in 
    // add to buffer if room
    if (spi_buff_pos < (sizeof (spi_rx_buff))){
      spi_rx_buff [spi_buff_pos++] = c;
      SPDR = c ;  // send back 
    }
    
    if (spi_buff_pos == 1) //second byte is the length of the message payload
      payload_len = c + 2; //address is the first, then comes the len, so we add 2
    
    if (spi_buff_pos == payload_len) {
      msgstate = MSGSTATE_BUSY_PROCESSING;
    } 
  }else{
      SPDR = spi_tx_buff;
  }
}

void setup() {
  
  //setup spi as slave
  pinMode(MISO,OUTPUT);   
  
  SPCR |= _BV(SPE);      // Turn on SPI in Slave Mode via the SPI Control Register
    // turn on interrupts
  SPCR |= _BV(SPIE);
  //SPI.attachInterrupt();
  //SPI.setClockDivider(SPI_CLOCK_DIV8);
  //SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
  //SPI.attachInterrupt(spi_on_receive);

  //setup silvermax conn
  Serial.begin(57600, SERIAL_9N1);
  
  swserial.attachInterrupt( handleRxChar );
  swserial.begin( 38400 );
  swserial.println("RDY");
  pinMode(RS485_RXEN_PIN, OUTPUT);
  digitalWrite(RS485_RXEN_PIN, LOW);  //enable reception

  
}


#define AWAIT_ACK_ADDR 1
#define AWAIT_ACK 2
volatile uint8_t awaitState = false;

#define RESP_ACK 1
#define RESP_UNKNOWN 0
#define RESP_NACK 0xFF

#define RCV_ERR_INVALID_STARTBYTE 1

typedef struct  {
  uint8_t rcv_addr ;
  uint8_t rcv_error ;
  byte ack_resp ;
  byte nack_msg ;
  bool resp_received ;
  byte no_of_words ;
} qt_resp_struct ;

volatile qt_resp_struct qt_resp = {0};

volatile uint8_t rcv_bit;

ISR(TIMER1_COMPA_vect) //rcv timeout timer
{
  //reset receive protocol state machine
  memset((void*) &qt_resp, 0, sizeof(qt_resp_struct));
  rcv_bit = 0;
  spi_send_stat(ERR_TIMEOUT_WAITING_SERVO_RESPONSE);
}

void start_await_resp_timer(void){

   //Timer1 will timeout if a response is not received in a timely manner
   //Timer Clock = 1/1024 of sys clock
   //Mode = CTC (Clear Timer On Compare)
   TCCR1B|=((1<<WGM12)|(1<<CS12)|(1<<CS10));
   OCR1A=6; //compare value: x*F_CPU/1024
   TIMSK1 |=(1<<OCIE1A);  //Output compare 1A interrupt enable
 
}


void serialEvent() { //callback for ack receival
   int r = Serial.read();
   rcv_bit ++;
   if (rcv_bit == 1){ //first bit is the address
      if ((r & 0x0100) != 0) 
      {
        //ss.print("ADDR ");
        //ss.println((r & 0x00FF), HEX);  
        qt_resp.rcv_addr = r & 0x00FF;  
        return;
      }else{
        qt_resp.rcv_error = RCV_ERR_INVALID_STARTBYTE;
      }
   } else if (rcv_bit == 2){
      if ( r & 0b10000000) { //if the eigth bit is set, it's an ack
        qt_resp.ack_resp = RESP_ACK;
      } else { //if the eigth bit is not set, it's an nack following the no of words
        qt_resp.no_of_words = (byte) r;
      } 
    } else if (rcv_bit == 3){
      if ( r == 0xff) qt_resp.ack_resp = RESP_NACK;
      else qt_resp.ack_resp = RESP_UNKNOWN;
    }
    if (rcv_bit == (qt_resp.no_of_words + 2)){
        memset((void*) &qt_resp, 0, sizeof(qt_resp_struct));
        qt_resp.resp_received = true;
        rcv_bit = 0;
    }
}

void loop() { 
    if (msgstate == MSGSTATE_BUSY_PROCESSING)
    {
      if (spi_buff_pos < 2) {
        spi_tx_buff = ERR_INVALID_COMMAND;
      }else{
        send_servo_msg(spi_rx_buff, spi_buff_pos);
        start_await_resp_timer();
      }
    //spi.tra (spibuff);
    spi_buff_pos = 0;
    
    msgstate = MSGSTATE_WAIT_START;
    }  // end of flag se
  /*if ( qt_resp.resp_received){
    swserial.print("R: ");
    swserial.println(qt_resp.ack_resp, HEX);
  }
  
  if (cmd_ptr != 0){
    //ss.print("DELAY ");
    //ss.print(interbit_delay); 
    cmd_ptr(addr);
    cmd_ptr = 0;
    //Serial.setTimeout(200);
    awaitState = AWAIT_ACK_ADDR;
  }*/
}
