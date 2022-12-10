from argparse import ArgumentParser
from requests_html import HTMLSession
from bs4 import BeautifulSoup
import re
import pickle

BIN_DAYS_URL = 'https://www.south-ayrshire.gov.uk/bin-days'
BIN_DAYS_POSTCODE_VAR = 'BINDAYS_PAGE1_POSTCODE'
session = HTMLSession()


def fetch_postcode_form():
    BIN_DAYS_FORM_ID = 'BINDAYS_FORM'
    # response = session.get(BIN_DAYS_URL)
    with open('./fetch_postcode_form_response.pk', 'rb') as f:
        response = pickle.load(f)
    soup = BeautifulSoup(response.text, "html.parser")
    form = soup.find("form", attrs={"id": BIN_DAYS_FORM_ID, "method": "post"})
    return form


def get_postcode_form_input_details(postcode_form, postcode):
    inputs = []
    for input in postcode_form.find_all("input"):
        d = {}
        for key, value in input.attrs.items():
            d[key] = value
            if key in insert:
                d[key] = insert[key]
        inputs.append(d)


def fetch_address_form(postcode_form, postcode):
    input_details = get_postcode_form_input_details(postcode_form, postcode)


def parse_args():
    parser = ArgumentParser()
    parser.add_argument("postcode")
    args = parser.parse_args()
    return args.__dict__


def validate_postcode(postcode):
    assert re.match(string=postcode,
                    pattern=r'^[A-Z]{1,2}[0-9][A-Z0-9]? ?[0-9][A-Z]{2}$'), f'{postcode} is not a valid postcode.'


def get_current_pdf_url(postcode):
    post_code_form = fetch_postcode_form()
    address_form = fetch_address_form(post_code_form, postcode)


def main(postcode):
    postcode = postcode.replace(" ", "").upper()
    validate_postcode(postcode)
    pdf_url = get_current_pdf_url(postcode)


if __name__ == '__main__':
    args = parse_args()
    main(**args)
