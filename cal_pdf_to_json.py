import re
import os
import json
import calendar
from PyPDF2 import PdfReader
from argparse import ArgumentParser



FESTIVE_PERIOD = ''  # ???  Must be a pdf rect?

MONTHS = {calendar.month_name[i].lower(): i for i in range(1, 13)}


# Parse Month header column. -> store as key:[] in dict
# Start Parsing Numerical. We know the 1st of the month. We know what days are sundays.
# After are first sunday, increment the month and append to the list under that key. (first date should be 1st)
# - list: months = month header
# - for date in num days if day = sunday. month .append day, month ++


class Month():
    def __init__(self, name: str, year: int) -> None:
        self.year = year
        self.name = name.lower()
        self.no = MONTHS[self.name]
        self.calendar = calendar.monthcalendar(self.year, self.no)
        self.bins = {}

    def __int__(self):
        return self.no

    def is_sunday(self, day: int) -> bool:
        return calendar.weekday(self.year, self.no, day) == calendar.SUNDAY

    def add_bin(self, code: str, day: int):
        self.bins[day] = code


class InvalidDateString(Exception):
    """"""


class BinCalParser:
    def __init__(self, months: list[Month]) -> None:
        self.months = months
        self.month = 0
        self.date = 1

    def _validate_code(self, code: str, day: int, month: Month, lookup: str):
        if len(code) > 6:  # Maximum Date length: GGr-31 (6)
            raise InvalidDateString(
                f'Parsed invalid date string while looking for {month.name} {day}  ({lookup})'
            )
        if not code:
            raise InvalidDateString(
                f'Failed to find day: {day} for Month: {month.name} in {lookup}'
            )

    def feed(self, string: str, week):
        string = string.replace(' ', '')
        for month in self.months:
            if len(month.calendar) > week:
                for day in month.calendar[week]:
                    # If day is 0 it is not on the calendar (First Monday of the month for example)
                    if day:
                        i = string.find(str(day)) + len(str(day))
                        code = string[:i]
                        self._validate_code(code, day, month, string[:7])
                        string = string[i:]
                        code = code.replace(str(day), "")
                        if code:
                            month.add_bin(code, day)


def normalize(text):
    # Lowercase, Only lines with numbers, strip whitespace
    text = text.lower()
    return [x.strip() for x in text.split('\n') if re.search(r'\d', x)]


def is_date_line(line):
    month = False
    year = False
    for word in line.split(' '):
        word = word.strip()
        if word in MONTHS:
            month = True
        if re.match('\d{4}', word):
            year = True
        if year and month:
            return True
    return False


def parse_dates(line: str) -> list[Month]:
    # We're assuming dates appear year first. e.g. "2021 december 2022 january"
    dates = []
    year = None
    for word in line.split(' '):
        word = word.strip()
        if re.match(r'\d{4}', word):
            year = int(word)
        if year and word in MONTHS:
            dates.append(Month(word, year))
            year = None
    return dates


def parse_bins(pdf_file):
    reader = PdfReader(pdf_file)
    page = reader.pages[0]
    text = page.extract_text()
    lines = normalize(text)
    all_months = []
    months = None
    parser = None
    week = 0
    for line in lines:
        if is_date_line(line):
            months = parse_dates(line)
            all_months.extend(months)
            parser = BinCalParser(months)
            week = 0
        elif parser:
            parser.feed(line, week)
            week += 1
    d = {}
    for month in all_months:
        d.setdefault(month.year, {})
        d[month.year][month.no] = month.bins
    return d


def parse_args():
    parser = ArgumentParser()
    parser.add_argument('pdf_file')
    args = parser.parse_args()
    assert os.path.isfile(args.pdf_file), f'Unknown File: {args.pdf_file}'
    return args.__dict__


if __name__ == '__main__':
    args = parse_args()
    _d = parse_bins(args["pdf_file"])
    with open(os.path.splitext(args['pdf_file'])[0]+'.json', 'w') as f:
        json.dump(_d, f)
