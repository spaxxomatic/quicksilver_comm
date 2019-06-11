

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
#include "quicksilver_commands.h"
//#include <serial9bit/HardwareSerial.h>

NeoSWSerial ss( 3, 4 );

#define RS485_RXEN_PIN 9

uint16_t interbit_delay = 500;
int msg[20];

void sendmsg(byte* msg, uint8_t len){
    for (uint8_t i=0; i<len; i++){ ss.print(msg[i], HEX); ss.print(':') ;}
    ss.println();
    uint8_t i;
    digitalWrite(RS485_RXEN_PIN, HIGH);  //enable transmit
    Serial.write(0x0100 + msg[0]); //first byte is the address and nieth bit must be set
    delayMicroseconds(interbit_delay);
    for (i=1; i<len; i++){
      Serial.write((int) msg[i]); 
      delayMicroseconds(interbit_delay);
    }
    digitalWrite(RS485_RXEN_PIN, LOW);  //enable reception    
}

void  send_simple_command(uint8_t addr, uint8_t command){
    char no_of_words  = 1;
    int crc = (0xff^(addr + command + no_of_words )) + 1; //2 complement : inverting the digity and adding one
    //uint16_t crc = (~(addr + no_of_words )) + 1; //1's complement +1 as indicated on page 19 of sivlermax manual
    crc = 0xFF&crc;
    uint8_t i = 0;
    byte msg[] = {addr, no_of_words, command, crc};    
    sendmsg(msg, 4);
    ss.print(addr, HEX); ss.print(' ');
    ss.print(no_of_words, HEX ); ss.print(' ');
    ss.print(command, HEX ); ss.print(' ');
    ss.println(crc, HEX );
}

void  send_halt_command(uint8_t addr){
    send_simple_command(addr, QS_CMD_HALT);
}

void  send_poll_command(uint8_t addr){
    char no_of_words  = 0;
    uint16_t crc = 0xFF & ((~(addr + no_of_words )) + 1); //1's complement +1 as indicated on page 19 of sivlermax manual
    byte msg[] = {addr, 0, crc};
    sendmsg(msg, 3);
}

void  send_stepdir_command(uint8_t addr){
    send_simple_command(addr, QS_CMD_STEPDIR);
}

void send_move_cmd(uint8_t addr){
//02 11 87 00 00 27 10 00 04 EA 4B 19 99 99 7F 00 00 00 00 2C
  char no_of_words  = 0x11;
  //byte msg[] = {addr, no_of_words, QS_CMD_MOVE, 0x00, 0x00, 0x27, 0x10,
  //          0x00, 0x04, 0xEA, 0x4B, 0x19, 0x99, 
  //          0x99, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00};
  byte msg[] = {addr, no_of_words, QS_CMD_MOVE, 0x00, 0xff, 0x27, 0x10,
            0x00, 0x04, 0xEA, 0x4B, 0x19, 0x99, 
            0x99, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00};

  int sum = 0;
  for (int i= 0; i < sizeof(msg); i++) {
    sum += msg[i];
  }
  int crc = (0xff^(sum)) + 1; //2 complement : inverting the digity and adding one
  msg[sizeof(msg)-1] = 0xff & crc; //last byte is the crc
  sendmsg(msg, sizeof(msg));
}


bool bSendStepdir = false;
bool bSendPoll = false;
bool bSendHalt = false;
bool bSendMove = false;
bool bSendPlainSerial = false;
#define ADDR_START 1
uint8_t addr = ADDR_START;
void handleRxChar( uint8_t c ){    
      if (c == 's'){
        bSendStepdir = true;
      }
      if (c == 'p'){
        bSendPoll = true;
      }   
      /*if (c == '1'){
        bSendPlainSerial = true;
      } */
      if (c == '+'){
        addr++;
      }        
      if (c == '-'){
        addr--;
      }  
      if (c == '8'){
        interbit_delay +=10;
      }        
      if (c == '2'){
        interbit_delay -=10;
      }                   
      if (c == 'h'){
        bSendHalt = ! bSendHalt ;
      } 
      if (c == 'm'){
        bSendMove = 1 ;
      }                 
      if (c == 'r'){ //toggle receive
         digitalWrite(RS485_RXEN_PIN, ! digitalRead(RS485_RXEN_PIN));  
      }               
}
    
#define LED_PIN 13 //arduino pro mini has a led on pin 13

//NewSoftSerial mySerial(10, 11); // RX, TX

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(57600, SERIAL_9N1);
  //SERIAL_9N2
  //SERIAL_9E1
  //SERIAL_9E2
  //SERIAL_9O1
  //SERIAL_9O2
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  ss.attachInterrupt( handleRxChar );
  ss.begin( 38400 );
  ss.println("Hello, world");
  uint16_t i = 0;
  /*for (i=0; i< 10; i++)
    Serial.println("abcd12345");
  */
  //Serial.println("Goodnight moon!");
  pinMode(RS485_RXEN_PIN, OUTPUT);
  digitalWrite(RS485_RXEN_PIN, LOW);  //enable reception
  // set the data rate for the SoftwareSerial port
  
  
}

uint8_t i;
void(*cmd_ptr)(uint8_t);  


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
      ss.print("?? ");
      
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

void loop() { // run over and over
  //check_serial_cmd(); //serial command avail?
//  if (ss.available()) {
    //Serial.write(ss.read());
//  }
  if ( m1 != 0){
    ss.print("M1");
    ss.println(m1, HEX);
    m1 = 0;
  }
  if ( m2 != 0){
    ss.print("M2");
    ss.println(m2, HEX);
    m2 = 0;
  }  
  if (bSendMove){
    bSendMove= false;
    cmd_ptr = send_move_cmd;
  }  
  if (bSendStepdir){
    bSendStepdir= false;
    cmd_ptr = send_stepdir_command;
  }
  if (bSendPoll){
    bSendPoll= false;
    cmd_ptr = send_poll_command;
  }
  if (bSendHalt){
    bSendHalt = false;
    cmd_ptr = send_halt_command;
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
