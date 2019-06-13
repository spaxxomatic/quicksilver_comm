
#include "qscomm.h"

#include <NeoSWSerial.h>

//#include <serial9bit/HardwareSerial.h>

NeoSWSerial swserial( 3, 4 );

int msg[20];

#define ADDR_START 1
uint8_t addr = ADDR_START;
uint8_t i;
void(*cmd_ptr)(uint8_t);  

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
      }   
}
    
#define LED_PIN 13 //arduino pro mini has a led on pin 13

void setup() {
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
  if ( qt_resp.resp_received){
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
  }
  /*
  if (Serial.available()) {
    ss.print("R: ");
    int r = Serial.read();
    if ((r & 0x0100) != 0) 
    {
      //ss.print("ADDR ");
      ss.println((r & 0x0100), HEX);        
    }    
    int r = Serial.read();
    ss.println("ACK!");  
  }
  */
}
