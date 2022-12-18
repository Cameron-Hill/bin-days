uint16_t ndays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

class Day
{
public:
  uint16_t day;
  uint16_t month;
  uint16_t year;
  uint16_t code;
  Day(){};
  Day(uint16_t theDay, uint16_t theMonth, uint16_t theYear, uint16_t code = 0)
  {
    day = theDay;
    month = theMonth;
    year = theYear;
  }
};

class Month
{
private:
  Day *days;

public:
  uint16_t month;
  uint16_t year;
  uint16_t length;

  Month() { days = new Day[32]; };
  Month(uint16_t theMonth, uint16_t theYear) : month(theMonth), year(theYear)
  {
    days = new Day[32];
    length = ndays[theMonth];
    if (month == 2 && isLeapYear())
    {
      length++;
    }
    for (uint16_t i = 0; i < length; i++)
    {
      days[i] = Day(i + 1, theMonth, theYear);
    }
  };

  bool isLeapYear()

  {
    if ((year + 2000) % 4 == 0)
    {
      if ((year + 2000) % 100 == 0)
      {
        if ((year + 2000) % 400 == 0)
        {
          return true;
        }
        else
        {
          return false;
        }
      }
      return true;
    }
    return false;
  }

  Day getDay(uint16_t day)
  {
    return days[day - 1];
  }
};

class Year
{
private:
  Month *months;

public:
  uint16_t year;

  Year() { months = new Month[12]; };
  Year(uint16_t theYear) : year(theYear)
  {
    months = new Month[12];
    for (uint16_t i = 0; i < 12; i++)
    {
      months[i] = Month(i + 1, year);
    }
  };

  Month getMonth(uint16_t month)
  {
    return months[month - 1];
  }
};

class Calendar
{
private:
public:
  Calendar(){};

  Year getYear(uint16_t year)
  {
    return Year(year);
  }

  Day getNextDay(Day day)
  {
    if (day.day == getYear(day.year).getMonth(day.month).length)
    {
      if (getYear(day.year).getMonth(day.month).month == 12)
      {
        return getYear(day.year + 1).getMonth(1).getDay(1);
      }
      return getYear(day.year).getMonth(day.month + 1).getDay(1);
    }
    return getYear(day.year).getMonth(day.month).getDay(day.day + 1);
  }
};