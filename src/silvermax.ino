
#include "qscomm.h"
/*
ISR(TIMER1_COMPA_vect) //1000 msec timer 
{
	if (t1_div%16 > 8) {ticker = '*';}
	else {ticker = ' ';};
	t1_div += 1;
	if (t1_div%32 == 0) //time to reset min/max
	{
		max_value = 0;
		min_value = 0xFFFF;
		
	} 
}

void timer_init(void){

   //Timer1 is used as 1 sec time base
   //Timer Clock = 1/1024 of sys clock
   //Mode = CTC (Clear Timer On Compare)
   TCCR1B|=((1<<WGM12)|(1<<CS12)|(1<<CS10));
   //Compare value=976
   OCR1A=976;
   TIMSK|=(1<<OCIE1A);  //Output compare 1A interrupt enable
 
}
*/

#include <NeoSWSerial.h>

//#include <serial9bit/HardwareSerial.h>

NeoSWSerial swserial( 3, 4 );

int msg[20];

bool bSendStepdir = false;
bool bSendPoll = false;
bool bSendHalt = false;
bool bSendMove = false;
bool bSendPlainSerial = false;
#define ADDR_START 1
uint8_t addr = ADDR_START;

uint8_t i;
void(*cmd_ptr)(uint8_t);  

void handleRxChar( uint8_t c ){    
      switch (c){
        case 's': 
          cmd_ptr = qs_enable_stepdir;break;
        case 'p':
          cmd_ptr = qs_poll;break;
        case 'h':
          cmd_ptr = qs_halt;break;          
        case 'm':
          cmd_ptr = qs_move;break; 
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

uint8_t m1 = 0;
uint8_t m2 = 0;

void serialEvent() { //callback for ack receival
   int r = Serial.read();
   if (awaitState == AWAIT_ACK_ADDR){
    if ((r & 0x0100) != 0) 
    {
      //ss.print("ADDR ");
      //ss.println((r & 0x00FF), HEX);  
      m1 = r & 0x00FF;  
      awaitState = AWAIT_ACK;      
      return;
    }else{
      swserial .print("?? ");
      
    }
   } else if (awaitState == AWAIT_ACK){
     if ( r == 0x80) {
       //ss.println("ACK ");
       m2 = 'A';  
     }else{
       //ss.print("NACK ");
       //ss.println(r, HEX);
       m2 = 'N';  
     }
     awaitState = 0;
     return;
   }
}

void loop() { 
  if ( m1 != 0){
    swserial.print("M1");
    swserial.println(m1, HEX);
    m1 = 0;
  }
  if ( m2 != 0){
    swserial.print("M2");
    swserial.println(m2, HEX);
    m2 = 0;
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
