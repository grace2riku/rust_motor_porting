
= ハードウェア構成
//footnote[techbookfest_9_EVKartMBD_URL][https://techbookfest.org/product/6581675934875648?productVariantID=5815321967460352]

移植作業の対象ハードウェアについて記載します。

対象ハードウェアは既刊技術書【EVカートで始めるモデルベース開発@<fn>{techbookfest_9_EVKartMBD_URL}】記載の
【インバーター基板 + 治具基板 + Arduino + ブラシレスモータ】を考えていました。
@<img>{DevelopmentEnvironment}です。
//image[DevelopmentEnvironment][想定していた動作確認環境]{
//}
@<img>{DevelopmentEnvironment}で実際にモータを回転させたかったのですが、インバータ基板を開発中に壊してしまったので代替ハードウェアで移植作業を行いました。

よって@<img>{rust_env_construction}の代替ハードウェアで開発することにしました。
//image[rust_env_construction][Rust動作確認環境]{
//}

@<img>{rust_env_construction}の代替ハードウェアの構成について説明します。

 1. Arduino-mega2560
 2. ホールセンサ代替DIPスイッチ：左からW・V・U。
 3. High側FET 代替LED：右からU・V・W
 4. Low側FET 代替LED：右からU・V・W
 5. スロットル (可変抵抗)

1はモータ制御を行うArduino MEGA 2560です。

2はホールセンサを代替しているDIP SWです。
@<img>{rust_env_construction}でスイッチを上側にするとHighレベル、下側にするとLowレベルになります。

3はHigh側FETを代替しているLEDです。正常に動作していれば可変抵抗の設定値によりPWM制御で調光されます。

4はLow側FETを代替しているLEDです。Highレベルで点灯、Lowレベルで消灯します。

5はスロットルを代替している可変抵抗です。正常に動作していれば可変抵抗の変更でHigh側FET代替のLEDが調光されます。

制御は既刊技術書と同じくArduino MEGA 2560を使用します。
