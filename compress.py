import os
import json
from datetime import datetime
from argparse import ArgumentParser

from BIN_TYPES import ENCODED
MAX_SIZE = 1024


def load_json(file):
    with open(file) as f:
        return json.load(f)


def write_encoded(text, file):
    out_file = os.path.join(os.path.dirname(
        file), '{}_encoded{}'.format(*os.path.splitext(file)))
    print(f'Writing to {out_file}')
    with open(out_file, 'w') as f:
        f.write(text)
    return out_file


def trim_historical(json_data):
    now = datetime.now()
    json_data = {k: v for k, v in json_data.items() if int(k) >= now.year}
    json_data[str(now.year)] = {k: v for k,
                                v in json_data[str(now.year)].items() if int(k) >= now.month}
    json_data[str(now.year)][str(now.month)] = {k: v for k, v in json_data[str(
        now.year)][str(now.month)].items() if int(k) >= now.day}
    return json_data


def ascii_char(n: int):
    return chr(n + 48)  # ascii 0 = 48


def encode_values(json_data):
    encoded = {}
    for year, month_d in json_data.items():
        year_key = ascii_char(int(year[2:]))
        encoded[year_key] = {}
        for month, days in month_d.items():
            encoded[year_key][ascii_char(int(month))] = {ascii_char(int(
                d)): ascii_char(ENCODED[v]) for d, v in days.items()}
    return encoded


def remove_whitespace(json_data):
    string = str(json_data)
    string = string.replace(' ', '').replace('\n', '').strip('{}')
    string = string.replace("'", "").replace('"', '')
    return string


def main(file):
    print(f'Compressing: {file}')
    original_size = os.path.getsize(file)
    print(f'Original Size: {original_size}')
    json = load_json(file)
    json = trim_historical(json)
    print(json)
    json = encode_values(json)
    text = remove_whitespace(json)
    out_file = write_encoded(text, file)
    print(f'Compressed Size: {os.path.getsize(out_file)}')


if __name__ == '__main__':
    _parser = ArgumentParser()
    _parser.add_argument('json_file')
    _json_file = _parser.parse_args().json_file
    assert os.path.isfile(_json_file), f'Cannot find file: {_json_file}'
    main(_json_file)