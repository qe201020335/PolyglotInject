"""
Update old localization file by adding new keys from dump of newer version game.

input files:
beat-saber.csv (required) Base game localization file, dumped by SiraLocalizer
polyglot-inject.csv (optional) localization file, from previous version of PolyglotInject (assets/polyglot-inject.csv)
sira-locale.csv (optional) SiraLocalization file, from Crowdin or downloaded by SiraLocalizer

output files:
polyglot-inject-new.csv - Updated localization file, to be used by PolyglotInject

Procedure:
- sira-locale.csv will be used as base
- keys deleted by beat games (not found in beat-saber.csv) will be removed
- for new keys added by beat games, if found in polyglot-inject.csv, use the translation from there
- If not, prompt for manual input
- If manual input is empty, skip
"""

import csv

# column index
KEY = 0
EN = 2
CNS = 19

# file names
CSV_NAME_BS = "beat-saber.csv"
CSV_NAME_PI = "polyglot-inject.csv"
CSV_NAME_SIRA = "sira-locale.csv"
CSV_NAME_PI_NEW = "polyglot-inject-new.csv"


def load_localization(file, throw=True):
    try:
        with open(file, 'r', encoding='utf-8') as csv_file:
            csv_reader = csv.reader(csv_file)
            return dict((row[KEY], row) for row in csv_reader)
    except FileNotFoundError:
        if throw:
            raise FileNotFoundError(f"File {file} not found")
        return {}


bs_og = load_localization(CSV_NAME_BS)
sira = load_localization(CSV_NAME_SIRA, throw=False)
pi_old = load_localization(CSV_NAME_PI, throw=False)

# find out what is missing from sira localizer
missing_keys = bs_og.keys() - sira.keys()
# keys that were deleted from the game
deleted_keys = sira.keys() - bs_og.keys()

print("\n\n")
print("WHAT IS DELETED FROM THE GAME: ")
print("")
for deleted_key in deleted_keys:
    print(deleted_key, sira[deleted_key][EN], sep=": ")
    sira.pop(deleted_key)
print()

print("\n\n")
print("WHAT IS MISSING FROM SIRA_LOCALIZER: ")
print("")
for missing_key in missing_keys:
    print(missing_key, bs_og[missing_key][EN], sep=": ")
print()

true_missing = {}
# adds the missing ones to sira with the CN translation from quest
for missing_key in missing_keys:
    if missing_key == '':
        continue

    row = bs_og[missing_key]
    if missing_key in pi_old:
        # found it in old PolyglotInject
        row[CNS] = pi_old[missing_key][CNS]
        print("Found a missing key from old PolyglotInject: ")
        print(row[EN], row[CNS], sep=": ")
    else:
        # not found, manual enter new translation
        true_missing[missing_key] = bs_og[missing_key][EN]
        print(f"Key '{missing_key}' not found in localization files.",
              f"EN: '{bs_og[missing_key][EN]}'",
              "Translation for CN? '__skip__' or leave empty to skip", sep="\n")
        translated = input()
        if translated != '':
            row[CNS] = translated
        else:
            print("skipped")
    print("")
    sira[missing_key] = row

print("//////////////////////")
print("New Entries Added")
for missing_key in true_missing:
    print(missing_key, sira[missing_key][EN], sira[missing_key][CNS], sep=": ")
print("//////////////////////")

# output new polyglot csv
with open(CSV_NAME_PI_NEW, 'w', encoding='utf-8', newline='') as new_csv:
    csv_writer = csv.writer(new_csv, quotechar='\"', quoting=csv.QUOTE_MINIMAL)
    csv_writer.writerows(sira.values())
