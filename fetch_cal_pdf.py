import os
import sys
import base64
import json
import urllib3
from argparse import ArgumentParser
from requests_html import HTMLSession
from bs4 import BeautifulSoup
import re
import pickle

urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

sys.setrecursionlimit(10000)  # Pickled Soup
session = HTMLSession()

USE_CACHE = False
BIN_DAYS_URL = 'https://www.south-ayrshire.gov.uk/bin-days'
BIN_DAYS_POSTCODE_VAR = 'BINDAYS_PAGE1_POSTCODE'
BIN_DAYS_PRE_RENDER_OUTER_FIELD = 'BINDAYS_PAGE3_FIELD15_OUTER'
BIN_DAYS_FIELD_ID = 'BINDAYS_PAGE3_FIELD15'
BIN_DAYS_FORM_ID = 'BINDAYS_FORM'
BIN_DAYS_FORM_DATA = 'BINDAYSFormData'
BIN_DAYS_SESSION_VARIABLES = 'BINDAYSSerializedVariables'
BIN_DAYS_PAGE_ID = 'PAGE3_1'
FORM_FIELD = 'FIELD15'


def cached(func):
    def wrapper(*args, **kwargs):
        func_id = func.__name__
        os.makedirs('.cache', exist_ok=True)
        cache_file = os.path.join('.cache', f'{func_id}.pk')
        if USE_CACHE and os.path.isfile(cache_file):
            with open(cache_file, 'rb') as f:
                print(f'Loading {cache_file}')
                response = pickle.load(f)
        else:
            response = func(*args, **kwargs)
            with open(cache_file, 'wb') as f:
                pickle.dump(response, f)
        return response
    return wrapper


@cached
def _fetch_postcode_form(url):
    return session.get(url)


def fetch_postcode_form() -> BeautifulSoup:
    print("Fetching Postcode form...")
    response = _fetch_postcode_form(BIN_DAYS_URL)
    response.raise_for_status()
    soup = BeautifulSoup(response.text, "html.parser")
    form = soup.find("form", attrs={"id": BIN_DAYS_FORM_ID, "method": "post"})
    return form


def get_payload_from_form(form: BeautifulSoup) -> dict:
    payload = {}
    search_term = ["input", "button"]
    for term in search_term:
        for input in form.find_all(term):
            if input.get("value"):
                payload[input["name"]] = input["value"]
    return payload


@cached
def _fetch_address_form(url, payload):
    return session.post(url, data=payload, allow_redirects=True)


@cached
def fetch_address_form(postcode_form: BeautifulSoup, postcode: str) -> BeautifulSoup:
    print(f"Fetching address form from postcode form : {postcode}")
    payload = get_payload_from_form(postcode_form)
    payload[BIN_DAYS_POSTCODE_VAR] = postcode
    url = postcode_form.get("action")
    response = _fetch_address_form(url, payload)
    response.raise_for_status()
    soup = BeautifulSoup(response.text, "html.parser")
    form = soup.find("form", attrs={"id": BIN_DAYS_FORM_ID, "method": "post"})
    return form


def get_address_payload_from_form(address_form: BeautifulSoup, address_identifier: str | None = None) -> dict:
    select = address_form.find("select")
    name = select["name"]
    options = select.find_all("option")
    if address_identifier:
        raise NotImplementedError(
            "Address specific payload not implmented yet")
    else:
        value = options[-1]["value"]
        print(
            f'Using default address: {options[-1].contents[0]}: {options[-1]["value"]}')
    return {name: value}


@cached
def _fetch_bin_days_form(url, payload):
    return session.post(url, data=payload)


def fetch_bin_days_page(address_form: BeautifulSoup, address_identifier: str | None = None) -> BeautifulSoup:
    payload = get_payload_from_form(address_form)
    address_payload = get_address_payload_from_form(
        address_form, address_identifier)
    payload.pop("BINDAYS_FORMACTION_BACK")
    payload.pop("BINDAYS_PAGE2_FIELD21")
    payload.update(address_payload)
    response = _fetch_bin_days_form(address_form["action"], payload)
    response.raise_for_status()
    soup = BeautifulSoup(response.text, "html.parser")
    return soup


def get_url_from_bin_days_page(bin_days_page: BeautifulSoup, form_data: dict):
    # The url we need is embeded in some javascript that get's rendered on load.
    # I don't know how to pass the session variables to the renderer so I'm just gonna parse the js
    bin_days_form = bin_days_page.find(
        "form", attrs={"id": BIN_DAYS_FORM_ID, "method": "post"})
    outer = bin_days_form.find(
        "li", attrs={"id": BIN_DAYS_PRE_RENDER_OUTER_FIELD}
    )
    anchor = None
    for script in outer.find_all("script"):
        for match in re.findall(r'<a\s*href=[^>]*', script.contents[0]):
            if 'gis.' in match and 'calendar' in match:
                anchor = match
                break
    cal_version = form_data[BIN_DAYS_PAGE_ID][FORM_FIELD]['calendar']
    a_split = anchor.split('+')
    a_split[1] = cal_version
    a_split = [x.strip() for x in a_split]
    a_split[0] = a_split[0].rsplit('"', 1)[0].split('"', 1)[1]
    a_split[2] = a_split[2].split('"', 1)[-1].split('"', 1)[0]
    return "".join(a_split).replace('\\', '')


def get_form_data_from_bin_days_page(bin_days_page: BeautifulSoup) -> dict:
    search_term = re.compile(fr'(var|let|const){BIN_DAYS_FORM_DATA}="[^"]*')
    data = None
    for script in bin_days_page.find_all("script"):
        if script.contents:
            match = re.search(search_term, script.contents[0].replace(" ", ""))
            if match:
                data = match.group().split('"', 1)[-1].encode(encoding="utf-8")
    if not data:
        raise Exception(f'Failed to find {BIN_DAYS_FORM_DATA}')
    decoded = base64.standard_b64decode(data)
    return json.loads(decoded)


def parse_args() -> dict:
    parser = ArgumentParser()
    parser.add_argument("postcode")
    args = parser.parse_args()
    return args.__dict__


def validate_postcode(postcode: str) -> None:
    assert re.match(string=postcode,
                    pattern=r'^[A-Z]{1,2}[0-9][A-Z0-9]? ?[0-9][A-Z]{2}$'), f'{postcode} is not a valid postcode.'


def get_current_pdf_url(postcode: str) -> str:
    post_code_form = fetch_postcode_form()
    address_form = fetch_address_form(post_code_form, postcode)
    bin_days_page = fetch_bin_days_page(address_form)
    form_data = get_form_data_from_bin_days_page(bin_days_page)
    pdf_url = get_url_from_bin_days_page(bin_days_page, form_data)
    return pdf_url


def download_pdf_to_file(pdf_url, output_file) -> None:
    response = session.get(pdf_url, verify=False)
    response.raise_for_status()
    print(f'Saving Bin Calendar to: {output_file}')
    with open(output_file, 'wb') as f:
        f.write(response.content)


def main(postcode: str) -> None:
    postcode = postcode.replace(" ", "").upper()
    validate_postcode(postcode)
    pdf_url = get_current_pdf_url(postcode)
    output_file = f"{postcode}_{os.path.basename(pdf_url).split('?')[0]}"
    download_pdf_to_file(pdf_url, output_file)


if __name__ == '__main__':
    args = parse_args()
    main(**args)
