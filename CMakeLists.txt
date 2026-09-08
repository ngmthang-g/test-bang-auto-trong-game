cmake_minimum_required(VERSION 3.24)
project(ThanLongAutoSettingsProbe VERSION 0.2.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "ThanLongAutoSettingsProbe is Windows-only")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "ThanLongAutoSettingsProbe must be built x64")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_library(ThanLongAutoSettingsBridge SHARED src/bridge.cpp src/auto_settings_parser.cpp)
target_include_directories(ThanLongAutoSettingsBridge PRIVATE src)
target_compile_definitions(ThanLongAutoSettingsBridge PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ThanLongAutoSettingsBridge PRIVATE user32 kernel32)

add_executable(ThanLongAutoSettingsProbe WIN32 src/main.cpp src/auto_settings_parser.cpp)
target_include_directories(ThanLongAutoSettingsProbe PRIVATE src)
target_compile_definitions(ThanLongAutoSettingsProbe PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0A00)
target_link_libraries(ThanLongAutoSettingsProbe PRIVATE user32 kernel32)

add_executable(auto_settings_parser_tests tests/auto_settings_parser_test.cpp src/auto_settings_parser.cpp)
target_include_directories(auto_settings_parser_tests PRIVATE src)
add_test(NAME auto_settings_parser_tests COMMAND auto_settings_parser_tests)

add_executable(ui_logic_tests tests/ui_logic_test.cpp)
target_include_directories(ui_logic_tests PRIVATE src)
add_test(NAME ui_logic_tests COMMAND ui_logic_tests)

add_executable(pickup_ui_logic_tests tests/pickup_ui_logic_test.cpp)
target_include_directories(pickup_ui_logic_tests PRIVATE src)
add_test(NAME pickup_ui_logic_tests COMMAND pickup_ui_logic_tests)

add_executable(protocol_layout_tests tests/protocol_layout_test.cpp)
target_include_directories(protocol_layout_tests PRIVATE src)
target_compile_definitions(protocol_layout_tests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME protocol_layout_tests COMMAND protocol_layout_tests)

if(MSVC)
    foreach(target ThanLongAutoSettingsBridge ThanLongAutoSettingsProbe auto_settings_parser_tests ui_logic_tests pickup_ui_logic_tests protocol_layout_tests)
        target_compile_options(${target} PRIVATE /utf-8 /W4 /permissive- /EHsc)
    endforeach()
endif()
