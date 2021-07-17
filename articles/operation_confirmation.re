= 動作確認結果

CからRustへの移植作業の実装は終了しましたが、Rust版の動作確認は完了できませんでした。
具体的には次の動作が確認できていません。

 * ホールセンサ代替のDIPスイッチを変更してもHigh側FETの代替LEDが期待とおりに点灯しない。
 * 1秒周期のLED点滅ができていない。

ホールセンサ代替のDIPスイッチ変更でLow側FET代替LEDは点灯・消灯しているので
ホールセンサの外部割り込み・Low側FETのGPIO出力は動作しているようです。
High側FETのPWM出力ができていないようです。

次に期待値であるCの動作を紹介したいと思います。

== 動作確認結果詳細(C版)
Cの動作確認結果です。

@<img>{HallandPWMControl}のとおりになっていれば期待値とおりです。
//image[HallandPWMControl][モータ制御のタイミングチャート]{
//}

=== 通電ステージ1動作確認(ホールセンサU=1, V=0, W=1)
ホールセンサ代替のDIPスイッチは次のようにピンに接続しています。

 * 6がホールセンサU
 * 7がホールセンサV
 * 8がホールセンサW

FET代替のLEDは次のように割り当てています。

 * 右上LED:U High側FET代替/右下LED:U Low側FET代替
 * 中央上側LED:V High側FET代替/中央下側LED:V Low側FET代替
 * 左上LED:W High側FET代替/左下LED:V Low側FET代替

@<img>{stage-1_photo}が動作確認時の写真です。
//image[stage-1_photo][通電ステージ1 動作確認時の写真]{
//}

DIPスイッチは上側でHighレベル、下側でLowレベルになります。

 * 6(ホールセンサU代替)がDIPスイッチ上側なのでHighレベル
 * 7(ホールセンサV代替)がDIPスイッチ下側なのでLowレベル
 * 8(ホールセンサW代替)がDIPスイッチ上側なのでHighレベル

つまり@<img>{HallandPWMControl}の通電ステージ1に該当します。

通電ステージ1はU High側FETがPWM出力、V Low側FETがHighレベルになります。
FET代替LEDもU High側・V Low側が点灯しています。

@<img>{stage-1}が通電ステージ1の波形データです。
//image[stage-1][通電ステージ1 動作確認時の波形]{
//}
U High側がPWM出力、V Low側がHighレベルになっているので通電ステージ1の動作は期待とおりです。

通電ステージ2〜6も同様に確認します。


=== 通電ステージ2動作確認(ホールセンサU=1, V=0, W=0)
@<img>{stage-2_photo}が動作確認時の写真です。
//image[stage-2_photo][通電ステージ2 動作確認時の写真]{
//}

@<img>{stage-2}が通電ステージ2の波形データです。
//image[stage-2][通電ステージ2 動作確認時の波形]{
//}
@<img>{HallandPWMControl}のとおりのため通電ステージ2の動作は期待とおりです。


=== 通電ステージ3動作確認(ホールセンサU=1, V=1, W=0)
@<img>{stage-3_photo}が動作確認時の写真です。
//image[stage-3_photo][通電ステージ3 動作確認時の写真]{
//}

@<img>{stage-3}が通電ステージ3の波形データです。
//image[stage-3][通電ステージ3 動作確認時の波形]{
//}
@<img>{HallandPWMControl}のとおりのため通電ステージ3の動作は期待とおりです。


=== 通電ステージ4動作確認(ホールセンサU=0, V=1, W=0)
@<img>{stage-4_photo}が動作確認時の写真です。
//image[stage-4_photo][通電ステージ4 動作確認時の写真]{
//}

@<img>{stage-4}が通電ステージ4の波形データです。
//image[stage-4][通電ステージ4 動作確認時の波形]{
//}
@<img>{HallandPWMControl}のとおりのため通電ステージ4の動作は期待とおりです。


=== 通電ステージ5動作確認(ホールセンサU=0, V=1, W=1)
@<img>{stage-5_photo}が動作確認時の写真です。
//image[stage-5_photo][通電ステージ5 動作確認時の写真]{
//}

@<img>{stage-5}が通電ステージ5の波形データです。
//image[stage-5][通電ステージ5 動作確認時の波形]{
//}
@<img>{HallandPWMControl}のとおりのため通電ステージ5の動作は期待とおりです。


=== 通電ステージ6動作確認(ホールセンサU=0, V=0, W=1)
@<img>{stage-6_photo}が動作確認時の写真です。
//image[stage-6_photo][通電ステージ6 動作確認時の写真]{
//}

@<img>{stage-6}が通電ステージ6の波形データです。
//image[stage-6][通電ステージ6 動作確認時の波形]{
//}
@<img>{HallandPWMControl}のとおりのため通電ステージ6の動作は期待とおりです。
