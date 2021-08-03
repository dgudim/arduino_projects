
// ***************************** НАСТРОЙКИ *****************************

// ----- настройки параметров
#define KEEP_SETTINGS 1     // хранить ВСЕ настройки в энергонезависимой памяти
#define RESET_SETTINGS 0    // сброс настроек в EEPROM памяти (поставить 1, прошиться, поставить обратно 0, прошиться. Всё)

// ----- настройки ленты
#define NUM_LEDS 120        // количество светодиодов (данная версия поддерживает до 410 штук)
byte BRIGHTNESS = 130;      // яркость по умолчанию (0 - 255)
byte EMPTY_BRIGHT = 35;     // яркость "не горящих" светодиодов (0 - 255)

// ----- пины подключения
#define SOUND_R A3         // аналоговый пин вход аудио, правый канал
#define SOUND_L A3         // аналоговый пин вход аудио, левый канал
#define SOUND_R_FREQ A1    // аналоговый пин вход аудио для режима с частотами (через кондер)
#define BTN_PIN 7          // кнопка переключения режимов (PIN --- КНОПКА --- GND)

#define POT_GND A0              // пин земля для потенциометра
#define IR_PIN 5                // пин ИК приёмника
#define LED_PIN 11         // пин светодиодной ленты
#define MLED_PIN 13         // пин стандартого светодиода

// ----- настройки радуги
float RAINBOW_STEP = 5.00;         // шаг изменения цвета радуги

// ----- отрисовка
#define MODE 1                    // режим при запуске
#define MAIN_LOOP 5             // период основного цикла отрисовки (по умолчанию 5)

// ----- сигнал
#define MONO 1                    // 1 - только один канал (ПРАВЫЙ!!!!! SOUND_R!!!!!), 0 - два канала
#define EXP 1.1                   // степень усиления сигнала (для более "резкой" работы) (по умолчанию 1.4)
#define POTENT 1                 // 1 - используем потенциометр, 0 - используется внутренний источник опорного напряжения 1.1 В
#define EMPTY_COLOR HUE_PURPLE    // цвет "не горящих" светодиодов. Будет чёрный, если яркость 0

// ----- нижний порог шумов
uint16_t LOW_PASS = 70;          // нижний порог шумов режим VU, ручная настройка
uint16_t SPEKTR_LOW_PASS = 30;    // нижний порог шумов режим спектра, ручная настройка
#define AUTO_LOW_PASS 0           // разрешить настройку нижнего порога шумов при запуске (по умолч. 0)
#define EEPROM_LOW_PASS 1         // порог шумов хранится в энергонезависимой памяти (по умолч. 1)
#define LOW_PASS_ADD 13           // "добавочная" величина к нижнему порогу, для надёжности (режим VU)
#define LOW_PASS_FREQ_ADD 3       // "добавочная" величина к нижнему порогу, для надёжности (режим частот)

// ----- режим шкала громкости
float SMOOTH = 0.8;               // коэффициент плавности анимации VU (по умолчанию 0.5)
#define MAX_COEF 1.8              // коэффициент громкости (максимальное равно срднему * этот коэф) (по умолчанию 1.8)

// ----- режим цветомузыки
float SMOOTH_FREQ = 1.0;          // коэффициент плавности анимации частот (по умолчанию 0.8)
float MAX_COEF_FREQ = 1.5;        // коэффициент порога для "вспышки" цветомузыки (по умолчанию 1.5)
#define SMOOTH_STEP 10            // шаг уменьшения яркости в режиме цветомузыки (чем больше, тем быстрее гаснет)
#define LOW_COLOR HUE_RED         // цвет низких частот
#define MID_COLOR HUE_GREEN       // цвет средних
#define HIGH_COLOR HUE_YELLOW     // цвет высоких

// ----- режим подсветки
byte LIGHT_COLOR = 0;             // начальный цвет подсветки
byte LIGHT_SAT = 255;             // начальная насыщенность подсветки
byte COLOR_SPEED = 100;
int RAINBOW_PERIOD = 1;
float RAINBOW_STEP_2 = 0.5;

// ----- режим анализатора спектра
#define LIGHT_SMOOTH 2

// ----- КНОПКИ СВОЕГО ПУЛЬТА -----
#define BUTT_PLUS             16754775
#define BUTT_MINUS            16769055
#define BUTT_PALETTE_MINUS    16720605
#define BUTT_PALETTE_PLUS     16712445
#define BUTT_OK               16761405
#define BUTT_0                16738455
#define BUTT_1                16724175
#define BUTT_2                16718055
#define BUTT_3                16743045
#define BUTT_4                16716015
#define BUTT_5                16726215
#define BUTT_6                16734885
#define BUTT_7                16728765
#define BUTT_8                16730805
#define BUTT_9                16732845
#define BUTT_CH_MINUS         16753245
#define BUTT_CH_PLUS          16769565
#define BUTT_LOW_PASS         16748655
#define BUTT_BRIGHTNESS_ONE   16750695
#define BUTT_BRIGHTNESS_TWO   16756815
#define BUTT_BRIGHTNESS_SYNC  16761405

// ------------------------------ ДЛЯ РАЗРАБОТЧИКОВ --------------------------------
#define MODE_AMOUNT 8      // количество режимов

#define STRIPE NUM_LEDS / 5
float freq_to_stripe = NUM_LEDS / 40; // /2 так как симметрия, и /20 так как 20 частот

#define FHT_N 64         // ширина спектра х2
#define LOG_OUT 1
#include <FHT.h>         // преобразование Хартли

#include <EEPROMex.h>

#define FASTLED_ALLOW_INTERRUPTS 1
#include "FastLED.h"
CRGB leds[NUM_LEDS];

//#include "GyverButton.h"
//GButton butt1(BTN_PIN);

#include <IRremote.h>
IRrecv irrecv(IR_PIN);
decode_results results;

// градиент-палитра от зелёного к красному
DEFINE_GRADIENT_PALETTE(soundlevel_gp) {
  0,    0,    255,  0,  // green
  100,  255,  255,  0,  // yellow
  150,  255,  100,  0,  // orange
  200,  255,  50,   0,  // red
  255,  255,  0,    0   // red
};
CRGBPalette32 myPal = soundlevel_gp;

boolean on = true;
boolean computerControlled = false;

int Rlenght, Llenght;
float RsoundLevel, RsoundLevel_f;
float LsoundLevel, LsoundLevel_f;

float averageLevel = 50;
int maxLevel = 100;
int MAX_CH = NUM_LEDS / 2;
int hue;
unsigned long main_timer, color_timer, rainbow_timer, eeprom_timer;
float averK = 0.006;
byte count;
float index = (float)255 / MAX_CH;   // коэффициент перевода для палитры
boolean lowFlag;
byte low_pass;
int RcurrentLevel, LcurrentLevel;
int colorMusic[3];
float colorMusic_f[3], colorMusic_aver[3];
boolean colorMusicFlash[3];
byte this_mode = MODE;
int thisBright[3];
boolean settings_mode;
byte uv_mode, light_mode;
int freq_max;
float freq_max_f, rainbow_steps;
int freq_f[32];
int this_color;
boolean running_flag[3], eeprom_flag;

float countDownDelay = 1000;
float currentDelay;
float recieverDelay;

float currentStrobeRed;
float currentStrobeGreen;
float currentStrobeBlue;

float strobeSmoothess = 33;

const int UV_SMOOTNESS = 2;

float prevValues[UV_SMOOTNESS];
int prevI;

int currentPalette = 0;
byte brightnessSync = 0;

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
// ------------------------------ ДЛЯ РАЗРАБОТЧИКОВ --------------------------------

void setup() {
  Serial.begin(1000000);
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(BRIGHTNESS);

  pinMode(MLED_PIN, OUTPUT);        //Режим пина для светодиода режима на выход
  digitalWrite(MLED_PIN, LOW); //Выключение светодиода режима

  pinMode(POT_GND, OUTPUT);
  digitalWrite(POT_GND, LOW);
  //butt1.setTimeout(900);

  Serial.println("[#63c8ff]INFO:Enabling IRin");
  irrecv.enableIRIn(); // Start the receiver
  Serial.println("[#63c8ff]INFO:Enabled IRin");

  // для увеличения точности уменьшаем опорное напряжение,
  // выставив EXTERNAL и подключив Aref к выходу 3.3V на плате через делитель
  // GND ---[10-20 кОм] --- REF --- [10 кОм] --- 3V3
  // в данной схеме GND берётся из А0 для удобства подключения
  if (POTENT) analogReference(EXTERNAL);
  else analogReference(INTERNAL);

  // жуткая магия, меняем частоту оцифровки до 18 кГц
  // команды на ассемблере, даже не спрашивайте, как это работает
  // поднимаем частоту опроса аналогового порта до 38.4 кГц, по теореме
  // Котельникова (Найквиста) частота дискретизации будет 19.2 кГц
  // http://yaab-arduino.blogspot.ru/2015/02/fast-sampling-from-analog-input.html
  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  sbi(ADCSRA, ADPS0);

  if (RESET_SETTINGS) EEPROM.write(100, 0);        // сброс флага настроек

  if (AUTO_LOW_PASS && !EEPROM_LOW_PASS) {         // если разрешена автонастройка нижнего порога шумов
    //autoLowPass();
  }
  if (EEPROM_LOW_PASS) {                // восстановить значения шумов из памяти
    LOW_PASS = EEPROM.readInt(70);
    SPEKTR_LOW_PASS = EEPROM.readInt(72);
  }

  // в 100 ячейке хранится число 100. Если нет - значит это первый запуск системы
  if (KEEP_SETTINGS) {
    if (EEPROM.read(100) != 100) {
      EEPROM.write(100, 100);
      updateEEPROM();
    } else {
      readEEPROM();
    }
  }
  
  Serial.println("mm"+(String)this_mode+"_"+(String)uv_mode+"_"+(String)light_mode);
  Serial.println("[#63c8ff]INFO:Initialization complete");
}

void loop() {
  currentDelay = millis();
  if(recieverDelay < currentDelay){
    serialTick(); //получение и обработка serial команд
  }
  if(!computerControlled){
    remoteTick();
    if(on && recieverDelay < currentDelay){
     mainLoop();       // главный цикл обработки и отрисовки
     eepromTick();     // проверка не пора ли сохранить настройки
    }
  }
}

void mainLoop() {
  // главный цикл отрисовки
    if (millis() - main_timer > MAIN_LOOP) {
      // сбрасываем значения
      RsoundLevel = 0;
      LsoundLevel = 0;

      // перваые два режима - громкость (VU meter)
      if (this_mode == 0 || this_mode == 1) {
        for (byte i = 0; i < 100; i ++) {                                 // делаем 100 измерений
          RcurrentLevel = analogRead(SOUND_R);                            // с правого
          if (!MONO) LcurrentLevel = analogRead(SOUND_L);                 // и левого каналов

          if (RsoundLevel < RcurrentLevel) RsoundLevel = RcurrentLevel;   // ищем максимальное
          if (!MONO) if (LsoundLevel < LcurrentLevel) LsoundLevel = LcurrentLevel;   // ищем максимальное
        }

        // фильтруем по нижнему порогу шумов
        RsoundLevel = map(RsoundLevel, LOW_PASS, 1023, 0, 500);
        if (!MONO)LsoundLevel = map(LsoundLevel, LOW_PASS, 1023, 0, 500);

        // ограничиваем диапазон
        RsoundLevel = constrain(RsoundLevel, 0, 500);
        if (!MONO)LsoundLevel = constrain(LsoundLevel, 0, 500);

        // возводим в степень (для большей чёткости работы)
        RsoundLevel = pow(RsoundLevel, EXP);
        if (!MONO)LsoundLevel = pow(LsoundLevel, EXP);

        // фильтр
        RsoundLevel_f = RsoundLevel * SMOOTH + RsoundLevel_f * (1 - SMOOTH);
        if (!MONO)LsoundLevel_f = LsoundLevel * SMOOTH + LsoundLevel_f * (1 - SMOOTH);

        float sum = RsoundLevel_f;
        for(int i = 0; i < UV_SMOOTNESS; i++){
           sum += prevValues[i];
        }
        float average = sum / (UV_SMOOTNESS + 1);
       
        prevValues[prevI] = RsoundLevel_f;
        prevI++;
        if(prevI > UV_SMOOTNESS - 1){
          prevI = 0;
        }

        RsoundLevel_f = average;
        
        if (MONO) LsoundLevel_f = RsoundLevel_f;  // если моно, то левый = правому

        // расчёт общей средней громкости с обоих каналов, фильтрация.
        // Фильтр очень медленный, сделано специально для автогромкости
        averageLevel = (float)(RsoundLevel_f + LsoundLevel_f) / 2 * averK + averageLevel * (1 - averK);

        // принимаем максимальную громкость шкалы как среднюю, умноженную на некоторый коэффициент MAX_COEF
        maxLevel = (float)averageLevel * MAX_COEF;

        // преобразуем сигнал в длину ленты (где MAX_CH это половина количества светодиодов)
        Rlenght = map(RsoundLevel_f, 0, maxLevel, 0, MAX_CH);
        Llenght = map(LsoundLevel_f, 0, maxLevel, 0, MAX_CH);

        // ограничиваем до макс. числа светодиодов
        Rlenght = constrain(Rlenght, 0, MAX_CH);
        Llenght = constrain(Llenght, 0, MAX_CH);

        animation();       // отрисовать
      }

      // 3-5 режим - цветомузыка
      if (this_mode == 2 || this_mode == 3 || this_mode == 4 || this_mode == 6 || this_mode == 7 || this_mode == 8) {
        analyzeAudio();
        colorMusic[0] = 0;
        colorMusic[1] = 0;
        colorMusic[2] = 0;
        for (int i = 0 ; i < 32 ; i++) {
          if (fht_log_out[i] < SPEKTR_LOW_PASS) fht_log_out[i] = 0;
        }
        // низкие частоты, выборка со 2 по 5 тон (0 и 1 зашумленные!)
        for (byte i = 2; i < 6; i++) {
          if (fht_log_out[i] > colorMusic[0]) colorMusic[0] = fht_log_out[i];
        }
        // средние частоты, выборка с 6 по 10 тон
        for (byte i = 6; i < 11; i++) {
          if (fht_log_out[i] > colorMusic[1]) colorMusic[1] = fht_log_out[i];
        }
        // высокие частоты, выборка с 11 по 31 тон
        for (byte i = 11; i < 32; i++) {
          if (fht_log_out[i] > colorMusic[2]) colorMusic[2] = fht_log_out[i];
        }
        freq_max = 0;
        for (byte i = 0; i < 30; i++) {
          if (fht_log_out[i + 2] > freq_max) freq_max = fht_log_out[i + 2];
          if (freq_max < 5) freq_max = 5;

          if (freq_f[i] < fht_log_out[i + 2]) freq_f[i] = fht_log_out[i + 2];
          if (freq_f[i] > 0) freq_f[i] -= LIGHT_SMOOTH;
          else freq_f[i] = 0;
        }
        freq_max_f = freq_max * averK + freq_max_f * (1 - averK);
        for (byte i = 0; i < 3; i++) {
          colorMusic_aver[i] = colorMusic[i] * averK + colorMusic_aver[i] * (1 - averK);  // общая фильтрация
          colorMusic_f[i] = colorMusic[i] * SMOOTH_FREQ + colorMusic_f[i] * (1 - SMOOTH_FREQ);      // локальная
          if (colorMusic_f[i] > ((float)colorMusic_aver[i] * MAX_COEF_FREQ)) {
            thisBright[i] = 255;
            colorMusicFlash[i] = true;
            running_flag[i] = true;
          } else colorMusicFlash[i] = false;
          if (thisBright[i] >= 0) thisBright[i] -= SMOOTH_STEP;
          if (thisBright[i] < EMPTY_BRIGHT) {
            thisBright[i] = EMPTY_BRIGHT;
            running_flag[i] = false;
          }
        }
        animation();
      }
      
      if (this_mode == 5) animation();

      FastLED.show();         // отправить значения на ленту

      if (this_mode != 6 && this_mode != 8 && uv_mode == 0 && this_mode != 2)
        FastLED.clear();          // очистить массив пикселей
        main_timer = millis();    // сбросить таймер
      }
}

void animation() {
  // согласно режиму
  
  switch (this_mode){
    case 0:
      switch(uv_mode){
      case 0:
      count = 0;
      for (int i = (MAX_CH - 1); i > ((MAX_CH - 1) - Rlenght); i--) {
        leds[i] = ColorFromPalette(myPal, (count * index));   // заливка по палитре " от зелёного к красному"
        count++;
      }
      count = 0;
      for (int i = (MAX_CH); i < (MAX_CH + Llenght); i++ ) {
        leds[i] = ColorFromPalette(myPal, (count * index));   // заливка по палитре " от зелёного к красному"
        count++;
      }
      if (EMPTY_BRIGHT > 0) {
        CRGB this_dark = CRGB(20, 10, 10);
        for (int i = ((MAX_CH - 1) - Rlenght); i >= 0; i--)
          leds[i] = this_dark;
        for (int i = MAX_CH + Llenght; i < NUM_LEDS; i++)
          leds[i] = this_dark;
      }
      break;
       case 1:
        leds[NUM_LEDS/2 - 1] = ColorFromPalette(myPal, (Rlenght - (MAX_CH - 1)) * index);
        leds[NUM_LEDS/2] = ColorFromPalette(myPal, (Rlenght - (MAX_CH - 1)) * index);
        leds[NUM_LEDS/2 + 1] = ColorFromPalette(myPal, (Rlenght - (MAX_CH - 1)) * index);
        for (int i = 0; i < NUM_LEDS / 2 - 1; i++) {
          leds[i] = leds[i + 1];
          leds[NUM_LEDS - i - 1] = leds[i];
        }
        break;
        case 2:
        leds[NUM_LEDS/2 - 1] = CHSV(Rlenght * 2.1, 190, 255);
        leds[NUM_LEDS/2] =  CHSV(Rlenght * 2.1, 190, 255);
        leds[NUM_LEDS/2 + 1] = CHSV(Rlenght * 2.1, 190, 255);
        for (int i = 0; i < NUM_LEDS / 2 - 1; i++) {
          leds[i] = leds[i + 1];
          leds[NUM_LEDS - i - 1] = leds[i];
        }
        break;
      }
      if(brightnessSync == 1){
         FastLED.setBrightness(BRIGHTNESS + Rlenght/1.5);
      }
      break;
    case 1:
      if (millis() - rainbow_timer > 30) {
        rainbow_timer = millis();
        hue = floor((float)hue + RAINBOW_STEP);
      }
      count = 0;
      for (int i = (MAX_CH - 1); i > ((MAX_CH - 1) - Rlenght); i--) {
        leds[i] = ColorFromPalette(RainbowColors_p, (count * index) / 2 - hue);  // заливка по палитре радуга
        count++;
      }
      count = 0;
      for (int i = (MAX_CH); i < (MAX_CH + Llenght); i++ ) {
        leds[i] = ColorFromPalette(RainbowColors_p, (count * index) / 2 - hue); // заливка по палитре радуга
        count++;
      }
      if (EMPTY_BRIGHT > 0) {
        CRGB this_dark = CRGB(20, 10, 10);
        for (int i = ((MAX_CH - 1) - Rlenght); i >= 0; i--)
          leds[i] = this_dark;
        for (int i = MAX_CH + Llenght; i < NUM_LEDS; i++)
          leds[i] = this_dark;
      }
      if(brightnessSync == 1){
         FastLED.setBrightness(BRIGHTNESS + Rlenght/1.5);
      }
      break;
    case 2:
      for (int i = 0; i < NUM_LEDS; i++) {
        if (i < STRIPE)          leds[i] = CHSV(HIGH_COLOR, 255, thisBright[2]);
        else if (i < STRIPE * 2) leds[i] = CHSV(MID_COLOR, 255, thisBright[1]);
        else if (i < STRIPE * 3) leds[i] = CHSV(LOW_COLOR, 255, thisBright[0]);
        else if (i < STRIPE * 4) leds[i] = CHSV(MID_COLOR, 255, thisBright[1]);
        else if (i < STRIPE * 5) leds[i] = CHSV(HIGH_COLOR, 255, thisBright[2]);
      }
      break;
    case 3:
      for (int i = 0; i < NUM_LEDS; i++) {
        if (i < NUM_LEDS / 3)          leds[i] = CHSV(HIGH_COLOR, 255, thisBright[2]);
        else if (i < NUM_LEDS * 2 / 3) leds[i] = CHSV(MID_COLOR, 255, thisBright[1]);
        else if (i < NUM_LEDS)         leds[i] = CHSV(LOW_COLOR, 255, thisBright[0]);
      }
      break;
    case 4:
      if (colorMusicFlash[2]) HIGHS();
      else if (colorMusicFlash[1]) MIDS();
      else if (colorMusicFlash[0]) LOWS();
      else SILENCE();
      break;
    case 5:
      switch (light_mode) {
        case 0: for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(LIGHT_COLOR, LIGHT_SAT, 255);
          break;
        case 1:
          if (millis() - color_timer > COLOR_SPEED) {
            color_timer = millis();
            if (++this_color > 255) this_color = 0;
          }
          for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(this_color, LIGHT_SAT, 255);
          break;
        case 2:
          if (millis() - rainbow_timer > 30) {
            rainbow_timer = millis();
            this_color += RAINBOW_PERIOD;
            if (this_color > 255) this_color = 0;
            if (this_color < 0) this_color = 255;
          }
          rainbow_steps = this_color;
          for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = CHSV((int)floor(rainbow_steps), 255, 255);
            rainbow_steps += RAINBOW_STEP_2;
            if (rainbow_steps > 255) rainbow_steps = 0;
            if (rainbow_steps < 0) rainbow_steps = 255;
          }
          break;
      }
      break;
    case 6:
      if (running_flag[2]) leds[NUM_LEDS / 2] = CHSV(HIGH_COLOR, 255, thisBright[2]);
      else if (running_flag[1]) leds[NUM_LEDS / 2] = CHSV(MID_COLOR, 255, thisBright[1]);
      else if (running_flag[0]) leds[NUM_LEDS / 2] = CHSV(LOW_COLOR, 255, thisBright[0]);
      else leds[NUM_LEDS / 2] = CHSV(EMPTY_COLOR, 255, EMPTY_BRIGHT);
      
      leds[(NUM_LEDS / 2) - 1] = leds[NUM_LEDS / 2];
      for (int i = 0; i < NUM_LEDS / 2 - 1; i++) {
        leds[i] = leds[i + 1];
        leds[NUM_LEDS - i - 1] = leds[i];
      }
      break;
    case 7:
    case 8:
      byte HUEindex = 0;
      for (int i = 0; i < NUM_LEDS / 2; i++) {
        byte this_bright = map(freq_f[(int)floor((NUM_LEDS / 2 - i) / freq_to_stripe)], 0, freq_max_f, 0, 255);
        this_bright = constrain(this_bright, 0, 255);
        if(this_bright > 10){
           leds[i] = CHSV(HUEindex, 190, this_bright);
           leds[NUM_LEDS - i - 1] = leds[i];
        }
        HUEindex += 4;
        if (HUEindex > 255) HUEindex = 0;
      }
      if(this_mode == 8){
        for (int i = 0; i < NUM_LEDS / 2 - 2; i++) {
           leds[i] = leds[i + 1];
           leds[NUM_LEDS - i - 1] = leds[i];
        }
      }
      break;
  }
}

void HIGHS() {
  currentStrobeRed = constrain(currentStrobeRed, 0, 255);
  currentStrobeGreen = constrain(currentStrobeGreen + strobeSmoothess, 0, 255);
  currentStrobeBlue = constrain(currentStrobeBlue, 0, 255);
  fillStrobe();
}
void MIDS() {
  currentStrobeRed = constrain(currentStrobeRed, 0, 255);
  currentStrobeGreen = constrain(currentStrobeGreen + strobeSmoothess, 0, 255);
  currentStrobeBlue = constrain(currentStrobeBlue, 0, 255);
  fillStrobe();
}
void LOWS() {
  currentStrobeRed = constrain(currentStrobeRed + strobeSmoothess, 0, 255);
  currentStrobeGreen = constrain(currentStrobeGreen, 0, 255);
  currentStrobeBlue = constrain(currentStrobeBlue, 0, 255);
  fillStrobe();
}
void SILENCE() {
  currentStrobeRed = constrain(currentStrobeRed - strobeSmoothess*1.5, 0, 255);
  currentStrobeGreen = constrain(currentStrobeGreen - strobeSmoothess*1.5, 0, 255);
  currentStrobeBlue = constrain(currentStrobeBlue - currentStrobeBlue*1.5, 0, 255);
  fillStrobe();
}

void fillStrobe(){
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB(currentStrobeRed, currentStrobeGreen, currentStrobeBlue);
}

// вспомогательная функция, изменяет величину value на шаг incr в пределах minimum.. maximum
int smartIncr(int value, int incr_step, int mininmum, int maximum) {
  int val_buf = value + incr_step;
  val_buf = constrain(val_buf, mininmum, maximum);
  return val_buf;
}

float smartIncrFloat(float value, float incr_step, float mininmum, float maximum) {
  float val_buf = value + incr_step;
  val_buf = constrain(val_buf, mininmum, maximum);
  return val_buf;
}

void serialTick(){
  if(Serial.available() > 0){
    byte comm = Serial.read();
    if(comm == 112){//p char
      on = !on;
      Serial.println("[#63c8ff]INFO:Power state:" + (String)(on ? "on" : "off"));
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB(0, 0, 0);
      }
      FastLED.show();
    }else if(comm == 99){//c char
      computerControlled = !computerControlled;
      Serial.println("[#63c8ff]INFO:Mode - " + (String)(computerControlled ? "external" : "self"));
    }else if(comm == 97){//a char
      brightnessSync = !brightnessSync;
      Serial.println("[#63c8ff]INFO:Brightness sync is " + (String)(brightnessSync ? "on" : "off"));
    }else if(comm == 108){//l char
      while(true){
        if(Serial.available() > 0){
          light_mode = Serial.read();
          light_mode = constrain(light_mode, 0, 2);
          Serial.println("[#63c8ff]INFO:Set light mode to " + (String)light_mode);
          eeprom_flag = true;
          eeprom_timer = millis();
          break;
        }
      }
    }else if(comm == 117){//u char
      while(true){
        if(Serial.available() > 0){
          uv_mode = Serial.read();
          uv_mode = constrain(uv_mode, 0, 2);
          Serial.println("[#63c8ff]INFO:Set uv mode to " + (String)uv_mode);
          eeprom_flag = true;
          eeprom_timer = millis();
          break;
        }
      }
    }else if(comm == 102){//f char
      if(!computerControlled){
        Serial.println("[#ffd063]WARNING:Can't execute, use external control");
      }
    }else if(comm == 109){//m char
      while(true){
        if(Serial.available() > 0){
          this_mode = (int)Serial.read();
          Serial.println("[#63c8ff]INFO:Set mode to " + (String)this_mode);
          eeprom_flag = true;
          eeprom_timer = millis();
          break;
        }
      }
    }else if(comm != 10){
      Serial.println("e");
    }
    if(computerControlled && comm == 102){
      byte currByte;
      int currLedIndex = 0;
      int currBufferIndex = 0;
      byte recievedColorArrayBuffer[3];
      while(true){
        if(currLedIndex >= NUM_LEDS){
              break;
        }
        if(Serial.available() > 0){
          currByte = Serial.read();
          if(currBufferIndex == 3){
            currBufferIndex = 0;
            leds[currLedIndex] = CRGB(recievedColorArrayBuffer[0], recievedColorArrayBuffer[1], recievedColorArrayBuffer[2]);
            currLedIndex ++;
          }
          recievedColorArrayBuffer[currBufferIndex] = currByte;
          currBufferIndex ++;
        }
      }
      FastLED.show();
    }
  }
}

void remoteTick() {
  if (irrecv.decode(&results)) { // если данные пришли
    eeprom_timer = millis();
    eeprom_flag = true;
    recieverDelay = currentDelay + countDownDelay; 
    irrecv.resume(); // Receive the next value
    switch (results.value) {
      // режимы
      case BUTT_1: this_mode = 0;
        break;
      case BUTT_2: this_mode = 1;
        break;
      case BUTT_3: this_mode = 2;
        break;
      case BUTT_4: this_mode = 3;
        break;
      case BUTT_5: this_mode = 4;
        break;
      case BUTT_6: this_mode = 5;
        break;
      case BUTT_7: this_mode = 6;
        break;
      case BUTT_8: this_mode = 7;
        FastLED.clear();
        break;
      case BUTT_9: this_mode = 8;
        break;
      case BUTT_LOW_PASS: fullLowPass();
        break;
      case BUTT_CH_PLUS:
        switch (this_mode) {
          case 5: if (++light_mode > 2) light_mode = 0;
            break;
          case 0: if (++uv_mode > 2) uv_mode = 0;
          break;
        }
        break;
      case BUTT_CH_MINUS:
        switch (this_mode) {
          case 5: if (--light_mode < 0) light_mode = 2;
            break;
          case 0: if (--uv_mode < 0) uv_mode = 2;
          break;
        }
        break;
      case BUTT_PLUS:
        MAX_COEF_FREQ += 0.1;
      case BUTT_MINUS:
        MAX_COEF_FREQ -= 0.1;
        break;
      case BUTT_BRIGHTNESS_SYNC:
        on = !on;
        for (int i = 0; i < NUM_LEDS; i++) {
          leds[i] = CRGB(0, 0, 0);
        }
        FastLED.show();
        break;
      case BUTT_PALETTE_MINUS:
          if (--currentPalette < 0) currentPalette = 6;
          switch(currentPalette){
              case 0:
               myPal = soundlevel_gp;
            break;
              case 1:
               myPal = CloudColors_p ;
            break;
             case 2:
               myPal = LavaColors_p ;
            break;
             case 3:
               myPal = OceanColors_p;
            break;
             case 4:
               myPal = ForestColors_p ;
            break;
             case 5:
               myPal = PartyColors_p ;
            break;
             case 6:
               myPal = HeatColors_p ;
            break;
          }
      break;
      case BUTT_PALETTE_PLUS:
          if (++currentPalette > 6) currentPalette = 0;
          switch(currentPalette){
              case 0:
               myPal = soundlevel_gp;
            break;
              case 1:
               myPal = CloudColors_p ;
            break;
             case 2:
               myPal = LavaColors_p ;
            break;
             case 3:
               myPal = OceanColors_p;
            break;
             case 4:
               myPal = ForestColors_p ;
            break;
             case 5:
               myPal = PartyColors_p ;
            break;
             case 6:
               myPal = HeatColors_p ;
            break;
          }
      break;
      default: eeprom_flag = false;   // если не распознали кнопку, не обновляем настройки!
        break;
    }
   
    delay(100);
  }
}

void autoLowPass() {
  // для режима VU
  delay(10);                                // ждём инициализации АЦП
  int thisMax = 0;                          // максимум
  int thisLevel;
  for (byte i = 0; i < 200; i++) {
    thisLevel = analogRead(SOUND_R);        // делаем 200 измерений
    if (thisLevel > thisMax)                // ищем максимумы
      thisMax = thisLevel;                  // запоминаем
    delay(4);                               // ждём 4мс
  }
  LOW_PASS = thisMax + LOW_PASS_ADD;        // нижний порог как максимум тишины + некая величина

  // для режима спектра
  thisMax = 0;
  for (byte i = 0; i < 100; i++) {          // делаем 100 измерений
    analyzeAudio();                         // разбить в спектр
    for (byte j = 2; j < 32; j++) {         // первые 2 канала - хлам
      thisLevel = fht_log_out[j];
      if (thisLevel > thisMax)              // ищем максимумы
        thisMax = thisLevel;                // запоминаем
    }
    delay(4);                               // ждём 4мс
  }
  SPEKTR_LOW_PASS = thisMax + LOW_PASS_FREQ_ADD;  // нижний порог как максимум тишины
  if (EEPROM_LOW_PASS && !AUTO_LOW_PASS) {
    EEPROM.updateInt(70, LOW_PASS);
    EEPROM.updateInt(72, SPEKTR_LOW_PASS);
  }
}

void analyzeAudio() {
  for (int i = 0 ; i < FHT_N ; i++) {
    int sample = analogRead(SOUND_R_FREQ);
    fht_input[i] = sample; // put real data into bins
  }
  fht_window();  // window the data for better frequency response
  fht_reorder(); // reorder the data before doing the fht
  fht_run();     // process the data in the fht
  fht_mag_log(); // take the output of the fht
}

void buttonTick() {
  /*
  butt1.tick();  // обязательная функция отработки. Должна постоянно опрашиваться
  if (butt1.isSingle())                              // если единичное нажатие
    if (++this_mode >= MODE_AMOUNT) this_mode = 0;   // изменить режим

  if (butt1.isHolded()) {     // кнопка удержана
    fullLowPass();
  }
  */
}
void fullLowPass() {
  digitalWrite(MLED_PIN, HIGH);   // включить светодиод
  FastLED.setBrightness(0); // погасить ленту
  FastLED.clear();          // очистить массив пикселей
  FastLED.show();           // отправить значения на ленту
  delay(500);               // подождать чутка
  autoLowPass();            // измерить шумы
  delay(500);               // подождать
  FastLED.setBrightness(BRIGHTNESS);  // вернуть яркость
  digitalWrite(MLED_PIN, LOW);    // выключить светодиод
}
void updateEEPROM() {
  EEPROM.updateByte(1, this_mode);
  EEPROM.updateByte(2, uv_mode);
  EEPROM.updateByte(3, light_mode);
  
  EEPROM.updateInt(4, RAINBOW_STEP);
  EEPROM.updateFloat(8, MAX_COEF_FREQ);
  EEPROM.updateInt(16, LIGHT_SAT);
  
  EEPROM.updateFloat(20, RAINBOW_STEP_2);
  EEPROM.updateFloat(28, SMOOTH);
  EEPROM.updateFloat(32, SMOOTH_FREQ);
  
  EEPROM.updateInt(40, LIGHT_COLOR);
  EEPROM.updateInt(44, COLOR_SPEED);
  EEPROM.updateInt(48, RAINBOW_PERIOD);
  
  Serial.println("[#63c8ff]INFO: Updated EEPROM");
}
void readEEPROM() {
  this_mode = EEPROM.readByte(1);
  uv_mode = EEPROM.readByte(2);
  light_mode = EEPROM.readByte(3);
  
  RAINBOW_STEP = EEPROM.readInt(4);
  MAX_COEF_FREQ = EEPROM.readFloat(8);
  LIGHT_SAT = EEPROM.readInt(16);
  
  RAINBOW_STEP_2 = EEPROM.readFloat(20);
  SMOOTH = EEPROM.readFloat(28);
  SMOOTH_FREQ = EEPROM.readFloat(32);
  
  LIGHT_COLOR = EEPROM.readInt(40);
  COLOR_SPEED = EEPROM.readInt(44);
  RAINBOW_PERIOD = EEPROM.readInt(48);
  
  Serial.println("[#63c8ff]INFO:Loaded saved settings from EEPROM");
}
void eepromTick() {
  if (eeprom_flag)
    if (millis() - eeprom_timer > 30000) {  // 30 секунд после последнего нажатия с пульта
      eeprom_flag = false;
      eeprom_timer = millis();
      updateEEPROM();
    }
}
