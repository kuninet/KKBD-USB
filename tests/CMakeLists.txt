cmake_minimum_required(VERSION 3.13)
project(kkbd_usb_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Phase 4 keymap テストの有効化フラグ（OFF のままでもビルドが通ること）
option(KKBD_PHASE4_DONE "Enable Phase 4 keymap tests" OFF)

# 警告を厳しく
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(NOT KKBD_PHASE4_DONE)
        # Phase 4 向けにコメントアウトされたテスト関数の警告を抑制（暫定処置）
        # Phase 4 完了後、test_keymap.c のRUN_TEST有効化に合わせて本オプションは削除する
        add_compile_options(-Wno-unused-function)
    endif()
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
add_test(NAME test_keymap COMMAND test_keymap)
