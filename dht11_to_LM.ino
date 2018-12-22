#include "DHTesp.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include "credentials.h"

#ifdef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP8266 ONLY!)
#error Select ESP8266 board.
#endif

DHTesp dht;
#define USE_SERIAL Serial
ESP8266WiFiMulti WiFiMulti;

// from https://github.com/zenmanenergy/ESP8266-Arduino-Examples/blob/master/helloWorld_urlencoded/urlencode.ino
String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
        if (c == ' '){
            encodedString+= '+';
        } else if (isalnum(c)){
            encodedString+=c;
        } else{
            code1=(c & 0xf)+'0';
            if ((c & 0xf) >9){
                code1=(c & 0xf) - 10 + 'A';
            }
            c=(c>>4)&0xf;
            code0=c+'0';
            if (c > 9){
                code0=c - 10 + 'A';
            }
            code2='\0';
            encodedString+='%';
            encodedString+=code0;
            encodedString+=code1;
            //encodedString+=code2;
        }
        yield();
    }
    return encodedString;   
}

void setup() {
    // turn off built in LED
    int pinnum;
    pinnum = 12;
    pinMode(pinnum, OUTPUT);
    digitalWrite(pinnum, LOW);
    pinnum = 13;
    pinMode(pinnum, OUTPUT);
    digitalWrite(pinnum, LOW);
    pinnum = 15;
    pinMode(pinnum, OUTPUT);
    digitalWrite(pinnum, LOW);

    // init serial
    USE_SERIAL.begin(115200);
    // init DHT sensor on GPIO 14
    dht.setup(14, DHTesp::DHT11);

    // init Wifi connection
    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }
    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
}

void update_lm_object(const char *address, const char *lm_datatype, float value)
{
    String output;
    HTTPClient http;
    StaticJsonDocument<200> doc;
    JsonObject root = doc.to<JsonObject>();

    http.begin(LM_URI);
    http.addHeader("Authorization", BASIC_AUTH);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    root["type"] = "text";
    root["datatype"] = lm_datatype;
    root["address"] = address;
    root["value"] = value;
    serializeJson(doc, output);
    output = "data=" + urlencode(output);

    USE_SERIAL.print("[HTTP] POST...\n");
    // start connection and send HTTP header
    int httpCode = http.POST(output);

    USE_SERIAL.println(output);

    // httpCode will be negative on error
    if(httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        USE_SERIAL.printf("[HTTP] POST... code: %d\n", httpCode);
        if(httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            USE_SERIAL.println(payload);
        }
    } else {
        USE_SERIAL.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
        // blink led to indicate error
        for (int i = 0; i < 10; i++) {
            digitalWrite(12, HIGH);
            delay(3000);
            digitalWrite(12, LOW);
        }
    }
    http.end();
}

void loop() {    
    // wait for WiFi connection
    if((WiFiMulti.run() == WL_CONNECTED)) {
        // send DHT sensor values to LM
        update_lm_object("1/4/12", "5001", dht.getHumidity());
        update_lm_object("1/4/13", "9001", dht.getTemperature());
    }
    delay(300000); // wait 5 minutes and do again
}

