# Phase 5 実機検証手順書

| 項目 | 内容 |
|------|------|
| 文書番号 | KKBD-USB-VERIF-PHASE5-001 |
| 作成日 | 2026-05-05 |
| 対応 Issue | #13 (Phase 5: 修飾キー対応) |
| 対応 Phase | Phase 5 |
| 対象 FR/NFR | FR-003（ASCII 変換・完全） |

---

## 1. はじめに

### 目的

本手順書は Phase 5 完了の判定として、Shift/Ctrl 修飾キー、特殊キー（Esc/Tab/Space/BS）、記号キー全般、Keypad 数字を実装し、設計書 §5.2/§5.3/§5.4 のテーブル仕様が完全に動作することを実機で確認するための手順を定める。

### 検証対象

- PR マージ前: `feature/13-phase5-modifier-keys` ブランチ
- PR マージ後: `main` ブランチでそのまま検証可能

### スコープ外

- NumLock 状態追跡は本 Phase スコープ外（Keypad は常に数字として送信）
- Alt/GUI 修飾キーは未対応（Phase 5 対象外）
- キーリピート（押し続けで連続送信）は Phase 6 のスコープ
- LED 送信可視化は Phase 6 のスコープ

---

## 2. 前提条件

### 必要ハードウェア

| ハードウェア | 数量 | 備考 |
|-------------|------|------|
| Raspberry Pi Pico（無印、RP2040） | 1 | Phase 4 で書き込み確認済みのもの |
| USB Micro-B ケーブル | 1 | データ通信対応（書き込み・電源供給用） |
| 開発用 PC | 1 | macOS / Linux / Windows |
| USB-シリアル変換アダプタ | 1 | 3.3V TTL 対応のもの（FT232RL, CP2102, CH340 など） |
| ジャンパーケーブル または ブレッドボード | 適量 | Pico GPIO とアダプタ接続用 |
| **USB OTG アダプタ または USB-A ↔ Micro-B（OTG）ケーブル** | 1 | Pico Micro-B コネクタを USB ホストとして使うため |
| **USB キーボード（HID Keyboard クラス、テンキー付き推奨）** | 1 以上 | TC-508 のテンキー検証に使用 |

> **注**: テンキーなしキーボードを使用する場合、TC-513〜TC-515 はスキップ可。
>
> Pico の Micro-B コネクタを USB ホストとして使う場合、外部回路または OTG アダプタで VBUS（5V）をデバイス側に給電する仕組みが必要。詳細は参考資料「USB 簡単ホスト」を参照してください。

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
git checkout feature/13-phase5-modifier-keys
# PRマージ後は main ブランチでそのまま検証可能
```

> 既にリポジトリを clone 済みの場合は `git fetch` を実行する。PR マージ前は続けて `git checkout feature/13-phase5-modifier-keys` を実行する。

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
KKBD-USB v0.1 (Phase 5) - Waiting for USB keyboard...
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
| TC-501 | Shift+`a` から Shift+`z` まで順に入力（Caps Lock OFF） | シリアル端末に `ABCDEFGHIJKLMNOPQRSTUVWXYZ` が表示される |
| TC-502 | Shift+`1` から Shift+`0` まで順に入力 | シリアル端末に `!@#$%^&*()` が表示される |
| TC-503 | 記号キー（Shift なし）`-`, `=`, `[`, `]`, `\`, `;`, `'`, `` ` ``, `,`, `.`, `/` を順に入力 | 該当記号がそのまま表示される |
| TC-504 | Shift+記号キー（同上）を順に入力 | `_`, `+`, `{`, `}`, `\|`, `:`, `"`, `~`, `<`, `>`, `?` が表示される |
| TC-505 | Ctrl+`a` を押す | 制御文字 0x01（SOH）が送信される |
| TC-506 | Ctrl+`c` を押す | 制御文字 0x03（ETX）が送信される |
| TC-507 | Ctrl+`m` を押す | 制御文字 0x0D（CR）が送信される |
| TC-508 | Ctrl+`[` を押す | 制御文字 0x1B（ESC）が送信される |
| TC-509 | Esc キーを押す | 0x1B が送信される |
| TC-510 | Tab キーを押す | 0x09 が送信される |
| TC-511 | Space キーを押す | 0x20 が送信される（スペース文字） |
| TC-512 | Backspace キーを押す | 0x08 が送信される（BS） |
| TC-513 | テンキー数字 1〜9 および 0 を押す（NumLock ON） | 該当数字が表示される |
| TC-514 | テンキー `.` を押す | `.` が表示される |
| TC-515 | テンキー Enter を押す | 行末コードが送信される（メイン Enter と同じ動作） |
| TC-516 | Ctrl+Shift+`a` を押す | 制御文字 0x01（Ctrl 優先、Shift は無視） |

**各 TC の補足:**

- **TC-501**: Caps Lock が ON になっていないことを事前に確認すること。
- **TC-502〜504**: キーボードの地域設定（JIS / US 配列）によって記号の位置が異なる場合がある。US 配列を前提とした期待値となっている。
- **TC-505〜508**: シリアル端末で制御文字が直接表示されないため、ログツールやプログラム経由で受信バイトを確認するのが確実。  
  受信バイト確認方法の例：
  - **macOS**: `screen -L /dev/tty.usbserial-XXXX 9600` で起動 → 同ディレクトリに `screenlog.0` が生成される → `hexdump -C screenlog.0` で確認
  - **Linux**: `stty -F /dev/ttyUSB0 9600 raw -echo && cat /dev/ttyUSB0 > log.bin` で取得 → `hexdump -C log.bin`
  - **Python (クロスプラットフォーム)**: `python3 -m serial.tools.miniterm /dev/ttyUSB0 9600 --hexlify` で受信バイトをリアルタイム HEX 表示
- **TC-507**: Ctrl+M は CR（0x0D）となり、Enter キーと同じバイトが送信される。ターミナルでの区別は困難なため、受信ログでバイト確認を推奨。
- **TC-513〜514**: NumLock ON を事前に確認すること。NumLock OFF の場合はキーボードが方向キーとして報告するため数字として受信されない場合がある。
- **TC-516**: Ctrl と Shift を同時押ししながら `a` を入力。Ctrl が優先されるため 0x01（SOH）が送信される。

---

## 4. 受け入れ条件チェックリスト

Issue #13 の受け入れ条件と対応する確認項目を以下に示す。**すべての項目にチェックが入った場合に Phase 5 合格** とする。

- [ ] **条件 1**: Shift+A〜Z で大文字 A〜Z が UART に送信される（TC-501 合格）
- [ ] **条件 2**: Shift+数字キー・記号キーで対応記号（`!`, `@`, `#`, `$` 等）が UART に送信される（TC-502/TC-504 合格）
- [ ] **条件 3**: Ctrl+A〜Z で制御文字 0x01〜0x1A が UART に送信される（TC-505〜TC-507 合格）
  - 補助 TC: TC-508（Ctrl+`[` → 0x1B）も合格すること（設計書 §5.4 の Ctrl+記号テーブル拡張範囲）
- [ ] **条件 4**: Esc キーで 0x1B が送信される（TC-509 合格）
- [ ] **条件 5**: Tab キーで 0x09 が送信される（TC-510 合格）
- [ ] **条件 6**: Backspace キーで 0x08（BS）が送信される（TC-512 合格）
- [ ] **条件 7**: Space キーで 0x20 が送信される（TC-511 合格）
- [ ] **条件 8**: ホスト側ユニットテスト（`KKBD_PHASE5_DONE=ON`）の 8 関数すべてがパスする
  - `test_keymap_convert_shift_uppercase`
  - `test_keymap_convert_shift_symbols`
  - `test_keymap_convert_symbols_no_shift`
  - `test_keymap_convert_symbols_with_shift`
  - `test_keymap_convert_ctrl_control_chars`
  - `test_keymap_convert_special_keys`
  - `test_keymap_convert_keypad`
  - `test_keymap_convert_priority_ctrl_over_shift`

### ホスト側ユニットテストの実行方法

```sh
cmake -S tests -B build-tests -DKKBD_PHASE4_DONE=ON -DKKBD_PHASE5_DONE=ON
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

---

## 5. 検証結果記録

検証完了後、以下の表に結果を記入して Issue #13 または PR のコメントに貼り付ける。

### 環境情報

| 項目 | 内容 |
|------|------|
| 検証実施日 | （実施日を記入） |
| 検証実施者 | （担当者名を記入） |
| 使用 Pico バリアント | Raspberry Pi Pico（無印、RP2040） |
| OS | （macOS / Linux / Windows を記入） |
| Pico SDK バージョン | 1.5.1 |
| USB-シリアル変換アダプタ | （実機検証時に使用したものを記入） |
| シリアル端末ソフト | （使用ソフトを記入） |
| 使用キーボード | （テンキー付き / なし、型番を記入） |
| USB OTG アダプタ / ケーブル | VBUS 給電パススルー対応 |
| ボーレート | 9600 |
| 行末コードジャンパー設定 | JP1=OPEN, JP2=OPEN → CR |

### TC 別結果

| TC | 操作 | 期待動作 | 結果 | 備考 |
|----|------|---------|------|------|
| TC-501 | Shift+`a`〜`z` 入力（Caps Lock OFF） | `ABCDEFGHIJKLMNOPQRSTUVWXYZ` 表示 | | |
| TC-502 | Shift+`1`〜`0` 入力 | `!@#$%^&*()` 表示 | | |
| TC-503 | 記号キー（Shift なし）11 種入力 | 該当記号そのまま表示 | | |
| TC-504 | Shift+記号キー 11 種入力 | `_+{}|:"~<>?` 表示 | | |
| TC-505 | Ctrl+`a` | 0x01（SOH）送信 | | ログツールで確認 |
| TC-506 | Ctrl+`c` | 0x03（ETX）送信 | | ログツールで確認 |
| TC-507 | Ctrl+`m` | 0x0D（CR）送信 | | Enter と同じバイト |
| TC-508 | Ctrl+`[` | 0x1B（ESC）送信 | | ログツールで確認 |
| TC-509 | Esc キー | 0x1B 送信 | | |
| TC-510 | Tab キー | 0x09 送信 | | |
| TC-511 | Space キー | 0x20 送信 | | |
| TC-512 | Backspace キー | 0x08（BS）送信 | | |
| TC-513 | テンキー数字 1〜9 および 0（NumLock ON） | 該当数字表示 | | テンキー付きキーボードのみ |
| TC-514 | テンキー `.` | `.` 表示 | | テンキー付きキーボードのみ |
| TC-515 | テンキー Enter | 行末コード送信（メイン Enter と同じ） | | テンキー付きキーボードのみ |
| TC-516 | Ctrl+Shift+`a` | 0x01（Ctrl 優先） | | |

### ユニットテスト結果

| テスト関数 | 結果 | 備考 |
|-----------|------|------|
| `test_keymap_convert_shift_uppercase` | | |
| `test_keymap_convert_shift_symbols` | | |
| `test_keymap_convert_symbols_no_shift` | | |
| `test_keymap_convert_symbols_with_shift` | | |
| `test_keymap_convert_ctrl_control_chars` | | |
| `test_keymap_convert_special_keys` | | |
| `test_keymap_convert_keypad` | | |
| `test_keymap_convert_priority_ctrl_over_shift` | | |

### 総合判定

| 項目 | 結果 |
|------|------|
| 受け入れ条件 1（TC-501） | |
| 受け入れ条件 2（TC-502/504） | |
| 受け入れ条件 3（TC-505〜507） | |
| 受け入れ条件 4（TC-509） | |
| 受け入れ条件 5（TC-510） | |
| 受け入れ条件 6（TC-512） | |
| 受け入れ条件 7（TC-511） | |
| 受け入れ条件 8（ユニットテスト 8 関数） | |
| **Phase 5 総合** | |
| 所感・備考 | |
| 添付（写真・動画） | - |

---

## 6. トラブルシューティング

| 症状 | 原因 | 対処 |
|------|------|------|
| Shift+英字で小文字が表示される | Shift テーブル不具合 / Caps Lock の影響 | Caps Lock OFF を確認、ファームウェアの再書き込み |
| Shift+数字が機能しない | キーボードの NumLock または Shift 認識の問題 | 別キーボードで試す、`tuh_hid_set_protocol(BOOT)` の動作確認 |
| Ctrl+英字で何も送信されない / 大文字が送信される | Ctrl 優先度ロジック不具合 | keymap_convert の優先度（Ctrl > Shift）を確認 |
| 記号キーで何も送信されない | 通常テーブルのエントリ不足 | 設計書 §5.2 と `s_normal_table` を照合 |
| テンキー数字が送信されない | NumLock OFF / キーボード固有の挙動 | NumLock ON を確認、別キーボードで試す |
| Ctrl+m と Enter の区別がつかない | 仕様通り（両方 0x0D） | 受信ログで送信タイミングを比較 |
| ホスト側テストがリンクエラー | `KKBD_PHASE4_DONE` / `KKBD_PHASE5_DONE` 未定義 | 両方を `-D` で指定するか cmake で `-DKKBD_PHASE5_DONE=ON` |
| `Compatibility with CMake < 3.5...` | CMake 4.x 互換性問題 | `scripts/build.sh` 経由でビルドする |
| `RPI-RP2` ボリュームが現れない | BOOTSEL を押さずに接続 / 充電専用ケーブル使用 | BOOTSEL を押しながら接続したか確認。USB ケーブルがデータ通信対応か確認 |
| `PICO_SDK_PATH` が設定されていない旨のエラー | 環境変数未設定 | `export PICO_SDK_PATH=/path/to/pico-sdk` を実施 |

---

## 7. 関連ドキュメント

| ドキュメント | 参照目的 |
|------------|---------|
| [`README.md`](../../README.md) | 開発環境構築・ビルド方法の詳細 |
| [`docs/design/設計書.md`](../design/設計書.md) | §5.1〜§5.5（テーブル仕様）、§5.2（HID Usage ID 変換テーブル）、§5.3（Shift テーブル）、§5.4（Ctrl テーブル） |
| [`docs/design/実装計画.md`](../design/実装計画.md) | Phase 5 の実装詳細・受け入れ条件 |
| [`docs/design/テスト戦略.md`](../design/テスト戦略.md) | L1（keymap テスト網羅）方針 |
| [`docs/requirements/要件定義.md`](../requirements/要件定義.md) | FR-003（ASCII コード変換） |
| [`docs/tests/phase1_実機検証手順.md`](phase1_実機検証手順.md) | Phase 1 検証手順（スタイル参考） |
| [`docs/tests/phase2_実機検証手順.md`](phase2_実機検証手順.md) | Phase 2 検証手順（ジャンパー設定・UART トラブルシューティング） |
| [`docs/tests/phase3_実機検証手順.md`](phase3_実機検証手順.md) | Phase 3 検証手順（USB ホスト接続確認） |
| [`docs/tests/phase4_実機検証手順.md`](phase4_実機検証手順.md) | Phase 4 検証手順（基本キー入力確認） |
| 参考: USB 簡単ホスト | https://q61.org/blog/2021/06/09/easyusbhost/ |

---

## 8. 次のステップ

Phase 5 の検証がすべての受け入れ条件で **合格** となった場合、Issue #14（Phase 6: 行末コード・キーリピート・LED）の実装・検証に進む。

検証結果に **不合格** が含まれる場合は、Issue #13 にトラブルシューティング結果を記録し、担当者と対処方針を協議する。
