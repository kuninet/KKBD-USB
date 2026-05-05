cmake_minimum_required(VERSION 3.13)
project(kkbd_usb_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Phase 4 keymap テストの有効化フラグ（OFF のままでもビルドが通ること）
option(KKBD_PHASE4_DONE "Enable Phase 4 keymap tests" OFF)

# 警告を厳しく
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(KKBD_PHASE4_DONE)
    add_compile_definitions(KKBD_PHASE4_DONE)
endif()

enable_testing()

# keymap テスト
add_executable(test_keymap
    ${CMAKE_CURRENT_SOURCE_DIR}/test_keymap.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/keymap.c
)
target_include_directories(test_keymap PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}
)
# Phase 4 向けにコメントアウトされたテスト関数の警告を抑制（test_keymap のみ）
# Phase 4 完了後、test_keymap.c の RUN_TEST 有効化に合わせて本オプションは削除する
target_compile_options(test_keymap PRIVATE -Wno-unused-function)
add_test(NAME test_keymap COMMAND test_keymap)

# config テスト
add_executable(test_config
    ${CMAKE_CURRENT_SOURCE_DIR}/test_config.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/config.c
)
target_include_directories(test_config PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}
)
# config.c は Pico SDK の pico/stdlib.h と hardware/gpio.h を include しているが
# ホストビルドではこれらは存在しない。ホスト側テストでは decode 関数のみを対象とするため、
# KKBD_HOST_TEST=1 を定義して Pico SDK 依存箇所をコンパイル対象から除外する。
target_compile_definitions(test_config PRIVATE KKBD_HOST_TEST=1)
add_test(NAME test_config COMMAND test_config)
