
= C-Rust 実装比較
//footnote[rust_code_link][https://github.com/grace2riku/avr-hal/tree/add_evkart]

CとRustの実装を比較します。

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

//list[io_def][IOピンの定義]{
    let dp = arduino_mega2560::Peripherals::take().unwrap();

    let mut pins = arduino_mega2560::Pins::new(
        dp.PORTA, dp.PORTB, dp.PORTC, dp.PORTD, dp.PORTE, dp.PORTF, dp.PORTG, dp.PORTH,
        dp.PORTJ, dp.PORTK, dp.PORTL,
    );
//}

=== 可変抵抗のAD変換
可変抵抗が接続されているAD変換ピンA0の初期設定をおこないます。

該当コードは次の部分です。
//list[hardwaresetup_ad][可変抵抗のAD変換]{
    let mut adc = arduino_mega2560::adc::Adc::new(dp.ADC, Default::default());
    let mut a0 = pins.a0.into_analog_input(&mut adc);
//}

=== タイマ
8-bit Timer/Counter0に10msタイマの周期を設定します。

 * boards/arduino-mega2560/examples/timer.rs init関数

該当コードは次の部分です。8-bit Timer/Counter0を示す【dp.TC0】を引数で渡しています。

タイマの関数はtimer.rsにまとめています。
main関数に書いてもよいと思いますが、タイマの機能はmainとは切り分けて独立させたかったので
このような実装にしました。

//list[hardwaresetup_timer][タイマの初期設定]{
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

tc0.***の***の記述がレジスタを示しています。
レジスタについての詳細は
マイコンデータシート(https://ww1.microchip.com/downloads/en/DeviceDoc/ATmega640-1280-1281-2560-2561-Datasheet-DS40002211A.pdf)
を参照ください。

このコードはタイマのモード設定です。
//emlist[タイマのモード設定]{
    tc0.tccr0a.write(|w| w.wgm0().ctc());
//}

タイマをCTCモードに設定しています。CTCモードはタイマカウントがコンペアレジスタ(ocr0a)の設定値と
一致したらタイマカウントをクリアするモードのようです。
マイコンデータシートのFigure 16-5. CTC Mode, Timing Diagramの動作になります。

このコードはコンペアレジスタの設定です。タイマカウント周波数(16MHz / 1024分周)で10msをつくりたいため、
156カウント(10ms / (1 / タイマカウント周波数) )を設定しています。

//emlist[コンペアレジスタの設定]{
    tc0.ocr0a.write(|w| unsafe { w.bits(TIMER_COUNTS as u8) });
//}


このコードはクロックの分周設定です。
ターゲット基板のシステムクロック16MHzを1024分周し、それをタイマカウントの動作クロックとします。
//emlist[クロックの分周設定]{
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
//emlist[タイマの割り込み有効化]{
    tc0.timsk0.write(|w| w.ocie0a().set_bit());
//}

割り込みハンドラはtimer.rsのTIMER0_COMPA()です。


=== ホールセンサの外部割り込み
ホールセンサが接続されているピンを入力・プルアップ設定します。

//emlist[ホールセンサの入力・プルアップ設定]{
    let hall_u = pins.d19.into_pull_up_input(&mut pins.ddr);
    let hall_v = pins.d20.into_pull_up_input(&mut pins.ddr);
    let hall_w = pins.d21.into_pull_up_input(&mut pins.ddr);
    hall_sensor::init(dp.EXINT, hall_u, hall_v, hall_w);
//}

その後、そのピンに外部割り込みの設定をします。
//emlist[ホールセンサの外部割り込み初期設定関数の呼び出し]{
    hall_sensor::init(dp.EXINT, hall_u, hall_v, hall_w);
//}

設定関数は次のファイルに定義しています。

 * boards/arduino-mega2560/examples/hall_sensor.rs init関数

タイマと同様、外部割り込みの関数を別ファイルにまとめています。
init関数全体は次のとおりです。
処理のまとまりごとに少し詳しく説明します。
//list[ext_interrupt_init][外部割り込み初期設定]{
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

このコードはinit関数の外でホールセンサの入力状態を知りたいので、
staic外部変数【HALL_*_PIN】に引数で渡された入力ポートを設定しています。
外部変数はすべて大文字にするのがRustのセオリーのようです。

static外部変数のアクセスはunsafeで括ります。
//emlist[ホールセンサ外部変数の設定]{
    unsafe {
        HALL_U_PIN = Some(u_phase);
        HALL_V_PIN = Some(v_phase);　
        HALL_W_PIN = Some(w_phase);
    }
//}

次のコードはホールセンサが接続されている外部割り込み端子の割り込みエッジを選択しています。
マイコンデータシートのeicraレジスタ(15.2.1　EICRA – External Interrupt Control Register A)を
参照すると何をしているかわかります。

ホールセンサは信号の立ち上がり・立ち下がりを検出したいので両エッジ割り込みを設定しています。

//emlist[ホールセンサ外部割り込みエッジ設定]{
    // INT2 hall sensor-U 両エッジ割り込みに設定
    ei.eicra.write(|w| w.isc2().bits(0x01));

    // INT1 hall sensor-V 両エッジ割り込みに設定
    ei.eicra.write(|w| w.isc1().bits(0x01));

    // INT0 hall sensor-W 両エッジ割り込みに設定
    ei.eicra.write(|w| w.isc0().bits(0x01));
//}

次のコードはホールセンサが接続されている外部割り込みピン(INT2,INT1,INT0)の割り込みを有効にしています。
マイコンの全割り込みを有効にしたあとに外部割り込みピンの信号レベルに変化があると割り込みハンドラにジャンプします。
外部割り込みの割り込みハンドラは次のとおりです。

 * INT2
 * INT1
 * INT0

//emlist[ホールセンサ外部割り込み有効化]{
    // INT2,1,0 interrupt enable
    ei.eimsk.write(|w| w.int().bits(0x07));
//}


=== LEDの出力ポート設定
次のコードがLEDの出力ポート設定です。

//emlist[LEDの出力ポート設定の全体]{
    let user_led = pins.d13.into_output(&mut pins.ddr);
    let hall_u_led = pins.d23.into_output(&mut pins.ddr);
    let hall_v_led = pins.d25.into_output(&mut pins.ddr);
    let hall_w_led = pins.d27.into_output(&mut pins.ddr);
    led::init(user_led, hall_u_led, hall_v_led, hall_w_led);
//}

main関数で次のLEDを出力ポートに設定しています。

 * Arduino MEGA基板に実装されているLED
 * ホールセンサU・V・W　確認用LED

//emlist[LEDの出力ポート設定]{
    let user_led = pins.d13.into_output(&mut pins.ddr);
    let hall_u_led = pins.d23.into_output(&mut pins.ddr);
    let hall_v_led = pins.d25.into_output(&mut pins.ddr);
    let hall_w_led = pins.d27.into_output(&mut pins.ddr);
//}

出力に設定したLEDをLED初期化に渡します。
//emlist[LED初期化関数の呼び出し]{
    led::init(user_led, hall_u_led, hall_v_led, hall_w_led);
//}

タイマと同じくLEDの関数をled.rsにまとめています。
次のコードがLED初期化です。
//list[led_init][LED初期化]{
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

やっていることは@<hd>{ホールセンサの外部割り込み}と同じで
init関数の外でLEDを操作したいのでstatic外部変数に引数のLEDポートを設定します。


=== FETのPWM出力、出力ポート設定
High側のFET3つ、Low側のFET3つを初期設定します。
このHigh側FETというのはPWM制御するピンに接続されているFETを指します。
Low側FETというのはPWM制御はせず、GPIO出力で制御するFETを指します。
次のコードが全体です。

//emlist[PWM初期設定全体]{
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
//emlist[PWM分周設定]{
    let mut timer3 = pwm::Timer3Pwm::new(dp.TC3, pwm::Prescaler::Prescale64);
//}

次のコードはHigh側FETにつながっている3つのピンを出力ポート、PWMに設定しています。
//emlist[High側FET PWM設定]{
    let fet_u_high_pin = pins.d5.into_output(&mut pins.ddr).into_pwm(&mut timer3);
    let fet_v_high_pin = pins.d2.into_output(&mut pins.ddr).into_pwm(&mut timer3);
    let fet_w_high_pin = pins.d3.into_output(&mut pins.ddr).into_pwm(&mut timer3);
//}

次のコードはLow側FETにつながっている3つのピンを出力ポートに設定しています。
//emlist[Low側FET GPIO出力設定]{
    let fet_u_low_pin = pins.d6.into_output(&mut pins.ddr);
    let fet_v_low_pin = pins.d7.into_output(&mut pins.ddr);
    let fet_w_low_pin = pins.d8.into_output(&mut pins.ddr);
//}

次のコードはHigh側FET, Low側FETの駆動するための初期設定をおこないます。
初期設定は次のファイルに定義しています。

 * boards/arduino-mega2560/examples/motor_control.rs pwm_init関数

//emlist[PWM初期設定関数の呼び出し]{
    motor_control::pwm_init(fet_u_high_pin, fet_v_high_pin, fet_w_high_pin,
                            fet_u_low_pin, fet_v_low_pin, fet_w_low_pin);
//}

初期設定のコードは次のとおりです。
//list[pwm_init][PWM初期設定]{
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

//emlist[FETのポートを外部変数に割り当て]{
    unsafe {
        FET_U_HIGH_PIN = Some(fet_u_high_pin);
        FET_V_HIGH_PIN = Some(fet_v_high_pin);
        FET_W_HIGH_PIN = Some(fet_w_high_pin);
        FET_U_LOW_PIN = Some(fet_u_low_pin);
        FET_V_LOW_PIN = Some(fet_v_low_pin);
        FET_W_LOW_PIN = Some(fet_w_low_pin);
    }
//}

初期設定後にモータが回転することがないようにすべてのFETをOFFしモータを停止します。
//emlist[FET OFF]{
    set_fet_stop_pattern();
//}

この関数を実行するとHigh側 FETのPWMを有効にします。
//emlist[PWM有効]{
    enable_pwm_fet_u_high();
    enable_pwm_fet_v_high();
    enable_pwm_fet_w_high();
//}

=== 割り込み許可
マイコンの割り込みを許可します。
外部割り込みの初期設定で個別に外部割り込みを許可していましたがこちらの割り込みを許可しないと外部割り込みも発生しません。
割り込みの大元の設定になります。
//emlist[割り込み許可]{
    // Enable interrupts
    unsafe {
        avr_device::interrupt::enable();
    }
//}


== 駆動パターン設定
FETに駆動パターンを設定します。
Cの該当関数は【setFETDrivePattern】でした。

駆動パターンの設定は大きく次の処理があります。

 * ホールセンサを読み出しモータ現在位置を取得する。
 * ホールセンサの値により6個のFETに適切な通電パターンを設定する。

この通電パターンが@<img>{HallandPWMControl}になります。
//image[HallandPWMControl][モータ制御のタイミングチャート]{
//}

Rustのモータ駆動パターンの最上位の関数は次のファイルに定義しています。

 * boards/arduino-mega2560/examples/motor_control.rs set_fet_drive_pattern関数

set_fet_drive_patternから次を行います。

 1. ホールセンサを読み出しモータ現在位置を取得する。
 2. ホールセンサの値により6個のFETに適切な通電パターンを設定する。

set_fet_drive_patterのコードは次のとおりです。

//list[set_fet_drive_pattern][駆動パターン設定]{
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
//list[get_position][ホールセンサ位置取得]{
pub fn get_position() -> u8{
    unsafe {
        (HALL_W_PIN.as_mut().unwrap().is_high().void_unwrap() as u8) << 2 |
        (HALL_V_PIN.as_mut().unwrap().is_high().void_unwrap() as u8) << 1 |
        HALL_U_PIN.as_mut().unwrap().is_high().void_unwrap() as u8
    }
}
//}
@<hd>{ホールセンサの外部割り込み}で設定したグローバル変数からホールセンサ入力ポート状態を取得します。
ホールセンサは3つ(U相・V相・W相)ありますが、U相は0bit目、V相は1bit目、W相は2bit目に割り当て戻り値として返します。

次のCのコード(main.cpp setFETDrivePattern関数)と等価です。
//emlist[C版 ホールセンサ位置取得]{
void setFETDrivePattern()
{
	byte hallSensorPosition;	// ホールセンサ位置
	
	hallSensorPosition = digitalRead(HALL_W_PORT) << 2 | 
						digitalRead(HALL_V_PORT) << 1 | 
						digitalRead(HALL_U_PORT);

//}

=== 通電パターン設定
通電パターン設定drive_fetは次のファイルに定義しています。

 * boards/arduino-mega2560/examples/motor_control.rs

drive_fetは引数にget_fet_drive_pattern関数の戻り値を設定しています。
コードはこちらです。

//emlist[通電パターン設定]{
    // ホールセンサの位置からFET各通電パターンを取得しFETを通電する
    drive_fet(get_fet_drive_pattern(_hall_sensor_position));
//}

get_fet_drive_pattern関数は前述したホールセンサ値を引数に渡すとFETの通電パターンを返す関数です。
drive_fet関数にget_fet_drive_pattern関数の戻り値を与えるとモータの駆動をおこないます。

get_fet_drive_pattern関数、drive_fet関数の順番に説明します。

==== ホールセンサ値からFET通電パターンの取得(get_fet_drive_pattern関数)
get_fet_drive_pattern関数は次のファイルに定義しています。

 * boards/arduino-mega2560/examples/motor_control.rs

Cの該当処理はmain.cpp setFETDrivePattern関数のswitch文です。

Rustのコードは次のとおりです。
個人的にこの関数はRustっぽい実装をしたと感じています。
引数にホールセンサ入力値(U相:0bit目、V相:1bit目、W相:2bit目)を与えると、
戻り値として次をタプル型として返します。

 1. U相 High側FET PWM Duty 設定値(0〜255)
 2. V相 High側FET PWM Duty 設定値(0〜255)
 3. W相 High側FET PWM Duty 設定値(0〜255)
 4. U相 Low側FET 出力ポートのレベル 設定値(true or false)
 5. V相 Low側FET 出力ポートのレベル 設定値(true or false)
 6. W相 Low側FET 出力ポートのレベル 設定値(true or false)

//list[get_fet_drive_pattern][ホールセンサ値からFET通電パターンの取得]{
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
//image[HallandPWMControl][モータ制御のタイミングチャート]{
//}

この図の通電ステージ1の場合、ホールセンサ入力値は次になります。

 * ホールセンサ U = Low → Highレベル 0bit目に割り当て
 * ホールセンサ V = Lowレベル 1bit目に割り当て
 * ホールセンサ W = Highレベル 2bit目に割り当て

FETの通電パターンの期待値は次になります。

 * U相High側FET → PWM制御　任意のDuty設定
 * U相Low側FET → GPIO Lowレベル出力
 * V相High側FET → PWM制御　0% Duty
 * V相Low側FET → GPIO Highレベル出力
 * W相High側FET → PWM制御　0% Duty
 * W相Low側FET → GPIO Lowレベル出力

get_fet_drive_pattern関数の引数hall_sensor_potionに5がセットされ関数呼び出しされます。
引数hall_sensor_potionが5の場合は次のmatch式が該当します。

//emlist[通電ステージ1の場合の条件]{
       HALL_SENSOR_POSITION_5 => (load_pwm_duty(), 0, 0, false, true, false),
//}

タプル型は左から次のデータとして実装しています。

 * U相High側FET, V相High側FET, W相High側FET, U相Low側FET, V相Low側FET, W相Low側FET
 
通電パターン1の場合は次の値を設定することを想定しています。

 * U相High側FET → PWM Duty (可変抵抗設定値)
 * V相High側FET → 0 (PWM Duty 0%)
 * W相High側FET → 0 (PWM Duty 0%)
 * U相Low側FET → false
 * V相Low側FET → true
 * W相Low側FET → false

Low側FETのbool型はこの後の処理でtrueだったらGPIO Highレベル出力、
falseだったらGPIO Lowレベルに解釈されポート出力します。

==== FET通電パターン設定(drive_fet関数)
get_fet_drive_pattern関数の戻り値をdrive_fet関数の引数として利用します。
drive_fet関数は引数に通電パターンを受け取るとモータ駆動するためにFETに設定します。
結果、モータが駆動します。

次がdrive_fet関数のコードです。
//list[drive_fet][FET通電パターン設定]{
fn drive_fet(uvw_phase_values: (u8, u8, u8, bool, bool, bool)){
    let (u_high, v_high, w_high, u_low, v_low, w_low) = uvw_phase_values;
    unsafe {
        FET_U_HIGH_PIN.as_mut().unwrap().set_duty(u_high);
        FET_V_HIGH_PIN.as_mut().unwrap().set_duty(v_high);
        FET_W_HIGH_PIN.as_mut().unwrap().set_duty(w_high);
        if u_low == true { FET_U_LOW_PIN.as_mut().unwrap().set_high().void_unwrap(); }
        else {FET_U_LOW_PIN.as_mut().unwrap().set_low().void_unwrap();}

        if v_low == true { FET_V_LOW_PIN.as_mut().unwrap().set_high().void_unwrap(); }
        else {FET_V_LOW_PIN.as_mut().unwrap().set_low().void_unwrap();}

        if w_low == true { FET_W_LOW_PIN.as_mut().unwrap().set_high().void_unwrap(); }
        else {FET_W_LOW_PIN.as_mut().unwrap().set_low().void_unwrap();}
    }
}
//}

FETに操作するための変数はすべてstatic外部変数として定義しているのでunsafeで括っています。

こちらのコードはHigh側FETの設定です。
High側FETはPWM機能で初期設定しているのでset_duty関数でPWM Dutyを設定しています。
//emlist[High側FETの設定]{
        FET_U_HIGH_PIN.as_mut().unwrap().set_duty(u_high);
        FET_V_HIGH_PIN.as_mut().unwrap().set_duty(v_high);
        FET_W_HIGH_PIN.as_mut().unwrap().set_duty(w_high);
//}

こちらのコードはLow側FETの設定です。
Low側FETはGPIO出力で初期設定しているのでset_high関数またはset_low関数で設定します。

//emlist[Low側FETの設定]{
        if u_low == true { FET_U_LOW_PIN.as_mut().unwrap().set_high().void_unwrap(); }
        else {FET_U_LOW_PIN.as_mut().unwrap().set_low().void_unwrap();}

        if v_low == true { FET_V_LOW_PIN.as_mut().unwrap().set_high().void_unwrap(); }
        else {FET_V_LOW_PIN.as_mut().unwrap().set_low().void_unwrap();}

        if w_low == true { FET_W_LOW_PIN.as_mut().unwrap().set_high().void_unwrap(); }
        else {FET_W_LOW_PIN.as_mut().unwrap().set_low().void_unwrap();}
//}

== 駆動停止設定
駆動停止設定set_fet_stop_patternは次のファイルに定義しています。

 * boards/arduino-mega2560/examples/motor_control.rs

コードはこちらです。
//list[set_fet_stop_pattern][駆動停止設定]{
pub fn set_fet_stop_pattern(){
    drive_fet((0, 0, 0, false, false, false));
}
//}

この関数呼び出しでモータが停止します。
@<hd>{FET通電パターン設定(drive_fet関数)}で説明したdrive_fet関数の引数に
High側FETはすべてDuty 0%、Low側FETはすべてGPIO出力 Lowを指定し呼び出します。
Cのコードmain.cpp setFETStopPattern関数と等価です。


== 周期処理
周期処理は次のファイルに定義しています。

 * boards/arduino-mega2560/examples/timer.rs
 * boards/arduino-mega2560/examples/evkart-main.rs

タイマ割り込みハンドラでは制御タイミングのフラグセット、1秒間隔のLED点滅を実行します。
Cと少し実装が違い、Rustでは制御タイミングのフラグセット・LED点滅のみ実行します。
タイマ割り込みハンドラからC同様にAD変換を実行するには
main関数のはじめで初期設定したAD変換の変数(a0)をグローバル化するなどの対応が考えられます。
今回はそこまで実装せず、タイマ割り込みハンドラでは制御タイミングのフラグをセットするだけで
AD変換はmain関数のループの中で制御フラグを確認して実行するようにしました。


タイマ割り込みハンドラのコードはこちらです。
//list[TIMER0_COMPA][タイマ割り込みハンドラ]{
#[avr_device::interrupt(atmega2560)]
fn TIMER0_COMPA() {
    unsafe {
        motor_control::set_speed_control_timing(true);

        COUNTER_1SEC += 1;
        if COUNTER_1SEC >= COUNT_1SEC {
            COUNTER_1SEC = 0;
            led::toggle_user();
        }
    }
}
//}


制御タイミングのフラグを確認し、可変抵抗のAD変換、PWM Duty設定は
@<list>{main_loop} evkart-main.rs main関数のloopの中で実行しています。

//list[main_loop][main関数のloop]{
loop {
    if motor_control::get_speed_control_timing() == true {
        let ad0:u16 = nb::block!(adc.read(&mut a0)).void_unwrap();
        motor_control::save_pwm_duty((ad0 / 4) as u8);
//            motor_control::save_pwm_duty(ad0);

        if motor_control::load_pwm_duty() > VOL_0PCT_POINT {
            if motor_control::get_drive_state() == motor_control::DriveState::Stop {
                motor_control::set_drive_state(motor_control::DriveState::Drive);
                //モータ停止中のため強制駆動する
                motor_control::set_fet_drive_pattern();
            }
        } else {
            motor_control::set_drive_state(motor_control::DriveState::Stop);
            motor_control::save_pwm_duty(0);
            // モータを停止する
            motor_control::set_fet_stop_pattern();
        }

        motor_control::set_speed_control_timing(false);
    }
//}

loopの中で呼び出している関数の機能は次のとおりです。

 * get_speed_control_timing関数：制御タイミングを示すフラグの取得。制御タイミングでtrueが返る。
 * save_pwm_duty関数: AD変換値をmotor_control.rsのグローバル変数に保存する。
 * load_pwm_duty関数: save_pwm_duty関数で保存したPWM Dutyを読み出す。
 * get_drive_state関数: モータ駆動状態を返す関数。初期値はモータ停止状態のDriveState::Stopを返す。
 * set_drive_state関数: モータ駆動状態を設定する関数。モータ駆動状態が変わったら引数に遷移後の状態をセットし呼び出す。
 * set_fet_drive_pattern関数: @<hd>{駆動パターン設定}を参照。
 * set_fet_stop_pattern関数: @<hd>{駆動停止設定}を参照。
 * set_speed_control_timing関数：制御タイミングの設定。trueが制御タイミングを示す。

if文がありますが次の条件で分岐します。

 * 可変抵抗がある閾値(==VOL_0PCT_POINT)を超えたらモータ速度調整を有効にする 
 * 可変抵抗閾値未満の場合はPWM Dutyを0%、モータ停止する
