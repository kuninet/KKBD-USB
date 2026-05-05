# Phase 3 実機検証手順書

| 項目 | 内容 |
|------|------|
| 文書番号 | KKBD-USB-VERIF-PHASE3-001 |
| 作成日 | 2026-05-05 |
| 対応 Issue | #11 (Phase 3: USBホスト基盤) |
| 対応 Phase | Phase 3 |
| 対象 FR/NFR | FR-001（部分・接続検出） |

---

## 1. はじめに

### 目的

本手順書は Phase 3 完了の判定として、TinyUSB を USB ホストモードで初期化し、USB キーボードの接続/切断時に UART にログが出力されることを確認するための手順を定める。

### 検証対象

- PR マージ前: `feature/11-phase3-usb-host` ブランチ
- PR マージ後: `main` ブランチでそのまま検証可能

---

## 2. 前提条件

### 必要ハードウェア

| ハードウェア | 数量 | 備考 |
|-------------|------|------|
| Raspberry Pi Pico（無印、RP2040） | 1 | Phase 2 で書き込み確認済みのもの |
| USB Micro-B ケーブル | 1 | データ通信対応（書き込み・電源供給用） |
| 開発用 PC | 1 | macOS / Linux / Windows |
| USB-シリアル変換アダプタ | 1 | 3.3V TTL 対応のもの（FT232RL, CP2102, CH340 など） |
| ジャンパーケーブル または ブレッドボード | 適量 | Pico GPIO とアダプタ接続用 |
| **USB OTG アダプタ または USB-A ↔ Micro-B（OTG）ケーブル** | 1 | Pico Micro-B コネクタを USB ホストとして使うため |
| **USB キーボード（HID Keyboard クラス）** | 1 以上 | 接続検出確認用 |
| （任意）USB マウス | 1 | 非キーボードフィルタ確認用（TC-305） |

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
git checkout feature/11-phase3-usb-host
# PRマージ後は main ブランチでそのまま検証可能
```

> 既にリポジトリを clone 済みの場合は `git fetch` を実行する。PR マージ前は続けて `git checkout feature/11-phase3-usb-host` を実行する。

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

2. USB OTG アダプタを Pico の Micro-B コネクタに接続する（USB キーボード接続の準備）

3. アダプタを PC に接続し、認識されたシリアルデバイスを確認する

   | OS | デバイス例 |
   |----|-----------|
   | macOS | `/dev/tty.usbserial-XXXX` 等（`ls /dev/tty.usb*` で確認） |
   | Linux | `/dev/ttyUSB0` 等（`ls /dev/ttyUSB*` で確認） |
   | Windows | `COM3` 等（デバイスマネージャーで確認） |

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

### Step 5: シリアル端末で起動バナー確認

シリアル端末を **9600 bps（または設定したボーレート）** で開く。

| OS | コマンド例 |
|----|-----------|
| macOS | `screen /dev/tty.usbserial-XXXX 9600` |
| Linux | `minicom -D /dev/ttyUSB0 -b 9600` |
| Windows | TeraTerm: シリアル設定でボーレートを選択 |

電源投入後、以下のメッセージが表示されることを確認する：

```
[USB] TinyUSB host initialized
KKBD-USB v0.1 (Phase 3) - Waiting for USB keyboard...
```

> **注**: 出力順序は `usb_host_init()` → `main()` の呼び出し順を反映している（先に USB 初期化ログ、次に起動バナー）。

> **注**: シリアル端末は電源投入前に開いておくと、起動直後のバナーを取りこぼさずに確認できる。

---

### Step 6: 各 TC 実施

**検証パターン一覧:**

| TC | 操作 | 期待ログ・動作 |
|----|------|--------------|
| TC-301 | 起動時にキーボード未接続 | 起動バナー表示後、Pico がクラッシュせず接続待ち状態を維持 |
| TC-302 | USB キーボード接続 | `[USB] Keyboard connected (addr=N, instance=M)` |
| TC-303 | USB キーボード切断 | `[USB] Keyboard disconnected` |
| TC-304 | 抜き差しを 5 回繰り返し | 毎回 connected / disconnected ログが正常に表示される |
| TC-305（任意） | USB マウス接続 | `[USB] Non-keyboard HID ignored (proto=0 または proto=2)` 等が出力され、Keyboard connected ログは出ない |

> **注 (TC-305)**: 標準 HID マウスは `proto=2` で認識されるが、一部のゲーミングマウスは Generic HID として `proto=0` で認識される場合がある。いずれの値でも `Keyboard connected` ログが出力されなければ合格。

**各 TC の手順:**

1. Step 5 の起動バナーを確認した状態からスタート（TC-301）
2. TC-302: USB OTG アダプタ経由でキーボードを接続し、ログを確認する
3. TC-303: キーボードを抜き、ログを確認する
4. TC-304: TC-302 / TC-303 の手順を 5 回繰り返し、ログが毎回正常に表示されることを確認する
5. TC-305（任意）: マウスを USB OTG アダプタ経由で接続し、Keyboard connected ログが出ないことを確認する

---

## 4. 受け入れ条件チェックリスト

Issue #11 の受け入れ条件と対応する確認項目を以下に示す。**すべての項目にチェックが入った場合に Phase 3 合格** とする。

- [ ] **条件 1**: USB キーボードを接続すると、シリアル端末に接続ログが表示される（TC-302 合格）
- [ ] **条件 2**: USB キーボードを抜くと、シリアル端末に切断ログが表示される（TC-303 合格）
- [ ] **条件 3**: キーボード未接続でも Pico がクラッシュせず、接続待ち状態を維持する（TC-301 合格）

---

## 5. 検証結果記録

検証完了後、以下の表に結果を記入して Issue #11 または PR のコメントに貼り付ける。

### 環境情報

| 項目 | 内容 |
|------|------|
| 検証実施日 | （記入） |
| 検証実施者 | （記入） |
| 使用 Pico バリアント | Raspberry Pi Pico（無印、RP2040）|
| OS | （記入） |
| Pico SDK バージョン | 1.5.1 |
| USB-シリアル変換アダプタ | （実機検証時に使用したもの）|
| シリアル端末ソフト | （記入）|
| 使用キーボード | （記入）|
| USB OTG アダプタ / ケーブル | （記入）|

### TC 別結果

| TC | 操作 | 期待ログ・動作 | 結果 | 備考 |
|----|------|--------------|------|------|
| TC-301 | 起動時にキーボード未接続 | Pico がクラッシュせず接続待ち状態を維持 | （合格 / 不合格） | （記入） |
| TC-302 | USB キーボード接続 | `[USB] Keyboard connected (addr=N, instance=M)` | （合格 / 不合格） | （記入） |
| TC-303 | USB キーボード切断 | `[USB] Keyboard disconnected` | （合格 / 不合格） | （記入） |
| TC-304 | 抜き差しを 5 回繰り返し | 毎回 connected / disconnected ログが正常に表示 | （合格 / 不合格） | 試行回数: （記入）|
| TC-305（任意） | USB マウス接続 | Keyboard connected ログが出ない（`proto=0` または `proto=2` で無視ログが出る） | （合格 / 不合格 / 未実施） | （記入） |

### 総合判定

| 項目 | 結果 |
|------|------|
| 受け入れ条件 1（TC-302） | （合格 / 不合格） |
| 受け入れ条件 2（TC-303） | （合格 / 不合格） |
| 受け入れ条件 3（TC-301） | （合格 / 不合格） |
| **Phase 3 総合** | （合格 / 不合格） |
| 所感・備考 | （記入） |
| 添付（写真・動画） | - |

---

## 6. トラブルシューティング

| 症状 | 原因 | 対処 |
|------|------|------|
| `tuh_init failed` ログ | TinyUSB 初期化失敗 | `tusb_config.h` の設定を確認、Pico SDK バージョン v1.5.1 以上を使用 |
| キーボード接続してもログ無し | OTG/VBUS 給電不足 | OTG アダプタは VBUS 給電パススルー機能のあるものを使用する。Pico の Micro-B コネクタは標準では VBUS が「入力」のため、キーボードへ 5V を供給するには (a) 給電機能付き OTG ケーブル、(b) Pico VBUS ピンへの外部 5V 給電、(c) 専用 USB ホスト変換回路 のいずれかが必要 |
| 接続ログ後にすぐ切断ログ | 過電流 / VBUS 不安定 | 別キーボードで試す、外部給電を検討する |
| マウス接続でも何も出ない | フィルタが正しく機能 | 期待通りの動作（TC-305 合格） |
| `Compatibility with CMake < 3.5...` | CMake 4.x 互換性問題 | `scripts/build.sh` 経由でビルドする |
| 起動バナー出ない | UART ボーレート不一致 / 電源問題 | Phase 2 検証手順書のトラブルシューティング参照 |
| `RPI-RP2` ボリュームが現れない | BOOTSEL を押さずに接続 / 充電専用ケーブル使用 | BOOTSEL を押しながら接続したか確認。USB ケーブルがデータ通信対応か確認 |
| `PICO_SDK_PATH` が設定されていない旨のエラー | 環境変数未設定 | `export PICO_SDK_PATH=/path/to/pico-sdk` を実施 |
| `arm-none-eabi-gcc` が見つからない | toolchain 未インストール | toolchain をインストール（README 参照） |

---

## 7. 関連ドキュメント

| ドキュメント | 参照目的 |
|------------|---------|
| [`README.md`](../../README.md) | 開発環境構築・ビルド方法の詳細 |
| [`docs/design/設計書.md`](../design/設計書.md) | §3.3（usb_host）、§6.2/6.5（USB シーケンス）、§7.1（状態遷移）、§11.2（tusb_config） |
| [`docs/design/実装計画.md`](../design/実装計画.md) | Phase 3 の実装詳細・受け入れ条件 |
| [`docs/design/テスト戦略.md`](../design/テスト戦略.md) | L4（実機テスト）方針 |
| [`docs/requirements/要件定義.md`](../requirements/要件定義.md) | FR-001（USB キーボード接続検出） |
| [`docs/tests/phase1_実機検証手順.md`](phase1_実機検証手順.md) | Phase 1 検証手順（スタイル参考・前提条件） |
| [`docs/tests/phase2_実機検証手順.md`](phase2_実機検証手順.md) | Phase 2 検証手順（スタイル参考・前提条件） |
| 参考: USB 簡単ホスト | https://q61.org/blog/2021/06/09/easyusbhost/ |

---

## 8. 次のステップ

Phase 3 の検証がすべての受け入れ条件で **合格** となった場合、Issue #12（Phase 4: 基本キー入力）の実装・検証に進む。

検証結果に **不合格** が含まれる場合は、Issue #11 にトラブルシューティング結果を記録し、担当者と対処方針を協議する。
