# README #
## Funktionen ##
### WiFi Manager ###
https://github.com/tzapu/WiFiManager<br>
Der WiFi Manager startet, wenn Pin D5 == 1 (Touch) bei PowerOn<br>
<br>
### Heartbeat ###
Flash an "LED_BUILTIN"<br>
<br>
### OTA ###
Update Over Air<br>
Konfiguriertes Passwort
```
ArduinoOTA.setPassword((const char *)"52364897");
```


### DNS Name ###

Der DNS Name setzt sich zusammen aus [hostName] und [ESPNodeID]<br>
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
<br>
Bsp.:<br>
SensorNode/166646/HTU21D_humidity<br>

#### Subscribe #### 
Farbe LED1<br>
Bsp: SensorNode/166646/IN/LED1<br>
Farbe LED2<br>
Bsp: SensorNode/166646/IN/LED1<br>
Farbe LED<br>
Bsp: SensorNode/166646/IN/LED<br>
Value #ff00ff<br>
<br>
Reset auslösen<br>
Bsp: SensorNode/166646/IN/RESET<br>

### Sensoren ###
Touch<br>
DS10B20 Temp OneWire<br>
TSL2561 Light I2C<br>
SHT20 Humidity, temperatur I2C<br>

### Aktoren ###

### Anschlussplan ###
WeMos ESP8266<br>
<br>
TX  1<br>
RX  3<br>
D1  5  SCL<br>
D2  4  SDA<br>
D3  0  (WAKE)<br>
D4  2  NEOPIXEL<br>
GND<br>
5V<br>
<br>
RST<br>
A0<br>
D0  16<br>
D5  14  Touch<br>
D6  12<br>
D7  13  DS10B20<br>
D8  15<br>
3V3<br>
<br>


