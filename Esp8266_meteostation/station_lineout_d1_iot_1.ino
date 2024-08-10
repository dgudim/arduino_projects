#include <ESP8266WiFi.h>
#include <CO2Sensor.h>
#include <IPStack.h>
#include <Countdown.h>
#include <MQTTClient.h>
#include <BH1750.h>
#include <Adafruit_BMP085.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <FastLED.h>
#include <Time.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3F, 16, 2);

#define MQTT_PORT 1883

#define DHTPIN D8
#define DHTTYPE DHT22
#define ONE_WIRE_BUS D9

#define PUBLISH_TOPIC1 "iot-2/evt/payload/fmt/json"
#define SUBSCRIBE_TOPIC "iot-2/cmd/+/fmt/json"
#define AUTHMETHOD "use-token-auth"
#define CLIENT_ID "d:r6rd8g:ESP8266D1:esp8266Danik"
#define MS_PROXY "r6rd8g.messaging.internetofthings.ibmcloud.com"
#define AUTHTOKEN "esp8266D1iottest"
#define NUM_LEDS    6
CRGB leds[NUM_LEDS];

DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

CO2Sensor co2Sensor(A0, 0.99, 100);
BH1750 lightMeter(0x23);
Adafruit_BMP085 bmp;

WiFiClient c;
IPStack ipstack(c);

MQTT::Client<IPStack, Countdown, 2048, 1> client = MQTT::Client<IPStack, Countdown, 2048, 1>(ipstack);

void messageArrived(MQTT::MessageData& md);

String deviceEvent;
const char* MY_SSID = "IDDQD";
const char* MY_PWD =  "3Doodler its supper 3d";

const char* MY_SSID2 = "DANIK";
const char* MY_PWD2 =  "qwerty_ccx";

const char* MY_SSID3 = "Vano";
const char* MY_PWD3 =  "vikont1vikont1";

const int DAY_BRIGHTNESS = 70;
const int NIGHT_BRIGHTNESS = 10;
int currentBrightness;

int rc = 0;
int humOK = 0;
int tempINOK = 0;
int heatIndOK = 0;
int timezone = 3;
int hour2 = 0; 
int dst = 0;
int hum = 0;
int temp = 0;
int tempValue = 0;
int tempValue_min = 0;
int hi = 0;
int temperatureIN_average = 0;
int displaystate = 4;
int co2 = 0;
int co2_smoothing[5] = {0, 0, 0, 0, 0};
int co2_graph[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int graphCycleSmoothing[2] = {0, 0};
int graphCycleDelay = 2;
int currentGraphCycle = graphCycleDelay;
int co2_led = 0;
int co2_led2 = 0;
int round_ = 0;
uint16_t lux = 0;
String data9 = "";
int pressure = 0;
int isOffline = false;

byte ch1[8] = {B00000,B00000,B00000,B00000,B00000,B00000,B00000,B11111};
byte ch2[8] = {B00000,B00000,B00000,B00000,B00000,B00000,B11111,B00000};
byte ch3[8] = {B00000,B00000,B00000,B00000,B00000,B11111,B00000,B00000};
byte ch4[8] = {B00000,B00000,B00000,B00000,B11111,B00000,B00000,B00000};
byte ch5[8] = {B00000,B00000,B00000,B11111,B00000,B00000,B00000,B00000};
byte ch6[8] = {B00000,B00000,B11111,B00000,B00000,B00000,B00000,B00000};
byte ch7[8] = {B00000,B11111,B00000,B00000,B00000,B00000,B00000,B00000};
byte ch8[8] = {B11111,B00000,B00000,B00000,B00000,B00000,B00000,B00000};

String printDigits(int digits) {
  String d;
  if (digits < 10) {
    d = "0" + String(digits);
  } else {
    d = String(digits);
  };
  return d;
}

void Display_leds(int number,int number_2, int Red, int Green, int Blue) {
  for(int i = number; i<=number_2; i++ ){
    leds[i-1] = CRGB( Green, Red, Blue);
    }
    FastLED.show();
}

void setup() {
  lcd.init();
  lcd.backlight();

  lcd.createChar(0, ch1);
  lcd.createChar(1, ch2);
  lcd.createChar(2, ch3);
  lcd.createChar(3, ch4);
  lcd.createChar(4, ch5);
  lcd.createChar(5, ch6);
  lcd.createChar(6, ch7);
  lcd.createChar(7, ch8);
  
  currentBrightness = 70;
  FastLED.addLeds<WS2812B, D7, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(currentBrightness); 
  Serial.begin(9600);
  WiFi.begin(MY_SSID, MY_PWD);
  Serial.println("connecting to home wl");
  lcd.setCursor(0, 0);
  lcd.print("Connecting");
  lcd.setCursor(0, 1);
  lcd.print("to home wl");
  delay(7000);
  Display_leds(1,2,255,0,0);
  sensors.begin();
  dht.begin();
  lightMeter.begin(BH1750_CONTINUOUS_HIGH_RES_MODE);
  if (!bmp.begin()) {
    Serial.println("Could not find BMP180 or BMP085 sensor at 0x77");
    while (1) {}
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("couldn't connect,trying other sources");
    lcd.clear();
    lcd.print("connecting to");
    Serial.println("connecting to phone wl");
    lcd.setCursor(0, 1);
    lcd.print(" phone wl");
    WiFi.begin(MY_SSID2, MY_PWD2);
    delay(7000);
    Display_leds(1,2,255,0,0);
    lcd.setCursor(0, 0);
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("couldn't connect,trying other sources");
    lcd.clear();
    lcd.print("connecting to");
    lcd.setCursor(0, 1);
    lcd.print(" home2 wl");
    Serial.println("connecting to home2 wl");
    WiFi.begin(MY_SSID3, MY_PWD3);
    delay(7000);
    Display_leds(1,2,255,0,0);
  }
  
 if (WiFi.status() != WL_CONNECTED)
  {
    lcd.setCursor(0, 0);
    lcd.clear();
    lcd.print("you are");
    lcd.setCursor(0, 1);
    lcd.print("OFFLINE!!");
    lcd.setCursor(0, 0);
    Display_leds(1,6,0,0,255);
    delay(5000);
    isOffline=true;
  }
  
  if (WiFi.status() == WL_CONNECTED)
  {
    lcd.setCursor(0, 0);
    lcd.clear();
    Serial.println("connected");
    lcd.print("Connected");
    Display_leds(3,4,127,127,0);
    delay(200);
  }
}

int getTemp(void) {
  sensors.requestTemperatures();
 int Temp_out = sensors.getTempCByIndex(0);
  return (Temp_out);
}

String getTime(void) {
  configTime(timezone * 3600, 0, "pool.ntp.org", "time.nist.gov");
  while (!time(nullptr)) {
    delay(1000);
  }
  time_t now = time(nullptr);
  int t = now ;
  int seconds = t % 60 ;
  int minutes = int(t / 60) % 60;
  int hours = int(t / 60 / 60) % 24 ;
  int z = t / 86400 + 719468;
  int era = (z >= 0 ? z : z - 146096) / 146097;
  unsigned doe = static_cast<unsigned>(z - era * 146097);
  unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int y = static_cast<int>(yoe) + era * 400;
  unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  unsigned mp = (5 * doy + 2) / 153;
  unsigned d = doy - (153 * mp + 2) / 5 + 1;
  unsigned m = mp + (mp < 10 ? 3 : -9);
  y += (m <= 2);
  String timeStamp = String(y) + "-" + printDigits(m) + "-" + printDigits(d) + " " + printDigits(hours) + ":" + printDigits(minutes) + ":" + printDigits(seconds);
  String hour_ = String(hours);
  return (timeStamp);
}

int getHour(void) {
  configTime(timezone * 3600, 0, "pool.ntp.org", "time.nist.gov");
  while (!time(nullptr)) {
    delay(1000);
  }
  time_t now = time(nullptr);
  int t = now ;
  int hours = int(t / 60 / 60) % 24 ;
  return (hours);
  }

void Display_all (void){

  int idealCo2 = 750;
  int criticalCo2 = 1100;

  int gradientRange = criticalCo2 - idealCo2;
  float gradientCoef = 255 / (float)gradientRange;

  round_ = min(max(co2 - idealCo2, 0), gradientRange);

  co2_led = min((int)(round_ * gradientCoef), 255);
  co2_led2 = 255 - co2_led;

  if (lux < 7) {
    currentBrightness = NIGHT_BRIGHTNESS;
    lcd.noBacklight();
  }else{
    currentBrightness = DAY_BRIGHTNESS;
    lcd.backlight();
  }
  FastLED.setBrightness(currentBrightness);
  
    Display_leds(4,6,co2_led,co2_led2,0);
    if (tempValue>=0){
      Display_leds(1,3,tempValue*8,0,255-(tempValue*8));
    }
   if (tempValue<0){
    Display_leds(1,3,0,tempValue_min*8,255-(tempValue_min*8));
   }
   if (tempValue*8>255){
    Display_leds(1,3,255,0,0);
   } 
   if (tempValue_min*8>255 && tempValue<0){
    Display_leds(1,3,0,255,0);
   }
    
    switch (displaystate) {
    case 1:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Pressure:");
      lcd.print(pressure);
      lcd.print("mmHg");
      lcd.setCursor(0, 1);
      lcd.print("inside temp:");
      lcd.print(temperatureIN_average);
      lcd.print("C");
      displaystate = 2;
      break;
    case 2:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Hummidity:");
      if (hum < 10){
        lcd.print("Error");
      }else{
        lcd.print(hum);
        lcd.print("%");
      }  
      lcd.setCursor(0, 1);
      lcd.print("outside temp:");
      lcd.print(tempValue);
      lcd.print("C");
      displaystate = 3;
      break;
    case 3:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Co2:");
      lcd.print(co2);
      lcd.print("  ");
      lcd.setCursor(0, 1);
      lcd.print("Light level:");
      lcd.print(lux);
      lcd.print("  ");
      displaystate = 4;
      break;
    case 4:
     int graphArrayLength = sizeof(co2_graph) / sizeof(co2_graph[0]);
     float gisplayHeightOffsetCoef = gradientRange / (float)16;
     int autoscrolls = graphArrayLength - 16;
     lcd.begin(16, 2);
     lcd.clear();
    for(int offset = autoscrolls; offset >= 0 ; offset--){
     for(int i = 0; i < 16; i++){
      int dOffset = autoscrolls - offset + i;
       int line = min(max((co2_graph[dOffset] - idealCo2) / gisplayHeightOffsetCoef, (float)1), (float)16) - 1;
       if(line < 8){
         lcd.setCursor(i, 1);
         lcd.write(byte(line));
         lcd.setCursor(i, 0);
         lcd.print(" ");
       }else{
         lcd.setCursor(i, 0);
         lcd.write(byte(line - 8));
         lcd.setCursor(i, 1);
         lcd.print(" ");
       }
      }
      delay(300);
     }
     for(int i = 1; i < graphCycleDelay; i++){
        graphCycleSmoothing[i-1] = graphCycleSmoothing[i];
     }
     graphCycleSmoothing[graphCycleDelay - 1] = co2;
     if(currentGraphCycle >= graphCycleDelay){
       int sum = 0;
       for(int i = 0; i < graphCycleDelay; i++){
           sum+=graphCycleSmoothing[i];
       }
       for(int i = 1; i < graphArrayLength; i++){
          co2_graph[i-1] = co2_graph[i];
       }
       co2_graph[graphArrayLength - 1] = sum / graphCycleDelay;
       currentGraphCycle = 0;
     }else{
       currentGraphCycle++; 
     }
    displaystate = 1;
    break;
    }
}

void Update_var(void){
  hour2 = getHour();
  int currentCo2 = co2Sensor.read();
  int co2Sum = 0;
  int smoothingArrayLength = sizeof(co2_smoothing) / sizeof(co2_smoothing[0]);
  for(int i = 0; i < smoothingArrayLength; i++){
      co2Sum += co2_smoothing[i];
      if(i > 0){
         co2_smoothing[i-1] = co2_smoothing[i];
      }
  }
  
  co2_smoothing[smoothingArrayLength - 1] = currentCo2;
  co2 = co2Sum / smoothingArrayLength;
  
  humOK = hum;
  tempINOK = temp;
  hum = dht.readHumidity();
  temp = dht.readTemperature();
  
  //workaround for DHT bad measuremnet from time to time (taking last good measurement)
  if (temp > 100) {
    temp = tempINOK;
    hum = humOK;
    delay(500);
  }

  tempValue = getTemp();
  tempValue_min = abs(tempValue);
  hi = dht.computeHeatIndex(tempValue, hum) + 3;
  rc = -1;
  lux = lightMeter.readLightLevel();
  temperatureIN_average = (int)((temp + bmp.readTemperature()) / 2) ;
  if (temperatureIN_average = bmp.readTemperature()/2){
    temperatureIN_average = (int)(bmp.readTemperature());
  }
    pressure = (int)((bmp.readPressure()) * 0.0075) ;
  }
  
  void Form_string (void){
    data9 = "{\"d\":{\"name\": \"esp8266Danik\",\"tempIN\": "   + (String) temperatureIN_average
                 +  ",\"tempOUT\": "  + (String) tempValue
                 +  ",\"HeatIndex\": "   + (String) hi
                 +  ",\"hum\": " + (String) hum
                 +  ",\"co2\": "    + (String) co2
                 +  ",\"pressure\": "   + (String) pressure
                 +  ",\"lightLevel\": "   + (String) lux
                 +  ",\"regTS\": \""   + getTime()
                 +  "\"}}";
    }
    
  void MQTT_Connect (void){
    if (!client.isConnected()) {
    Serial.print("Connecting using Registered mode with clientid : ");
    Serial.print(CLIENT_ID);
    Serial.print(" to MQTT Broker : ");
    lcd.clear();
    lcd.print("Connecting to");
    lcd.setCursor(0, 1);
    lcd.print("MQTT Broker");
    Serial.println(MS_PROXY);
    Display_leds(5,6,0,255,0);
    rc = ipstack.connect(MS_PROXY, MQTT_PORT);
    MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
    options.MQTTVersion = 3;
    options.clientID.cstring = CLIENT_ID;
    options.username.cstring = AUTHMETHOD;
    options.password.cstring = AUTHTOKEN;
    options.keepAliveInterval = 10;
    rc = -1;
    rc = client.connect(options);
    delay(2000);
    Display_leds(1,6,0,0,0);
    // for future use to process messages from server
    //unsubscribe the topic, if it had subscribed it before.
    //   Serial.print(" on topic : 88");
    //   client.unsubscribe(SUBSCRIBE_TOPIC);
    //    Serial.print(" on topic : 99");
    //Try to subscribe for commands
    //    if ((rc = client.subscribe(SUBSCRIBE_TOPIC, MQTT::QOS0, messageArrived)) != 0) {
    //            Serial.print("Subscribe failed with return code : ");
    //            Serial.println(rc);
    //    } else {
    //          Serial.println("Subscribed\n");
    //    }
    Serial.println("Connected");
    lcd.setCursor(0, 0);
    lcd.clear();
    lcd.print("Connected !");
    delay(1000);
    Serial.println("Sensor data Device Event (JSON)");
    Serial.println("____________________________________________________________________________");
  }
}

void Send_data(void){
  MQTT::Message message;
  message.qos = MQTT::QOS0;
  message.retained = false;
  delay(500);
  Serial.println(data9);
  char json[2048];
  strncpy(json, data9.c_str(), sizeof(json));
  message.payload = json;
  message.payloadlen = strlen(json);
  rc = client.publish(PUBLISH_TOPIC1, message);
  delay(1000);
  
  if (rc != 0) {
    Serial.print("Message publish failed with return code : ");
    Serial.println(rc);
    lcd.setCursor(0, 0);
    lcd.clear();
    lcd.print("Error:");
    lcd.print(rc);
  }
  client.yield(100);
}
  
  void loop() {
  Update_var(); //writing new sensor measurements to variables
  Form_string (); //forming a string to send to the cloud
  if(!isOffline){
     MQTT_Connect (); //if client is not connected (usually once)
  }
  Display_all(); //show information on leds and screen
  if(!isOffline){
     Send_data(); //send data to the cloud
  }
  //delay(10000);
}

/*
  void messageArrived(MQTT::MessageData& md) {
  Serial.print("\nMessage Received\t");
  MQTT::Message &message = md.message;
  int topicLen = strlen(md.topicName.lenstring.data) + 1;

  char * topic = md.topicName.lenstring.data;
  topic[topicLen] = '\0';

  int payloadLen = message.payloadlen + 1;

  char * payload = (char*)message.payload;
  payload[payloadLen] = '\0';

  String topicStr = topic;
  String payloadStr = payload;

  if (strstr(topic, "/cmd/blink") != NULL) {
    Serial.print("Command IS Supported : ");
    Serial.print(payload);
    Serial.println("\t.....\n");

  } else {
    Serial.println("Command Not Supported:");
  }
  }
*/
