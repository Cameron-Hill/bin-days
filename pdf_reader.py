from PyPDF2 import PdfReader


reader = PdfReader("KA91JE_Calendar_110.pdf")
number_of_pages = len(reader.pages)
page = reader.pages[0]
text = page.extract_text()

print(text)


FOOD_CADY = 'F-'
PURPLE_LIDDED_BIN = 'P-'
GREEN_AND_GREY_BINS = 'GGr-'
BROWN_BIN = 'Br-'
BLUE_BIN = 'B-'
GREY_BIN = 'Gr-'
GREEN_AND_BLUE_BINS = 'GB-'
FESTIVE_PERIOD = ''  #???  Must be a pdf rect?  


# Parse Month header column. -> store as key:[] in dict
# Start Parsing Numerical. We know the 1st of the month. We know what days are sundays.
# After are first sunday, increment the month and append to the list under that key. (first date should be 1st)
# - list: months = month header
# - for date in num days if day = sunday. month .append day, month ++
