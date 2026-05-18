# labelImgCpp Feature Parity Design

## Goal

`labelImgCpp` is a Qt 6 Widgets / CMake / MSVC port that keeps the Python application as the behavioral reference until the C++ version is accepted for daily use.

## Feature Matrix

| Area | Python reference | C++ status | Acceptance |
| --- | --- | --- | --- |
| Image files | Open file, open directory, startup directory, file list, previous/next, delete image | Implemented | Directory navigation keeps current file selected and visible; startup directory arguments populate the file list. |
| Annotation files | Auto-load VOC > YOLO > CreateML, open external annotation | Implemented | Loading XML/TXT/JSON updates format, shapes, and verified state. |
| Save formats | VOC, YOLO, CreateML, save/save-as, save dir | Implemented | Output round-trips with Chinese paths/labels and class order. |
| Labels | List selection, visibility checkbox, filter, edit, delete, difficult | Implemented | List and canvas selection stay synchronized. |
| Canvas drawing | Create rectangle, square mode, cancel, finish, preview | Implemented | W enters create mode with cross cursor; click/tiny drags do not leave low-pixel boxes; Esc cancels; Return finalizes current box. |
| Canvas editing | Select, move, vertex drag, one-pixel nudging, bounds | Implemented | Shapes and vertices cannot move outside the pixmap. |
| Context menus | Canvas and label list menus, right-drag pan | Implemented | Canvas and label-list menus expose edit/copy/delete plus Copy here/Move here; right-drag only scrolls after drag threshold to avoid jitter. |
| View | Manual zoom, fit window, fit width, brightness | Implemented | Wheel zoom preserves the mouse anchor through scrollbar adjustment; fit modes update on resize; brightness reset returns to 50. |
| Settings | Language, save dir, format, recent files, geometry/state, colors | Implemented | Close/reopen restores user-visible state. |
| Modes | Beginner/advanced create/edit split | Implemented | Toolbar contents switch between beginner and advanced workflows; W and Ctrl+J keep mode state synchronized. |
| Help | Info and shortcuts | Implemented | Help menu opens local dialogs without network dependency. |

## Implementation Boundaries

- `MainWindow` owns menus, toolbars, docks, file state, settings, annotation loading/saving, language refresh, and user dialogs.
- `Canvas` owns pixmap rendering, shape hit testing, create/edit modes, vertex editing, constrained moves, zoom/brightness state, and mouse/keyboard interaction.
- `Shape` owns rectangle geometry primitives used by both canvas and tests.
- `AnnotationIO` stays UI-free and is the compatibility layer for VOC, YOLO, and CreateML.

## Regression Commands

```powershell
cmake -S cpp -B cpp/build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="D:\Qt\6.11.0\msvc2022_64"
cmake --build cpp/build --config Release
$env:PATH='D:\Qt\6.11.0\msvc2022_64\bin;' + $env:PATH
ctest --test-dir cpp/build -C Release --output-on-failure
python -m unittest discover tests
```

## Remaining Gaps

- Add MainWindow-level tests around mouse-centered scroll adjustment and right-click copy/move menu wiring.
- Add packaging scripts around `windeployqt` after feature parity stabilizes.
- Continue comparing toolbar action ordering against Python during manual QA.
