#include <EEPROM.h>

#include "Arduino.h"
#include "Wire.h"
#include "uRTCLib.h"

// A4 -> SDA  -> RED
// A5 -> SCL  -> BLUE

// const static int STANDARD_DELAY_TIME=60000;
const static int STANDARD_DELAY_TIME = 5000;
const static int ALERT_DELAY_TIME = 1000;
const static uint8_t NO_BIN = 0;
const static uint8_t GREY_BIN = 1;
const static uint8_t BROWN_BIN = 2;
const static uint8_t FOOD_CADY = 3;
const static uint8_t BLUE_BIN = 4;
const static uint8_t GREEN_BIN = 5;
const static uint8_t PURPLE_BIN = 6;
const static uint8_t NO_CAL_DATA = 255;

const static uint8_t RED_1 = 2;
const static uint8_t GREEN_1 = 4;
const static uint8_t BLUE_1 = 6;
const static uint8_t RED_2 = 8;
const static uint8_t GREEN_2 = 10;
const static uint8_t BLUE_2 = 12;

const static uint8_t BIN_CODE_SIZE = 7;

const static uint8_t CONCURRENT_CONTROLLERS=2;

struct BIN_CODE {
  void (*action)();
  uint8_t code;
};


struct Date {
  uint16_t day;
  uint16_t month;
  uint16_t year;
};

struct parsedGroup {
  uint8_t code;
  uint16_t year;
  uint16_t month;
  uint16_t day;
};

uint8_t ndays[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
int delayTime = STANDARD_DELAY_TIME;
uint8_t controllerArraySize = 0;
uint8_t controllerArrayPtr = 0;

struct pinGroup {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t orange;
  uint8_t white;
};

pinGroup pinGroups[CONCURRENT_CONTROLLERS] = {pinGroup{ 2, 4, 6, 8, 10 }, pinGroup{ 3, 5, 7, 9, 11 }};

class LEDController {
public:
  uint16_t id=0;
  bool white = false;
  bool orange = false;
  bool red = false;
  bool green = false;
  bool blue = false;

  LEDController() =default;
  LEDController(uint16_t i, bool r, bool g, bool b, bool o, bool w) {
    red = r;
    green = g;
    blue = b;
    white = w;
    orange = o;
    id=i;
  }

  void apply(pinGroup group, uint8_t val){
    if (red){digitalWrite(group.red, val);}
    if (green){digitalWrite(group.green, val);}
    if (blue){digitalWrite(group.blue, val);}
    if (white){digitalWrite(group.white, val);}
    if (orange){digitalWrite(group.orange, val);}
  }

  void set(pinGroup group) {
    Serial.print("Controller ");
    Serial.print(id);
    Serial.println("Set");
    apply(group, HIGH);
  }

  void clear(pinGroup group){
    Serial.print("Controller ");
    Serial.print(id);
    Serial.println("Cleared");
    apply(group, LOW);
  }  
};

LEDController controllerArray[BIN_CODE_SIZE];

class Parser {
private:
  char stage = '0';
  bool isKey = true;
  uint16_t year = 0;
  uint16_t month = 0;
  uint16_t day = 0;

public:
  parsedGroup parse(char c) {
    // Switch 1: Set Context
    switch (c) {
      case '{':
        stage++;
        isKey = true;
        break;

      case '}':
        stage--;
        isKey = true;
        break;

      case ',':
        isKey = true;
        break;

      case ':':
        isKey = false;
        break;

      default:
        // Switch 2 set values
        uint8_t val = uint8_t(c) - 48;
        switch (stage) {
          case '1':
            year = val;
            month = 0;
            day = 0;
            break;

          case '2':
            month = val;
            day = 0;
            break;

          case '3':
            if (isKey) {
              day = val;
            } else {
              return parsedGroup{ val, year, month, day };
            }
        }
        break;
    }
    return parsedGroup{ 0, year, month, day };
  }
};

uRTCLib rtc(0x68);

const int MAX_BIN_SIZE = 1024;
char binData[MAX_BIN_SIZE];
int binDataSize = 0;
Date today;

int getIntSize(int x) {
  x = abs(x);
  return (x < 10 ? 1 : (x < 100 ? 2 : (x < 1000 ? 3 : (x < 10000 ? 4 : (x < 100000 ? 5 : (x < 1000000 ? 6 : (x < 10000000 ? 7 : (x < 100000000 ? 8 : (x < 1000000000 ? 9 : 10)))))))));
}

bool isCalUpdate(char *update, int size) {
  char search[] = { 'c', 'a', 'l', ':' };
  for (int i = 0; i < 4; i++) {
    if (update[i] != search[i]) {
      return false;
    };
  }
  return true;
}

bool isReadUpdate(char *update, int size) {
  char search[] = { 'r', 'e', 'a', 'd', ':' };
  for (int i = 0; i < 5; i++) {
    if (update[i] != search[i]) {
      return false;
    };
  }
  return true;
}

void writeIntIntoEEPROM(int address, int number) {
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

int readIntFromEEPROM(int address) {
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

bool saveCalUpdate(char *update, int updateSize) {
  int size = updateSize - 4;  // remove the cal:
  if (size + sizeof(int) > EEPROM.length()) {
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
      EEPROM.write(i + addr, update[i + 4]);
    }
    Serial.println("Done...");
  }
  return true;
}

int readCalSize() {
  return readIntFromEEPROM(0);
}

void readCal(char *cal, int calSize) {
  int addr = sizeof(int);
  for (int i = 0; i < calSize; i++) {
    cal[i] = EEPROM.read(addr + i);
  }
  return cal;
}

void loadBinData() {
  binDataSize = readCalSize();
  readCal(binData, binDataSize);
}

bool requestUpdate() {
  if (Serial.available()) {
    char buffer[MAX_BIN_SIZE];
    int len = Serial.readBytesUntil('\n', buffer, MAX_BIN_SIZE);
    if (isCalUpdate(buffer, len)) {
      saveCalUpdate(buffer, len);
    } else if (isReadUpdate(buffer, len)) {
      int calSize = readCalSize();
      readCal(buffer, calSize);
      for (int i = 0; i < calSize; i++) {
        Serial.print(buffer[i]);
      }
    }
    free(buffer);
  }
}


uint8_t mPow(uint8_t a, uint8_t b = 2) {
  uint8_t x = 1;
  for (uint8_t i = 0; i < b; i++) {
    x = x * a;
  }
  return x;
}

int getCode() {
  Parser parser = Parser();
  for (int i = 0; i < binDataSize; i++) {
    auto p = parser.parse(binData[i]);
    if (p.code) {
      Serial.print(p.year);
      Serial.print("/");
      Serial.print(p.month);
      Serial.print("/");
      Serial.print(p.day);
      Serial.print("  :  ");
      Serial.println(p.code);
    }
  }
}

uint8_t getCode(Date date) {
  Parser parser = Parser();
  for (int i = 0; i < binDataSize; i++) {
    auto p = parser.parse(binData[i]);
    Date p_date = Date{ p.day, p.month, p.year };
    if (p.code && p.year == date.year && p.month == date.month && p.day == date.day) {
      return p.code;
    } else if (greaterThan(p_date, date)) {
      return 0;
    }
  }
  return -1;
}

bool isLeapYear(Date date) {
  if ((date.year + 2000) % 4 == 0) {
    if ((date.year + 2000) % 100 == 0) {
      if ((date.year + 2000) % 400 == 0) {
        return true;
      } else {
        return false;
      }
    }
    return true;
  }
  return false;
}

bool greaterThan(Date a, Date b) {
  if (a.year > b.year) {
    return true;
  } else if (a.year == b.year && a.month > b.month) {
    return true;
  } else if (a.year == b.year && a.month == b.month && a.day > b.day) {
    return true;
  }
  return false;
}

Date getTommorrow(Date date) {
  bool leap = isLeapYear(date) && date.month == 2;
  if (date.day == ndays[date.month] + leap) {
    if (date.month == 12) {
      return Date{ 1, 1, date.year + 1 };
    }
    return Date{ 1, date.month + 1, date.year };
  }
  return Date{ date.day + 1, date.month, date.year };
}

void printDate(Date date, bool newLine = true) {
  Serial.print(date.year);
  Serial.print("/");
  Serial.print(date.month);
  Serial.print("/");
  if (newLine) {
    Serial.println(date.day);
  } else {
    Serial.print(date.day);
  }
}

void initializePinGroups(){
  for (int i=0; i<CONCURRENT_CONTROLLERS; i++){
      pinMode(pinGroups[i].red, OUTPUT);
      pinMode(pinGroups[i].green, OUTPUT);
      pinMode(pinGroups[i].blue, OUTPUT);
      pinMode(pinGroups[i].orange, OUTPUT);
      pinMode(pinGroups[i].white, OUTPUT);
  }
}

void addController(LEDController controller){
  for (int i=0; i< controllerArraySize; i++){
    if (controller.id == controllerArray[i].id){
      return;
    }
  }
  controllerArray[controllerArraySize]  = controller;
  controllerArraySize ++;
  Serial.print("Controller Added: ");
  Serial.println(controller.id);
}

void clearControllers(){
  free(controllerArray);
  controllerArraySize=0;
  controllerArrayPtr=0;
}

void applyControllers() {
  if (controllerArraySize) {
    for (uint8_t i = 0; i < CONCURRENT_CONTROLLERS; i++) {
      controllerArray[(controllerArrayPtr + i) % controllerArraySize].clear(pinGroups[i]);
    }

    controllerArrayPtr = (controllerArrayPtr + 2) % controllerArraySize;
    for (uint8_t i = 0; i < CONCURRENT_CONTROLLERS; i++) {
      controllerArray[(controllerArrayPtr + i) % controllerArraySize].set(pinGroups[i]);
      Serial.print("Assigned Controller: ");
      Serial.print(controllerArray[(controllerArrayPtr + i) % controllerArraySize].id);
      Serial.print("  To pin group:  ");
      Serial.println(i);
    }
  }
}

void codeGrey() {
  // Serial.println("CODE GREY");
  addController(LEDController(1, true, true, true, false, false));

}
void codeBrown() {
  // Serial.println("CODE BROWN");
  addController(LEDController(2, false, false, false, true, false));
}
void codeFood() {
  addController(LEDController(3, false, false, false, false, true));
}
void codeBlue() {
  // Serial.println("CODE BLUE");
  addController(LEDController(4, false, false, true, false, false));
}
void codeGreen() {
  // Serial.println("CODE GREEN");
  addController(LEDController(5, false, true, false, false, false));
}
void codePurple() {
  // Serial.println("CODE PURPLE");
  addController(LEDController(6, true, false, true, false, false));
}
void noCalData() {
  Serial.println("No More Cal Data");
}

BIN_CODE BIN_CODES[] = {
  BIN_CODE{ &codeGrey, mPow(2, 0) }, BIN_CODE{ &codeBrown, mPow(2, 1) },
  BIN_CODE{ &codeFood, mPow(2, 2) }, BIN_CODE{ &codeBlue, mPow(2, 3) },
  BIN_CODE{ &codeGreen, mPow(2, 4) }, BIN_CODE{ &codePurple, mPow(2, 5) },
  BIN_CODE{ &noCalData, 255 }
};


void callActions(uint8_t code, bool alert = false) {
  // I am sure there are smarter ways to do this but y'know
  for (uint8_t j = BIN_CODE_SIZE; j > 0; j--) {
    if (code >= BIN_CODES[j - 1].code) {
      code -= BIN_CODES[j - 1].code;
      BIN_CODES[j - 1].action();
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Initialising...");
  URTCLIB_WIRE.begin();
  loadBinData();
  initializePinGroups();
  rtc.refresh();
  today = Date{ rtc.day(), rtc.month(), rtc.year() };
  Serial.println("Ready!");
  printDate(today);
}



void loop() {
  // printDate(today, false);
  // uint16_t hour = rtc.hour();
  // uint8_t code = getCode(today);
  // if (code > 0 && hour < 10) {
  //   if (hour > 6) {
  //     delayTime = ALERT_DELAY_TIME;
  //     // callActions(code, true);
  //   } else {
  //     delayTime = STANDARD_DELAY_TIME;
  //     // callActions(code);
  //   }

  // } else {
  //   Date tommorrow = getTommorrow(today);
  //   code = getCode(tommorrow);
  //   delayTime = STANDARD_DELAY_TIME;
  //   if (code > 0) {
  //     printDate(tommorrow);
  //     callActions(code);
  //   } else {
  //     clearControllers();
  //   }
  // }
  // today = getTommorrow(today);
  
  
  codeGrey();
  codeBrown();
  // codeGreen();
  codeBlue();
  codeFood();
  codePurple();

  if (Serial) {
    requestUpdate();
  }

  applyControllers();



  delay(delayTime);
}
