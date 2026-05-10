#!/usr/bin/env python
# -*- coding: utf-8 -*-
import re
import os
import sys
import locale
from libs.ustr import ustr

from PySide6.QtCore import *
from PySide6.QtCore import QStringConverter


LANGUAGES = {
    'en': 'English',
    'zh-CN': '简体中文',
    'zh-TW': '繁體中文',
    'ja-JP': '日本語',
}


def normalize_language(locale_str):
    if not locale_str:
        return 'en'

    normalized = str(locale_str).split('.', 1)[0].split('@', 1)[0].replace('_', '-')
    normalized_lower = normalized.lower()
    for lang_code in LANGUAGES:
        if normalized_lower == lang_code.lower():
            return lang_code

    language = normalized_lower.split('-', 1)[0]
    if language == 'zh':
        return 'zh-TW' if normalized_lower in ('zh-tw', 'zh-hk', 'zh-mo', 'zh-hant') else 'zh-CN'
    if language == 'ja':
        return 'ja-JP'
    if language == 'en':
        return 'en'
    return 'en'


def get_system_language():
    try:
        default_locale = locale.getlocale()
        locale_str = default_locale[0] if default_locale and default_locale[0] else os.getenv('LANG')
    except:
        print('Invalid locale')
        locale_str = None
    return normalize_language(locale_str)


class StringBundle:

    __create_key = object()

    def __init__(self, create_key, locale_str):
        assert(create_key == StringBundle.__create_key), "StringBundle must be created using StringBundle.getBundle"
        self.id_to_message = {}
        paths = self.__create_lookup_fallback_list(locale_str)
        for path in paths:
            self.__load_bundle(path)

    @classmethod
    def get_bundle(cls, locale_str=None):
        if locale_str is None:
            locale_str = get_system_language()
        else:
            locale_str = normalize_language(locale_str)

        return StringBundle(cls.__create_key, locale_str)

    def get_string(self, string_id):
        assert(string_id in self.id_to_message), "Missing string id : " + string_id
        return self.id_to_message[string_id]

    def __create_lookup_fallback_list(self, locale_str):
        result_paths = []
        # Use filesystem path instead of Qt resources
        base_path = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'resources', 'strings', 'strings.properties')
        result_paths.append(base_path)
        if locale_str is not None:
            # Try the full locale first (e.g. zh-CN)
            dir_path = os.path.dirname(result_paths[-1])
            result_paths.append(os.path.join(dir_path, 'strings-' + locale_str + '.properties'))
            # Then try individual tags as fallback
            tags = re.split('[^a-zA-Z]', locale_str)
            for tag in tags:
                dir_path = os.path.dirname(os.path.dirname(result_paths[-1]))
                result_paths.append(os.path.join(dir_path, 'strings', 'strings-' + tag + '.properties'))

        return result_paths

    def __load_bundle(self, path):
        PROP_SEPERATOR = '='
        # Try filesystem first, fall back to Qt resources
        if os.path.exists(path):
            with open(path, 'r', encoding='utf-8') as f:
                for line in f:
                    line = line.strip()
                    if not line or line.startswith('#'):
                        continue
                    if PROP_SEPERATOR in line:
                        key_value = line.split(PROP_SEPERATOR, 1)
                        key = key_value[0].strip()
                        value = key_value[1].strip().strip('"')
                        self.id_to_message[key] = value
        else:
            # Fall back to Qt resources
            f = QFile(path)
            if f.exists():
                if f.open(QIODevice.ReadOnly | QFile.Text):
                    text = QTextStream(f)
                    text.setEncoding(QStringConverter.Utf8)

                while not text.atEnd():
                    line = ustr(text.readLine())
                    key_value = line.split(PROP_SEPERATOR)
                    key = key_value[0].strip()
                    value = PROP_SEPERATOR.join(key_value[1:]).strip().strip('"')
                    self.id_to_message[key] = value

                f.close()
