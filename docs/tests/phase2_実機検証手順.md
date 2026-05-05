# Phase 2 実機検証手順書

| 項目 | 内容 |
|------|------|
| 文書番号 | KKBD-USB-VERIF-PHASE2-001 |
| 作成日 | 2026-05-05 |
| 対応 Issue | #10 (Phase 2: ジャンパー読取とUART送信) |
| 対応 Phase | Phase 2 |
| 対象 FR/NFR | FR-005, FR-006, FR-008 |

---

## 1. はじめに

### 目的

本手順書は Phase 2 完了の判定として、起動時にジャンパーピン JP1〜JP4 を読み取り、設定されたボーレート/行末コードで `"KKBD-USB Ready\r\n"` を 1 秒毎に UART 送信できることを確認するための手順を定める。

### 検証対象

- PR マージ前: `feature/10-phase2-config-uart` ブランチ
- PR マージ後: `main` ブランチでそのまま検証可能

---

## 2. 前提条件

### 必要ハードウェア

| ハードウェア | 数量 | 備考 |
|-------------|------|------|
| Raspberry Pi Pico（無印、RP2040） | 1 | Phase 1 で書き込み確認済みのもの |
| USB Micro-B ケーブル | 1 | データ通信対応 |
| 開発用 PC | 1 | macOS / Linux / Windows |
| **USB-シリアル変換アダプタ** | 1 | 3.3V TTL 対応のもの（FT232RL, CP2102, CH340 など） |
| ジャンパーケーブル または ブレッドボード | 適量 | Pico GPIO とアダプタ接続用、ジャンパー設定用 |

### 接続図（テキスト）

```
Pico GPIO 0 (UART0 TX) ─── USB-シリアル RX
Pico GND               ─── USB-シリアル GND

Pico GPIO 10 (JP1) ─── (OPEN/SHORTで GND と接続切替)
Pico GPIO 11 (JP2) ─── (OPEN/SHORTで GND と接続切替)
Pico GPIO 12 (JP3) ─── (OPEN/SHORTで GND と接続切替)
Pico GPIO 13 (JP4) ─── (OPEN/SHORTで GND と接続切替)
```

> **注**: SHORT = GPIO を GND に接続、OPEN = GPIO 未接続（内蔵プルアップにより High）

### 必要ソフトウェア

| ソフトウェア | バージョン | 備考 |
|------------|-----------|------|
| Pico SDK | v1.5.1 以上 | 環境変数 `PICO_SDK_PATH` 設定済みであること |
| `arm-none-eabi-gcc` | 10 以上 | ARM クロスコンパイラ |
| `cmake` | 3.13 以上 | ビルドシステム生成ツール |
| `ninja` | 最新安定版 | 推奨（`make` でも可） |
| `git` | 2.x 以上 | リポジトリ取得用 |
| **シリアル端末ソフト** | - | macOS: `screen` または `minicom`、Linux: `minicom`、Windows: TeraTerm 等 |

各ツールのインストール方法は [`README.md`](../../README.md) の「開発環境」セクションを参照。

---

## 3. 検証手順

### Step 1: リポジトリ取得とブランチ切替

```sh
git clone https://github.com/kuninet/KKBD-USB.git
cd KKBD-USB

# PRマージ前のみ：feature ブランチをチェックアウト
git checkout feature/10-phase2-config-uart
# PRマージ後は main ブランチでそのまま検証可能
```

> 既にリポジトリを clone 済みの場合は `git fetch` を実行する。PR マージ前は続けて `git checkout feature/10-phase2-config-uart` を実行する。

---

### Step 2: ファームウェアビルド

#### 推奨: ビルドスクリプト経由

```sh
export PICO_SDK_PATH=$HOME/pico-sdk    # 実際のパスに置き換える
./scripts/build.sh --clean             # 初回または再ビルド時
# 通常時は `./scripts/build.sh` のみで OK
```

`scripts/build.sh` は CMake 4.x 利用時のワークアラウンド（`CMAKE_POLICY_VERSION_MINIMUM=3.5`）を自動で適用する。

#### 手動実行する場合

```sh
export PICO_SDK_PATH=$HOME/pico-sdk
# CMake 4.0 以上を使用する場合は以下も必須（Pico SDK 1.5.1 互換性対応）
export CMAKE_POLICY_VERSION_MINIMUM=3.5
cmake -S . -B build -G Ninja
cmake --build build
```

> **注**: `CMAKE_POLICY_VERSION_MINIMUM` はシェルで export する必要がある。CMakeLists.txt 内の `set(ENV{})` ではビルド時に ninja が spawn する子 CMake に伝播しないため不可。

**期待結果:**

| 確認項目 | 期待値 |
|---------|--------|
| cmake 設定フェーズ | エラーなく完了し `build/` ディレクトリが生成される |
| ビルドフェーズ | エラーなく完了する（警告は許容） |
| 成果物 | `build/src/kkbd_usb.uf2` が生成される |
| ファイルサイズ | 数十 KB〜数百 KB 程度 |

**確認コマンド:**

```sh
ls -la build/src/kkbd_usb.uf2
```

---

### Step 3: 配線

1. USB-シリアル変換アダプタを Pico に接続する

   | Pico 側 | USB-シリアル側 |
   |---------|---------------|
   | GPIO 0（UART0 TX） | RX |
   | GND | GND |

2. アダプタを PC に接続し、認識されたシリアルデバイスを確認する

   | OS | デバイス例 |
   |----|-----------|
   | macOS | `/dev/tty.usbserial-XXXX` 等（`ls /dev/tty.usb*` で確認） |
   | Linux | `/dev/ttyUSB0` 等（`ls /dev/ttyUSB*` で確認） |
   | Windows | `COM3` 等（デバイスマネージャーで確認） |

> **注意**: 配線は **電源を入れる前**（Pico を USB 接続する前）に行うこと。

---

### Step 4: Pico への書き込み

1. Pico の **BOOTSEL ボタン** を **押し続けたまま** USB ケーブルで PC に接続する
2. PC のファイルマネージャに `RPI-RP2` というボリュームが表示されることを確認する
3. BOOTSEL ボタンを離す
4. `build/src/kkbd_usb.uf2` を `RPI-RP2` にコピーする

   | OS | コマンド / 操作 |
   |----|----------------|
   | macOS | `cp build/src/kkbd_usb.uf2 /Volumes/RPI-RP2/` |
   | Linux | `cp build/src/kkbd_usb.uf2 /media/$USER/RPI-RP2/` |
   | Windows | エクスプローラで `RPI-RP2` ドライブへドラッグ＆ドロップ |

5. コピー完了後、Pico は自動的に再起動する

---

### Step 5: 各ジャンパー設定で動作確認

各テストケースについて、下記の手順を繰り返す。

**各 TC の手順:**

1. Pico から USB ケーブルを抜き、電源を切る
2. ジャンパー設定を行う（SHORT にする GPIO ピンを GND へ接続）
3. シリアル端末を**該当ボーレート**で開いておく

   | OS | コマンド例（9600 bps の場合） |
   |----|------------------------------|
   | macOS | `screen /dev/tty.usbserial-XXXX 9600` |
   | Linux | `minicom -D /dev/ttyUSB0 -b 9600` |
   | Windows | TeraTerm: シリアル設定でボーレートを選択 |

4. Pico に USB ケーブルを接続して電源を投入する
5. シリアル端末に `KKBD-USB Ready` が 1 秒毎に繰り返し表示されることを確認する
6. 文字化けが発生しないこと（= ボーレートが一致していること）を確認する

**検証パターン一覧:**

| 検証ID | JP1 | JP2 | JP3 | JP4 | 期待ボーレート | 期待動作 |
|--------|-----|-----|-----|-----|--------------|---------|
| TC-201 | OPEN | OPEN | OPEN | OPEN | 9600 bps | `KKBD-USB Ready` が 1 秒毎に表示される |
| TC-202 | OPEN | OPEN | SHORT | OPEN | 19200 bps | `KKBD-USB Ready` が 1 秒毎に表示される |
| TC-203 | OPEN | OPEN | OPEN | SHORT | 38400 bps | `KKBD-USB Ready` が 1 秒毎に表示される |
| TC-204 | OPEN | OPEN | SHORT | SHORT | 115200 bps | `KKBD-USB Ready` が 1 秒毎に表示される |
| TC-205 | SHORT | SHORT | OPEN | OPEN | 9600 bps | 予約パターンでも `KKBD-USB Ready` が 9600 bps で表示される（クラッシュせず起動シーケンスが完走する）。完全な行末コード検証は Phase 6（Enter キー入力時に CR/LF/CRLF を切り替える検証）で実施。 |

> **JP1/JP2 と JP3/JP4 の意味:**
> - JP1/JP2: 行末コード選択（SHORT=GND接続 → bit=0）
> - JP3/JP4: ボーレート選択（SHORT=GND接続 → bit=0）
> - 各 GPIO は内蔵プルアップ設定であるため、OPEN 時は High（1）として読み取られる

---

### Step 6: 行末コード確認（簡易）

本 Phase では `"KKBD-USB Ready\r\n"`（CR+LF を文字列にハードコード）が送信されることで、UART 送信パスが正常に動作していることを確認する。

> **注**: JP1/JP2 による行末コード選択の完全な検証は Phase 6 で実施する。本 Phase では `\r\n` が送信されていることが確認できれば十分である。

---

## 4. 受け入れ条件チェックリスト

Issue #10 の受け入れ条件と対応する確認項目を以下に示す。**すべての項目にチェックが入った場合に Phase 2 合格** とする。

- [ ] **条件 1**: ジャンパーなし（全 OPEN = 9600 bps）で起動し、PC のシリアル端末に `KKBD-USB Ready` が表示される（TC-201 合格）
- [ ] **条件 2**: JP3/JP4 の各パターン（4 種類）を変えて再起動し、それぞれのボーレートで文字列が受信できる（TC-201〜TC-204 全合格）
- [ ] **条件 3**: 予約パターン（JP1/JP2 両方 SHORT）で起動した際、CR 相当のデフォルト動作となる（TC-205 合格）

---

## 5. 検証結果記録

検証完了後、以下の表に結果を記入して Issue #10 または PR のコメントに貼り付ける。

### 環境情報

| 項目 | 内容 |
|------|------|
| 検証実施日 | |
| 検証実施者 | |
| 使用 Pico バリアント | Raspberry Pi Pico（無印、RP2040）|
| OS | |
| Pico SDK バージョン | |
| USB-シリアル変換アダプタ | |
| シリアル端末ソフト | |

### TC 別結果

| TC | ジャンパー設定 | 期待ボーレート | 結果 | 備考 |
|----|--------------|--------------|------|------|
| TC-201 | 全 OPEN | 9600 bps | ☐ 合格 / ☐ 不合格 | |
| TC-202 | JP3=SHORT, 他 OPEN | 19200 bps | ☐ 合格 / ☐ 不合格 | |
| TC-203 | JP4=SHORT, 他 OPEN | 38400 bps | ☐ 合格 / ☐ 不合格 | |
| TC-204 | JP3=SHORT, JP4=SHORT | 115200 bps | ☐ 合格 / ☐ 不合格 | |
| TC-205 | JP1=SHORT, JP2=SHORT | 9600 bps（CR フォールバック）| ☐ 合格 / ☐ 不合格 | |

### 総合判定

| 項目 | 結果 |
|------|------|
| 受け入れ条件 1（TC-201） | ☐ 合格 / ☐ 不合格 |
| 受け入れ条件 2（TC-201〜TC-204） | ☐ 合格 / ☐ 不合格 |
| 受け入れ条件 3（TC-205） | ☐ 合格 / ☐ 不合格 |
| **Phase 2 総合** | ☐ **合格** / ☐ **不合格** |
| 所感・備考 | |
| 添付（写真・動画） | - |

---

## 6. トラブルシューティング

| 症状 | 原因 | 対処 |
|------|------|------|
| 何も受信されない | TX/GND 配線不良、ボーレート不一致、Pico 電源未投入 | 配線・電源・端末ボーレートを確認。TX と RX を逆に繋いでいないか確認 |
| 文字化けする | ボーレート不一致 | シリアル端末側のボーレートをジャンパー設定に合わせる |
| ジャンパーを変えても動作が変わらない | GPIO 読み取りタイミング、配線不良 | 電源を完全に切ってから設定変更し再投入。GND 接続を確認 |
| `KKBD-USB Ready` が表示されない | 起動シーケンスでハング、またはシリアル端末の起動タイミング | シリアル端末を先に開いた状態で電源を入れ直す。9600 bps で確認 |
| `RPI-RP2` ボリュームが現れない | BOOTSEL を押さずに接続 / 充電専用ケーブル使用 | BOOTSEL を押しながら接続したか確認。USB ケーブルがデータ通信対応か確認 |
| `PICO_SDK_PATH` が設定されていない旨のエラー | 環境変数未設定 | `export PICO_SDK_PATH=/path/to/pico-sdk` を実施 |
| `arm-none-eabi-gcc` が見つからない | toolchain 未インストール | toolchain をインストール（README 参照） |
| `Compatibility with CMake < 3.5 has been removed from CMake.`（pioasm/elf2uf2 サブビルド） | CMake 4.0+ と Pico SDK 1.5.1 同梱 TinyUSB の互換性問題 | `scripts/build.sh` を使用するか、シェルで `export CMAKE_POLICY_VERSION_MINIMUM=3.5` を実行してからビルドする |

---

## 7. 関連ドキュメント

| ドキュメント | 参照目的 |
|------------|---------|
| [`README.md`](../../README.md) | 開発環境構築・ビルド方法の詳細 |
| [`docs/design/設計書.md`](../design/設計書.md) | §3.4-3.5（UART送信・ジャンパー読取処理）、§4（ピンアサイン） |
| [`docs/design/実装計画.md`](../design/実装計画.md) | Phase 2 の実装詳細・受け入れ条件 |
| [`docs/design/テスト戦略.md`](../design/テスト戦略.md) | L1（config テスト）、L4（実機テスト）方針 |
| [`docs/requirements/要件定義.md`](../requirements/要件定義.md) | FR-005（ボーレート設定）、FR-006（UART送信）、FR-008（起動時初期化）、§2.4（ジャンパー仕様） |
| [`docs/tests/phase1_実機検証手順.md`](phase1_実機検証手順.md) | Phase 1 検証手順（スタイル参考・前提条件） |

---

## 8. 次のステップ

Phase 2 の検証がすべての受け入れ条件で **合格** となった場合、Issue #11（Phase 3: USB ホスト基盤）の実装・検証に進む。

検証結果に **不合格** が含まれる場合は、Issue #10 にトラブルシューティング結果を記録し、担当者と対処方針を協議する。
