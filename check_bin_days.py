from argparse import ArgumentParser
from requests_html import HTMLSession
from bs4 import BeautifulSoup
import re
import pickle

BIN_DAYS_URL = 'https://www.south-ayrshire.gov.uk/bin-days'
BIN_DAYS_POSTCODE_VAR = 'BINDAYS_PAGE1_POSTCODE'
BIN_DAYS_FORM_ID = 'BINDAYS_FORM'
session = HTMLSession()


def fetch_postcode_form():
    print("Fetching Postcode form...")
    # response = session.get(BIN_DAYS_URL)
    # with open('./fetch_postcode_form_response.pk', 'wb') as f:
        # pickle.dump(response, f)
    with open('./fetch_postcode_form_response.pk', 'rb') as f:
        response = pickle.load(f)
    soup = BeautifulSoup(response.text, "html.parser")
    form = soup.find("form", attrs={"id": BIN_DAYS_FORM_ID, "method": "post"})
    return form


def get_payload_from_form(form):
    payload = {}
    search_term = ["input", "button"]
    for term in search_term:
        for input in form.find_all(term):
            if input.get("value"):
                payload[input["name"]] = input["value"]
    return payload


def fetch_address_form(postcode_form, postcode):
    print(f"Fetching address form from postcode form : {postcode}")
    payload = get_payload_from_form(postcode_form)
    payload[BIN_DAYS_POSTCODE_VAR] = postcode
    url = postcode_form.get("action")
    # response = session.post(url, data=payload, allow_redirects=True)
    # with open("./fetch_address_form_response.pk", 'wb') as f:
    #     pickle.dump(response, f)
    with open("./fetch_address_form_response.pk", 'rb') as f:
        response = pickle.load(f)

    soup = BeautifulSoup(response.text, "html.parser")
    form = soup.find("form", attrs={"id": BIN_DAYS_FORM_ID, "method": "post"})
    return form


def get_address_payload_from_form(address_form, address_identifier=None):
    select = address_form.find("select")
    name = select["name"]
    options = select.find_all("option")
    if address_identifier:
        raise NotImplementedError(
            "Address specific payload not implmented yet")
    else:
        value = options[-1]["value"]
        print(
            f'Using default address: {options[-1][""]}: {options[-1]["value"]}')
    return {name: value}


def fetch_bin_days_page(address_form, address_identifier=None):
    payload = get_payload_from_form(address_form)
    address_payload = get_address_payload_from_form(
        address_form, address_identifier)
    payload.update(address_payload)
    # response = session.post(address_form["action"], data=payload)
    # with open("./fetch_bin_days_response.pk", 'wb') as f:
        # pickle.dump(response, f)
    with open("./fetch_bin_days_response.pk", 'rb') as f:
        response = pickle.dump(f)
    a = 1


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
    bin_days_page = fetch_bin_days_page(address_form)


def main(postcode):
    postcode = postcode.replace(" ", "").upper()
    validate_postcode(postcode)
    pdf_url = get_current_pdf_url(postcode)


if __name__ == '__main__':
    args = parse_args()
    main(**args)
