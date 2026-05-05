# Phase 6 実機検証手順書

| 項目 | 内容 |
|------|------|
| 文書番号 | KKBD-USB-VERIF-PHASE6-001 |
| 作成日 | 2026-05-05 |
| 対応 Issue | #14 (Phase 6: 行末コード・キーリピート・LED) |
| 対応 Phase | Phase 6 |
| 対象 FR/NFR | FR-004（行末コード）、FR-007（キーリピート）、FR-010（LED インジケータ） |

---

## 1. はじめに

### 目的

本手順書は Phase 6 完了の判定として、行末コード送信、キーリピート（500ms 初回 + 50ms 間隔）、LED 5 状態（BOOT/WAIT/MOUNTED/TX/ERROR）の動作を実機で確認するための手順を定める。

### 検証対象

- PR マージ前: `feature/14-phase6-line-ending-keyrepeat-led` ブランチ
- PR マージ後: `main` ブランチでそのまま検証可能

### スコープ外

- ERROR 状態の意図的な再現（TC-612）は任意とする（`tuh_init` 失敗を強制する手段が存在しないため）
- Phase 5 以前の機能（Shift/Ctrl/記号等）は本 Phase の検証対象外（検証済み）

---

## 2. 前提条件

### 必要ハードウェア

| ハードウェア | 数量 | 備考 |
|-------------|------|------|
| Raspberry Pi Pico（無印、RP2040） | 1 | Phase 5 で書き込み確認済みのもの |
| USB Micro-B ケーブル | 1 | データ通信対応（書き込み・電源供給用） |
| 開発用 PC | 1 | macOS / Linux / Windows |
| USB-シリアル変換アダプタ | 1 | 3.3V TTL 対応のもの（FT232RL, CP2102, CH340 など） |
| ジャンパーケーブル または ブレッドボード | 適量 | Pico GPIO とアダプタ接続用、JP1/JP2 切替用 |
| **USB OTG アダプタ または USB-A ↔ Micro-B（OTG）ケーブル** | 1 | Pico Micro-B コネクタを USB ホストとして使うため |
| **USB キーボード（HID Keyboard クラス）** | 1 以上 | TC-604〜TC-611 のリピート・行末コード検証に使用 |

> Pico の Micro-B コネクタを USB ホストとして使う場合、外部回路または OTG アダプタで VBUS（5V）をデバイス側に給電する仕組みが必要。詳細は参考資料「USB 簡単ホスト」を参照してください。

### 接続図（テキスト）

```
Pico GPIO 0 (UART0 TX) ─── USB-シリアル RX
Pico GND               ─── USB-シリアル GND

Pico Micro-B コネクタ  ─── USB OTG アダプタ ─── USB キーボード

Pico GPIO 10 (JP1) ─── ジャンパーピン（OPEN / SHORT 切替）
Pico GPIO 11 (JP2) ─── ジャンパーピン（OPEN / SHORT 切替）
```

> **注意**: 配線は **電源を入れる前**（Pico を USB 接続する前）に行うこと。ジャンパー設定変更は必ず **電源を切った状態** で行い、変更後に再起動して設定を読み取らせること。

### 必要ソフトウェア

| ソフトウェア | バージョン | 備考 |
|------------|-----------|------|
| Pico SDK | v1.5.1 以上 | 環境変数 `PICO_SDK_PATH` 設定済みであること |
| `arm-none-eabi-gcc` | 10 以上 | ARM クロスコンパイラ |
| `cmake` | 3.13 以上 | ビルドシステム生成ツール |
| `ninja` | 最新安定版 | 推奨（`make` でも可） |
| `git` | 2.x 以上 | リポジトリ取得用 |
| **シリアル端末ソフト** | - | macOS: `screen` または `minicom`、Linux: `minicom`、Windows: TeraTerm 等 |
| **python3-serial**（任意） | - | `python3 -m serial.tools.miniterm` で 16 進表示確認に使用 |

各ツールのインストール方法は [`README.md`](../../README.md) の「開発環境」セクションを参照。

---

## 3. 検証手順

### Step 1: リポジトリ取得とブランチ切替

```sh
git clone https://github.com/kuninet/KKBD-USB.git
cd KKBD-USB

# PRマージ前のみ：feature ブランチをチェックアウト
git checkout feature/14-phase6-line-ending-keyrepeat-led
# PRマージ後は main ブランチでそのまま検証可能
```

> 既にリポジトリを clone 済みの場合は `git fetch` を実行する。PR マージ前は続けて `git checkout feature/14-phase6-line-ending-keyrepeat-led` を実行する。

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

2. 行末コード検証用ジャンパーを設定する（Step 5 の各 TC に合わせて変更）

   | TC | JP1 | JP2 | 行末コード |
   |----|-----|-----|-----------|
   | TC-607 | OPEN | OPEN | CR（0x0D） |
   | TC-608 | SHORT | OPEN | LF（0x0A） |
   | TC-609 | OPEN | SHORT | CRLF（0x0D 0x0A） |

3. USB OTG アダプタを Pico の Micro-B コネクタに接続する（USB キーボード接続の準備）

4. アダプタを PC に接続し、認識されたシリアルデバイスを確認する

   | OS | デバイス例 |
   |----|-----------|
   | macOS | `/dev/tty.usbserial-XXXX` 等（`ls /dev/tty.usb*` で確認） |
   | Linux | `/dev/ttyUSB0` 等（`ls /dev/ttyUSB*` で確認） |
   | Windows | `COM3` 等（デバイスマネージャーで確認） |

5. Pico への書き込み

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
KKBD-USB v0.1 (Phase 6) - Waiting for USB keyboard...
```

> **注**: バナーに `(Phase 6)` が表示されることで、正しいファームウェアが書き込まれていることを確認する。シリアル端末は電源投入前に開いておくと、起動直後のバナーを取りこぼさずに確認できる。

LED の状態確認：
- 起動直後: BOOT 状態で点灯
- `[USB] TinyUSB host initialized` 表示後: WAIT_DEVICE 状態に遷移し、低速点滅（約 500ms トグル）に切替

次に USB OTG アダプタ経由で USB キーボードを接続し、以下のログと LED を確認する：

```
[USB] Keyboard connected (addr=N, instance=M)
```

LED が常時点灯（MOUNTED 状態）に切り替わることを確認する。

---

### Step 5: 各 TC の実施

**検証パターン一覧:**

| TC | 操作 | 期待動作 |
|----|------|---------|
| TC-601 | キーボード未接続で起動 | 起動直後 BOOT 点灯 → `[USB] TinyUSB host initialized` 表示後 LED が低速点滅（500ms トグル）に切替 |
| TC-602 | キーボード接続 | LED が常時点灯（MOUNTED） |
| TC-603 | キー押下時の LED | 送信のたびに短時間消灯ではなく「短時間点灯維持→復帰」が観察される（連続送信中はほぼ点灯維持） |
| TC-604 | `a` を 3 秒押し続ける | 約 500ms 経過後にリピート開始、約 50ms 間隔で `a` が連投される |
| TC-605 | リピート中にキーを離す | リピートが即座に停止する |
| TC-606 | `a` を押しっぱなしの状態で `b` を追加で押す | `b` が新規キーダウンとして 1 文字送信される。`a` のリピートは停止し、`b` のリピートは新たに 500ms 初回遅延を待ってから 50ms 間隔で開始される。 |
| TC-607 | JP1=OPEN, JP2=OPEN で起動し Enter 押下 | `0x0D`（CR）のみ送信 |
| TC-608 | JP1=SHORT, JP2=OPEN で起動し Enter 押下 | `0x0A`（LF）のみ送信 |
| TC-609 | JP1=OPEN, JP2=SHORT で起動し Enter 押下 | `0x0D 0x0A`（CRLF）送信 |
| TC-610 | キーボード切断 | LED が WAIT_DEVICE に戻り低速点滅再開 |
| TC-611 | Enter キー長押し | Enter は連投されず 1 回のみ行末コード送信（リピート対象外） |
| TC-612（任意） | ERROR 状態の意図的再現 | `tuh_init` 失敗を強制する手段がないため、コードレビューで状態遷移を確認するか、TC-602 接続後に擬似的に切断+ノイズ等で観察する |

**各 TC の補足:**

- **TC-603**: LED 観察は短時間（30ms）のため目視判別困難。長押しでリピート発火中の連続点灯で確認可能。
- **TC-604〜606**: リピート速度（TC-604）は受信ログを `screen -L` で残し、`hexdump -C screenlog.0` で `61 61 61 ...` の連続を確認するのが確実。
- **TC-607〜609**: 行末コードは `python3 -m serial.tools.miniterm /dev/tty.usbserial-XXXX 9600 -f hexlify` で 16 進確認推奨。ジャンパー設定変更は **電源を切った状態** で行い、変更後に再書き込みまたは再起動すること。
- **TC-611**: Enter キーが連投されないことは、リピート登録の除外実装確認。単発押しと長押しの両方で 1 回しか行末コードが送信されないことを確認する。

---

## 4. 受け入れ条件チェックリスト

Issue #14 の受け入れ条件と対応する確認項目を以下に示す。**すべての項目にチェックが入った場合に Phase 6 合格** とする。

- [ ] **条件 1**: JP1/JP2 が OPEN/OPEN の場合、Enter で CR（0x0D）が送信される（TC-607）
- [ ] **条件 2**: JP1 が SHORT の場合、Enter で LF（0x0A）が送信される（TC-608）
- [ ] **条件 3**: JP2 が SHORT の場合、Enter で CRLF（0x0D 0x0A）が送信される（TC-609）
- [ ] **条件 4**: キーを押し続けると、約 500ms 後から約 50ms 間隔でキーリピートが動作する（TC-604）
- [ ] **条件 5**: キーボード未接続時に LED が点灯（待ち受け状態）する（TC-601、本実装では低速点滅）
- [ ] **条件 6**: キーボード接続時に LED が点灯（接続済み状態）する（TC-602）
- [ ] **条件 7**: UART 送信のたびに LED が短時間点滅する（TC-603）

> **注**: 受け入れ条件 5 では Issue 文言「点灯」だが、本実装は「**低速点滅**」（設計書 §7.2 LED 状態遷移）。仕様詳細化として記録。

### ホスト側ユニットテストの実行方法

```sh
cmake -S tests -B build-tests -DKKBD_PHASE4_DONE=ON -DKKBD_PHASE5_DONE=ON -DKKBD_PHASE6_DONE=ON
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

`test_keyrepeat` の 6 関数全パスを確認（`keyrepeat_decide` 純粋関数のテスト）。

---

## 5. 検証結果記録

検証完了後、以下の表に結果を記入して Issue #14 または PR のコメントに貼り付ける。

### 環境情報

| 項目 | 内容 |
|------|------|
| 検証実施日 | （実施日を記入） |
| 検証実施者 | （実施者を記入） |
| 使用 Pico バリアント | Raspberry Pi Pico（無印、RP2040） |
| OS | （macOS / Linux / Windows） |
| Pico SDK バージョン | 1.5.1 |
| USB-シリアル変換アダプタ | （実機検証時に使用したもの） |
| シリアル端末ソフト | （使用したソフトウェアを記入） |
| 使用キーボード | （使用したキーボードを記入） |
| USB OTG アダプタ / ケーブル | VBUS 給電パススルー対応 |
| ボーレート | 9600 |
| 行末コードジャンパー設定 | （TC ごとに変更） |

### TC 別結果

| TC | 操作 | 期待動作 | 結果 | 備考 |
|----|------|---------|------|------|
| TC-601 | キーボード未接続で起動 | BOOT 点灯 → 低速点滅切替 | | |
| TC-602 | キーボード接続 | LED 常時点灯（MOUNTED） | | |
| TC-603 | キー押下時の LED | 短時間点灯維持→復帰 | | |
| TC-604 | `a` を 3 秒押し続ける | 約 500ms 後リピート開始、約 50ms 間隔で連投 | | |
| TC-605 | リピート中にキーを離す | リピート即座に停止 | | |
| TC-606 | `a` 押しっぱなし中に `b` 追加押下 | `b` が 1 文字送信、`a` のリピート停止、`b` が 500ms 遅延後 50ms 間隔でリピート開始 | | |
| TC-607 | JP1=OPEN, JP2=OPEN で Enter | 0x0D（CR）のみ送信 | | |
| TC-608 | JP1=SHORT, JP2=OPEN で Enter | 0x0A（LF）のみ送信 | | |
| TC-609 | JP1=OPEN, JP2=SHORT で Enter | 0x0D 0x0A（CRLF）送信 | | |
| TC-610 | キーボード切断 | WAIT_DEVICE に戻り低速点滅再開 | | |
| TC-611 | Enter キー長押し | 1 回のみ行末コード送信 | | |
| TC-612（任意） | ERROR 状態の意図的再現 | コードレビューで状態遷移確認 | | |

### ユニットテスト結果

| テスト関数 | 結果 | 備考 |
|-----------|------|------|
| `test_keyrepeat` 全 6 関数 | | `keyrepeat_decide` 純粋関数テスト |

### 総合判定

| 項目 | 結果 |
|------|------|
| 受け入れ条件 1（TC-607） | |
| 受け入れ条件 2（TC-608） | |
| 受け入れ条件 3（TC-609） | |
| 受け入れ条件 4（TC-604） | |
| 受け入れ条件 5（TC-601） | |
| 受け入れ条件 6（TC-602） | |
| 受け入れ条件 7（TC-603） | |
| **Phase 6 総合** | |
| 所感・備考 | |
| 添付（写真・動画） | - |

---

## 6. トラブルシューティング

| 症状 | 原因 | 対処 |
|------|------|------|
| LED が常時点滅し続ける（接続後も） | LED 状態が WAIT_DEVICE のまま | `usb_host_init` / mount コールバックが正常呼出されているか UART ログで確認 |
| キーリピートが効かない | `keyrepeat_register` が呼ばれていない / `keyrepeat_task` が呼ばれていない | `usb_host_task` → `tuh_task` のチェーン、main ループの `keyrepeat_task()` 呼出を確認 |
| キーリピートが止まらない | 全キーリリース検出ロジック不具合 | `usb_host.c` の `any_repeatable_key` 判定を確認 |
| Enter が連投される | Enter リピート対象外実装の不具合 | `tuh_hid_report_received_cb` で `is_enter` のときに `register` を呼んでいないか確認 |
| LED 送信時点滅が見えない | 送信間隔が早すぎて点灯維持に見える | 単発のキーで確認、または `screen -L` ログで送信タイミング確認 |
| 行末コード CR/LF が逆 | ジャンパー実装ミス | `docs/tests/phase2_実機検証手順.md` で再確認 |
| 起動バナーが `(Phase 5)` のまま | 古いファームウェアが書き込まれている | ビルドを `--clean` 付きで再実行し、改めて書き込む |
| `RPI-RP2` ボリュームが現れない | BOOTSEL を押さずに接続 / 充電専用ケーブル使用 | BOOTSEL を押しながら接続したか確認。USB ケーブルがデータ通信対応か確認 |
| `PICO_SDK_PATH` が設定されていない旨のエラー | 環境変数未設定 | `export PICO_SDK_PATH=/path/to/pico-sdk` を実施 |
| `Compatibility with CMake < 3.5...` | CMake 4.x 互換性問題 | `scripts/build.sh` 経由でビルドする |

---

## 7. 関連ドキュメント

| ドキュメント | 参照目的 |
|------------|---------|
| [`README.md`](../../README.md) | 開発環境構築・ビルド方法の詳細 |
| [`docs/design/設計書.md`](../design/設計書.md) | §3.7（led）、§3.8（keyrepeat）、§7.2（LED 状態遷移）、§9.1（タイミング定数） |
| [`docs/design/実装計画.md`](../design/実装計画.md) | Phase 6 の実装詳細・受け入れ条件 |
| [`docs/design/テスト戦略.md`](../design/テスト戦略.md) | L1（keyrepeat 含む）方針 |
| [`docs/requirements/要件定義.md`](../requirements/要件定義.md) | FR-004（行末コード）、FR-007（キーリピート）、FR-010（LED インジケータ） |
| [`docs/tests/phase1_実機検証手順.md`](phase1_実機検証手順.md) | Phase 1 検証手順（スタイル参考） |
| [`docs/tests/phase2_実機検証手順.md`](phase2_実機検証手順.md) | Phase 2 検証手順（ジャンパー設定・UART トラブルシューティング） |
| [`docs/tests/phase3_実機検証手順.md`](phase3_実機検証手順.md) | Phase 3 検証手順（USB ホスト接続確認） |
| [`docs/tests/phase4_実機検証手順.md`](phase4_実機検証手順.md) | Phase 4 検証手順（基本キー入力確認） |
| [`docs/tests/phase5_実機検証手順.md`](phase5_実機検証手順.md) | Phase 5 検証手順（修飾キー対応確認） |
| 参考: USB 簡単ホスト | https://q61.org/blog/2021/06/09/easyusbhost/ |

---

## 8. 次のステップ

Phase 6 の検証がすべての受け入れ条件で **合格** となった場合、Issue #15（Phase 7: 異常系処理）の実装・検証に進む。

検証結果に **不合格** が含まれる場合は、Issue #14 にトラブルシューティング結果を記録し、担当者と対処方針を協議する。
