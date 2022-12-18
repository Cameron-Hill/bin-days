#include "uRTCLib.h"
#include <EEPROM.h>


int getIntSize(int x) {
  x = abs(x);
  return (x < 10 ? 1 : (x < 100 ? 2 : (x < 1000 ? 3 : (x < 10000 ? 4 : (x < 100000 ? 5 : (x < 1000000 ? 6 : (x < 10000000 ? 7 : (x < 100000000 ? 8 : (x < 1000000000 ? 9 : 10)))))))));
}

bool isCalUpdate(String update) {
  return update.startsWith("cal:") && update.length() > 7;
}

bool isReadUpdate(String update){
  return update.startsWith("read:");  
}

String decodeCal(char cal[], int calSize){

  return cal;
}

void writeIntIntoEEPROM(int address, int number)
{ 
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

int readIntFromEEPROM(int address)
{
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

bool saveCalUpdate(String update) {
  int size = update.length() - 4;
  char cal[size];
  update.substring(4).toCharArray(cal, size);
  Serial.print("Cal: ");
  Serial.println(cal);
  Serial.print("Cal Length: ");
  Serial.println(size);
  Serial.print("EEPROM LENGTH: ");
  Serial.println(EEPROM.length());
  if (size + getIntSize(size) > EEPROM.length()) {
    Serial.print("Calendar update too large: ");
    Serial.println(size + getIntSize(size));
    return false;
  } else {

    int addr = 0;
    Serial.println("Writing Size to EEPROM:");
    writeIntIntoEEPROM(addr, size);
    addr += sizeof(int);
    Serial.print("Writing Cal to EEPROM from :");
    Serial.println(addr);
    for (int i = 0; i < addr + size; i++) {
      EEPROM.write(i + addr, cal[i]);
    }
    Serial.println("Done...");
  }
  return true;
}

int readCalSize(){
  return readIntFromEEPROM(0);
}

void readCal(char* cal, int calSize){
  int addr = sizeof(int);
  for (int i = 0; i< calSize; i++){
    cal[i] = EEPROM.read(addr+i);
  }
  return cal;
}

bool requestUpdate() {
  String update = Serial.readString();
  if (update.length()) {
    if (isCalUpdate(update)) {
      saveCalUpdate(update);
    } else if(isReadUpdate(update)){   
      int calSize = readCalSize();
      char cal[calSize];
      readCal(cal, calSize);
      decodeCal(cal, calSize);
    }
  }
}

void setup() {
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial) {
    requestUpdate();
  }
}
