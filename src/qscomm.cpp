#include "quicksilver_commands.h"
#include "qscomm.h"

void send_servo_msg(byte* msg, uint8_t len){
    #ifdef DEBUG_DUMP_MSG
    for (uint8_t i=0; i<len; i++){ swserial.print(msg[i], HEX); swserial.print(':') ;}
    #endif

    digitalWrite(RS485_RXEN_PIN, HIGH);  //enable transmit
    Serial.write(0x0100 + msg[0]); //first byte is the address, the nineth bit must be set
    int sum = msg[0]; //needed for computing the crc
    delayMicroseconds(INTERBIT_DELAY_US);
    for (uint8_t i=1; i<len; i++){
      sum += msg[i];
      Serial.write((int) msg[i]); 
      delayMicroseconds(INTERBIT_DELAY_US);
    }
    uint16_t crc;
    /*if (len == 2){ // a poll command 
      crc = 0xFF & ((~(msg[0])) + 1); //1's complement +1 as indicated on page 19 of silvermax manual
    }else{
      crc = 0xFF & ((0xff^(sum)) + 1); //2 complement : inverting the digity and adding one
    }*/
    crc = 0xFF & ((0xff^(sum)) + 1); //2 complement : inverting the digity and adding one
    Serial.write(crc); 
    delayMicroseconds(INTERBIT_DELAY_US);    
    digitalWrite(RS485_RXEN_PIN, LOW);  //enable reception    
    #ifdef DEBUG_DUMP_MSG
      swserial.println(crc, HEX);
    #endif
}

void  send_simple_command(uint8_t addr, uint8_t command){
    //int crc = (0xff^(addr + command + no_of_words )) + 1; //2 complement : inverting the digity and adding one
    //uint16_t crc = (~(addr + no_of_words )) + 1; //1's complement +1 as indicated on page 19 of sivlermax manual
    //crc = 0xFF&crc;    
    byte msg[] = {addr, 1, command};    
    send_servo_msg(msg, 3);
    /*
    ss.print(addr, HEX); ss.print(' ');
    ss.print(no_of_words, HEX ); ss.print(' ');
    ss.print(command, HEX ); ss.print(' ');
    ss.println(crc, HEX );
    */
}

void  qs_halt(uint8_t addr){
    send_simple_command(addr, QS_CMD_HALT);
}

void  qs_poll(uint8_t addr){
    //will it work with the default crc check ??
    //uint16_t crc = 0xFF & ((~(addr)) + 1); //1's complement +1 as indicated on page 19 of silvermax manual
    //byte msg[] = {addr, 0, crc};
    //sendmsg(msg, 3);
    uint16_t crc = 0xFF & ((~(addr)) + 1); //1's complement +1 as indicated on page 19 of silvermax manual
    swserial.println(crc, HEX);
    byte msg[] = {addr,0};
    send_servo_msg(msg, sizeof(msg));
}

void  qs_enable_stepdir(uint8_t addr){
    send_simple_command(addr, QS_CMD_STEPDIR);
}

void  qs_move_abs_timebased(uint8_t addr, long position, long acc_time, long total_time){
  byte no_of_words  = 0x11;
 
  byte msg[] = {addr, no_of_words, QS_CMD_MOVEABS_TIMEBASED, 
  (byte) position>>24, (byte) position>>16, (byte) position>>8, (byte) position,
            0x00, 0x04, 0xEA, 0x4B, 0x19, 0x99, 
            0x99, 0x7F, 0x00, 0x00, 0x00, 0x00};
  send_servo_msg(msg, sizeof(msg));
}

void qs_move_rel_velocitybased(uint8_t addr){
  byte no_of_words  = 0x11;
 
  byte msg[] = {addr, no_of_words, QS_CMD_MOVEREL_VELOCITYBASED,
            0x00, 0x02, 0x27, 0x10, //distance
            0x00, 0x14, 0xEA, 0x4B, //acceleration
            0x22, 0xFF, 0x99, 0x7F, //velocity
            0x00, 0x00, 0x00, 0x00}; //stop enable / stop state

  /* int sum = 0;
  for (int i= 0; i < sizeof(msg); i++) {
    sum += msg[i];
  }
  int crc = (0xff^(sum)) + 1; //2 complement : inverting the digity and adding one
  msg[sizeof(msg)-1] = 0xff & crc; //last byte is the crc
  */
  send_servo_msg(msg, sizeof(msg));
}

