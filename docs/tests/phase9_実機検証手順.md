# Phase 9 実機検証手順書

| 項目 | 内容 |
|------|------|
| 文書番号 | KKBD-USB-VERIF-PHASE9-001 |
| 作成日 | 2026-05-08 |
| 対応 Issue | #31 |
| 対応 Phase | Phase 9 |
| 対象機能 | UART1 モニタパススルー / ログ振替 / ローカルエコー（JP5） |

---

## 1. はじめに

### 目的

本手順書は Phase 9 完了の判定として、以下の機能を実機で確認するための手順を定める。

- **Plan A（UART1 パススルー）**: SBC の UART 応答を UART0 RX（GPIO 1）で受信し、UART1 TX（GPIO 4）経由で PC ターミナルへ素通しする。
- **ログ振替**: 起動バナー・USB 接続ログ等の制御ログが UART0（SBC 側）には出力されず、UART1（モニタ側）のみに出力されることを確認する。
- **JP5 ローカルエコー**: JP5（GPIO 14）の OPEN/SHORT 設定により、キー入力の UART1 側エコーが ON/OFF 切替できることを確認する。

### スコープ外

- Phase 1〜6 までの機能（行末コード・キーリピート・LED・修飾キー等）は本 Phase の検証対象外（検証済み）。
- UART1 RX（GPIO 5）は使用しない（モニタは送信専用）。

---

## 2. 前提条件

### 必要ハードウェア

| ハードウェア | 数量 | 備考 |
|-------------|------|------|
| Raspberry Pi Pico（無印、RP2040） | 1 | Phase 9 ファームウェアが書き込み済みのもの |
| USB Micro-B ケーブル | 1 | データ通信対応（書き込み・電源供給用） |
| 開発用 PC | 1 | macOS / Linux / Windows |
| **USB-Serial 変換アダプタ（モニタ用、必須）** | **1** | **3.3V TTL 対応（FT232RL / CP2102 / CH340 等）。UART1 TX（GPIO 4）→ 変換器 RX に接続** |
| USB-Serial 変換アダプタ（SBC 疑似用、任意） | 1 | SBC の代わりに UART0 TX（GPIO 0）を受信確認する場合に使用 |
| SBC（SBC6800 / KZ80 等） または 疑似 SBC | 1 | UART0 TX（GPIO 0）への受信確認用。別の USB-Serial 変換器を UART0 TX に繋いで疑似 SBC として代用可 |
| USB OTG アダプタ または USB-A ↔ Micro-B（OTG）ケーブル | 1 | Pico Micro-B コネクタを USB ホストとして使うため |
| USB キーボード（HID Keyboard クラス） | 1 以上 | すべての TC に使用 |
| ジャンパーケーブル または ブレッドボード | 適量 | GPIO と変換器接続用・JP1〜JP5 切替用 |

### 接続図（テキスト）

```
Pico GPIO 0 (UART0 TX)  ───── SBC UART RX または疑似 SBC（USB-Serial 変換器 RX）
Pico GPIO 1 (UART0 RX)  ───── SBC UART TX（SBC 応答を受信）
Pico GPIO 4 (UART1 TX)  ───── USB-Serial 変換器（モニタ用） RX
Pico GND                ───── 上記変換器 GND（GND 共通必須）

Pico Micro-B コネクタ   ───── USB OTG アダプタ ───── USB キーボード

Pico GPIO 10 (JP1) ─── ジャンパーピン（OPEN / SHORT 切替）
Pico GPIO 11 (JP2) ─── ジャンパーピン（OPEN / SHORT 切替）
Pico GPIO 12 (JP3) ─── ジャンパーピン（OPEN / SHORT 切替）
Pico GPIO 13 (JP4) ─── ジャンパーピン（OPEN / SHORT 切替）
Pico GPIO 14 (JP5) ─── ジャンパーピン（OPEN / SHORT 切替、Phase 9 新規）
```

> **注意**: 配線は **電源を入れる前**（Pico を USB 接続する前）に行うこと。ジャンパー設定変更は必ず **電源を切った状態** で行い、変更後に再起動して設定を読み取らせること。

> UART1 モニタ変換器のロジックレベルは **3.3V** に設定すること（Pico GPIO は 3.3V 出力）。

### 必要ソフトウェア

| ソフトウェア | バージョン | 備考 |
|------------|-----------|------|
| Pico SDK | v1.5.1 以上 | 環境変数 `PICO_SDK_PATH` 設定済みであること |
| `arm-none-eabi-gcc` | 10 以上 | ARM クロスコンパイラ |
| `cmake` | 3.13 以上 | ビルドシステム生成ツール |
| `ninja` | 最新安定版 | 推奨（`make` でも可） |
| `git` | 2.x 以上 | リポジトリ取得用 |
| **シリアル端末ソフト（2 系統）** | - | macOS: `screen` または `minicom`、Linux: `minicom`、Windows: TeraTerm 等。UART0（SBC 側）と UART1（モニタ側）で 2 ウィンドウ同時に開く |
| `python3-serial`（任意） | - | `python3 -m serial.tools.miniterm -f hexlify` で行末コード確認に使用 |

---

## 3. 検証手順

### Step 1: リポジトリ取得とブランチ切替

```sh
git clone https://github.com/kuninet/KKBD-USB.git
cd KKBD-USB

# PR マージ前のみ：feature ブランチをチェックアウト
git checkout feature/31-phase9-uart-monitor
# PR マージ後は main で OK
```

---

### Step 2: ファームウェアビルド

#### 推奨: ビルドスクリプト経由

```sh
export PICO_SDK_PATH=$HOME/pico-sdk    # 実際のパスに置き換える
./scripts/build.sh --clean             # 初回または再ビルド時
```

#### 手動実行する場合

```sh
export PICO_SDK_PATH=$HOME/pico-sdk
export CMAKE_POLICY_VERSION_MINIMUM=3.5
cmake -S . -B build -G Ninja
cmake --build build
```

**期待結果:**

| 確認項目 | 期待値 |
|---------|--------|
| cmake 設定フェーズ | エラーなく完了し `build/` ディレクトリが生成される |
| ビルドフェーズ | エラーなく完了する（警告は許容） |
| 成果物 | `build/src/kkbd_usb.uf2` が生成される |
| バイナリサイズ増加 | Phase 6 ビルド比 +5KB 以内（C-AC-07 相当） |

---

### Step 3: 配線・Pico への書き込み

1. UART1 モニタ用 USB-Serial 変換器を Pico に接続する

   | Pico 側 | USB-Serial 側（モニタ用） |
   |---------|--------------------------|
   | GPIO 4（UART1 TX） | RX |
   | GND | GND |

2. UART0 送受信用の接続（SBC または疑似 SBC）

   | Pico 側 | SBC 側（または疑似 SBC） |
   |---------|-------------------------|
   | GPIO 0（UART0 TX） | UART RX |
   | GPIO 1（UART0 RX） | UART TX |
   | GND | GND |

3. ジャンパーを初期設定にする（TC 実施前）

   | ジャンパー | 初期設定 | 用途 |
   |-----------|---------|------|
   | JP1 | OPEN | 行末コード bit0 |
   | JP2 | OPEN | 行末コード bit1 |
   | JP3 | OPEN | ボーレート bit0 |
   | JP4 | OPEN | ボーレート bit1 |
   | JP5 | **OPEN** | ローカルエコー OFF（Phase 9 新規） |

4. Pico への書き込み

   1. Pico の **BOOTSEL ボタン** を **押し続けたまま** USB ケーブルで PC に接続する
   2. `RPI-RP2` ボリュームが表示されることを確認する
   3. BOOTSEL ボタンを離す
   4. `build/src/kkbd_usb.uf2` を `RPI-RP2` にコピーする

      | OS | コマンド / 操作 |
      |----|----------------|
      | macOS | `cp build/src/kkbd_usb.uf2 /Volumes/RPI-RP2/` |
      | Linux | `cp build/src/kkbd_usb.uf2 /media/$USER/RPI-RP2/` |
      | Windows | エクスプローラで `RPI-RP2` ドライブへドラッグ＆ドロップ |

   5. コピー完了後、Pico は自動的に再起動する

---

### Step 4: シリアル端末 2 系統の起動

**UART1 モニタ端末**（必須、Phase 9 検証の主役）を先に開く。

| OS | コマンド例 |
|----|-----------|
| macOS | `screen /dev/tty.usbserial-XXXX 9600`（モニタ用変換器のデバイス名） |
| Linux | `minicom -D /dev/ttyUSB1 -b 9600`（モニタ用） |
| Windows | TeraTerm: モニタ用変換器のポートを選択、9600 bps |

**UART0 端末**（SBC 側または疑似 SBC 確認用、任意）も必要に応じて開く。

---

### Step 5: 各 TC の実施

---

## 4. テストケース

### テストケース一覧

| TC | 内容 | 前提・操作 | 期待結果 |
|----|------|-----------|---------|
| TC-901 | 起動バナーが UART1 に出る・UART0 に出ない | Pico を電源投入（JP1〜JP5 全 OPEN） | UART1 端末に `KKBD-USB v0.1 (Phase 9) - UART monitor enabled, waiting for USB keyboard...\r\n` が表示される。UART0 端末には何も出ない |
| TC-902 | USB キーボード接続ログが UART1 に出る・UART0 に出ない | USB キーボードを OTG 経由で接続 | UART1 端末に `[USB] Keyboard connected (addr=N, instance=M)\r\n` が表示される。UART0 端末には出ない |
| TC-903 | キー入力時、UART0 に ASCII 送信（既存機能のリグレッション確認） | JP5=OPEN のまま、キー `a` を押す | UART0 側（SBC 側）に `a`（0x61）が受信される。UART1 にはエコーが出ない（JP5=OPEN） |
| TC-904 | SBC 応答が UART1 経由で PC に表示 | 疑似 SBC から UART0 RX（GPIO 1）に `hello\r\n` のバイト列を送信する | UART1 端末に `hello\r\n` が表示される（無加工パススルー確認） |
| TC-905 | JP5=OPEN: ローカルエコー OFF | JP5 未接続（OPEN）のまま `a` を押す | UART0 に `a`（0x61）が届く。UART1 には SBC 応答のみ表示され、自分のキー入力（`a`）はエコーされない |
| TC-906 | JP5=SHORT: ローカルエコー ON | JP5 を電源 OFF 状態で GND（Pin 18 または 23）に SHORT 接続後、電源再投入して `a` を押す | UART0 と UART1 の両方に `a`（0x61）が送信される |
| TC-907 | JP5=SHORT で Enter 押下時のエコー | JP5=SHORT の状態で Enter を押す | UART0 と UART1 の両方に行末コード（JP1/JP2 設定通り：初期 CR = 0x0D）が送信される |
| TC-908 | JP3/JP4 ボーレート変更時の UART1 連動 | JP3=SHORT, JP4=OPEN（19200 bps）に変更して電源再投入。UART1 端末も 19200 bps に設定して再接続 | UART1 端末（19200 bps）で起動バナーが正常に受信できる。旧ボーレート（9600 bps）設定のままでは文字化けする |
| TC-909 | キーボード切断ログが UART1 に出る | USB キーボードを抜く | UART1 端末に `[USB] Keyboard disconnected\r\n` が表示される。UART0 には出ない |
| TC-910 | 連続入力時のドロップ（ベストエフォート確認） | キーを 10 秒間連続押下（例: `a` を押しっぱなし） | UART0・UART1 双方で大半のキャラクタが届く。UART1 TX が満杯時のドロップは許容（ベストエフォート方針）。システムクラッシュや完全停止は不可 |

---

### TC 別補足

- **TC-901 / TC-902 / TC-909**: ログの UART0/UART1 振り分けは本 Phase の核心。UART0 端末（疑似 SBC 側）で何も出ないことを必ず確認する。
- **TC-904**: 疑似 SBC から UART0 RX（GPIO 1）に送る方法として、もう 1 本の USB-Serial 変換器の TX を GPIO 1 に接続し、PC から手動でバイト列を送信する方法が使いやすい。`python3 -m serial.tools.miniterm /dev/ttyUSB0 9600` 等で送信側ターミナルを開き、`hello` と入力する。
- **TC-905 / TC-906**: JP5 設定変更は **必ず電源を切った状態** で行い、変更後に Pico を再接続して再起動させること。
- **TC-907**: 行末コードの確認は `python3 -m serial.tools.miniterm /dev/tty.usbserial-XXXX 9600 -f hexlify` で 16 進表示推奨。`0d`（CR）または設定通りのコードが UART1 にも届くことを確認する。
- **TC-908**: JP3/JP4 の設定変更も **電源を切った状態** で実施すること。UART1 端末のボーレートを変更してから Pico に再接続する。
- **TC-910**: `screen -L` でログを記録し、`hexdump -C screenlog.0` で連続する `61`（= `a`）を確認するのが確実。完全なゼロドロップは要求しない（ベストエフォート）。

---

## 5. 受け入れ条件チェックリスト

Issue #31 の受け入れ条件と対応する確認項目を以下に示す。**すべての項目にチェックが入った場合に Phase 9 合格** とする。

- [ ] **C-AC-01**: 起動バナーおよび `[USB]` 系ログが UART1（GPIO 4）のみに出力され、UART0（GPIO 0）には出力されない（TC-901, TC-902, TC-909）
- [ ] **C-AC-02**: USB キーボード → SBC への送信機能が従来どおり動作する（Phase 1〜6 リグレッションなし）（TC-903）
- [ ] **C-AC-03**: SBC 側で送信した応答（`echo hello` 等）が UART1 経由で PC ターミナルに表示される（TC-904）
- [ ] **C-AC-04**: JP5=OPEN のとき、自分のキー入力が UART1 にエコーされない（TC-905）
- [ ] **C-AC-05**: JP5=SHORT のとき、自分のキー入力が UART0 と UART1 の両方に送信される（TC-906, TC-907）
- [ ] **C-AC-06**: JP3/JP4 によるボーレート変更が UART1 にも追従する（TC-908）
- [ ] **C-AC-07**: バイナリサイズ増加が Phase 6 比 +5KB 以内（ビルド確認）

---

## 6. 検証結果記録

検証完了後、以下の表に結果を記入して Issue #31 または PR のコメントに貼り付ける。

### 環境情報

| 項目 | 内容 |
|------|------|
| 検証実施日 | （実施時に記入） |
| 検証実施者 | （実施者名） |
| 使用 Pico バリアント | Raspberry Pi Pico（無印、RP2040） |
| OS | （macOS / Linux / Windows） |
| Pico SDK バージョン | （実施時に記入） |
| UART1 モニタ用 USB-Serial 変換アダプタ | （型番） |
| SBC または疑似 SBC | （型番・代用手段） |
| シリアル端末ソフト | （ソフト名・バージョン） |
| 使用キーボード | （キーボード名・配列） |
| USB OTG アダプタ / ケーブル | VBUS 給電パススルー対応 |
| 初期ボーレート | 9600 |
| JP1〜JP5 初期設定 | 全 OPEN |

### TC 別結果

| TC | 内容 | 期待結果 | 結果 | 備考 |
|----|------|---------|------|------|
| TC-901 | 起動バナーが UART1 に出る・UART0 に出ない | UART1 に起動バナー表示、UART0 には出ない | | |
| TC-902 | USB キーボード接続ログが UART1 に出る | UART1 に接続ログ表示、UART0 には出ない | | |
| TC-903 | キー `a` を押す（既存機能リグレッション） | UART0 に `a`（0x61）受信、UART1 にエコーなし | | |
| TC-904 | SBC 応答が UART1 に表示 | UART1 に `hello\r\n` 表示 | | |
| TC-905 | JP5=OPEN: ローカルエコー OFF | UART1 に自分のキー入力がエコーされない | | |
| TC-906 | JP5=SHORT: ローカルエコー ON | UART0・UART1 両方に `a`（0x61）送信 | | |
| TC-907 | JP5=SHORT で Enter 押下 | UART0・UART1 両方に行末コード送信 | | |
| TC-908 | JP3=SHORT, JP4=OPEN で 19200 bps 確認 | UART1（19200 bps）で起動バナー受信 | | |
| TC-909 | キーボード切断ログ | UART1 に切断ログ表示、UART0 には出ない | | |
| TC-910 | 10 秒連続入力のドロップ確認 | 大半が届く（クラッシュなし） | | |

### 総合判定

| 受け入れ条件 | 結果 |
|-------------|------|
| C-AC-01（ログ振替、TC-901/902/909） | |
| C-AC-02（リグレッション、TC-903） | |
| C-AC-03（SBC 応答パススルー、TC-904） | |
| C-AC-04（JP5=OPEN エコー OFF、TC-905） | |
| C-AC-05（JP5=SHORT エコー ON、TC-906/907） | |
| C-AC-06（ボーレート追従、TC-908） | |
| C-AC-07（バイナリサイズ +5KB 以内） | |
| **Phase 9 総合** | |
| 所感・備考 | |
| 添付（写真・動画） | |

---

## 7. トラブルシューティング

| 症状 | 原因の候補 | 対処 |
|------|-----------|------|
| UART1 に何も出ない（起動バナーも出ない） | 配線ミス / ロジックレベル不一致 | GPIO 4（Pin 6）→ 変換器 RX、GND 共通接続を再確認。変換器のロジックレベルを 3.3V に設定する |
| UART1 に文字化けが出る | ボーレート不一致 | UART1 端末のボーレートを JP3/JP4 設定値（初期: 9600 bps）に合わせる |
| UART0 に起動バナーが出てしまう | 古いファームウェアが書き込まれている | ビルドを `--clean` 付きで再実行し、改めて書き込む |
| ローカルエコーが効かない（JP5=SHORT なのに） | JP5 接続不良 / 電源再投入忘れ | GPIO 14（Pin 19）と GND（Pin 18 または 23）の接続を確認。JP5 変更後は必ず電源再投入する |
| ローカルエコーが切れない（JP5=OPEN なのに） | プルアップ不足 / 接触抵抗 | JP5 のジャンパーケーブルを完全に外す。GPIO 14 は内蔵プルアップで HIGH（= OPEN = エコー OFF） |
| SBC 応答が UART1 に届かない | UART0 RX（GPIO 1）配線ミス / SBC TX 出力確認 | GPIO 1（Pin 2）→ SBC TX の接続を確認。SBC のシリアル出力が有効になっているか確認する |
| ボーレート変更後に UART1 が文字化けする | ジャンパー変更後に電源再投入していない | JP3/JP4 変更は電源 OFF 状態で行い、変更後に Pico を再接続すること |
| 連続入力でシステムがフリーズする | バッファ満杯時の処理不具合 | UART1 TX が満杯時はドロップが期待動作。クラッシュや完全停止が発生した場合は Issue #31 に報告する |
| 起動バナーに `(Phase 9)` が表示されない | 古いファームウェア | ビルドを `--clean` 付きで再実行し、改めて書き込む |
| `RPI-RP2` ボリュームが現れない | BOOTSEL を押さずに接続 / 充電専用ケーブル使用 | BOOTSEL を押しながら接続したか確認。USB ケーブルがデータ通信対応か確認 |

---

## 8. 関連ドキュメント

| ドキュメント | 参照目的 |
|------------|---------|
| [`README.md`](../../README.md) | 開発環境構築・ビルド方法・ピンアサイン |
| [`docs/design/将来計画_応答モニタ_PlanA.md`](../design/将来計画_応答モニタ_PlanA.md) | Plan A の設計詳細・受け入れ条件（A-AC-01〜06） |
| [`docs/design/設計書.md`](../design/設計書.md) | ピンアサイン・UART 設計詳細 |
| [`docs/manual/01_ハードウェア構成.md`](../manual/01_ハードウェア構成.md) | ピンアサイン表・JP5 仕様 |
| [`docs/manual/02_組み立て手順.md`](../manual/02_組み立て手順.md) | UART モニタ用配線手順（Step 6） |
| [`docs/manual/04_使い方.md`](../manual/04_使い方.md) | §10 Phase 9 UART モニタ機能の運用手順 |
| [`docs/images/wiring_diagram.drawio`](../images/wiring_diagram.drawio) | 配線図（JP5・UART1・UART0 RX 追記済み） |
| [`docs/tests/phase6_実機検証手順.md`](phase6_実機検証手順.md) | Phase 6 検証手順（スタイル参考） |

---

## 9. 次のステップ

Phase 9 の検証がすべての受け入れ条件で **合格** となった場合、Issue #31 を Closed にし、次の Phase（Plan B または追加機能）の実装・検証に進む。

検証結果に **不合格** が含まれる場合は、Issue #31 にトラブルシューティング結果を記録し、担当者と対処方針を協議する。
