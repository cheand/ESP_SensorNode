# README #
## Funktionen ##
### WiFi Manager ###
https://github.com/tzapu/WiFiManager
Der WiFi Manager startet bei Touch (Pin D5) bei PowerOn

### Heartbeat ###
Flash an "LED_BUILTIN"

### OTA ###
Update Over Air
Konfiguriertes Passwort
```
ArduinoOTA.setPassword((const char *)"52364897");
```

### DNS Name ###
Der DNS Name setzt sich zusammen aus [hostName] und [ESPNodeID]
Konfigurierbarer [hostName]
```
char hostName[30] = "SensorNode-";
```

### MQTT Syntax ###
#### Publish ####
Daten werden an den MQTT Server Übertragen. <br>
Der Topic setzt sich zusammen aus [mqttPrefix] [ESPNodeID] [Sensor]<br>
Konfigurierbarer [mqttPrefix]
```
const char mqttPrefix[] = "SensorNode/";
```

Bsp.:
SensorNode/166646/HTU21D_humidity

#### Subscribe #### 
Farbe LED1<br>
Bsp: SensorNode/166646/IN/LED1<br>
Farbe LED2<br>
Bsp: SensorNode/166646/IN/LED1<br>
Farbe LED<br>
Bsp: SensorNode/166646/IN/LED<br>
Value #ff00ff<br>

Reset auslösen<br>
Bsp: SensorNode/166646/IN/RESET<br>

### Sensoren ###
Touch
DS10B20 Temp OneWire
TSL2561 Light I2C
SHT20 Humidity, temperatur I2C

### Aktoren ###

### Anschlussplan ###
WeMos ESP8266

TX  1
RX  3
D1  5  SCL
D2  4  SDA
D3  0  (WAKE)
D4  2  NEOPIXEL
GND
5V

RST
A0
D0  16
D5  14  Touch
D6  12
D7  13  DS10B20
D8  15
3V3



