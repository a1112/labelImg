"""Execute the real save/navigation methods with fake UI and writers.

No QApplication, user settings, image files or Qt installation are needed.
"""
import ast
import errno
from html import escape
import os
from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest.mock import Mock

SOURCE = Path(__file__).resolve().parents[1] / 'labelImg.py'
METHODS = {'save_labels', 'save_file', 'save_file_as', '_save_file', 'may_continue',
           'close_file', 'closeEvent', 'file_item_double_clicked'}


class LabelFileError(Exception):
    pass


FORMATS = SimpleNamespace(PASCAL_VOC=1, YOLO=2, CREATE_ML=3)
BUTTONS = SimpleNamespace(Yes=1, No=2, Cancel=3)


def window(writer=None, label_format=1):
    tree = ast.parse(SOURCE.read_text(encoding='utf-8'), str(SOURCE))
    main = next(n for n in tree.body if isinstance(n, ast.ClassDef) and n.name == 'MainWindow')
    methods = [n for n in main.body if isinstance(n, ast.FunctionDef) and n.name in METHODS]
    assert len(methods) == len(METHODS)
    namespace = dict(os=os, sys=SimpleNamespace(platform='darwin'), errno=errno, escape=escape, ustr=str, LabelFile=Mock(),
                     LabelFileError=LabelFileError, LabelFileFormat=FORMATS,
                     XML_EXT='.xml', TXT_EXT='.txt', JSON_EXT='.json', QMessageBox=BUTTONS)
    exec(compile(ast.Module(body=methods, type_ignores=[]), str(SOURCE), 'exec'), namespace)
    Window = type('Window', (), {name: namespace[name] for name in METHODS})
    w = Window()
    w.label_file = writer or Mock()
    w.label_file_format = label_format
    w.canvas = SimpleNamespace(shapes=[], verified=False, setEnabled=Mock())
    w.file_path = '/synthetic/image.png'
    w.image_data = b'fake image'
    w.image = SimpleNamespace(isNull=lambda: False)
    w.line_color = w.fill_color = SimpleNamespace(getRgb=lambda: (0, 0, 0, 255))
    w.label_hist = []
    w.dirty = True
    w.default_save_dir = '/synthetic'
    w.error_message = Mock()
    w.statusBar = Mock()
    w.save_file_dialog = Mock(return_value='/selected/new-name')
    w.discard_changes_dialog = Mock(return_value=BUTTONS.Yes)
    w.reset_state = Mock()
    w.toggle_actions = Mock()
    w.actions = SimpleNamespace(saveAs=Mock())
    w.set_clean = Mock(side_effect=lambda: setattr(w, 'dirty', False))
    return w


class SaveRecoveryTests(unittest.TestCase):
    def test_each_writer_failure_keeps_dirty_and_blocks_navigation(self):
        cases = ((1, 'save_pascal_voc_format'), (2, 'save_yolo_format'), (3, 'save_create_ml_format'))
        for fmt, method in cases:
            for failure in (PermissionError(errno.EACCES, 'denied'),
                            PermissionError(errno.EPERM, 'revoked'),
                            FileNotFoundError(errno.ENOENT, 'missing'),
                            OSError(errno.ENOSPC, 'full'),
                            OSError(errno.EINVAL, 'invalid path'), LabelFileError('bad label')):
                with self.subTest(fmt=fmt, failure=failure):
                    w = window(label_format=fmt)
                    getattr(w.label_file, method).side_effect = failure
                    self.assertFalse(w.may_continue())
                    self.assertTrue(w.dirty)
                    w.set_clean.assert_not_called()
                    w.statusBar.assert_not_called()
                    w.error_message.assert_called_once()

    def test_denial_message_is_specific_and_html_escaped(self):
        w = window()
        w.label_file.save_pascal_voc_format.side_effect = PermissionError(errno.EACCES, '<folder>')
        self.assertFalse(w.save_file())
        message = w.error_message.call_args.args[1]
        self.assertIn('denied', message)
        self.assertIn('Save As', message)
        self.assertIn('&lt;folder&gt;', message)
        self.assertNotIn('Full Disk Access', message)

    def test_new_label_failure_restores_previous_save_state(self):
        w = window()
        w.label_file = None
        factory = w.save_labels.__func__.__globals__['LabelFile']
        factory.return_value.save_pascal_voc_format.side_effect = OSError(errno.ENOSPC, 'full')
        self.assertFalse(w.save_file())
        self.assertIsNone(w.label_file)
        self.assertTrue(w.dirty)

    def test_chooser_cancel_blocks_navigation_and_keeps_data(self):
        w = window()
        w.default_save_dir = None
        w.label_file = None
        w.save_file_dialog.return_value = ''
        self.assertFalse(w.may_continue())
        self.assertTrue(w.dirty)
        w.error_message.assert_not_called()

    def test_successful_save_allows_navigation(self):
        w = window()
        self.assertTrue(w.may_continue())
        self.assertFalse(w.dirty)
        w.set_clean.assert_called_once()
        w.statusBar().showMessage.assert_called_once()

    def test_explicit_discard_proceeds_without_saving(self):
        w = window()
        w.discard_changes_dialog.return_value = BUTTONS.No
        w.save_file = Mock()
        self.assertTrue(w.may_continue())
        w.save_file.assert_not_called()

    def test_cancel_stays_without_saving(self):
        w = window()
        w.discard_changes_dialog.return_value = BUTTONS.Cancel
        w.save_file = Mock()
        self.assertFalse(w.may_continue())
        w.save_file.assert_not_called()

    def test_clean_navigation_does_not_prompt(self):
        w = window()
        w.dirty = False
        self.assertTrue(w.may_continue())
        w.discard_changes_dialog.assert_not_called()

    def test_close_does_not_reset_after_denial(self):
        w = window()
        w.label_file.save_pascal_voc_format.side_effect = PermissionError(errno.EACCES, 'denied')
        w.close_file()
        w.reset_state.assert_not_called()
        self.assertTrue(w.dirty)

    def test_window_close_is_ignored_after_denial(self):
        w = window()
        event = SimpleNamespace(ignore=Mock())
        w.label_file.save_pascal_voc_format.side_effect = PermissionError(errno.EACCES, 'denied')
        w.closeEvent(event)
        event.ignore.assert_called_once()
        self.assertTrue(w.dirty)

    def test_double_click_does_not_navigate_after_denial(self):
        w = window()
        w.m_img_list = ['/synthetic/image.png', '/synthetic/next.png']
        w.cur_img_idx = 0
        w.load_file = Mock()
        w.label_file.save_pascal_voc_format.side_effect = PermissionError(errno.EACCES, 'denied')
        w.file_item_double_clicked(SimpleNamespace(text=lambda: '/synthetic/next.png'))
        self.assertEqual(w.cur_img_idx, 0)
        w.load_file.assert_not_called()

    def test_save_as_retry_succeeds_then_clears_dirty(self):
        w = window()
        w.label_file.save_pascal_voc_format.side_effect = [PermissionError(errno.EACCES, 'denied'), None]
        self.assertFalse(w.save_file())
        self.assertTrue(w.dirty)
        self.assertTrue(w.save_file_as())
        self.assertFalse(w.dirty)
        self.assertEqual(w.label_file.save_pascal_voc_format.call_args.args[0], '/selected/new-name.xml')

    def test_save_as_cancel_preserves_state(self):
        w = window()
        w.save_file_dialog.return_value = ''
        self.assertFalse(w.save_file_as())
        self.assertTrue(w.dirty)
        w.label_file.save_pascal_voc_format.assert_not_called()


if __name__ == '__main__':
    unittest.main()
