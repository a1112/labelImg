import os
import unittest
from unittest.mock import patch

from libs.stringBundle import StringBundle, normalize_language

class TestStringBundle(unittest.TestCase):

    def test_loadDefaultBundle_withoutError(self):
        str_bundle = StringBundle.get_bundle('en')
        self.assertEqual(str_bundle.get_string("openDir"), 'Open Dir', 'Fail to load the default bundle')

    def test_fallback_withoutError(self):
        str_bundle = StringBundle.get_bundle('zh-TW')
        self.assertEqual(str_bundle.get_string("openDir"), u'\u958B\u555F\u76EE\u9304', 'Fail to load the zh-TW bundle')

    def test_setInvaleLocaleToEnv_printErrorMsg(self):
        with patch.dict(os.environ, {'LC_ALL': 'UTF-8', 'LANG': 'UTF-8'}):
            str_bundle = StringBundle.get_bundle()
        self.assertEqual(str_bundle.get_string("openDir"), 'Open Dir', 'Fail to load the default bundle')

    def test_normalize_language_matches_supported_locale(self):
        self.assertEqual(normalize_language('zh_CN'), 'zh-CN')
        self.assertEqual(normalize_language('zh_TW.UTF-8'), 'zh-TW')
        self.assertEqual(normalize_language('ja'), 'ja-JP')
        self.assertEqual(normalize_language('UTF-8'), 'en')


if __name__ == '__main__':
    unittest.main()
