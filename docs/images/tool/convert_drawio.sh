#!/usr/bin/env bash
# =============================================================================
# convert_drawio.sh
# draw.io ファイル(.drawio)を png または jpeg 形式に変換するスクリプト
#
# 注意: このスクリプトは docs/images/tool/ に配置して使用することを前提とします。
#       ファイル指定なしの場合、スクリプト位置から相対的に docs/images/ 以下を
#       検索対象とします。
#
# 使用方法:
#   ./convert_drawio.sh [input.drawio] [output_format(png|jpeg)] [output_dir]
#
# 引数:
#   input.drawio  : 変換対象ファイル（省略時: docs/images/*.drawio を全て対象）
#   output_format : 出力形式 png または jpeg（省略時: png）
#   output_dir    : 出力先ディレクトリ（省略時: 入力ファイルと同じディレクトリ）
#
# 対応環境:
#   Mac / Linux / Windows(Git Bash)
# =============================================================================

set -euo pipefail

# -----------------------------------------------------------------------------
# 定数・デフォルト値
# -----------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_FORMAT="png"

# -----------------------------------------------------------------------------
# カラー出力ヘルパー
# -----------------------------------------------------------------------------
_info()    { echo "[INFO]  $*"; }
_success() { echo "[OK]    $*"; }
_skip()    { echo "[SKIP]  $*"; }
_warn()    { echo "[WARN]  $*" >&2; }
_error()   { echo "[ERROR] $*" >&2; }

# -----------------------------------------------------------------------------
# OS 判定と draw.io バイナリのパス解決
# -----------------------------------------------------------------------------
detect_drawio() {
    local os
    os="$(uname -s 2>/dev/null || echo "Windows")"

    case "${os}" in
        Darwin)
            # Mac
            local mac_path="/Applications/draw.io.app/Contents/MacOS/draw.io"
            if [[ -x "${mac_path}" ]]; then
                echo "${mac_path}"
                return 0
            fi
            # Homebrew Cask などで別パスにインストールされている場合
            if command -v drawio &>/dev/null; then
                command -v drawio
                return 0
            fi
            ;;
        Linux)
            # Linux: 絶対パス候補を -x でチェック
            for candidate in /opt/drawio/drawio /usr/bin/drawio /usr/local/bin/drawio; do
                if [[ -x "${candidate}" ]]; then
                    echo "${candidate}"
                    return 0
                fi
            done
            # PATH 上の drawio をコマンド名のみで検索
            if command -v drawio &>/dev/null; then
                command -v drawio
                return 0
            fi

            ;;
        MINGW*|MSYS*|CYGWIN*|Windows*)
            # Windows (Git Bash / MSYS2 / Cygwin)
            local win_path="/c/Program Files/draw.io/draw.io.exe"
            if [[ -x "${win_path}" ]]; then
                echo "${win_path}"
                return 0
            fi
            if command -v draw.io &>/dev/null; then
                command -v draw.io
                return 0
            fi
            ;;
    esac

    return 1
}

# -----------------------------------------------------------------------------
# 引数バリデーション
# -----------------------------------------------------------------------------
validate_format() {
    local fmt="$1"
    case "${fmt}" in
        png|jpeg|jpg)
            # jpg は jpeg として扱う
            echo "${fmt/jpg/jpeg}"
            return 0
            ;;
        *)
            _error "サポートされていない出力形式です: '${fmt}'"
            _error "使用可能な形式: png, jpeg"
            return 1
            ;;
    esac
}

# -----------------------------------------------------------------------------
# タイムスタンプ比較（出力ファイルが最新なら変換をスキップ）
# -----------------------------------------------------------------------------
is_up_to_date() {
    local src="$1"
    local dst="$2"

    if [[ ! -f "${dst}" ]]; then
        return 1  # 出力ファイルが存在しない → 変換が必要
    fi

    local os
    os="$(uname -s 2>/dev/null || echo "Windows")"

    local src_mtime dst_mtime

    case "${os}" in
        Darwin)
            src_mtime=$(stat -f "%m" "${src}" 2>/dev/null) || return 1
            dst_mtime=$(stat -f "%m" "${dst}" 2>/dev/null) || return 1
            ;;
        *)
            src_mtime=$(stat -c "%Y" "${src}" 2>/dev/null) || return 1
            dst_mtime=$(stat -c "%Y" "${dst}" 2>/dev/null) || return 1
            ;;
    esac

    if [[ "${dst_mtime}" -ge "${src_mtime}" ]]; then
        return 0  # 最新 → スキップ可能
    fi
    return 1  # 古い → 変換が必要
}

# -----------------------------------------------------------------------------
# 1ファイルを変換する
# -----------------------------------------------------------------------------
convert_file() {
    local drawio_bin="$1"
    local input_file="$2"
    local format="$3"
    local output_dir="$4"

    if [[ ! -f "${input_file}" ]]; then
        _error "入力ファイルが見つかりません: ${input_file}"
        return 1
    fi

    local basename
    basename="$(basename "${input_file}" .drawio)"
    local output_file="${output_dir}/${basename}.${format}"

    # タイムスタンプ比較によるスキップ判定
    if is_up_to_date "${input_file}" "${output_file}"; then
        _skip "${input_file} -> ${output_file} (変更なし、スキップ)"
        return 0
    fi

    # 出力ディレクトリ作成
    mkdir -p "${output_dir}"

    _info "${input_file} -> ${output_file} を変換中..."

    # draw.io CLI でエクスポート
    # --export      : エクスポートモード
    # --format      : 出力形式
    # --output      : 出力先ファイルパス
    # --no-sandbox  : Linux の root 環境や CI 環境でのクラッシュ回避（任意）
    # stderrを一時ファイルにキャプチャし、失敗時のみ表示する
    local stderr_tmp
    stderr_tmp="$(mktemp)"

    if "${drawio_bin}" \
            --export \
            --format "${format}" \
            --output "${output_file}" \
            "${input_file}" 2>"${stderr_tmp}"; then
        _success "${output_file} を生成しました"
        rm -f "${stderr_tmp}"
    else
        # 失敗時はstderrを表示して原因を確認できるようにする
        if [[ -s "${stderr_tmp}" ]]; then
            _warn "変換時のエラー出力:"
            cat "${stderr_tmp}" >&2
        fi
        rm -f "${stderr_tmp}"

        # --no-sandbox オプションを付けてリトライ（Linux CI 向け）
        _warn "変換失敗。--no-sandbox で再試行します..."
        if "${drawio_bin}" \
                --export \
                --format "${format}" \
                --output "${output_file}" \
                --no-sandbox \
                "${input_file}"; then
            _success "${output_file} を生成しました (--no-sandbox)"
        else
            _error "${input_file} の変換に失敗しました"
            return 1
        fi
    fi
}

# -----------------------------------------------------------------------------
# メイン処理
# -----------------------------------------------------------------------------
main() {
    local input_file="${1:-}"
    local format="${2:-${DEFAULT_FORMAT}}"
    local output_dir="${3:-}"

    # draw.io バイナリ検出
    local drawio_bin
    if ! drawio_bin="$(detect_drawio)"; then
        _error "draw.io デスクトップアプリが見つかりません。"
        _error "以下を確認してください:"
        _error "  Mac:     /Applications/draw.io.app"
        _error "  Linux:   /opt/drawio/drawio  または  drawio (PATH上)"
        _error "  Windows: C:\\Program Files\\draw.io\\draw.io.exe"
        exit 1
    fi
    _info "draw.io バイナリ: ${drawio_bin}"

    # フォーマットバリデーション
    if ! format="$(validate_format "${format}")"; then
        exit 1
    fi

    local error_count=0

    if [[ -n "${input_file}" ]]; then
        # -------------------------------------------------------------------
        # ファイル指定あり: 指定ファイルのみ変換
        # -------------------------------------------------------------------
        # 出力ディレクトリ未指定なら入力ファイルと同じディレクトリ
        if [[ -z "${output_dir}" ]]; then
            output_dir="$(cd "$(dirname "${input_file}")" && pwd)"
        fi

        # set -e 下では || の右辺が評価されるとエラーとみなされないため、
        # || error_count=... パターンで部分失敗をカウントしつつ処理を継続できる
        convert_file "${drawio_bin}" "${input_file}" "${format}" "${output_dir}" \
            || error_count=$((error_count + 1))
    else
        # -------------------------------------------------------------------
        # ファイル指定なし: docs/images/**/*.drawio を全て変換
        # -------------------------------------------------------------------
        local search_base
        # スクリプト位置 docs/images/tool/ から docs/images/ へ遡る
        search_base="$(cd "${SCRIPT_DIR}/.." && pwd)"

        # .drawio ファイルを再帰的に検索
        local found=0
        while IFS= read -r -d '' file; do
            found=1
            local file_dir
            file_dir="$(cd "$(dirname "${file}")" && pwd)"
            local dst_dir="${output_dir:-${file_dir}}"
            # set -e 下では || の右辺が評価されるとエラーとみなされないため、
            # || error_count=... パターンで部分失敗をカウントしつつ処理を継続できる
            convert_file "${drawio_bin}" "${file}" "${format}" "${dst_dir}" \
                || error_count=$((error_count + 1))
        done < <(find "${search_base}" -name "*.drawio" -type f -print0 2>/dev/null)

        if [[ "${found}" -eq 0 ]]; then
            _warn "変換対象の .drawio ファイルが見つかりませんでした: ${search_base}"
            _warn "ファイル指定なしの場合は docs/images/ 以下が対象です。"
        fi
    fi

    if [[ "${error_count}" -gt 0 ]]; then
        _error "${error_count} 件の変換が失敗しました。"
        exit 1
    fi

    _info "全ての変換が完了しました。"
}

main "$@"
