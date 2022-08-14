"""
Generate localization file for PolyglotInject. Basically replace EN with CN translation
"""

import csv

# column index
KEY = 0
EN = 2
CNS = 19

with open("sira-new.csv", 'r', encoding='utf-8') as csv2:
    csv_reader2 = csv.reader(csv2)
    sira = dict((row[KEY], row) for row in csv_reader2)
    # # print(ids2)

with open("beat-saber.csv", 'r', encoding='utf-8') as csv3:
    csv_reader3 = csv.reader(csv3)
    bs_og = dict((row[KEY], row) for row in csv_reader3)

for key in sira:
    if key == '':
        continue
    if key in bs_og and sira[key][CNS] != '':
        bs_og[key][EN] = sira[key][CNS]
        bs_og[key][CNS] = sira[key][CNS]

# output new polyglot csv
with open("Localization_CNMOD_new.csv", 'w', encoding='utf-8', newline='') as new_csv:
    csv_writer = csv.writer(new_csv, quotechar='\"', quoting=csv.QUOTE_MINIMAL)
    csv_writer.writerows(bs_og.values())
