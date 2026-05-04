# KKBD-USB

![開発進行中](https://img.shields.io/badge/status-WIP-yellow)
![MCU](https://img.shields.io/badge/MCU-RP2040-blue)
![言語](https://img.shields.io/badge/言語-C%2FC%2B%2B-brightgreen)

> **注意: 現在開発進行中のプロジェクトです。ファームウェアは未実装です。**

## 概要

KKBD-USB は、SBC6800 や KZ80 マイコンなど、UART 経由で稼働するシングルボードコンピューター（SBC）をスタンドアロン化するためのキーボードインターフェースボードです。

Raspberry Pi Pico（RP2040）の USB ホスト機能を利用し、USB キーボードの入力を UART 信号に変換して SBC へ出力します。これにより、PC なしで SBC 単体をキーボード操作できる環境を実現します。

## システム構成図

![システム構成図](docs/images/system_overview.png)

## 主な機能

- USB キーボードの入力を UART 信号に変換して出力
- ボーレートをジャンパーピンで選択可能（9600 / 19200 / 38400 / 115200 bps）
- 行末コードをジャンパーピンで選択可能（CRLF / CR / LF）
- Raspberry Pi Pico（RP2040）の内蔵 USB ホスト機能（TinyUSB）を使用

## ハードウェア仕様

| 項目 | 仕様 |
|------|------|
| MCU | Raspberry Pi Pico（RP2040） |
| USB ホスト | Pico 内蔵 USB ホスト機能（TinyUSB） |
| 開発言語 | C/C++（Raspberry Pi Pico SDK） |
| UART 出力レベル | TTL レベル |
| UART フォーマット | 8N1 |

### ジャンパーピン設定

#### 行末コード選択（JP1 / JP2）

| JP1 | JP2 | 行末コード |
|-----|-----|-----------|
| OFF | OFF | CR+LF |
| ON  | OFF | CR のみ |
| OFF | ON  | LF のみ |

#### ボーレート選択（JP3 / JP4）

| JP3 | JP4 | ボーレート |
|-----|-----|-----------|
| OFF | OFF | 9600 bps |
| ON  | OFF | 19200 bps |
| OFF | ON  | 38400 bps |
| ON  | ON  | 115200 bps |

## ディレクトリ構成

```
KKBD-USB/
├── README.md
├── docs/
│   ├── requirements/
│   │   └── 要件概要.md          # 要件定義.md は別PRでマージ後に追加予定
│   └── images/
│       ├── system_overview.drawio
│       ├── system_overview.png
│       └── tool/
│           └── convert_drawio.sh
# --- 以下は今後作成予定 ---
# └── src/
```

## 開発環境

> 今後追記予定

## ビルド方法

> 今後追記予定

ファームウェアは現在未実装です。実装が進み次第、ビルド手順を追記します。

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
