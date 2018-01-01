//Open ToDo s
//Sensoren Zuschaltbar
//measure (Maßeinheiten der Messwerte als Wert an MQTT Server übertragen)

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <Adafruit_NeoPixel.h>
#include <OneWire.h>              // https://github.com/PaulStoffregen/OneWire
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <SparkFunHTU21D.h>
#include <Ticker.h>
#include <EEPROM.h>
#include <PubSubClient.h>         // https://github.com/knolleary/pubsubclient/releases/tag/v2.6
#include <Log2String.h>           //selbst entwickelte Lib

#define EnableTSL2561           1
#define EnableHTU21D            1
#define EnableDallasTemperature 1
#define EnableOTA               1

//SYSTEM
unsigned long currentMillis = 0;

//WiFi Manager
bool shouldSaveConfig = false;

//ESP
int configmode = 0;
long  chipid;
char chipidChar[8] = "";
char hostName[30] = "SensorNode-";
const char mqttPrefix[] = "SensorNode/";

Log2String LogESPAlias;
Log2String LogESPMAC;
Log2String LogESPFreeHeap;

// Ticker
Ticker ticker;
int tickerCount = 0;

//Touch
const int button1Pin = D5;
bool button1State = 0;
bool button1State_last = 0;
Log2String LogTouch1;

//DS10B20 Temp OneWire
OneWire  myOneWire(D7);  // on pin 10 (a 4.7K resistor is necessary)
DallasTemperature myDS10B20(&myOneWire);
float DS10B20_temperatur;
Log2String LogDS10B20;

//TSL2561 Light I2C
Adafruit_TSL2561_Unified myTSL2561 = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
float TSL2561_brightness;
Log2String LogTSL2561;

//SHT20 Humidity, temperatur I2C
HTU21D myHTU21D;
float HTU21D_humidity;
Log2String LogHTU21D_humidity;

float HTU21D_temperatur;
float HTU21D_temperatur_offset = -2.9;
Log2String LogHTU21D_temperatur;

//NEOPIXEL
const int NEOPIXEL_PIN = D6; //Neopixel Pin
const int NEOPIXEL_COUNT = 2; //Neopixel Anzahl
Adafruit_NeoPixel myNEOPIXEL = Adafruit_NeoPixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

int NEO_R = 0;
int NEO_G = 0;
int NEO_B = 0;
int NEO_00R = 0;
int NEO_00G = 0;
int NEO_00B = 0;
int NEO_01R = 0;
int NEO_01G = 0;
int NEO_01B = 0;
int NEO_BRIGHT = 255;
long number;

//MQTT
#define EEPROM_SALT 1611
typedef struct {
  int   salt = EEPROM_SALT;
  char mqttServer[40] = "192.168.1.10";
  char mqttPort[6] = "8080";
  char mqttUser[20] = "mqttuser";
  char mqttPass[20] = "mqttpass";
  char mqttAlias[20] = "AliasName";
} WMSettings;
WMSettings settings;

WiFiManager wifiManager;  // wifimanager Instanz
WiFiClient wifiClient;

// MQTT
PubSubClient mqttClient(wifiClient);
// MQTT subscribe
char subscribe_Topic[30] = "";
String Ssubscribe_Topic = "";
String arrivedValue;

//callback notifying us of the need to save config
// Muss vor "setup()" stehen
void saveConfigCallback () {
  shouldSaveConfig = true;
}

// Empfangen von MQTT Daten
void callback(char* mqttTopic, byte* mqttPayload, unsigned int length) { //nicht verschieben!, Funktioniert nur, wenn vor SETUP eingebunden.

  // handle message arrived
  Serial.println("### Message arrived");
  Serial.println();
  Serial.println("---------");
  Serial.print("Topic: ");
  Serial.println(String(mqttTopic));
  Serial.print("Value: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)mqttPayload[i]);
  }
  Serial.println("");
  Serial.println("---------");

  arrivedValue = String((char*)mqttPayload).substring(1, 7);

  Serial.print("arrivedValue: ");
  Serial.println(arrivedValue);
  Serial.println("---------");

  number = (long) strtol( &arrivedValue[0], NULL, 16); // http://stackoverflow.com/questions/28104559/arduino-strange-behavior-converting-hex-to-rgb
  NEO_R = number >> 16;
  NEO_G = number >> 8 & 0xFF;
  NEO_B = number & 0xFF;
  /*
    Serial.println("String to Int");
    Serial.println(NEO_R);
    Serial.println(NEO_G);
    Serial.println(NEO_B);
    Serial.println("---------");
  */
  // Aktoren setzen
  if (String(mqttTopic) == (mqttPrefix + String(ESP.getChipId(), HEX) + "/IN/LED")) {
    Serial.println("Daten fuer alle LED");
    NEO_00R = NEO_R;
    NEO_00G = NEO_G;
    NEO_00B = NEO_B;
    NEO_01R = NEO_R;
    NEO_01G = NEO_G;
    NEO_01B = NEO_B;
  }

  if (String(mqttTopic) == (mqttPrefix + String(ESP.getChipId(), HEX) + "/IN/LED1")) {
    Serial.println("Daten fuer LED 01");
    NEO_00R = NEO_R;
    NEO_00G = NEO_G;
    NEO_00B = NEO_B;
  }
  if (String(mqttTopic) == (mqttPrefix + String(ESP.getChipId(), HEX) + "/IN/LED2")) {
    Serial.println("Daten fuer LED 02");
    NEO_01R = NEO_R;
    NEO_01G = NEO_G;
    NEO_01B = NEO_B;
  }

  if (String(mqttTopic) == (mqttPrefix + String(ESP.getChipId(), HEX) + "/IN/RESET")) {
    //Serial.println("Reset Daten");
    configmode = 1;
  }

  Serial.println("### Message arrived END ###");
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    if (mqttClient.connect(chipidChar)) {
      //Daten empfangen
      mqttClient.subscribe(subscribe_Topic);
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//### SETUP ################################################################################
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("");  Serial.println(".1");  Serial.println("..2");  Serial.println("...3");

  // Heartbeat
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  ticker.attach(0.01, setupHeartbeatTick); // langsames Blinken

  //EEPROM
  EEPROM.begin(512);
  EEPROM.get(0, settings);
  EEPROM.end();

  // Hostname
  chipid = ESP.getChipId();
  ultoa(chipid, chipidChar, HEX);  // befüllt chipid_Topic char mit chipid long
  strcat(hostName, chipidChar);  // hängt chipid_Topic an hostName an

  currentMillis = millis();

  //Touch
  pinMode(button1Pin, INPUT);
  LogTouch1.setValueName(mqttPrefix + String(ESP.getChipId(), HEX) + "/Touch1");
  LogTouch1.setTimeBetweenSend(300000);

  //DS10B20 Temp OneWire
  myDS10B20.begin();
  LogDS10B20.setValueName(mqttPrefix + String(ESP.getChipId(), HEX) + "/DS10B20_temperatur"); // String für Topic
  LogDS10B20.setTimeBetweenSend(300000); // Intervall im ms, wenn mindestens 1x gesendet wird
  LogDS10B20.setValueChange(0.10); // Messwertänderung, bei der gesendet wird (nur für flow Variablen!

  //TSL2561 Light I2C
  myTSL2561.begin();
  LogTSL2561.setValueName(mqttPrefix + String(ESP.getChipId(), HEX) + "/TSL2561_brightness");
  LogTSL2561.setTimeBetweenSend(300000);
  LogTSL2561.setValueChange(1.00);

  //SHT20 Humidity, temperatur I2C
  myHTU21D.begin();
  LogHTU21D_humidity.setValueName(mqttPrefix + String(ESP.getChipId(), HEX) + "/HTU21D_humidity");
  LogHTU21D_humidity.setTimeBetweenSend(300000);
  LogHTU21D_humidity.setValueChange(1.00);
  LogHTU21D_temperatur.setValueName(mqttPrefix + String(ESP.getChipId(), HEX) + "/HTU21D_temperatur");
  LogHTU21D_temperatur.setTimeBetweenSend(300000);
  LogHTU21D_temperatur.setValueChange(0.10);

  //ESP
  LogESPAlias.setValueName(mqttPrefix + String(ESP.getChipId(), HEX) + "/ESPAlias");
  LogESPAlias.setValue(String(settings.mqttAlias));
  LogESPAlias.setTimeBetweenSend(60000);
  LogESPMAC.setValueName(mqttPrefix + String(ESP.getChipId(), HEX) + "/ESPMAC");
  LogESPMAC.setValue(String(WiFi.macAddress()));
  LogESPMAC.setTimeBetweenSend(60000);
  LogESPFreeHeap.setValueName(mqttPrefix + String(ESP.getChipId(), HEX) + "/ESPFreeHeap");
  LogESPFreeHeap.setValue(String(ESP.getFreeHeap()));
  LogESPFreeHeap.setTimeBetweenSend(60000);

  //NEOPIXEL
  myNEOPIXEL.setBrightness(NEO_BRIGHT);
  myNEOPIXEL.setPixelColor(0, myNEOPIXEL.Color(0, 0, 0));
  myNEOPIXEL.setPixelColor(1, myNEOPIXEL.Color(0, 0, 0));
  myNEOPIXEL.begin();
  myNEOPIXEL.show();

  //WiFiManager
  wifi_station_set_hostname(hostName);  //sdk, non-Arduino function
  //WiFiManager wifiManager;
  wifiManager.setDebugOutput(false);
  wifiManager.setTimeout(300);
  wifiManager.setCustomHeadElement("<style>button{border:1;border-radius:0;background-color:#cccccc;color:#000000;}</style>");
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  // wifiManager.setAPCallback(configModeCallback);
  // wifiManager.resetSettings();

  WiFiManagerParameter custom_mqtt_text1("MQTT Configuration");
  wifiManager.addParameter(&custom_mqtt_text1);

  //WiFiManagerParameter custom_mqtt_text2("Server");
  //wifiManager.addParameter(&custom_mqtt_text2);
  WiFiManagerParameter custom_mqttServer("server", "mqtt server", settings.mqttServer, 40);
  wifiManager.addParameter(&custom_mqttServer);

  //WiFiManagerParameter custom_mqtt_text3("Port");
  //wifiManager.addParameter(&custom_mqtt_text3);
  WiFiManagerParameter custom_mqttPort("port", "mqtt port", settings.mqttPort, 6);
  wifiManager.addParameter(&custom_mqttPort);

  //WiFiManagerParameter custom_mqtt_text4("User");
  //wifiManager.addParameter(&custom_mqtt_text4);
  WiFiManagerParameter custom_mqttUser("user", "mqtt user", settings.mqttUser, 20);
  wifiManager.addParameter(&custom_mqttUser);

  //WiFiManagerParameter custom_mqtt_text5("Pass");
  //wifiManager.addParameter(&custom_mqtt_text5);
  WiFiManagerParameter custom_mqttPass("pass", "mqtt pass", settings.mqttPass, 20);
  wifiManager.addParameter(&custom_mqttPass);

  //WiFiManagerParameter custom_mqtt_text6("Alias");
  //wifiManager.addParameter(&custom_mqtt_text6);
  WiFiManagerParameter custom_mqttAlias("alias", "mqtt alias", settings.mqttAlias, 20);
  wifiManager.addParameter(&custom_mqttAlias);

  // Prüfen ob Taste beim Start gedrückt ist --> Start Config Portal
  if (digitalRead(button1Pin) == 1) {
    myNEOPIXEL.setPixelColor(0, myNEOPIXEL.Color(255, 0, 0));
    myNEOPIXEL.setPixelColor(1, myNEOPIXEL.Color(255, 0, 0));
    myNEOPIXEL.setBrightness(255);
    myNEOPIXEL.show();
    wifiManager.startConfigPortal(hostName); //startet AP
  } else {
    if (!wifiManager.autoConnect(hostName))   { //startet AP, wenn Connect mit gespeicherten Daten nicht möglich
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      ESP.reset();
    }
    Serial.println("WiFi connected");
  }

  if (shouldSaveConfig) {
    // Werte im EEPROM speichern
    strcpy(settings.mqttServer, custom_mqttServer.getValue());
    strcpy(settings.mqttPort, custom_mqttPort.getValue());
    strcpy(settings.mqttUser, custom_mqttUser.getValue());
    strcpy(settings.mqttPass, custom_mqttPass.getValue());
    strcpy(settings.mqttAlias, custom_mqttAlias.getValue());

    EEPROM.begin(512);
    EEPROM.put(0, settings);
    EEPROM.end();

    myNEOPIXEL.setPixelColor(0, myNEOPIXEL.Color(0, 0, 255));
    myNEOPIXEL.setPixelColor(1, myNEOPIXEL.Color(0, 0, 255));
    myNEOPIXEL.setBrightness(255);
    myNEOPIXEL.show();
    delay (2000);
    ESP.reset();
  }

  // MQTT
  Serial.println("----MQTT Verbindung herstellen----");
  Serial.print("MQTT ID       :" ); Serial.println(chipidChar);
  Serial.print("MQTT Server   :" ); Serial.println(settings.mqttServer);
  Serial.print("MQTT Port     :" ); Serial.println(settings.mqttPort);
  Serial.print("MQTT User     :" ); Serial.println(settings.mqttUser);
  Serial.print("MQTT Passwort :" ); Serial.println(settings.mqttPass);
  Serial.println("");

  mqttClient.setServer(settings.mqttServer, atoi(settings.mqttPort));
  mqttClient.setCallback(callback);

  if (mqttClient.connect (chipidChar, settings.mqttUser, settings.mqttPass)) { //mit MQTT PW
    //if (mqttClient.connect (chipidChar)) { //ohne MQTT PW
    Serial.println("Verbindung zum MQTT Server erfolgreich");
  } else {
    Serial.println("Verbindung zum MQTT Server NICHT erfolgreich");
  }
  Serial.print("MQTT client state :" ); Serial.println(mqttClient.state()); //https://pubsubclient.knolleary.net/api.html#state

  //MQTT subscribe
  Ssubscribe_Topic = String(mqttPrefix) + String(ESP.getChipId(), HEX) + "/IN/#";
  Ssubscribe_Topic.toCharArray(subscribe_Topic, Ssubscribe_Topic.length() + 1);

  Serial.println("");
  Serial.print("MQTT SubTopic :" ); Serial.println(subscribe_Topic);
  if (mqttClient.subscribe(subscribe_Topic)) {
    Serial.println("Subscribe zum MQTT Server erfolgreich");
  } else {
    Serial.println("Subscribe zum MQTT Server NICHT erfolgreich");
  }
  Serial.println("----MQTT Verbindung---------------");

  //OTA
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(hostName);
  ArduinoOTA.setPassword((const char *)"52364897"); // No authentication by default

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

//### LOOP ################################################################################
void loop() {
  //OTA
  ArduinoOTA.handle();

  //RESET Anforderung
  if (configmode == 1) {
    myNEOPIXEL.setPixelColor(0, myNEOPIXEL.Color(255, 0, 0));
    myNEOPIXEL.setPixelColor(1, myNEOPIXEL.Color(255, 0, 0));
    myNEOPIXEL.setBrightness(255);
    myNEOPIXEL.show();
    delay (2000);
    myNEOPIXEL.setPixelColor(0, myNEOPIXEL.Color(0, 0, 0));
    myNEOPIXEL.setPixelColor(1, myNEOPIXEL.Color(0, 0, 0));
    myNEOPIXEL.setBrightness(0);
    myNEOPIXEL.show();
    ESP.reset();
  }

  //keine Verbindung zum MQTT Server --> Neustart
  if (configmode == 2) {
    myNEOPIXEL.setPixelColor(0, myNEOPIXEL.Color(0, 255, 255));
    myNEOPIXEL.setPixelColor(1, myNEOPIXEL.Color(0, 255, 255));
    myNEOPIXEL.setBrightness(255);
    myNEOPIXEL.show();
    delay (5000);
    myNEOPIXEL.setPixelColor(0, myNEOPIXEL.Color(0, 0, 0));
    myNEOPIXEL.setPixelColor(1, myNEOPIXEL.Color(0, 0, 0));
    myNEOPIXEL.setBrightness(0);
    myNEOPIXEL.show();
    ESP.reset();
  }

  //MQTT
  if (!mqttClient.connected()) {
    reconnect();
  }
  if (mqttClient.state() != 0) {
    Serial.print("MQTT client state :" ); Serial.println(mqttClient.state()); //https://pubsubclient.knolleary.net/api.html#state
    configmode = 2;
  }
  mqttClient.loop();

  // ### Messen
  if ( digitalRead(button1Pin) == LOW ) {
    currentMillis = millis();
  }

  //Touch
  button1State = digitalRead(button1Pin);
  LogTouch1.setValue(digitalRead(button1Pin));

  //DS10B20 Temp OneWire
  myDS10B20.requestTemperatures();
  LogDS10B20.setValue(myDS10B20.getTempCByIndex(0));

  //TSL2561 Light I2C
  digitalWrite(LED_BUILTIN, HIGH); // LED Aus vor Lichtmessung
  sensors_event_t TSL2561_event;
  myTSL2561.getEvent(&TSL2561_event);
  LogTSL2561.setValue(TSL2561_event.light);

  //SHT20 Humidity, temperatur I2C
  LogHTU21D_humidity.setValue(myHTU21D.readHumidity());
  LogHTU21D_temperatur.setValue(myHTU21D.readTemperature() + HTU21D_temperatur_offset);

  // ### Verarbeiten

  // ### Aktoren setzen
  myNEOPIXEL.setPixelColor(0, myNEOPIXEL.Color(NEO_00R, NEO_00G, NEO_00B));
  myNEOPIXEL.setPixelColor(1, myNEOPIXEL.Color(NEO_01R, NEO_01G, NEO_01B));
  myNEOPIXEL.setBrightness(NEO_BRIGHT);
  myNEOPIXEL.show();

  // ### Ausgeben MQTT
  if (LogTouch1.getSendRequired()) {
    mqttClient.publish(LogTouch1.getCValueName(), LogTouch1.getCValue());
    // Serial.println(" mqttClient.publish LogTouch1");
    LogTouch1.setSendNow();
  }

  if (LogDS10B20.getSendRequired()) {
    mqttClient.publish(LogDS10B20.getCValueName(), LogDS10B20.getCValue());
    LogDS10B20.setSendNow();
  }

  if (LogTSL2561.getSendRequired()) {
    mqttClient.publish(LogTSL2561.getCValueName(), LogTSL2561.getCValue());
    LogTSL2561.setSendNow();
  }

  if (LogHTU21D_humidity.getSendRequired()) {
    mqttClient.publish(LogHTU21D_humidity.getCValueName(), LogHTU21D_humidity.getCValue());
    LogHTU21D_humidity.setSendNow();
  }

  if (LogHTU21D_temperatur.getSendRequired()) {
    mqttClient.publish(LogHTU21D_temperatur.getCValueName(), LogHTU21D_temperatur.getCValue());
    LogHTU21D_temperatur.setSendNow();
  }
  //ESP
  if (LogESPAlias.getSendRequired()) {
    mqttClient.publish(LogESPAlias.getCValueName(), LogESPAlias.getCValue());
    LogESPAlias.setSendNow();
  }
  if (LogESPMAC.getSendRequired()) {
    mqttClient.publish(LogESPMAC.getCValueName(), LogESPMAC.getCValue());
    LogESPMAC.setSendNow();
  }
  if (LogESPFreeHeap.getSendRequired()) {
    mqttClient.publish(LogESPFreeHeap.getCValueName(), LogESPFreeHeap.getCValue());
    LogESPFreeHeap.setSendNow();
  }
}

//### FUNCTIONEN ################################################################################
void setupHeartbeatTick() { // langsames Blinken, Heartbeat
  if (tickerCount == 100) {
    digitalWrite(LED_BUILTIN, LOW);
    tickerCount = 0;
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
    tickerCount = tickerCount + 1;
  }
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
}

