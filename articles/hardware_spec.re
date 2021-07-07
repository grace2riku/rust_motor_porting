
= ハードウェア構成
//footnote[techbookfest_9_EVKartMBD_URL][https://techbookfest.org/product/6581675934875648?productVariantID=5815321967460352]

移植作業の対象ハードウェアについて記載します。

対象ハードウェアは既刊技術書【EVカートで始めるモデルベース開発@<fn>{techbookfest_9_EVKartMBD_URL}】記載の
【インバーター基板 + 治具基板 + Arduino + ブラシレスモータ】を考えていました。

//comment{
・MBD本の図 4.1 動作確認環境 の写真を挿入する。
//}

実際にモータを回転させたかったのですが、インバータ基板を開発中に壊してしまったので代替ハードウェアで移植作業を行いました。

//comment{
・代替ハードウェアの写真(Shinjuku rs #16 の資料9ページ)を挿入する。
・どの部品を何に代替したか記載する。
//}

制御はArduino MEGA 2560を使用します。
