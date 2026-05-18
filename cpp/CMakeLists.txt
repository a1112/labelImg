cmake_minimum_required(VERSION 3.24)

project(labelImgCpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Xml Test)

set(CORE_SOURCES
    src/core/AnnotationIO.cpp
    src/core/AnnotationIO.h
    src/core/LabelListModel.cpp
    src/core/LabelListModel.h
    src/core/PerformanceMonitor.cpp
    src/core/PerformanceMonitor.h
    src/core/ResourcePaths.cpp
    src/core/ResourcePaths.h
    src/core/Shape.cpp
    src/core/Shape.h
    src/core/StringBundle.cpp
    src/core/StringBundle.h
)

add_library(labelImgCppCore STATIC ${CORE_SOURCES})
target_include_directories(labelImgCppCore PUBLIC src)
target_link_libraries(labelImgCppCore PUBLIC Qt6::Core Qt6::Gui Qt6::Xml)
if(WIN32)
    target_link_libraries(labelImgCppCore PUBLIC Psapi)
endif()
target_compile_definitions(labelImgCppCore PUBLIC
    LABELIMG_REPO_ROOT="${CMAKE_CURRENT_LIST_DIR}/.."
)

add_executable(labelImgCpp
    src/main.cpp
    src/ui/Canvas.cpp
    src/ui/Canvas.h
    src/ui/FramelessTitleBar.cpp
    src/ui/FramelessTitleBar.h
    src/ui/MiniMapOverlay.cpp
    src/ui/MiniMapOverlay.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
)
target_link_libraries(labelImgCpp PRIVATE labelImgCppCore Qt6::Widgets)

enable_testing()

add_executable(labelImgCppTests tests/test_core.cpp)
target_link_libraries(labelImgCppTests PRIVATE labelImgCppCore Qt6::Test)
add_test(NAME labelImgCppTests COMMAND labelImgCppTests)

add_executable(labelImgCppUiTests
    tests/test_ui.cpp
    src/ui/Canvas.cpp
    src/ui/Canvas.h
    src/ui/FramelessTitleBar.cpp
    src/ui/FramelessTitleBar.h
    src/ui/MiniMapOverlay.cpp
    src/ui/MiniMapOverlay.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
)
target_link_libraries(labelImgCppUiTests PRIVATE labelImgCppCore Qt6::Widgets Qt6::Test)
add_test(NAME labelImgCppUiTests COMMAND labelImgCppUiTests)
