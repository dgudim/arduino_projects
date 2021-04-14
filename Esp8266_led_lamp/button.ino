
bool brightDirection;

const int A_BUTTON = 11926804;
const int B_BUTTON = 11926808;
const int C_BUTTON = 11926801;
const int D_BUTTON = 11926802;

bool startButtonHolding;
bool buttonHeld;
bool buttonClicked;

const int holdTimeout = 500; //(ms)
int currentTimer;
int nextCancelTimer;

int value;
int lastValue;
int heldRecievedValues;

int clickTimer;
const int clickEveryN = 300; //(ms)

void buttonTick()
{
  currentTimer = millis();

  if (mySwitch.available()) {
        value = mySwitch.getReceivedValue();
        mySwitch.resetAvailable();
        if(lastValue == value || (lastValue == 0 && value > 0)){
          heldRecievedValues ++;
          nextCancelTimer = currentTimer + holdTimeout;
        }
        if(heldRecievedValues > 9){
          buttonClicked = false;
          buttonHeld = true;
          if(!startButtonHolding){
            brightDirection = !brightDirection;
            startButtonHolding = true;
            #ifdef GENERAL_DEBUG
            Serial.println("");
            Serial.print("Changed b direction to ");
            Serial.print(brightDirection);
            Serial.println("");
            #endif
          }
        }
        lastValue = value;
  }else if(currentTimer > nextCancelTimer){
        if(heldRecievedValues < 9 && heldRecievedValues > 0){
          buttonClicked = true;
          buttonHeld = false;
        }
  }
  
  if (value == A_BUTTON && buttonClicked)
  {
    if (dawnFlag)
    {
      manualOff = true;
      dawnFlag = false;
      FastLED.setBrightness(modes[currentMode].Brightness);
      changePower();
    }
    else
    {
      ONflag = !ONflag;
      changePower();
    }
    settChanged = true;
    eepromTimeout = millis();
    loadingFlag = true;

    #if (USE_MQTT)
    if (espMode == 1U)
    {
      MqttManager::needToPublish = true;
    }
    #endif
  }

  if (ONflag && value == B_BUTTON && buttonClicked)
  {
    if (++currentMode >= (int8_t)MODE_AMOUNT) currentMode = 0;
    FastLED.setBrightness(modes[currentMode].Brightness);
    loadingFlag = true;
    settChanged = true;
    eepromTimeout = millis();
    FastLED.clear();
    delay(1);

    #if (USE_MQTT)
    if (espMode == 1U)
    {
      MqttManager::needToPublish = true;
    }
    #endif
  }

  if (ONflag && value == D_BUTTON && buttonClicked)
  {
    if (--currentMode < 0) currentMode = MODE_AMOUNT - 1;
    FastLED.setBrightness(modes[currentMode].Brightness);
    loadingFlag = true;
    settChanged = true;
    eepromTimeout = millis();
    FastLED.clear();
    delay(1);

    #if (USE_MQTT)
    if (espMode == 1U)
    {
      MqttManager::needToPublish = true;
    }
    #endif
  }

/*
  if (value == 0)                                     // вывод IP на лампу
  {
    if (espMode == 1U)
    {
      loadingFlag = true;
      
      #if defined(MOSFET_PIN) && defined(MOSFET_LEVEL)      // установка сигнала в пин, управляющий MOSFET транзистором, матрица должна быть включена на время вывода текста
      digitalWrite(MOSFET_PIN, MOSFET_LEVEL);
      #endif

      while(!fillString(WiFi.localIP().toString().c_str(), CRGB::White)) { delay(1); ESP.wdtFeed(); }

      #if defined(MOSFET_PIN) && defined(MOSFET_LEVEL)      // установка сигнала в пин, управляющий MOSFET транзистором, соответственно состоянию вкл/выкл матрицы или будильника
      digitalWrite(MOSFET_PIN, ONflag || (dawnFlag && !manualOff) ? MOSFET_LEVEL : !MOSFET_LEVEL);
      #endif

      loadingFlag = true;
    }
  }
*/


  if (value == C_BUTTON && buttonClicked)                                     // вывод текущего времени бегущей строкой
  {
    printTime(thisTime, true, ONflag);
  }


  if (ONflag && value == C_BUTTON && buttonHeld && heldRecievedValues > 50)   // смена рабочего режима лампы: с WiFi точки доступа на WiFi клиент или наоборот
  {
    espMode = (espMode == 0U) ? 1U : 0U;
    EepromManager::SaveEspMode(&espMode);

    #ifdef GENERAL_DEBUG
    LOG.printf_P(PSTR("Рабочий режим лампы изменён и сохранён в энергонезависимую память\nНовый рабочий режим: ESP_MODE = %d, %s\nРестарт...\n"),
      espMode, espMode == 0U ? F("без WiFi") : F("WiFi клиент (подключение к роутеру)"));
    delay(1000);
    #endif

    showWarning(CRGB::Red, 3000U, 500U);                    // мигание красным цветом 3 секунды - смена рабочего режима лампы, перезагрузка
    ESP.restart();
  }
  
  // кнопка нажата и удерживается
  if (ONflag && value == A_BUTTON && buttonHeld && clickTimer < currentTimer){
        uint8_t delta = modes[currentMode].Brightness < 10U // определение шага изменения яркости: при яркости [1..10] шаг = 1, при [11..16] шаг = 3, при [17..255] шаг = 15
          ? 1U
          : 5U;
        modes[currentMode].Brightness =
          constrain(brightDirection
            ? modes[currentMode].Brightness + delta
            : modes[currentMode].Brightness - delta,
          1, 255);
        FastLED.setBrightness(modes[currentMode].Brightness);

        #ifdef GENERAL_DEBUG
        LOG.printf_P(PSTR("Новое значение яркости: %d\n"), modes[currentMode].Brightness);
        #endif
        settChanged = true;
        eepromTimeout = millis();
        clickTimer = currentTimer + clickEveryN;
  }
  
  if (ONflag && value == B_BUTTON && buttonHeld && clickTimer < currentTimer){
        modes[currentMode].Speed = constrain(brightDirection ? modes[currentMode].Speed + 3 : modes[currentMode].Speed - 3, 0, 256);
        if(modes[currentMode].Speed == 256){
          modes[currentMode].Speed = 1;
        }
        if(modes[currentMode].Speed == 0){
          modes[currentMode].Speed = 255;
        }
        #ifdef GENERAL_DEBUG
        LOG.printf_P(PSTR("Новое значение скорости: %d\n"), modes[currentMode].Speed);
        #endif
        settChanged = true;
        eepromTimeout = millis();
        clickTimer = currentTimer + clickEveryN;
  }
  
  if (ONflag && value == D_BUTTON && buttonHeld && clickTimer < currentTimer){
        modes[currentMode].Scale = constrain(brightDirection ? modes[currentMode].Scale + 2 : modes[currentMode].Scale - 2, 0, 101);
        if(modes[currentMode].Scale == 101){
          modes[currentMode].Scale = 1;
        }
        if(modes[currentMode].Scale == 0){
          modes[currentMode].Scale = 100;
        }
        #ifdef GENERAL_DEBUG
        LOG.printf_P(PSTR("Новое значение масштаба: %d\n"), modes[currentMode].Scale);
        #endif
        settChanged = true;
        eepromTimeout = millis();
        clickTimer = currentTimer + clickEveryN;
  }

/*
  // кнопка отпущена после удерживания
  if (ONflag && startButtonHolding)      // кнопка отпущена после удерживания, нужно отправить MQTT сообщение об изменении яркости лампы
  {
    //startButtonHolding = false;
    loadingFlag = true;

    #if (USE_MQTT)
    if (espMode == 1U)
    {
      MqttManager::needToPublish = true;
    }
    #endif
  }
*/
 
    if(currentTimer > nextCancelTimer){
        heldRecievedValues = 0;
        lastValue = 0;
        value = 0;
        startButtonHolding = false;
        buttonClicked = false;
        buttonHeld = false;
    }
}
