# KKBD-USB

![開発進行中](https://img.shields.io/badge/status-WIP-yellow)
![MCU](https://img.shields.io/badge/MCU-RP2040-blue)
![言語](https://img.shields.io/badge/言語-C%2FC%2B%2B-brightgreen)

> **注意: 現在開発進行中のプロジェクトです。Phase 1〜6 + Phase 9 完了、Phase 7 未着手。**
>
> **暫定稼働 OK**: Phase 1〜6 完了で「USB キーボード接続 → ASCII 変換 → UART 送信」の **メイン機能は実機検証済み・通常用途で使用可能** です。Phase 7（異常系処理: tuh_init 失敗時セーフモード、受信失敗リトライ、Phantom 詳細対応、複数キーボード等）と Phase 8（24 時間連続動作・複数キーボード互換性総合検証）が未実装のため正式リリースではありませんが、一般的な USB キーボードを SBC へ接続する用途では問題なく動作します。Phase 9 でモニタ機能（UART1 経由で SBC 応答を PC で確認可能）が追加されました。

## 概要

KKBD-USB は、SBC6800 や KZ80 マイコンなど、UART 経由で稼働するシングルボードコンピューター（SBC）をスタンドアロン化するためのキーボードインターフェースボードです。

Raspberry Pi Pico（RP2040）の USB ホスト機能を利用し、USB キーボードの入力を UART 信号に変換して SBC へ出力します。これにより、PC なしで SBC 単体をキーボード操作できる環境を実現します。

---

> **重要: 本ボードは「USB キーボード → UART 単方向送信」専用のキーボードインターフェース代替ボードです。**
>
> - **送信のみ**: USB キーボードのキー入力を ASCII 化して UART で送信します。SBC 応答の **UART1 経由でのモニタ機能** は Phase 9 で追加されましたが、これは開発者向けのデバッグ用途であり、SBC のターミナル端末として動作するわけではありません。
> - **受信・表示機能なし**: SBC からの UART 応答を読み取って画面に表示する機能はありません（シリアルターミナルではありません）。
> - **想定構成**: SBC 側で VRAM 付きビデオボード等の独立した表示系統を用意することを前提とします。本ボードは「PC 側のキーボードを使ったターミナル接続」の代わりに「SBC スタンドアロン化のためのキーボード単独入力」を提供します。
> - **対象用途**: SBC6800 / KZ80 など、「キーボード入力 + 別系統の文字表示（VRAM ボード等）」で構成されるレトロ SBC のスタンドアロン化。

---

## システム構成図

![システム構成図](docs/images/system_overview.png)

## 主な機能

- USB キーボードの入力を UART 信号に変換して出力
- SBC からの UART 応答を別チャネル（UART1）経由で PC ターミナルでモニタ可能（Phase 9）
- キー入力のローカルエコーをジャンパー JP5 で ON/OFF 選択（Phase 9）
- ボーレートをジャンパーピンで選択可能（9600 / 19200 / 38400 / 115200 bps）
- 行末コードをジャンパーピンで選択可能（CRLF / CR / LF）
- Raspberry Pi Pico（RP2040）の内蔵 USB ホスト機能（TinyUSB）を使用

## 実装ステータス

| Phase | 内容 | 状態 |
|-------|------|------|
| Phase 1 | ビルド環境構築・Lチカ | 完了（実機検証済み） |
| Phase 2 | ジャンパー読取とUART送信 | 完了（実機検証済み） |
| Phase 3 | USBホスト基盤（TinyUSB） | 完了（実機検証済み） |
| Phase 4 | 基本キー入力（英数字） | 完了（実機検証済み） |
| Phase 5 | 修飾キー対応 | 完了（実機検証済み） |
| Phase 6 | 行末コード・キーリピート・LED | 完了（実機検証済み） |
| Phase 7 | 異常系処理 | 未着手 |
| Phase 8 | 実機検証 | 未着手 |
| Phase 9 | UARTモニタ + ログ振替 + ローカルエコー | 完了（実機検証待ち） |

詳細は [`docs/design/実装計画.md`](docs/design/実装計画.md) を参照してください。

### 将来計画（検討中）

- [SBC 応答モニタ機能の検討（Plan A: UART1 パススルー / Plan B: スタンドアロン端末化）](docs/design/将来計画_応答モニタ_概要.md) — Phase 9 で基本的な UART1 モニタ（デバッグ用）を実装済み。フル端末化（Plan B）等の発展は引き続き検討中、Issue [#27](https://github.com/kuninet/KKBD-USB/issues/27) で追跡
- [5V SBC（MC6850 / 8251）接続のための電圧レベル変換ガイド](docs/design/将来計画_5V_SBC接続_電圧レベル変換.md) — 検討中・未着手、Issue [#29](https://github.com/kuninet/KKBD-USB/issues/29) で追跡

## ユーザー向けマニュアル

KKBD-USB の組み立て・ビルド・使い方の詳細手順は [`docs/manual/`](docs/manual/) を参照してください。

| マニュアル | 内容 |
|---|---|
| [01_ハードウェア構成](docs/manual/01_ハードウェア構成.md) | 必要部品一覧、システム構成図、Pico ピンレイアウト |
| [02_組み立て手順](docs/manual/02_組み立て手順.md) | 配線手順、ジャンパー設定、配線図 |
| [03_ビルドと書き込み](docs/manual/03_ビルドと書き込み.md) | Pico SDK セットアップ、ファームウェアビルド・書き込み |
| [04_使い方](docs/manual/04_使い方.md) | 起動・接続、LED ステータス、シリアル端末、設定変更 |
| [05_OTGと電源](docs/manual/05_OTGと電源.md) | USB OTG ケーブル、外部電源の選定と配線 |
| [06_キーボード対応状況](docs/manual/06_キーボード対応状況.md) | 対応キーボード、配列、対応キー一覧、制限事項 |
| [07_トラブルシューティング](docs/manual/07_トラブルシューティング.md) | 各種トラブルの症状・原因・対処を統合 |

## ハードウェア仕様

| 項目 | 仕様 |
|------|------|
| MCU | Raspberry Pi Pico（RP2040） |
| USB ホスト | Pico 内蔵 USB ホスト機能（TinyUSB） |
| 開発言語 | C/C++（Raspberry Pi Pico SDK） |
| UART 出力レベル | TTL レベル |
| UART フォーマット | 8N1 |

### Pico ピンアサイン

本プロジェクトで使用する Raspberry Pi Pico の GPIO 一覧。配線時の参照用。

| 機能 | GPIO 番号 | Pico 物理ピン番号 | 方向 | 内部設定 | 備考 |
|------|----------|-------------------|------|---------|------|
| UART0 TX | GPIO 0 | 1 | 出力 | UART 機能 | SBC の RX へ接続 |
| UART0 RX | GPIO 1 | 2 | 入力 | UART 機能 | SBC の TX から受信（応答モニタ用、Phase 9） |
| GND | - | 3, 8, 13, 18, 23, 28, 38 | - | - | UART・ジャンパー共通グラウンド |
| UART1 TX | GPIO 4 | 6 | 出力 | UART 機能 | USB-Serial 変換器の RX へ（モニタ出力、Phase 9） |
| JP1（行末コード bit0） | GPIO 10 | 14 | 入力 | 内蔵プルアップ | OPEN=未接続 / SHORT=GND ピンへ接続 |
| JP2（行末コード bit1） | GPIO 11 | 15 | 入力 | 内蔵プルアップ | 同上 |
| JP3（ボーレート bit0） | GPIO 12 | 16 | 入力 | 内蔵プルアップ | 同上 |
| JP4（ボーレート bit1） | GPIO 13 | 17 | 入力 | 内蔵プルアップ | 同上 |
| JP5（ローカルエコー） | GPIO 14 | 19 | 入力 | 内蔵プルアップ | OPEN=エコー OFF / SHORT=エコー ON（Phase 9） |
| LED（ステータス表示） | GPIO 25 | 内蔵 | 出力 | - | Pico オンボード LED |
| USB | 内蔵 | Micro-B コネクタ | - | USB ホスト | USB OTG アダプタ経由でキーボード接続 |

> **配線方法**: 各 JP は **OPEN（未接続）** または **SHORT（GND ピンへ接続）** の 2 状態。プルアップ設定により OPEN 時は High、SHORT 時は Low として読み取られる。

### ジャンパーピン設定

#### 行末コード選択（JP1 = GPIO 10 / JP2 = GPIO 11）

| JP1 | JP2 | 行末コード | 送出バイト列 |
|-----|-----|-----------|-------------|
| OPEN | OPEN | CR | 0x0D |
| SHORT | OPEN | LF | 0x0A |
| OPEN | SHORT | CRLF | 0x0D 0x0A |
| SHORT | SHORT | （予約） | - |

> 予約パターン（JP1=SHORT, JP2=SHORT）読み取り時は CR にフォールバックする（要件定義 FR-004 補足）。

#### ボーレート選択（JP3 = GPIO 12 / JP4 = GPIO 13）

| JP3 | JP4 | ボーレート |
|-----|-----|-----------|
| OPEN | OPEN | 9600 bps |
| SHORT | OPEN | 19200 bps |
| OPEN | SHORT | 38400 bps |
| SHORT | SHORT | 115200 bps |

#### ローカルエコー選択（JP5 = GPIO 14、Phase 9）

| JP5 | ローカルエコー | 動作 |
|-----|---------------|------|
| OPEN | OFF（デフォルト） | キー入力は UART0 にのみ送信、UART1 には SBC 応答のみ |
| SHORT | ON | キー入力は UART0 と UART1 の両方に送信（モニタ側でも自分のキー入力を確認可能） |

> Phase 9 で追加。出荷デフォルトは OPEN（エコー OFF）。デバッグ時など、PC ターミナルで「自分が打ったキー + SBC 応答」を両方確認したい場合に SHORT にする。

## ディレクトリ構成

```
KKBD-USB/
├── README.md
├── CMakeLists.txt              # ルートCMake（Pico SDK統合）
├── pico_sdk_import.cmake       # Pico SDK インポート
├── .gitignore
├── docs/
│   ├── requirements/
│   │   ├── 要件概要.md
│   │   └── 要件定義.md
│   ├── design/
│   │   ├── 設計書.md
│   │   ├── 実装計画.md
│   │   └── テスト戦略.md
│   ├── manual/                 # ユーザー向けマニュアル（01〜07）
│   ├── tests/                  # 実機検証手順（Phase毎）
│   └── images/
│       ├── system_overview.drawio + .png
│       ├── architecture.drawio + .png
│       ├── wiring_diagram.drawio + .png   # 配線図
│       └── tool/
│           └── convert_drawio.sh
├── src/                        # ファームウェア（Pico SDK ビルド）
│   ├── CMakeLists.txt
│   ├── main.c                  # エントリポイント
│   ├── tusb_config.h           # TinyUSB ホストモード設定
│   ├── usb_host.c/h            # USBホスト処理
│   ├── keymap.c/h              # HID Usage ID → ASCII
│   ├── uart_out.c/h            # UART送信、行末コード
│   ├── uart_monitor.c/h        # UARTモニタ（UART1経由SBC応答転送、Phase 9）
│   ├── config.c/h              # ジャンパー読取
│   ├── led.c/h                 # LEDインジケータ
│   └── keyrepeat.c/h           # キーリピート
└── tests/                      # ホスト側ユニットテスト（独立CMake）
    ├── CMakeLists.txt
    ├── test_framework.h
    ├── test_keymap.c
    ├── test_config.c
    └── test_keyrepeat.c
```

## 開発環境

主要ツール: Raspberry Pi Pico SDK v1.5.1 以上、`arm-none-eabi-gcc` 10 以上、CMake 3.13 以上（CMake 4.x の場合は `scripts/build.sh` 推奨）、Ninja（推奨）。

詳細なインストール手順・OS 別セットアップ・Pico SDK のセットアップは [`docs/manual/03_ビルドと書き込み.md`](docs/manual/03_ビルドと書き込み.md) を参照してください。

## ビルド方法

```sh
export PICO_SDK_PATH=$HOME/pico-sdk
./scripts/build.sh           # 通常ビルド
./scripts/build.sh --clean   # build/ 削除して再ビルド
```

ビルドが成功すると `build/src/kkbd_usb.uf2` が生成されます。

詳細（CMake 4.x 互換、手動ビルド、Pico への書き込み手順、ホスト側ユニットテスト実行）は [`docs/manual/03_ビルドと書き込み.md`](docs/manual/03_ビルドと書き込み.md) を参照してください。

## 実機検証

各 Phase の実機検証手順は [`docs/tests/`](docs/tests/) を参照してください。

- Phase 1: [`docs/tests/phase1_実機検証手順.md`](docs/tests/phase1_実機検証手順.md)
- Phase 2: [`docs/tests/phase2_実機検証手順.md`](docs/tests/phase2_実機検証手順.md)
- Phase 3: [`docs/tests/phase3_実機検証手順.md`](docs/tests/phase3_実機検証手順.md)
- Phase 4: [`docs/tests/phase4_実機検証手順.md`](docs/tests/phase4_実機検証手順.md)
- Phase 5: [`docs/tests/phase5_実機検証手順.md`](docs/tests/phase5_実機検証手順.md)
- Phase 6: [`docs/tests/phase6_実機検証手順.md`](docs/tests/phase6_実機検証手順.md)
- Phase 9: [`docs/tests/phase9_実機検証手順.md`](docs/tests/phase9_実機検証手順.md) — 実機検証待ち

トラブルシューティングは [`docs/manual/07_トラブルシューティング.md`](docs/manual/07_トラブルシューティング.md) に統合・カテゴリ化されています。

## draw.io 変換ツールの使い方

`docs/images/tool/convert_drawio.sh` を使用すると、draw.io ファイル（`.drawio`）を PNG 画像に変換できます。

```bash
cd docs/images/tool
./convert_drawio.sh ../system_overview.drawio
```

変換後の画像は `docs/images/` に出力されます。

## ライセンス

TBD

## 参考リンク

- [USB 簡単ホスト（TinyUSB を使った RP2040 USB ホスト実装例）](https://q61.org/blog/2021/06/09/easyusbhost/)
- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)
- [TinyUSB](https://github.com/hathach/tinyusb)
