#include <Servo.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <ArduinoJson.h> 
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); 

int pos = 180;    

const char* thingspeakApiKey = "YOUR_THINGSPEAK_READ_API_KEY";
const char* thingspeakWriteApiKey = "YOUR_THINGSPEAK_WRITE_API_KEY";

bool wifiConnected = false;

Servo servoMotor;
ESP8266WebServer server(80);

unsigned long previousMillis = 0; 
const long interval = 5000; 
String lastUpdatedTimestamp = "";

void setup() {
  Serial.begin(115200);

  delay(3000);

  EEPROM.begin(512);
  int readSsidLength = (EEPROM.read(0) << 8) | EEPROM.read(1);
  int readPasswordLength = (EEPROM.read(2) << 8) | EEPROM.read(3);

  Serial.println("SSID EEPROM'dan okunuyor");
  String checkSalt;
  String savedSSID;
  String savedPassword;

  for (int i = 0; i < 9; ++i)
  {
    checkSalt += char(EEPROM.read(i + 4));
  }
  for (int i = 0; i < readSsidLength; ++i)
  {
    savedSSID += char(EEPROM.read(i + 13));
  }
  for (int i = 0; i < readPasswordLength; ++i)
  {
    savedPassword += char(EEPROM.read(i + readSsidLength + 13));
  }
  if (checkSalt == "catfeeder") {
    Serial.println("EEPROM icerisinde kaydedilmis SSID ve sifre bulundu.");
    Serial.println(savedSSID);
    Serial.println(savedPassword);
    WiFi.begin(savedSSID, savedPassword);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFi'ya baglandi!");

    timeClient.begin();
    timeClient.update();
    checkThingSpeakChannel();
  } else {
    Serial.println("EEPROM icerisinde gecerli bir SSID ve sifre bulunamadi.");
    startAccessPoint();
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      Serial.print("Updating timestamp of thing speak channel: ");
      Serial.println(previousMillis);
      updateThingSpeakChannel();
      checkThingSpeakChannel();
    }
  }
  server.handleClient();
}

void statusPage() {
  String html = "<h1>Server is running...</h1>";
  server.send(200, "text/html", html);
}

void saveCredentials() {
  EEPROM.begin(512);  

  String salting = "catfeeder";

  String ssid = server.arg("ssid");
  String password = server.arg("password");

  int ssidLength = ssid.length();
  int passwordLength = password.length();

  Serial.println("SSID ve sifre kaydediliyor...");

  EEPROM.write(0, ssidLength >> 8); 
  EEPROM.write(1, ssidLength & 0xFF); 

  EEPROM.write(2, passwordLength >> 8); 
  EEPROM.write(3, passwordLength & 0xFF); 

  for (int i = 0; i < salting.length(); ++i)
  {
    EEPROM.write(i + 4, salting[i]);
    Serial.print("Wrote: ");
    Serial.println(salting[i]);
  }
  for (int i = 0; i < ssidLength; ++i)
  {
    EEPROM.write(i + 4 + salting.length(), ssid[i]);
    Serial.print("Wrote: ");
    Serial.println(ssid[i]);
  }
  for (int i = 0; i < passwordLength; ++i)
  {
    EEPROM.write(i + 4 + salting.length() + ssidLength, password[i]);
    Serial.print("Wrote: ");
    Serial.println(password[i]);
  }
  EEPROM.commit(); 
  EEPROM.end(); 

  server.send(200, "text/plain", "Bilgiler kaydedildi. ESP8266 aginiza baglanmaya calisacak.");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi'ya baglandi!");

  WiFi.softAPdisconnect();

  checkThingSpeakChannel();
}

void resetCredentials() {
  EEPROM.begin(512);  
  String salting = "xxxxxxxxxxx";

  for (int i = 0; i < salting.length(); ++i)
  {
    EEPROM.write(i + 4, salting[i]);
    Serial.print("Wrote: ");
    Serial.println(salting[i]);
  }

  EEPROM.commit(); 
  EEPROM.end(); 
}

void startAccessPoint() {
  const char *ssid = "CatFeeder";  
  const char *password = "catfeeder123";  

  Serial.println();
  Serial.print("Erisim noktasi olusturuluyor...");
  WiFi.softAP(ssid, password);

  IPAddress IP(192, 168, 4, 1);  
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(IP, gateway, subnet);

  startServer();
}

void startServer() {
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP adresi: ");
  Serial.println(myIP);
  server.on("/", statusPage);
  server.on("/save", HTTP_POST, saveCredentials);  
  server.begin();
  Serial.println("HTTP server baslatildi.");
}

void checkThingSpeakChannel() {
  WiFiClient client;
  if (client.connect("api.thingspeak.com", 80)) {
    Serial.println("ThingSpeak baglantisi kuruldu.");

    client.print("GET /channels/2266765/fields/1.json?api_key=");
    client.print(thingspeakApiKey);
    client.println("&results=1");

    String response = "";
    while (client.connected()) {
      String line = client.readStringUntil('\r');
      response += line;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);

    if (!error) {
      String timestamp = doc["feeds"][0]["field1"];
      
      if (timestamp != lastUpdatedTimestamp) {
        servoMotor.write(90);
        delay(3000);
        servoMotor.write(0);
        lastUpdatedTimestamp = timestamp; 
      } else {
        Serial.println("Same entry as the last one. Skipping...");
      }
    } else {
      Serial.println("JSON objesi olusturulurken hata meydana geldi.");
    }
  } else {
    Serial.println("ThingSpeak baglantisi basarisiz oldu.");
  }
  client.stop();
}

void updateThingSpeakChannel() {
  if (WiFi.status() == WL_CONNECTED) {
    timeClient.update();
    unsigned long epochTime = timeClient.getEpochTime();
    
    WiFiClient client;
    if (client.connect("api.thingspeak.com", 80)) {
      Serial.println("ThingSpeak'e veri gonderiliyor...");

      String postStr = "api_key=";
      postStr += thingspeakWriteApiKey;
      postStr += "&field2=";
      postStr += String(epochTime);

      client.print("POST /update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(postStr.length());
      client.print("\n\n");
      client.print(postStr);

      String response = client.readString();
      Serial.println(response);

      client.stop();
      Serial.println("Veri gonderildi.");
    } else {
      Serial.println("ThingSpeak baglantisi basarisiz oldu.");
    }
  }
}


