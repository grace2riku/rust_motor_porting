
= モータ制御ロジックの要件と実現手段

移植対象の【モータ制御ロジック】の要件と実現手段について記載します。

== モータ現在位置の取得
　モータを駆動するためにはモータの現在位置を取得する必要があります。
モータの現在位置を取得するためにホールセンサを使用します。
ホールセンサから現在位置を取得するために制御マイコンの外部割り込みを3つ使います。


== モータ制御タイミングの生成・取得
　モータ速度を変更するために一定周期で速度を設定します。
一定周期の制御タイミングをつくるために制御マイコンのタイマ機能を使います。


== モータ速度変更
　モータの速度調整するために可変抵抗を使用します。
可変抵抗から速度設定値を取得するために制御マイコンのAD変換機能を1つ使います。


== モータ駆動
//footnote[FET_footnote][電気でON・OFFで可能なスイッチとお考えください。]
　モータを駆動するためにインバータ回路の6個のFET@<fn>{FET_footnote}に適切なタイミングで、適切な通電パターンを設定する必要があります。

通電パターンの設定に制御マイコンのPWMを3つ、出力ポートを3つ使います。

//comment{
・FETについて注釈で説明すると良いかもしれない。
//}


