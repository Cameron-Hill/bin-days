NO_BIN = ''
ESCAPE_CHAR = '-'
GREY_BIN = 'gr'
BROWN_BIN = 'br'
FOOD_CADY = 'f'
BLUE_BIN = 'b'
GREEN_BIN = 'g'
PURPLE_BIN = 'p'


# It's important that the 2 character bins come first.
# Beacuse this is the order we will parse the code in.
# e.g. 'ggr' - 'gr' = 'g',  'g' - 'g' = ''
BINS = [
    GREY_BIN,
    BROWN_BIN,
    FOOD_CADY,
    BLUE_BIN,
    GREEN_BIN,
    PURPLE_BIN,
]

# If we ever have more than 7 bins this will fail because we're giving 1 bit to each bin
# plus 1 bit for adding 48 to avoid empty characters
# Also, that's way too many bins
assert len(BINS) < 8, 'Too many bins!'

ENCODED = {key: 2**i for i, key in enumerate(BINS)}
REVERSE_ENCODED = {v: k for k, v in ENCODED.items()}


for k, v in ENCODED.items():
    print(f'{k}:: {v}')
