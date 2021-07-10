
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

タイマの関数はtimer.rsにまとめています。
main関数に書いてもよいと思いますが、タイマの機能はmainとは切り分けて独立させたかったので
このような実装にしました。

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
割り込みハンドラにジャンプします。
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

タイマと同様、外部割り込みの関数を別ファイルにまとめています。
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
外部変数はすべて大文字にするのがRustのセオリーのようです。

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
マイコンの全割り込みを有効にしたあとに外部割り込みピンの信号レベルに変化があると割り込みハンドラにジャンプします。
外部割り込みの割り込みハンドラは次のとおりです。

 * INT2
 * INT1
 * INT0

//cmd{
    // INT2,1,0 interrupt enable
    ei.eimsk.write(|w| w.int().bits(0x07));
//}


=== LEDの出力ポート設定
次のコードがLEDの出力ポート設定です。

//cmd{
    let user_led = pins.d13.into_output(&mut pins.ddr);
    let hall_u_led = pins.d23.into_output(&mut pins.ddr);
    let hall_v_led = pins.d25.into_output(&mut pins.ddr);
    let hall_w_led = pins.d27.into_output(&mut pins.ddr);
    led::init(user_led, hall_u_led, hall_v_led, hall_w_led);
//}

main関数で次のLEDを出力ポートに設定しています。
 * Arduino MEGA基板に実装されているLED
 * ホールセンサーU・V・W　確認用LED

//cmd{
    let user_led = pins.d13.into_output(&mut pins.ddr);
    let hall_u_led = pins.d23.into_output(&mut pins.ddr);
    let hall_v_led = pins.d25.into_output(&mut pins.ddr);
    let hall_w_led = pins.d27.into_output(&mut pins.ddr);
//}

出力に設定したLEDをLED初期化に渡します。
//cmd{
    led::init(user_led, hall_u_led, hall_v_led, hall_w_led);
//}

タイマと同じくLEDの関数をled.rsにまとめています。
次のコードがLED初期化です。
//cmd{
pub fn init(user_led:port::portb::PB7<port::mode::Output>,
            hall_u_led:port::porta::PA1<port::mode::Output>,
            hall_v_led:port::porta::PA3<port::mode::Output>,
            hall_w_led:port::porta::PA5<port::mode::Output>){
    unsafe {
        USER_LED_PIN = Some(user_led);
        HALL_U_LED_PIN = Some(hall_u_led);
        HALL_V_LED_PIN = Some(hall_v_led);
        HALL_W_LED_PIN = Some(hall_w_led);
    }
}
//}

やっていることは@<hd>{ホールセンサの外部割り込み初期設定}と同じで
init関数の外でLEDを操作したいのでstatic外部変数に引数のLEDを設定します。


=== FETのPWM出力、出力ポート設定
High側のFET3つ、Low側のFET3つを初期設定します。
このHigh側FETというのはPWM制御するピンに接続されているFETを指します。
Low側FETというのはPWM制御はせず、GPIO出力で制御するFETを指します。
次のコードが全体です。

//cmd{
    let mut timer3 = pwm::Timer3Pwm::new(dp.TC3, pwm::Prescaler::Prescale64);
    let fet_u_high_pin = pins.d5.into_output(&mut pins.ddr).into_pwm(&mut timer3);
    let fet_v_high_pin = pins.d2.into_output(&mut pins.ddr).into_pwm(&mut timer3);
    let fet_w_high_pin = pins.d3.into_output(&mut pins.ddr).into_pwm(&mut timer3);
    let fet_u_low_pin = pins.d6.into_output(&mut pins.ddr);
    let fet_v_low_pin = pins.d7.into_output(&mut pins.ddr);
    let fet_w_low_pin = pins.d8.into_output(&mut pins.ddr);

    motor_control::pwm_init(fet_u_high_pin, fet_v_high_pin, fet_w_high_pin,
                            fet_u_low_pin, fet_v_low_pin, fet_w_low_pin);
//}

次のコードはTimer/Counter3でクロックを64分周したTimer3Pwmを定義しています。

クロックは16MHzなので16(MHz)/ 64(分周)でPWM周波数は250kHzになります。
//cmd{
    let mut timer3 = pwm::Timer3Pwm::new(dp.TC3, pwm::Prescaler::Prescale64);
//}

次のコードはHigh側FETにつながっている3つのピンを出力ポート、PWMに設定しています。
//cmd{
    let fet_u_high_pin = pins.d5.into_output(&mut pins.ddr).into_pwm(&mut timer3);
    let fet_v_high_pin = pins.d2.into_output(&mut pins.ddr).into_pwm(&mut timer3);
    let fet_w_high_pin = pins.d3.into_output(&mut pins.ddr).into_pwm(&mut timer3);
//}

次のコードはLow側FETにつながっている3つのピンを出力ポートに設定しています。
//cmd{
    let fet_u_low_pin = pins.d6.into_output(&mut pins.ddr);
    let fet_v_low_pin = pins.d7.into_output(&mut pins.ddr);
    let fet_w_low_pin = pins.d8.into_output(&mut pins.ddr);
//}

次のコードはHigh側FET, Low側FETの駆動するための初期設定をおこないます。
初期設定は次のファイルに定義しています。

 * boards/arduino-mega2560/examples/motor_control.rs pwm_init関数

//cmd{
    motor_control::pwm_init(fet_u_high_pin, fet_v_high_pin, fet_w_high_pin,
                            fet_u_low_pin, fet_v_low_pin, fet_w_low_pin);
//}

初期設定のコードは次のとおりです。
//cmd{
pub fn pwm_init(fet_u_high_pin:port::porte::PE3<port::mode::Pwm<pwm::Timer3Pwm>>, 
                fet_v_high_pin:port::porte::PE4<port::mode::Pwm<pwm::Timer3Pwm>>,
                fet_w_high_pin:port::porte::PE5<port::mode::Pwm<pwm::Timer3Pwm>>,
                fet_u_low_pin:port::porth::PH3<port::mode::Output>,
                fet_v_low_pin:port::porth::PH4<port::mode::Output>,
                fet_w_low_pin:port::porth::PH5<port::mode::Output>){

    unsafe {
        FET_U_HIGH_PIN = Some(fet_u_high_pin);
        FET_V_HIGH_PIN = Some(fet_v_high_pin);
        FET_W_HIGH_PIN = Some(fet_w_high_pin);
        FET_U_LOW_PIN = Some(fet_u_low_pin);
        FET_V_LOW_PIN = Some(fet_v_low_pin);
        FET_W_LOW_PIN = Some(fet_w_low_pin);
    }

    set_fet_stop_pattern();

    enable_pwm_fet_u_high();
    enable_pwm_fet_v_high();
    enable_pwm_fet_w_high();
}
//}

次のコードはホールセンサと同様にHigh側FET、Low側FETのピンを初期設定の外で操作したいので
staticグローバル変数にピンを設定します。

//cmd{
    unsafe {
        FET_U_HIGH_PIN = Some(fet_u_high_pin);
        FET_V_HIGH_PIN = Some(fet_v_high_pin);
        FET_W_HIGH_PIN = Some(fet_w_high_pin);
        FET_U_LOW_PIN = Some(fet_u_low_pin);
        FET_V_LOW_PIN = Some(fet_v_low_pin);
        FET_W_LOW_PIN = Some(fet_w_low_pin);
    }
//}

初期設定後にモータが回転することがないようにすべてのFETをOFFしモータを停止しておきます。
//cmd{
    set_fet_stop_pattern();
//}

この関数を実行するとHigh FETのPWMを有効にします。
//cmd{
    enable_pwm_fet_u_high();
    enable_pwm_fet_v_high();
    enable_pwm_fet_w_high();
//}

=== 割り込み許可
マイコンの割り込みを許可します。
外部割り込みの初期設定で個別に外部割り込みを許可していましたがこちらの割り込みを許可しないと外部割り込みも発生しません。
割り込みの大元の設定になります。
//cmd{
    // Enable interrupts
    unsafe {
        avr_device::interrupt::enable();
    }
//}


== 駆動パターン設定
FETに駆動パターンを設定します。
Cの該当関数は【setFETDrivePattern】でした。

駆動パターンの設定は大きく次の処理があります。
 * ホールセンサーを読み出しモータ現在位置を取得する。
 * ホールセンサーの値により6個のFETに適切な通電パターンを設定する。

この通電パターンが@<img>{HallandPWMControl}になります。
//image[HallandPWMControl][モーター制御のタイミングチャート]{
//}

Rustのモータ駆動パターンの最上位の関数は次のファイルに定義しています。

 * boards/arduino-mega2560/examples/motor_control.rs set_fet_drive_pattern関数

set_fet_drive_patternから次を行います。
 1. ホールセンサーを読み出しモータ現在位置を取得する。
 2. ホールセンサーの値により6個のFETに適切な通電パターンを設定する。

set_fet_drive_patterのコードは次のとおりです。

//cmd{
pub fn set_fet_drive_pattern(){
    // ホールセンサの位置を取得する
    let _hall_sensor_position = hall_sensor::get_position();
    // ホールセンサの位置からFET各通電パターンを取得しFETを通電する
    drive_fet(get_fet_drive_pattern(_hall_sensor_position));
}
//}

ホールセンサ位置取得、 通電パターン設定の実装を掘り下げて説明します。

=== ホールセンサ位置取得
ホールセンサ位置取得get_positionは次のファイルに定義しています。

 * boards/arduino-mega2560/examples/hall_sensors.rs

コードはこちらです。
//cmd{
pub fn get_position() -> u8{
    unsafe {
        (HALL_W_PIN.as_mut().unwrap().is_high().void_unwrap() as u8) << 2 |
        (HALL_V_PIN.as_mut().unwrap().is_high().void_unwrap() as u8) << 1 |
        HALL_U_PIN.as_mut().unwrap().is_high().void_unwrap() as u8
    }
}
//}
@<hd>{ホールセンサの外部割り込み初期設定}で設定したグローバル変数からホールセンサー入力ポート状態を取得します。
ホールセンサーは3つ(U相・V相・W相)ありますが、U相は0bit目、V相は1bit目、W相は2bit目に割り当て戻り値として返します。

次のCのコード(main.cpp setFETDrivePattern関数より引用)と等価です。
//cmd{
void setFETDrivePattern()
{
	byte hallSensorPosition;	// ホールセンサー位置
	
	hallSensorPosition = digitalRead(HALL_W_PORT) << 2 | 
						digitalRead(HALL_V_PORT) << 1 | 
						digitalRead(HALL_U_PORT);

//}

=== 通電パターン設定
通電パターン設定drive_fetは次のファイルに定義しています。

 * boards/arduino-mega2560/examples/motor_control.rs

motor_controlは引数にget_fet_drive_pattern関数の戻り値を設定しています。
コードはこちらです。

//cmd{
    // ホールセンサの位置からFET各通電パターンを取得しFETを通電する
    drive_fet(get_fet_drive_pattern(_hall_sensor_position));
//}

get_fet_drive_pattern関数は前述したホールセンサー値を引数に渡すとFETの通電パターンを返す関数です。
drive_fet関数にget_fet_drive_pattern関数の戻り値を与えるとモータの駆動をおこないます。

get_fet_drive_pattern関数、drive_fet関数の順番に説明します。

==== ホールセンサ値からFET通電パターンの取得(get_fet_drive_pattern関数)
get_fet_drive_pattern関数は次のファイルに定義しています。

 * boards/arduino-mega2560/examples/motor_control.rs

Cの該当処理はmain.cpp setFETDrivePattern関数のswitch文です。

Rustのコードは次のとおりです。
個人的にこの関数はRustっぽい実装をしたと感じています。
引数にホールセンサー入力値(U相:0bit目、V相:1bit目、W相:2bit目)を与えると、
戻り値として次のデータをタプル型として返します。

 1. U相 High側FET PWM Duty 設定値(0〜255)
 2. V相 High側FET PWM Duty 設定値(0〜255)
 3. W相 High側FET PWM Duty 設定値(0〜255)
 4. U相 Low側FET 出力ポートのレベル 設定値(true or false)
 5. V相 Low側FET 出力ポートのレベル 設定値(true or false)
 6. W相 Low側FET 出力ポートのレベル 設定値(true or false)

//cmd{
fn get_fet_drive_pattern(hall_sensor_potion:u8) -> (u8, u8, u8, bool, bool, bool) {
//fn get_fet_drive_pattern(hall_sensor_potion:u8) -> (u16, u16, u16, bool, bool, bool) {
    match hall_sensor_potion {
        HALL_SENSOR_POSITION_5 => (load_pwm_duty(), 0, 0, false, true, false),
        HALL_SENSOR_POSITION_1 => (load_pwm_duty(), 0, 0, false, false, true),
        HALL_SENSOR_POSITION_3 => (0, load_pwm_duty(), 0, false, false, true),
        HALL_SENSOR_POSITION_2 => (0, load_pwm_duty(), 0, true, false, false),
        HALL_SENSOR_POSITION_6 => (0, 0, load_pwm_duty(), true, false, false),
        HALL_SENSOR_POSITION_4 => (0, 0, load_pwm_duty(), false, true, false),
        _ => (0, 0, 0, false, false, false),
    }
}
//}

動作の一例を説明します。
@<hd>{駆動パターン設定}で書いたタイミングチャートを再掲します。

//image[HallandPWMControl][モーター制御のタイミングチャート]{
//}



== 駆動停止設定



== 周期処理
