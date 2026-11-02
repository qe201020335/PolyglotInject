"""
Update the localization file from a dump of a newer version of the game.

input files:
beat-saber.csv (required) Base game localization file, dumped by SiraLocalizer or the PC version of PolyglotInject
sira-locale.csv (optional) SiraLocalization file, from Crowdin or downloaded by SiraLocalizer
polyglot-inject.csv (optional) localization file, from previous version of PolyglotInject (assets/polyglot-inject.csv)

output files:
polyglot-inject-new.csv - Updated localization file, to be used by PolyglotInject

Procedure:
- Every row starts as a copy of the game's row, so keys removed from the game are gone
  and languages the game supports keep the game's text
- Columns of other languages are copied from sira-locale.csv if its English matches the game's
- The game's own Simplified Chinese is from NetEase's version and must never be used
- Simplified Chinese is only reused if the English it was translated from matches the game's, looking in this order:
    1. sira-locale.csv, same key
    2. polyglot-inject.csv, same key
    3. sira-locale.csv, any key with the same English
    4. polyglot-inject.csv, any key with the same English
  Steps 3 and 4 are skipped if the matches have different translations
- Whatever is left is prompted for at the end
"""

import csv
import sys

# column index
KEY = 0
FIRST_LANGUAGE = 2
EN = 2
FR = 3
ES = 4
DE = 5
JA = 18
CNS = 19
KO = 21

# languages the game supports, these keep the game's text
SUPPORTED_LANGUAGES = {EN, FR, ES, DE, JA, KO}

# file names
CSV_NAME_BS = "beat-saber.csv"
CSV_NAME_PI = "polyglot-inject.csv"
CSV_NAME_SIRA = "sira-locale.csv"
CSV_NAME_PI_NEW = "polyglot-inject-new.csv"

# rows of the game's file that aren't localization keys
IGNORED_KEYS = {'', 'STRING ID', 'LANGUAGE_CODE', 'LANGUAGE_DIRECTION'}
# losing more keys than this from the previous file means something is wrong, likely an incomplete dump
MAX_REMOVED_KEYS = 20


def load_localization(file, throw=True):
    try:
        # Don't pass newline='' here. sira-locale.csv has stray \r in multi-line text and this removes them.
        with open(file, 'r', encoding='utf-8') as csv_file:
            csv_reader = csv.reader(csv_file)
            return dict((row[KEY], row) for row in csv_reader if row)
    except FileNotFoundError:
        if throw:
            raise FileNotFoundError(f"File {file} not found")
        return {}


def has_text(row, column):
    return column < len(row) and row[column].strip() != ''


def is_translation_of(row, english):
    return row[EN].strip() == english and has_text(row, CNS)


def find_by_key(table, key, english):
    row = table.get(key)
    return row if row is not None and is_translation_of(row, english) else None


def find_by_english(table, english):
    rows = [row for row in table.values() if is_translation_of(row, english)]
    # the same English can need different translations depending on where it is used
    return rows[0] if len({row[CNS].strip() for row in rows}) == 1 else None


def find_translation(sources, key, english):
    for name, table in sources:
        row = find_by_key(table, key, english)
        if row is not None:
            return name, row
    for name, table in sources:
        row = find_by_english(table, english)
        if row is not None:
            return name, row
    return None, None


def find_outdated_translation(sources, key):
    for _, table in sources:
        row = table.get(key)
        if row is not None and has_text(row, CNS):
            return row
    return None


bs_og = load_localization(CSV_NAME_BS)
sira = load_localization(CSV_NAME_SIRA, throw=False)
pi_old = load_localization(CSV_NAME_PI, throw=False)

width = max(len(row) for table in (bs_og, sira) for row in table.values())
sources = (("SiraLocalizer", sira), ("old PolyglotInject", pi_old))

# copy the game, then take the languages the game doesn't support from sira, except for Simplified Chinese
output = {}
for key, game_row in bs_og.items():
    if key in IGNORED_KEYS:
        continue

    row = game_row + [''] * (width - len(game_row))
    sira_row = sira.get(key, [])
    if sira_row and sira_row[EN].strip() != row[EN].strip():
        sira_row = []
    for column in range(FIRST_LANGUAGE, width):
        if column not in SUPPORTED_LANGUAGES:
            row[column] = sira_row[column] if column < len(sira_row) else ''
    row[CNS] = ''
    output[key] = row

# keys that were deleted from the game
deleted_keys = sorted(sira.keys() - output.keys() - IGNORED_KEYS)

print("WHAT IS DELETED FROM THE GAME: ")
for deleted_key in deleted_keys:
    print(deleted_key, sira[deleted_key][EN], sep=": ")
print()

removed_keys = sorted(pi_old.keys() - output.keys())
print("WHAT IS REMOVED FROM THE PREVIOUS POLYGLOT_INJECT: ")
for key in removed_keys:
    print(key, pi_old[key][EN], sep=": ")
print()

if len(removed_keys) > MAX_REMOVED_KEYS:
    print(f"{len(removed_keys)} keys would be removed. Is beat-saber.csv a complete dump? Enter 'yes' to continue")
    if input() != 'yes':
        sys.exit(1)

# reuse what we can, and leave the rest to the prompts
true_missing = []
for key, row in output.items():
    english = row[EN].strip()
    if english == '':
        continue

    source_name, source = find_translation(sources, key, english)
    if source is not None:
        row[CNS] = source[CNS]
        if source is not sira.get(key):
            print(f"Reused the translation of {source[KEY]} from {source_name} for {key}: ", row[EN], row[CNS], sep=": ")
    elif english == key:
        print(f"Skipped placeholder {key}")
    else:
        true_missing.append(key)
print()

print(f"{len(true_missing)} keys need a translation")
for index, missing_key in enumerate(true_missing, start=1):
    outdated = find_outdated_translation(sources, missing_key)
    print(f"[{index}/{len(true_missing)}] Key '{missing_key}' not found in localization files.",
          f"EN: '{output[missing_key][EN]}'", sep="\n")
    if outdated is None:
        print("Translation for CN? '__skip__' or leave empty to skip")
    else:
        print(f"The English has changed from: '{outdated[EN]}'",
              f"Its translation: '{outdated[CNS]}'",
              "Translation for CN? Leave empty to keep it, or '__skip__' to skip", sep="\n")

    translated = input().strip()
    if translated == '' and outdated is not None:
        translated = outdated[CNS]
    if translated in ('', '__skip__'):
        print("skipped")
    else:
        output[missing_key][CNS] = translated
    print("")

print("//////////////////////")
print("New Entries Added")
for missing_key in true_missing:
    print(missing_key, output[missing_key][EN], output[missing_key][CNS], sep=": ")
print("//////////////////////")

# output new polyglot csv
with open(CSV_NAME_PI_NEW, 'w', encoding='utf-8', newline='') as new_csv:
    csv_writer = csv.writer(new_csv, quotechar='\"', quoting=csv.QUOTE_MINIMAL)
    csv_writer.writerows(output.values())
