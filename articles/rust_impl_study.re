
= Rust実装検討
//footnote[footnote_HAL][HAL: Hardware Abstraction Layerの略称]
//footnote[avr-rust_link][https://www.avr-rust.com]
//footnote[avr-hal_link][https://github.com/Rahix/avr-hal]

CのコードをRustに移植するときにArduino IDEのようにマイコンのハードウェアを
隠蔽(レジスタを意識しない)するライブラリ(以降、HAL@<fn>{footnote_HAL}と呼びます)がないか探しました。

リセットからmain関数までのブート処理は書かない方向性に決めました。

今回の主目的は【CからRustへの移植】のため、マイコンの低レイヤ部分を意識しないようにしたかったからです。

HALを探していくと次の2つをみつけました。

 1. avr-rust@<fn>{avr-rust_link}
 2. avr-hal@<fn>{avr-hal_link}

== avr-rust
最初にみつけたのがavr-rustでした。

avr-rustの環境構築を試してみましたが私のホストPC(Mac)では環境構築がエラーになりました。

avr-rustは環境構築ができなかったため使用を諦めました。

== avr-hal
avr-halは環境構築、Lチカのサンプルプログラムまですんなりできました。

よってCからRustへの移植もavr-halを使うことに決めました。

