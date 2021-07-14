
= 感想
CからRustの移植作業を実施した個人的な感想です。

Rustに関すること、avr-halの2つの観点で書きます。

== Rust
Rustに関する感想です。

=== モダン
Cよりモダンに書ける、と感じました。
具体的には以降で記載する項目です。

=== 型推論
変数宣言の際に型を明示しなくても型を推論してくれるのは嬉しいです。

=== デフォルトmut
変数宣言の際、【mut】キーワードをつけないと変更可能になるのが個人的によいと感じています。

=== mod, pubキーワード
Cだと外部に公開したい関数はヘッダファイルに書き、呼び出し側でヘッダファイルをインクルードするのが定番かと思います。
Rustでは【mod】キーワードでモジュールを読み込めて、外部に公開したい関数は【pub】キーワードをつければよくてCに比べ楽・簡潔と感じました。

=== unsafe
外部変数へのアクセスはunsafeでくくらないとコンパイルエラーになります。
unsafeを書くと危険なこと・いけないことをしているという意識になってきます。
ただ、危険なコードが明示される、という点でよいのではないかと思いました。


=== コンパイラ
コンパイラが親切です。
Rustのコーディングルールもウォーニングとして出力してくれますし、
コンパイラのメッセージに従うと他の人にもわかりやすい・読みやすいコードになると予想できます。

== avr-hal
Arduinoを操作するためのavr-halの感想です。

=== RustでArduinoを動かすハードル
環境構築も簡単でavr-halでArduinoを動かすハードルは低いと感じました。
今回はArduino MEGAを使用しましたが、他のArduinoシリーズにも対応しており幅広く使えそうです。

=== ハードウェアの制御
avr-halにはArduinoのペリフェラルを制御するサンプルコードが同梱されているので、割と簡単に動作確認が可能でした。
割り込みに関してもサンプルプログラムがあったのでそちらを参考にして実現できました@<fn>{interrupt_example}。

//footnote[interrupt_example][boards/sparkfun-pro-micro/examples/pro-micro-interrupt.rs　INT6の割り込みハンドラ INT6()]

=== ソースコードの構成
今回移植作業したソースコードは次の点で冗長です。

 * 使わないソースコード(Arduino MEGA以外のターゲット)がある

avr-halはArduino MEGA以外にも対応しているので、Arduino MEGA以外のソースコードもあります。
今回はArduino MEGAを対象にしたのでそれ以外のコードは不要です。
avr-halのgit hubリポジトリからforkして移植作業を行っていき、特に未使用なコードを削除していなかったので
このようなソースコードの構成になっています。

README.mdに書いてある【Starting your own project】の手順をおこなうと対象のArduinoのみでソースコードを構成できます。

=== グローバル関数
今回悩んだのはグローバル関数の扱いです。

一例を挙げるとhall_sensor.rsの外部変数です。

グローバル変数の定義は次です。
//cmd{
// hall sensor-U d19, PD2
static mut HALL_U_PIN: Option<port::portd::PD2<port::mode::Input<port::mode::PullUp>>> = None;
//}
これはホールセンサーU相の値を取得するための外部変数です。
Option型・Noneで定義しておき、hall_sensor.rsのinit関数で代入しています。

//cmd{
pub fn init(ei: arduino_mega2560::pac::EXINT, 
            u_phase: port::portd::PD2<port::mode::Input<port::mode::PullUp>>,
            v_phase: port::portd::PD1<port::mode::Input<port::mode::PullUp>>,
            w_phase: port::portd::PD0<port::mode::Input<port::mode::PullUp>>){

    unsafe {
        HALL_U_PIN = Some(u_phase);
        HALL_V_PIN = Some(v_phase);
        HALL_W_PIN = Some(w_phase);
    }
//}


どこに悩んだかというと型の定義です。この部分です。
//cmd{
Option<port::portd::PD2<port::mode::Input<port::mode::PullUp>>>
//}

//footnote[Option_link][boards/arduino-leonardo/examples/leonardo-interrupt.rs　のグローバル変数 PIN]

グローバル変数をOption型・Noneで定義しておき、初期化関数で代入という手法は
avr-halのサンプルプログラム@<fn>{Option_link}でもあったのでそれを真似しようと考えました。
ただ、言うは易し行うは難しで、この暗号のような型の構造がよくわかりませんでした。

そこで【rust-analyzer】で@<img>{rust-analyzer_focus}のように
avr-halのサンプルプログラムの型にフォーカス -> 定義へ移動 を繰り返し、
今回の移植の場合はどう実装すればよいのか少しずつ理解していきました。

//image[rust-analyzer_focus][rust-analyzerで解析]{
//}

結果、コンパイルができて期待とおりの動作を確認できました。

