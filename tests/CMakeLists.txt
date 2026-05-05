cmake_minimum_required(VERSION 3.13)
project(kkbd_usb_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Phase フラグ（OFF のままでもビルドが通ること）
# Phase 進行に応じた段階的テスト有効化フラグ。
#
# 重要: Phase N の keymap 実装が完了した後は、
#   それ以前の Phase の単独ビルド（例: Phase 4 only）はテスト失敗する場合がある。
#   理由: keymap.c が最新 Phase の実装に置き換わると、過去 Phase の「未対応キー」
#   テストが現実装と矛盾するため。
#   推奨: 最新 Phase 完了後は対応するフラグをすべて ON にしてビルドする。
#   例: Phase 5 完了時点では `-DKKBD_PHASE4_DONE=ON -DKKBD_PHASE5_DONE=ON` を指定。
option(KKBD_PHASE4_DONE "Enable Phase 4 (keymap basic keys/Enter) tests" OFF)
option(KKBD_PHASE5_DONE "Enable Phase 5 (keymap Shift/Ctrl/special keys) tests" OFF)
# 注: KKBD_PHASE6_DONE は test_keyrepeat には影響しない
# （test_keyrepeat は keyrepeat_decide の純粋関数のみテストするため、
#  Phase 6 完了/未完了に関係なくビルド・実行可能）。
# フラグは将来 keymap.c のような Phase 別ガード化に備えた予約フラグ。
option(KKBD_PHASE6_DONE "Enable Phase 6 (keyrepeat/LED) tests" OFF)

# 警告を厳しく
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(KKBD_PHASE4_DONE)
    add_compile_definitions(KKBD_PHASE4_DONE)
endif()
if(KKBD_PHASE5_DONE)
    add_compile_definitions(KKBD_PHASE5_DONE)
endif()
if(KKBD_PHASE6_DONE)
    add_compile_definitions(KKBD_PHASE6_DONE)
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
# Phase 5 完全有効化までは Phase 5 用テスト関数が unused-function になるため警告を抑制
# Phase 5 完了後（KKBD_PHASE5_DONE=ON 時）は全テスト関数が使用されるため抑制不要
if(NOT KKBD_PHASE5_DONE)
    target_compile_options(test_keymap PRIVATE -Wno-unused-function)
endif()
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

# keyrepeat テスト（Phase 6）
add_executable(test_keyrepeat
    ${CMAKE_CURRENT_SOURCE_DIR}/test_keyrepeat.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/keyrepeat.c
)
target_include_directories(test_keyrepeat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(test_keyrepeat PRIVATE KKBD_HOST_TEST=1)
add_test(NAME test_keyrepeat COMMAND test_keyrepeat)
