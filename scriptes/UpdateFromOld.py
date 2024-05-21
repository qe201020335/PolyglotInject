"""
Update Old SiraLocalizer localization file by adding new keys from dump of newer version game.

input files:
Localization_CNMOD.csv (Optional) Old Quest CN Localization (where EN has CN strings)
sira-locale.csv (required) SiraLocalization file, from Crowdin or a previous version of PolyglotInject (the sira-new.csv in the assets folder)
beat-saber.csv (required) Base game localization file, dumped by SiraLocalizer
"""

import csv

# column index
KEY = 0
EN = 2
CNS = 19

try:
    # load CN translation from previous version
    with open("Localization_CNMOD.csv", 'r', encoding='utf-8') as csv1:
        csv_reader1 = csv.reader(csv1)
        quest_CN = dict((row[KEY], row[EN]) for row in csv_reader1)
        # print(quest_CN)
except FileNotFoundError:
    quest_CN = {}

with open("sira-locale.csv", 'r', encoding='utf-8') as csv2:
    csv_reader2 = csv.reader(csv2)
    sira = dict((row[KEY], row) for row in csv_reader2)
    # # print(ids2)

with open("beat-saber.csv", 'r', encoding='utf-8') as csv3:
    csv_reader3 = csv.reader(csv3)
    bs_og = dict((row[KEY], row) for row in csv_reader3)

# find out what is missing from the old sira localizer
intersect = bs_og.keys() & sira.keys()
missing_keys = bs_og.keys() - intersect
print("\n\n")
print("WHAT IS MISSING FROM THE OLD SiraLocal FILE: ")
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
    if missing_key in quest_CN:
        # found it in quest
        row[CNS] = quest_CN[missing_key]
        print("Found a missing key from old PolyglotInject: ")
        print(row[EN], quest_CN[missing_key], sep=": ")
    else:
        # not found, manual enter new translation
        true_missing[missing_key] = bs_og[missing_key][EN]
        print(f"Key '{missing_key}' not found in quest.",
              f"EN: '{bs_og[missing_key][EN]}'",
              "Translation for CN? '__skip__' or leave empty to skip", sep="\n")
        trans = input()
        if trans != '__skip__' and trans != '':
            row[CNS] = trans
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
with open("sira-new.csv", 'w', encoding='utf-8', newline='') as new_csv:
    csv_writer = csv.writer(new_csv, quotechar='\"', quoting=csv.QUOTE_MINIMAL)
    csv_writer.writerows(sira.values())
