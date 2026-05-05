# Phase 4 実機検証手順書

| 項目 | 内容 |
|------|------|
| 文書番号 | KKBD-USB-VERIF-PHASE4-001 |
| 作成日 | 2026-05-05 |
| 対応 Issue | #12 (Phase 4: 基本キー入力) |
| 対応 Phase | Phase 4 |
| 対象 FR/NFR | FR-002（キー入力受付）、FR-003（ASCII 変換・部分） |

---

## 1. はじめに

### 目的

本手順書は Phase 4 完了の判定として、USB キーボードからの英数字入力（A-Z, 0-9）を ASCII に変換して UART 送信できること、Enter キーで行末コードを送信できること、未対応キーは無視されること、キーを押し続けても連投されないことを実機で確認するための手順を定める。

### 検証対象

- PR マージ前: `feature/12-phase4-basic-keyinput` ブランチ
- PR マージ後: `main` ブランチでそのまま検証可能

### スコープ外

- Shift/Ctrl/特殊キー（記号変換・大文字変換・制御文字）は Phase 5 のスコープ
- キーリピート（押し続けで連続送信）は Phase 6 のスコープ
- LED 送信可視化は Phase 6 のスコープ

---

## 2. 前提条件

### 必要ハードウェア

| ハードウェア | 数量 | 備考 |
|-------------|------|------|
| Raspberry Pi Pico（無印、RP2040） | 1 | Phase 3 で書き込み確認済みのもの |
| USB Micro-B ケーブル | 1 | データ通信対応（書き込み・電源供給用） |
| 開発用 PC | 1 | macOS / Linux / Windows |
| USB-シリアル変換アダプタ | 1 | 3.3V TTL 対応のもの（FT232RL, CP2102, CH340 など） |
| ジャンパーケーブル または ブレッドボード | 適量 | Pico GPIO とアダプタ接続用 |
| **USB OTG アダプタ または USB-A ↔ Micro-B（OTG）ケーブル** | 1 | Pico Micro-B コネクタを USB ホストとして使うため |
| **USB キーボード（HID Keyboard クラス）** | 1 以上 | キー入力確認用 |

> **注**: Pico の Micro-B コネクタを USB ホストとして使う場合、外部回路または OTG アダプタで VBUS（5V）をデバイス側に給電する仕組みが必要。詳細は参考資料「USB 簡単ホスト」を参照してください。

### 接続図（テキスト）

```
Pico GPIO 0 (UART0 TX) ─── USB-シリアル RX
Pico GND               ─── USB-シリアル GND

Pico Micro-B コネクタ  ─── USB OTG アダプタ ─── USB キーボード
```

> **注意**: 配線は **電源を入れる前**（Pico を USB 接続する前）に行うこと。

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
git checkout feature/12-phase4-basic-keyinput
# PRマージ後は main ブランチでそのまま検証可能
```

> 既にリポジトリを clone 済みの場合は `git fetch` を実行する。PR マージ前は続けて `git checkout feature/12-phase4-basic-keyinput` を実行する。

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

### Step 3: 配線・Pico への書き込み

1. USB-シリアル変換アダプタを Pico に接続する

   | Pico 側 | USB-シリアル側 |
   |---------|---------------|
   | GPIO 0（UART0 TX） | RX |
   | GND | GND |

2. USB OTG アダプタを Pico の Micro-B コネクタに接続する（USB キーボード接続の準備）

3. アダプタを PC に接続し、認識されたシリアルデバイスを確認する

   | OS | デバイス例 |
   |----|-----------|
   | macOS | `/dev/tty.usbserial-XXXX` 等（`ls /dev/tty.usb*` で確認） |
   | Linux | `/dev/ttyUSB0` 等（`ls /dev/ttyUSB*` で確認） |
   | Windows | `COM3` 等（デバイスマネージャーで確認） |

4. Pico への書き込み

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

### Step 4: シリアル端末起動・起動バナー確認・キーボード接続

シリアル端末を **9600 bps（または設定したボーレート）** で開く。

| OS | コマンド例 |
|----|-----------|
| macOS | `screen /dev/tty.usbserial-XXXX 9600` |
| Linux | `minicom -D /dev/ttyUSB0 -b 9600` |
| Windows | TeraTerm: シリアル設定でボーレートを選択 |

電源投入後、以下のメッセージが表示されることを確認する：

```
[USB] TinyUSB host initialized
KKBD-USB v0.1 (Phase 4) - Waiting for USB keyboard...
```

> **注**: シリアル端末は電源投入前に開いておくと、起動直後のバナーを取りこぼさずに確認できる。

次に USB OTG アダプタ経由で USB キーボードを接続し、以下のログを確認する：

```
[USB] Keyboard connected (addr=N, instance=M)
```

---

### Step 5: 各 TC の実施

**検証パターン一覧:**

| TC | 操作 | 期待動作 |
|----|------|---------|
| TC-401 | キーボードで `a` から `z` まで順に入力 | シリアル端末に `abcdefghijklmnopqrstuvwxyz` が表示される |
| TC-402 | キーボードで `0` から `9` まで順に入力 | シリアル端末に `0123456789` が表示される |
| TC-403 | Enter キーを押す（メイン Enter） | 行末コードがジャンパー設定通りに送信される。<br>JP1=OPEN, JP2=OPEN → CR (0x0D)<br>JP1=SHORT, JP2=OPEN → LF (0x0A)<br>JP1=OPEN, JP2=SHORT → CRLF (0x0D 0x0A)<br>JP1=SHORT, JP2=SHORT → 予約パターン（CR にフォールバック） |
| TC-404 | F1/F2/矢印キー/Insert/Delete 等を押す | シリアル端末には何も表示されない |
| TC-405 | `a` を 3 秒間押し続ける | `a` が **1 文字のみ** 表示される（Phase 4 ではキーリピート未実装、押し続けでも 1 文字。複数文字が送信された場合は差分検出ロジックの不具合）。Phase 6 でキーリピートが実装される予定。 |
| TC-406 | Shift キーや Ctrl キーを単独で押す | シリアル端末には何も表示されない（修飾キー単体は無視） |
| TC-407 | テンキーの Enter を押す | 行末コードが送信される（メイン Enter と同じ動作） |
| TC-408 | `hello` を入力し Enter、`world` を入力し Enter | `hello` + CR（またはジャンパー設定の行末コード）+ `world` + CR が表示される |

**各 TC の補足:**

- **TC-401**: Phase 4 では Shift 未対応のため、A〜Z キーを押しても送信されるのは小文字 a〜z である。これは正常な動作（大文字への対応は Phase 5）。
- **TC-403/TC-407**: ジャンパー設定に応じた行末コードの確認方法はシリアル端末のバイナリ表示機能（`xxd` や TeraTerm のバイナリモード等）を利用する。デフォルト（JP1=OPEN, JP2=OPEN）は CR（`0x0D`）。
- **TC-404**: 記号キー（`-`, `=`, `[` 等）も Phase 4 では未対応のため何も送信されない。これは仕様通りの動作（Phase 5 で対応）。
- **TC-405**: 押し続けても 1 文字しか表示されないことを確認する。複数文字が送信された場合は差分検出ロジックの不具合。

---

## 4. 受け入れ条件チェックリスト

Issue #12 の受け入れ条件と対応する確認項目を以下に示す。**すべての項目にチェックが入った場合に Phase 4 合格** とする。

- [x] **条件 1**: USB キーボードで A〜Z キーを押すと、小文字 a〜z が UART に送信される（TC-401 合格）
- [x] **条件 2**: 0〜9 キーを押すと、数字 0〜9 が UART に送信される（TC-402 合格）
- [x] **条件 3**: 未対応キー（F キー、矢印キー等）を押しても UART への誤送信が発生しない（TC-404 合格）
- [x] **条件 4**: キーを押し続けても、離して再度押さない限り同じ文字が繰り返し送信されない（TC-405 合格）
- [x] **条件 5**: 同じキーを押し続けた場合の差分検出ロジックが正しく動作する（TC-405 合格）
- [x] **条件 6**: ホスト側ユニットテスト（`KKBD_PHASE4_DONE=ON`）の 4 関数（`test_keymap_convert_lowercase`, `test_keymap_convert_digits`, `test_keymap_convert_unsupported`, `test_keymap_is_enter`）すべてがパスする

> **注**: Issue #12 受け入れ条件 6 は当初「7 関数」と記載されていたが、本 PR で Phase 4/5 にテスト分割したため Phase 4 範囲は 4 関数とした。残り 3 関数（Shift/Ctrl/特殊キー）は Phase 5 で `KKBD_PHASE5_DONE=ON` 時に有効化される。Issue #12 のコメントで本変更を明記すること。

### ホスト側ユニットテストの実行方法

```sh
cmake -S tests -B build-tests -DKKBD_PHASE4_DONE=ON
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

---

## 5. 検証結果記録

検証完了後、以下の表に結果を記入して Issue #12 または PR のコメントに貼り付ける。

### 環境情報

| 項目 | 内容 |
|------|------|
| 検証実施日 | 2026-05-05 |
| 検証実施者 | k-ogata |
| 使用 Pico バリアント | Raspberry Pi Pico（無印、RP2040） |
| OS | macOS |
| Pico SDK バージョン | 1.5.1 |
| USB-シリアル変換アダプタ | （実機検証時に使用したもの） |
| シリアル端末ソフト | （macOS 標準） |
| 使用キーボード | 複数 HID インターフェース搭載キーボード（Phase 3 と同一） |
| USB OTG アダプタ / ケーブル | VBUS 給電パススルー対応 |
| ボーレート | 9600 |
| 行末コードジャンパー設定 | JP1=OPEN, JP2=OPEN → CR |

### TC 別結果

| TC | 操作 | 期待動作 | 結果 | 備考 |
|----|------|---------|------|------|
| TC-401 | `a`〜`z` 入力 | `abcdefghijklmnopqrstuvwxyz` 表示 | ☑ 合格 | 英字 26 文字すべて小文字で受信 |
| TC-402 | `0`〜`9` 入力 | `0123456789` 表示 | ☑ 合格 | 数字 10 文字すべて受信 |
| TC-403 | Enter キー押下 | 行末コード送信（ジャンパー設定に従う） | ☑ 合格 | デフォルト設定で CR (0x0D) 受信 |
| TC-404 | F1/F2/矢印キー等押下 | シリアル端末に何も表示されない | ☑ 合格 | 未対応キーは無視 |
| TC-405 | `a` を 3 秒押し続け | `a` が 1 文字のみ表示される | ☑ 合格 | 差分検出ロジック正常動作・連投なし |
| TC-406 | Shift/Ctrl 単独押下 | シリアル端末に何も表示されない | ☑ 合格 | 修飾キー単独押下は送信なし |
| TC-407 | テンキー Enter 押下 | 行末コード送信（メイン Enter と同じ） | ☑ 合格 | Keypad Enter (0x58) も正常認識 |
| TC-408 | `hello`+Enter+`world`+Enter | `hello`+行末コード+`world`+行末コード | ☑ 合格 | 連続入力 + Enter 正常 |

### ユニットテスト結果

| テスト関数 | 結果 | 備考 |
|-----------|------|------|
| `test_keymap_convert_lowercase` | ☑ 合格 | a/b/z 変換確認 |
| `test_keymap_convert_digits` | ☑ 合格 | 1, 0 変換確認 |
| `test_keymap_convert_unsupported` | ☑ 合格 | 11 ケース境界値全パス |
| `test_keymap_is_enter` | ☑ 合格 | Enter / Keypad Enter / 非Enter 判定確認 |

### 総合判定

| 項目 | 結果 |
|------|------|
| 受け入れ条件 1（TC-401） | ☑ 合格 |
| 受け入れ条件 2（TC-402） | ☑ 合格 |
| 受け入れ条件 3（TC-404） | ☑ 合格 |
| 受け入れ条件 4（TC-405） | ☑ 合格 |
| 受け入れ条件 5（TC-405） | ☑ 合格 |
| 受け入れ条件 6（ユニットテスト 4 関数） | ☑ 合格 |
| **Phase 4 総合** | ☑ **合格** |
| 所感・備考 | 英数字入力 + Enter + 未対応キー無視 + 押し続け差分検出 + 修飾キー単独押下無視のすべてが実機で確認できた。Shift/Ctrl 修飾は Phase 5、キーリピート/LED 可視化は Phase 6 で実装予定。|
| 添付（写真・動画） | - |

---

## 6. トラブルシューティング

| 症状 | 原因 | 対処 |
|------|------|------|
| キーを押しても何も表示されない | Phase 3 で接続ログが出ていない / キーボード未認識 | Phase 3 検証手順書のトラブルシューティング参照 |
| 大文字 A が送信される | Phase 4 範囲外（Shift 修飾は Phase 5） | 仕様通り。Phase 4 では小文字 a〜z が正しい動作 |
| 記号キー（`-`, `=` 等）で何も送信されない | Phase 4 範囲外（記号キーは Phase 5） | 仕様通り |
| Enter で何も送信されない | UART / ジャンパー設定のミス | Phase 2 検証手順書のトラブルシューティング参照 |
| 同じキーで連投される | 差分検出ロジック不具合 | `s_prev_report` の更新タイミングを確認、ファームウェアの再書き込み |
| ホスト側テストがリンクエラー | `KKBD_PHASE4_DONE` 未定義 | `-DKKBD_PHASE4_DONE=1` を gcc に追加、または cmake で `-DKKBD_PHASE4_DONE=ON` を指定 |
| `Compatibility with CMake < 3.5...` | CMake 4.x 互換性問題 | `scripts/build.sh` 経由でビルドする |
| `RPI-RP2` ボリュームが現れない | BOOTSEL を押さずに接続 / 充電専用ケーブル使用 | BOOTSEL を押しながら接続したか確認。USB ケーブルがデータ通信対応か確認 |
| `PICO_SDK_PATH` が設定されていない旨のエラー | 環境変数未設定 | `export PICO_SDK_PATH=/path/to/pico-sdk` を実施 |
| `arm-none-eabi-gcc` が見つからない | toolchain 未インストール | toolchain をインストール（README 参照） |

---

## 7. 関連ドキュメント

| ドキュメント | 参照目的 |
|------------|---------|
| [`README.md`](../../README.md) | 開発環境構築・ビルド方法の詳細 |
| [`docs/design/設計書.md`](../design/設計書.md) | §3.3（keymap）、§5.2（HID Usage ID 変換テーブル）、§6.3（キー入力シーケンス） |
| [`docs/design/実装計画.md`](../design/実装計画.md) | Phase 4 の実装詳細・受け入れ条件 |
| [`docs/design/テスト戦略.md`](../design/テスト戦略.md) | L1（ホストユニットテスト）方針 |
| [`docs/requirements/要件定義.md`](../requirements/要件定義.md) | FR-002（キー入力受付）、FR-003（ASCII コード変換） |
| [`docs/tests/phase1_実機検証手順.md`](phase1_実機検証手順.md) | Phase 1 検証手順（スタイル参考・前提条件） |
| [`docs/tests/phase2_実機検証手順.md`](phase2_実機検証手順.md) | Phase 2 検証手順（ジャンパー設定・UART トラブルシューティング） |
| [`docs/tests/phase3_実機検証手順.md`](phase3_実機検証手順.md) | Phase 3 検証手順（USB ホスト接続確認） |
| 参考: USB 簡単ホスト | https://q61.org/blog/2021/06/09/easyusbhost/ |

---

## 8. 次のステップ

Phase 4 の検証がすべての受け入れ条件で **合格** となった場合、Issue #13（Phase 5: 修飾キー対応）の実装・検証に進む。

検証結果に **不合格** が含まれる場合は、Issue #12 にトラブルシューティング結果を記録し、担当者と対処方針を協議する。
