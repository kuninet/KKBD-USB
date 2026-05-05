#!/usr/bin/env bash
# KKBD-USB ファームウェアビルドスクリプト
#
# 機能:
#   - CMake 4.x 利用時に CMAKE_POLICY_VERSION_MINIMUM=3.5 を自動設定
#     （Pico SDK 1.5.1 同梱 TinyUSB pioasm/elf2uf2 の互換性ワークアラウンド）
#   - PICO_SDK_PATH のチェック
#   - configure（必要なら）と build をまとめて実行
#
# 使い方:
#   export PICO_SDK_PATH=$HOME/pico-sdk
#   ./scripts/build.sh           # 標準ビルド
#   ./scripts/build.sh --clean   # build/ を削除して再ビルド
#
# Pico SDK 2.x にアップグレードすれば本スクリプトの workaround は不要。

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

# --- オプション処理 ---
CLEAN=0
for arg in "$@"; do
    case "$arg" in
        --clean) CLEAN=1 ;;
        -h|--help)
            sed -n '1,18p' "$0"
            exit 0
            ;;
        *)
            echo "[build.sh] 未知のオプション: $arg" >&2
            exit 1
            ;;
    esac
done

# --- PICO_SDK_PATH チェック ---
if [ -z "${PICO_SDK_PATH:-}" ]; then
    echo "[build.sh] エラー: PICO_SDK_PATH が未設定です" >&2
    echo "  例: export PICO_SDK_PATH=\$HOME/pico-sdk" >&2
    exit 1
fi

if [ ! -d "$PICO_SDK_PATH" ]; then
    echo "[build.sh] エラー: PICO_SDK_PATH=$PICO_SDK_PATH が存在しません" >&2
    exit 1
fi

# --- CMake 4.x 互換ワークアラウンド ---
CMAKE_VERSION_FULL=$(cmake --version | head -1 | awk '{print $3}')
CMAKE_MAJOR=$(echo "$CMAKE_VERSION_FULL" | cut -d. -f1)
if [ "$CMAKE_MAJOR" -ge 4 ]; then
    export CMAKE_POLICY_VERSION_MINIMUM=3.5
    echo "[build.sh] CMake $CMAKE_VERSION_FULL 検出 → CMAKE_POLICY_VERSION_MINIMUM=3.5 を設定"
fi

# --- clean ---
if [ "$CLEAN" -eq 1 ] && [ -d build ]; then
    echo "[build.sh] build/ を削除"
    rm -rf build
fi

# --- configure ---
if [ ! -d build ] || [ ! -f build/CMakeCache.txt ]; then
    echo "[build.sh] configure 実行"
    cmake -S . -B build -G Ninja
fi

# --- build ---
echo "[build.sh] build 実行"
cmake --build build

echo "[build.sh] 成功: build/src/kkbd_usb.uf2"
