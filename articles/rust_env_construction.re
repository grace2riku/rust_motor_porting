
= Rust環境構築
//footnote[avr-hal_link][https://github.com/Rahix/avr-hal]
//footnote[avr-hal_branch_link][https://github.com/grace2riku/avr-hal/tree/add_evkart]

CからRustへの移植にavr-halを使うことを決めました。

環境構築の手順を記載します。

avr-halのmainブランチ 2021年4月7日　885e8ec6 のコミットをforkしました@<fn>{avr-hal_branch_link}。
そのREADME.mdのQuickstartに記載の手順を実施しました。
注意点としては本家avr-hal最新の環境構築手順と違っている点です@<fn>{avr-hal_link}。
移植作業では【nightly-2021-01-07】でコンパイルしています。本家avr-halではstableでコンパイルしているようです。

環境構築はMacで実施しました。

== Rustのインストール
Quickstartの手順を行う前にRustのインストールが必要です。

Rust公式の手順でインストールします。

== avr-hal Quickstartの実行
avr-halのQuickstartの手順を実行します。

ビルドするファイル名、ファイル書き込みのコマンドがREADME.mdと違うので次に書きます。

コンパイルは次のコマンドを実行します。
//cmd{
cargo +nightly-2021-01-07 build --example evkart-main
//}

elfファイルの書き込みは次のコマンドを実行します。
//cmd{
avrdude -patmega2560 -cwiring -P/dev/cu.usbmodem141401 -b115200 -D
 -Uflash:w:../../target/avr-atmega2560/debug/examples/evkart-main.elf:e
//}

紙面の都合上、改行しています。コマンドは改行しないで実行します。

【-P/dev/cu.usbmodem141401】の部分はお使いの環境で変わるかと思います。

