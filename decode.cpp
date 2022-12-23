#include <iostream>

uint8_t pow(uint8_t a, uint8_t b = 2) {
  uint8_t x = 1;
  for (uint8_t i = 0; i < b; i++) {
    x = x * a;
  }
  return x;
}

uint8_t BIN_CODE_SIZE = 6;

struct BIN_CODE {
  void (*action)();
  uint8_t code;
};

void codeGrey() { std::cout << "CODE GREY\n"; }
void codeBrown() { std::cout << "CODE BROWN\n"; }
void codeFood() { std::cout << "CODE FOOD\n"; }
void codeBlue() { std::cout << "CODE BLUE\n"; }
void codeGreen() { std::cout << "CODE GREEN\n"; }
void codePurple() { std::cout << "CODE PURPLE\n"; }

BIN_CODE BIN_CODES[] = {
    BIN_CODE{&codeGrey, pow(2, 0)},  BIN_CODE{&codeBrown, pow(2, 1)},
    BIN_CODE{&codeFood, pow(2, 2)},  BIN_CODE{&codeBlue, pow(2, 3)},
    BIN_CODE{&codeGreen, pow(2, 4)}, BIN_CODE{&codePurple, pow(2, 5)},

};

struct parsedGroup {
  uint8_t code;
  uint16_t year;
  uint16_t month;
  uint16_t day;
};

uint16_t ndays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

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
              return parsedGroup{val, year, month, day};
            }
        }
        break;
    }
    return parsedGroup{0, year, month, day};
  }
};

int main(int argc, char const *argv[]) {
  char input[] = "{F:{<:{L:D}},G:{1:{4:<,;:4,B:E,I:T},2:{1:<,8:D,?:5,F:4}}})";
  unsigned int len = 57;

  for (int i = 0; i < BIN_CODE_SIZE; i++) {
    std::cout << "code:  " << uint16_t(BIN_CODES[i].code);
    BIN_CODES[i].action();
  }

  Parser parser = Parser();
  for (int i = 0; i < len; i++) {
    parsedGroup group = parser.parse(input[i]);
    if (group.code) {
      std::cout << group.year << "/" << group.month << "/" << group.day
                << " -- " << uint16_t(group.code) << "\n";
      uint8_t code = group.code;
      for (uint8_t j = BIN_CODE_SIZE; j > 0; j--) {
        if (code >= BIN_CODES[j - 1].code) {
          code -= BIN_CODES[j - 1].code;
          BIN_CODES[j - 1].action();
        }
      }
    }
  }

  return 0;
}
