
import os
from unittest import TestCase
from unittest.mock import patch

from PySide6.QtCore import QEvent, QPointF, Qt
from PySide6.QtGui import QKeyEvent, QMouseEvent
from PySide6.QtTest import QTest

from libs.constants import SETTING_LANGUAGE
from libs.shape import Shape
from labelImg import get_main_app


class InMemorySettings(object):
    def __init__(self, initial=None):
        self.data = dict(initial or {})
        self.save_count = 0

    def __setitem__(self, key, value):
        self.data[key] = value

    def __getitem__(self, key):
        return self.data[key]

    def get(self, key, default=None):
        return self.data.get(key, default)

    def load(self):
        return True

    def save(self):
        self.save_count += 1
        return True


class TestMainWindow(TestCase):

    app = None
    win = None

    def setUp(self):
        self.settings = InMemorySettings()
        self.settings_patcher = patch('labelImg.Settings', return_value=self.settings)
        self.settings_patcher.start()
        self.locale_patcher = patch('libs.stringBundle.locale.getlocale', return_value=('zh_CN', 'UTF-8'))
        self.locale_patcher.start()
        self.app, self.win = get_main_app()

    def tearDown(self):
        self.win.set_clean()
        self.win.close()
        self.app.quit()
        self.locale_patcher.stop()
        self.settings_patcher.stop()

    def test_language_uses_system_locale_first_then_switches_live_and_persists(self):
        self.assertEqual(self.settings.get(SETTING_LANGUAGE), 'zh-CN')
        self.assertEqual(self.win.menus.file.title(), '文件(&F)')
        checked_languages = [
            action.data()
            for action in self.win.language_menu.actions()
            if action.isChecked()
        ]
        self.assertEqual(checked_languages, ['zh-CN'])
        self.assertGreaterEqual(self.settings.save_count, 1)

        self.win.change_language('en')

        self.assertEqual(self.settings.get(SETTING_LANGUAGE), 'en')
        self.assertEqual(self.win.menus.file.title(), '&File')
        self.assertEqual(self.win.language_menu.title(), 'Language')
        self.assertGreaterEqual(self.settings.save_count, 2)

        image_path = os.path.join(os.path.dirname(__file__), 'test.512.512.bmp')
        self.win.load_file(image_path)

        self.win.add_light(-10, False)
        self.assertEqual(self.win.light_widget.value(), 40)

        self.win.set_light(50, True)
        self.assertEqual(self.win.light_widget.value(), 50)

        self.win.label_list.clear()
        self.win.items_to_shapes.clear()
        self.win.shapes_to_items.clear()
        first_shape = Shape(label='first')
        second_shape = Shape(label='second')
        self.win.canvas.shapes = [first_shape, second_shape]
        self.win.add_label(first_shape)
        self.win.add_label(second_shape)

        self.win.combo_selection_changed(0)
        self.assertEqual(self.win.label_list.item(0).checkState(), Qt.Checked)

        self.win.select_label_row(0)
        self.win.delete_selected_shape()
        self.assertEqual(self.win.current_item().text(), 'second')

        third_shape = Shape(label='third')
        self.win.canvas.shapes.append(third_shape)
        self.win.add_label(third_shape)
        self.win.label_list.clearSelection()
        self.win.label_list.setCurrentItem(None)
        self._send_key_event(Qt.Key_C)
        self.assertEqual(self.win.current_item().text(), 'second')

        self._send_key_event(Qt.Key_C)
        self.assertEqual(self.win.current_item().text(), 'third')

        self._send_key_event(Qt.Key_C)
        self.assertEqual(self.win.current_item().text(), 'second')

        self._send_key_event(Qt.Key_Z)
        self.assertEqual(self.win.current_item().text(), 'third')

        self._send_key_event(Qt.Key_S)
        self.assertEqual(self.win.label_list.count(), 1)
        self.assertEqual(self.win.current_item().text(), 'second')
        self.win.set_clean()

        self.win.canvas.shapes.clear()
        self.win.label_list.clear()
        self.win.items_to_shapes.clear()
        self.win.shapes_to_items.clear()
        first_shape = Shape(label='first')
        second_shape = Shape(label='second')
        third_shape = Shape(label='third')
        self.win.canvas.shapes = [first_shape, second_shape, third_shape]
        self.win.add_label(first_shape)
        self.win.add_label(second_shape)
        self.win.add_label(third_shape)

        self.win.canvas.setFocus()
        QTest.keyClick(self.win.canvas, Qt.Key_C)
        self.assertEqual(self.win.current_item().text(), 'first')

        self.win.label_list.clearSelection()
        self.win.label_list.setCurrentItem(None)
        self._send_key_event(Qt.Key_Z)
        self.assertEqual(self.win.current_item().text(), 'first')

        self._send_key_event(Qt.Key_C)
        self.assertEqual(self.win.current_item().text(), 'second')

        self._send_key_event(Qt.Key_C)
        self._send_key_event(Qt.Key_C)
        self.assertEqual(self.win.current_item().text(), 'first')

        self.win.select_label_row(1)
        self._send_key_event(Qt.Key_S)
        self.assertEqual(self.win.label_list.count(), 2)
        self.assertEqual(self.win.current_item().text(), 'third')

        self._send_key_event(Qt.Key_S)
        self.assertEqual(self.win.label_list.count(), 1)
        self.assertEqual(self.win.current_item().text(), 'first')

        self._send_key_event(Qt.Key_Q)
        self.assertEqual(self.win.label_list.count(), 0)
        self.assertEqual(self.win.canvas.shapes, [])
        self.assertTrue(self.win.no_shapes())
        self.win.set_clean()

        self.win.m_img_list = [image_path]
        self.win.file_list_widget.clear()
        self.win.file_list_widget.addItem(image_path)
        self.win.load_file(image_path)
        self.assertGreaterEqual(self.win.file_list_widget.font().pointSize(), 11)
        self.assertTrue(self.win.file_list_widget.item(0).isSelected())

    def _send_mouse_event(self, event_type, pos, button, buttons):
        event = QMouseEvent(
            event_type,
            pos,
            pos,
            button,
            buttons,
            Qt.NoModifier
        )
        if event_type == QEvent.MouseButtonPress:
            self.win.canvas.mousePressEvent(event)
        elif event_type == QEvent.MouseMove:
            self.win.canvas.mouseMoveEvent(event)
        elif event_type == QEvent.MouseButtonRelease:
            self.win.canvas.mouseReleaseEvent(event)

    def _send_key_event(self, key):
        event = QKeyEvent(QEvent.KeyPress, key, Qt.NoModifier)
        self.win.keyPressEvent(event)
