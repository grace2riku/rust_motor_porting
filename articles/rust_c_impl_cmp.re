
= C-Rust 実装比較
//footnote[rust_code_link][https://github.com/grace2riku/avr-hal/tree/add_evkart]

CとRustの実装を比較します。

Rustのコードは次に置きました。

Cのコード・@<chapref>{src_code_spec}の説明、Rustのコード@<fn>{rust_code_link}を
適宜参照しながら本章を見ていただくとよいと思います。

== ハードウェア初期設定
Cの該当部分はmain.cppのsetup関数です。

Rustの該当部分は次になります。

 * boards/arduino-mega2560/examples/evkart-main.rs main関数 21行目〜73行目

各種ハードウェアの初期設定のコードがあるので以降で説明します。

=== IOピンの定義
Arduino MEGAのIOピンのデータ構造を定義します。
ここで定義した変数を使用し、各種ハードウェアの初期化を行います。

該当コードは次の部分です。

//cmd{
    let dp = arduino_mega2560::Peripherals::take().unwrap();

    let mut pins = arduino_mega2560::Pins::new(
        dp.PORTA, dp.PORTB, dp.PORTC, dp.PORTD, dp.PORTE, dp.PORTF, dp.PORTG, dp.PORTH,
        dp.PORTJ, dp.PORTK, dp.PORTL,
    );
//}

=== 可変抵抗のAD変換の初期設定
可変抵抗が接続されているAD変換ピンA0の初期設定をおこないます。

該当コードは次の部分です。
//cmd{
    let mut adc = arduino_mega2560::adc::Adc::new(dp.ADC, Default::default());
    let mut a0 = pins.a0.into_analog_input(&mut adc);
//}

=== タイマの初期設定
8-bit Timer/Counter0に10msタイマの周期を設定します。

 * boards/arduino-mega2560/examples/timer.rs init関数

該当コードは次の部分です。8-bit Timer/Counter0を示す【dp.TC0】を引数で渡しています。

//cmd{
pub fn init(tc0: arduino_mega2560::pac::TC0) {
    // Configure the timer for the above interval (in CTC mode)
    // and enable its interrupt.
    tc0.tccr0a.write(|w| w.wgm0().ctc());
    tc0.ocr0a.write(|w| unsafe { w.bits(TIMER_COUNTS as u8) });
    tc0.tccr0b.write(|w| match PRESCALER {
        8 => w.cs0().prescale_8(),
        64 => w.cs0().prescale_64(),
        256 => w.cs0().prescale_256(),
        1024 => w.cs0().prescale_1024(),
        _ => panic!(),
    });
    tc0.timsk0.write(|w| w.ocie0a().set_bit());
}
//}

@<chapref>{rust_impl_study}で「マイコンの低レイヤ部分を意識しないようにしたかった」と
書きましたがレジスタにアクセスしないといけない場合もありました。

//footnote[mcu_datasheet_link][https://ww1.microchip.com/downloads/en/DeviceDoc/ATmega640-1280-1281-2560-2561-Datasheet-DS40002211A.pdf]

tc0.***の***の記述がレジスタを示しています。レジスタについての詳細はマイコンデータシート@<fn>{mcu_datasheet_link}を参照ください。

このコードはタイマのモード設定です。
//cmd{
    tc0.tccr0a.write(|w| w.wgm0().ctc());
//}

タイマをCTCモードに設定しています。CTCモードはタイマカウントが@<kw>{コンペアレジスタ, ocr0a}の設定値と
一致したらタイマカウントをクリアするモードのようです。
マイコンデータシート@<fn>{mcu_datasheet_link}のFigure 16-5. CTC Mode, Timing Diagramの動作になります。


このコードはコンペアレジスタの設定です。タイマカウント周波数(16MHz / 1024分周)で10msをつくりたいため、
156カウント(10ms / (1 / タイマカウント周波数) )を設定しています。

//cmd{
    tc0.ocr0a.write(|w| unsafe { w.bits(TIMER_COUNTS as u8) });
//}


このコードはクロックの分周設定です。
ターゲット基板のシステムクロック16MHzを1024分周し、それをタイマカウントの動作クロックとします。
//cmd{
    tc0.tccr0b.write(|w| match PRESCALER {
        8 => w.cs0().prescale_8(),
        64 => w.cs0().prescale_64(),
        256 => w.cs0().prescale_256(),
        1024 => w.cs0().prescale_1024(),
        _ => panic!(),
    });
//}


このコードはカウンタとコンペアレジスタが一致した時の割り込みを有効にしています。
マイコンの全割り込みを有効にしたあとにカウンタとコンペアレジスタが一致すると
割り込みハンドラにジャンプし割り込みハンドラの処理を実行します。
//cmd{
    tc0.timsk0.write(|w| w.ocie0a().set_bit());
//}

割り込みハンドラはtimer.rsのTIMER0_COMPA()です。


=== ホールセンサの外部割り込み初期設定
ホールセンサが接続されているピンを入力・プルアップ設定します。

//cmd{
    let hall_u = pins.d19.into_pull_up_input(&mut pins.ddr);
    let hall_v = pins.d20.into_pull_up_input(&mut pins.ddr);
    let hall_w = pins.d21.into_pull_up_input(&mut pins.ddr);
    hall_sensor::init(dp.EXINT, hall_u, hall_v, hall_w);
//}

その後、そのピンに外部割り込みの設定をします。
//cmd{
    hall_sensor::init(dp.EXINT, hall_u, hall_v, hall_w);
//}

設定関数は次のファイルに定義しています。
 * boards/arduino-mega2560/examples/hall_sensor.rs init関数

init関数全体は次のとおりです。
処理のまとまりごとに少し詳しく説明します。
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

    // INT2 hall sensor-U 両エッジ割り込みに設定
    ei.eicra.write(|w| w.isc2().bits(0x01));

    // INT1 hall sensor-V 両エッジ割り込みに設定
    ei.eicra.write(|w| w.isc1().bits(0x01));

    // INT0 hall sensor-W 両エッジ割り込みに設定
    ei.eicra.write(|w| w.isc0().bits(0x01));

    // INT2,1,0 interrupt enable
    ei.eimsk.write(|w| w.int().bits(0x07));
}
//}

このコードはinit関数の外でホールセンサーの入力状態を知りたいので、
staic外部変数【HALL_*_PIN】に引数で渡された入力ポートを設定しています。

static外部変数のアクセスはunsafeで括ります。
//cmd{
    unsafe {
        HALL_U_PIN = Some(u_phase);
        HALL_V_PIN = Some(v_phase);　
        HALL_W_PIN = Some(w_phase);
    }
//}

次のコードはホールセンサーが接続されている外部割り込み端子の割り込みエッジを選択しています。
マイコンデータシート@<fn>{mcu_datasheet_link}のeicraレジスタ(15.2.1　EICRA – External Interrupt Control Register A)を
参照すると何をしているかわかります。

ホールセンサーは信号の立ち上がり・立ち下がりを検出したいので両エッジ割り込みを設定しています。

//cmd{
    // INT2 hall sensor-U 両エッジ割り込みに設定
    ei.eicra.write(|w| w.isc2().bits(0x01));

    // INT1 hall sensor-V 両エッジ割り込みに設定
    ei.eicra.write(|w| w.isc1().bits(0x01));

    // INT0 hall sensor-W 両エッジ割り込みに設定
    ei.eicra.write(|w| w.isc0().bits(0x01));
//}

次のコードはホールセンサーが接続されている外部割り込みピン(INT2,INT1,INT0)の割り込みを有効にしています。
マイコンの全割り込みを有効にしたあとに外部割り込みピンの信号レベルに変化があると割り込みハンドラにジャンプし
割り込み処理を行います。
外部割り込みの割り込みハンドラは次のとおりです。

 * INT2

 * INT1

 * INT0



//cmd{
    // INT2,1,0 interrupt enable
    ei.eimsk.write(|w| w.int().bits(0x07));
//}


=== LEDの出力ポート設定

=== FETのPWM出力、出力ポート設定

=== 割り込み許可


== 駆動パターン設定



== 駆動停止設定



== 周期処理
