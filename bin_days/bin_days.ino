#include "Arduino.h"
#include "Wire.h"
#include "uRTCLib.h"
#include <EEPROM.h>


struct Date {
  int day;
  int month;
  int year;
};

uint16_t ndays[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

class Parser {
private:
  char stage = '0';
  bool isKey = true;
  uint16_t year = 0;
  uint16_t month = 0;
  uint16_t day = 0;

  struct parsedGroup {
    uint16_t code;
    uint16_t year;
    uint16_t month;
    uint16_t day;
  };

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
        uint16_t val = uint16_t(c) - 48;
        switch (stage) {
          case '1':
            year = val;
            break;

          case '2':
            month = val;
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
    if (update[i] != search[i]) { return false; };
  }
  return true;
}

bool isReadUpdate(char *update, int size) {
  char search[] = { 'r', 'e', 'a', 'd', ':' };
  for (int i = 0; i < 5; i++) {
    if (update[i] != search[i]) { return false; };
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

int getCode(Date date) {
  Parser parser = Parser();
  for (int i = 0; i < binDataSize; i++) {
    auto p = parser.parse(binData[i]);
    if (p.code && p.year == date.year && p.month == date.month && p.day == date.day) {
      return p.code;
    }
  }
  return 0;
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


void setup() {
  Serial.begin(9600);
  Serial.println("Started...");
  URTCLIB_WIRE.begin();
  loadBinData();
  rtc.refresh();
  today = Date{ rtc.day(), rtc.month(), rtc.year() };
}


void printDate(Date date) {
  Serial.print("Year: ");
  Serial.print(date.year);
  Serial.print(" Month: ");
  Serial.print(date.month);
  Serial.print(" Day: ");
  Serial.println(date.day);
}

void loop() {

  printDate(today);
  int code = getCode(today);
  Serial.print("Code: ");
  Serial.println(code);
  today = getTommorrow(today);

  if (Serial) {
    requestUpdate();
  }

  delay(500);
}
