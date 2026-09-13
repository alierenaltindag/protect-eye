#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import json
import os
import sys

from i18n_data.group1 import LANGUAGES_G1
from i18n_data.group2 import LANGUAGES_G2
from i18n_data.group3 import LANGUAGES_G3

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
I18N_DIR = os.path.join(ROOT_DIR, "resources", "i18n")
EN_PATH = os.path.join(I18N_DIR, "en.json")

with open(EN_PATH, "r", encoding="utf-8") as f:
    en_ref = json.load(f)

expected_keys = set(en_ref["strings"].keys())
expected_short_tips_count = len(en_ref["short_tips"])
expected_long_tips_count = len(en_ref["long_tips"])
expected_short_ex_count = len(en_ref["short_exercises"])
expected_long_ex_count = len(en_ref["long_exercises"])

all_langs = {}
all_langs.update(LANGUAGES_G1)
all_langs.update(LANGUAGES_G2)
all_langs.update(LANGUAGES_G3)

print(f"Loaded {len(all_langs)} languages to build: {list(all_langs.keys())}")

for code, data in all_langs.items():
    assert data["code"] == code, f"Code mismatch in {code}: {data['code']}"
    assert "name" in data and len(data["name"]) > 0, f"Missing name in {code}"
    
    # Check strings
    str_keys = set(data["strings"].keys())
    missing = expected_keys - str_keys
    extra = str_keys - expected_keys
    if missing:
        raise ValueError(f"Language {code} is missing strings: {missing}")
    if extra:
        raise ValueError(f"Language {code} has extra strings: {extra}")
        
    # Check tips
    if len(data["short_tips"]) != expected_short_tips_count:
        raise ValueError(f"Language {code} short_tips count mismatch: {len(data['short_tips'])} != {expected_short_tips_count}")
    if len(data["long_tips"]) != expected_long_tips_count:
        raise ValueError(f"Language {code} long_tips count mismatch: {len(data['long_tips'])} != {expected_long_tips_count}")
        
    # Check exercises
    if len(data["short_exercises"]) != expected_short_ex_count:
        raise ValueError(f"Language {code} short_exercises count mismatch: {len(data['short_exercises'])} != {expected_short_ex_count}")
    if len(data["long_exercises"]) != expected_long_ex_count:
        raise ValueError(f"Language {code} long_exercises count mismatch: {len(data['long_exercises'])} != {expected_long_ex_count}")
        
    for i, ex in enumerate(data["short_exercises"]):
        ref_ex = en_ref["short_exercises"][i]
        assert ex["visual_type"] == ref_ex["visual_type"], f"Exercise {i} visual_type mismatch in {code}: {ex['visual_type']} != {ref_ex['visual_type']}"
        assert ex["badge"], f"Exercise {i} empty badge in {code}"
        assert ex["title"], f"Exercise {i} empty title in {code}"
        assert ex["instruction"], f"Exercise {i} empty instruction in {code}"

    for i, ex in enumerate(data["long_exercises"]):
        ref_ex = en_ref["long_exercises"][i]
        assert ex["visual_type"] == ref_ex["visual_type"], f"Long exercise {i} visual_type mismatch in {code}: {ex['visual_type']} != {ref_ex['visual_type']}"
        assert ex["badge"], f"Long exercise {i} empty badge in {code}"
        assert ex["title"], f"Long exercise {i} empty title in {code}"
        assert ex["instruction"], f"Long exercise {i} empty instruction in {code}"

    out_file = os.path.join(I18N_DIR, f"{code}.json")
    with open(out_file, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)
    print(f"Successfully generated {out_file} ({data['name']})")

print(f"All {len(ALL_LANGS)} languages successfully validated and generated!")
