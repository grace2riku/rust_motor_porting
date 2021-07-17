
= 移植元ソースコードの動作
//footnote[c_code][https://github.com/grace2riku/EVKartArduinoIDE]
//footnote[techbookfest_9_EVKartMBD_URL][https://techbookfest.org/product/6581675934875648?productVariantID=5815321967460352]

移植元Cソースコードはこちらにあります@<fn>{c_code}。

== モータ制御概要
モータ制御は既刊技術書@<fn>{techbookfest_9_EVKartMBD_URL}の1.3 制御方法を参照ください。

== ソースコード説明
ソースコードは6つありますがモータ制御ロジックは次の2ファイルで構成しています。

 * main.cpp
 * EvKartPin.h

main.cppにはモータ制御ロジックが実装されています。

EvKartPin.hにはmain.cppのモータ制御ロジックが使用、参照する次のデータを定義しています。

 * FETのピン番号の定義
 * ホールセンサのピン番号の定義
 * ホールセンサ割り込み番号(INTn)の定義　n = 0,1,2
 * ホールセンサLEDのピン番号の定義
 * 可変抵抗を接続しているAD変換ピン番号の定義
 * 1秒のカウント値
 * 1秒周期で点滅するLEDのピン番号の定義


=== ハードウェア初期設定
ハードウェアの初期設定を行います。

該当関数は【setup】です。

Arduino IDEを使うと電源ON後一度だけsetup関数が呼び出され、その後loop関数が繰り返し実行されます。

setup関数ではハードウェア初期設定を行います。

 * IOポートの入出力設定

 * 外部割り込みポートの選択・割り込みハンドラの設定・割り込みエッジの設定

 * タイマーの周期時間設定、コールバック関数の設定


=== 駆動パターン設定
FETに駆動パターンを設定します。

該当関数は【setFETDrivePattern】です。

ホールセンサの値を読み取り、通電パターンを設定します。

この関数は次のタイミングで実行されます。

 * モータ停止状態から駆動するとき

 * ホールセンサの信号に変化があったとき


=== 駆動停止設定
モータを駆動停止します。

該当関数は【setFETStopPattern】です。

FETをOFFし、モータの通電を止めて停止させます。

この関数は次のタイミングで実行されます。

 * モータ駆動状態から停止するとき

 * ホールセンサの信号が異常で通電パターンが存在しないとき


=== 周期処理
10ms周期で実行する周期的な処理です。

該当関数は【expireTimer】です。

次の処理を実行します。

 * 可変抵抗の電圧値をAD変換する。この値でモータ速度が変わる

 * 可変抵抗の電圧値でモータの駆動・停止を判断する

 * 1秒経過したらLEDを点滅する
