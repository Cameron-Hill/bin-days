#include <iostream>
using namespace std;

uint16_t ndays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// This script has a WHOLE lot of faith in the input value...

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
              return parsedGroup{val, year, month, day};
            }
        }
        break;
    }
    return parsedGroup{0, year, month, day};
  }
};

class Day {
 public:
  uint16_t day;
  uint16_t month;
  uint16_t year;
  uint16_t code = 0;
  Day() { setCode(0); };
  Day(uint16_t theDay, uint16_t theMonth, uint16_t theYear,
      uint16_t theCode = 0) {
    day = theDay;
    month = theMonth;
    year = theYear;
    code = theCode;
  }
  void setCode(uint16_t theCode) { code = theCode; }
};

class Month {
 private:
  Day days[32];

 public:
  uint16_t month;
  uint16_t year;
  uint16_t length;
  bool sb = false;
  Month() = default;
  Month(uint16_t theMonth, uint16_t theYear) : month(theMonth), year(theYear) {
    length = ndays[theMonth];
    if (month == 2 && isLeapYear()) {
      length++;
    }
    for (uint16_t i = 0; i < length; i++) {
      days[i] = Day(i + 1, theMonth, theYear);
    }
  };

  bool isLeapYear()

  {
    if ((year + 2000) % 4 == 0) {
      if ((year + 2000) % 100 == 0) {
        if ((year + 2000) % 400 == 0) {
          return true;
        } else {
          return false;
        }
      }
      return true;
    }
    return false;
  }

  Day *getDayPtr(uint16_t day) { return &days[day - 1]; }
  Day getDay(uint16_t day) { return days[day - 1]; }
};

class Year {
 private:
  Month months[12];

 public:
  uint16_t year;
  bool hasCode = false;

  Year() = default;
  Year(uint16_t theYear) : year(theYear) {
    for (uint16_t i = 0; i < 12; i++) {
      months[i] = Month(i + 1, year);
    }
  };

  Month *getMonthPtr(uint16_t month) { return &months[month - 1]; }
  Month getMonth(uint16_t month) { return months[month - 1]; }
};

class Calendar {
 private:
  char *bin_data;
  uint16_t bin_data_length;

 public:
  Calendar() {
    bin_data = new char[0];
    bin_data_length = 0;
  };

  Calendar(char *bin_code, uint16_t bin_code_size) {
    bin_data = new char[bin_code_size];
    bin_data = bin_code;
    bin_data_length = bin_code_size;
  }

  Year getYear(uint16_t year) {
    Year y = Year(year);
    Parser parser = Parser();
    for (uint16_t i = 0; i < bin_data_length; i++) {
      auto p = parser.parse(bin_data[i]);
      if (p.code && p.year == year) {
        Month *m = y.getMonthPtr(p.month);
        Day *d = m->getDayPtr(p.day);
        d->code = p.code;
      } else if (p.year > year) {
        return y;
      }
    }
    return y;
  }

  Day getNextDay(Day day) {
    if (day.day == getYear(day.year).getMonth(day.month).length) {
      if (getYear(day.year).getMonth(day.month).month == 12) {
        return getYear(day.year + 1).getMonth(1).getDay(1);
      }
      return getYear(day.year).getMonth(day.month + 1).getDay(1);
    }
    return getYear(day.year).getMonth(day.month).getDay(day.day + 1);
  }
};

int main() {
  char input[] =
      "{F:{<:{C:1,J:1,M:4}},G:{1:{2:1,5:5,9:1,<:3,@:1,C:6,G:1,J:7,N:1},2:{6:"
      "1,9:3,=:1,@:5,D:1,G:4,K:1}}}";
  uint16_t input_length = 97;
  Calendar cal = Calendar(input, input_length);

  Day today = cal.getYear(22).getMonth(12).getDay(28);
  Day tommorrow = cal.getNextDay(today);
  cout << today.code << "\n";
  cout << tommorrow.code << "\n";
}