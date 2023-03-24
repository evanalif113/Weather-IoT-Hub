
#define BLYNK_TEMPLATE_ID "TMPLnpPSbLMm"
#define BLYNK_TEMPLATE_NAME "WeatherStationESP32"

#define BLYNK_FIRMWARE_VERSION "0.1.14"

#define BLYNK_PRINT Serial
#define BLYNK_DEBUG

#define APP_DEBUG

// Uncomment your board, or configure a custom board in Settings.h
#define USE_ESP32_DEV_MODULE

#include "BlynkEdgent.h"
#include <Adafruit_Sensor.h>
#include "Adafruit_SHT31.h"
#include <Adafruit_BMP280.h>
#include <Firebase.h>
#include "time.h"

#define API_KEY       "AIzaSyDvrdfnD-0Cy1g4F0PX8mfwWo6PLzQNgtg"
#define USER_EMAIL    "evanalifwidhyatma@gmail.com"
#define USER_PASSWORD "bakayaro"
#define DATABASE_URL  "https://jeris-education-default-rtdb.asia-southeast1.firebasedatabase.app/"
const char* ntpServer = "time.google.com";

int analogInPin  = 39;    // Analog input pin
int sensorValue;          // Analog Output of Sensor
float calibration = 0.47 ; // Check Battery voltage using multimeter & add/subtract the value
int bat_percentage;

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float temperature;
float humidity;
float pressure;
float dewpoint;
float heat;
int ram;
int rssi;
int timestamp;
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_BMP280 bmp;
// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
// Variable to save USER UID
String uid;
// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String temperaturePath = "/temperature";
String humidityPath = "/humidity";
String pressurePath = "/pressure";
String dewpointPath = "/dewpoint";
String heatindexPath = "/heatindex";
String timePath = "/timestamp";
String ramPath = "/espheapram";
String parentPath;
FirebaseJson json;

//unsigned int myChannelNumber = 2;
//const char* myWriteAPIKey = "1CAB7D9FTRFKIJDY";

//String apiKey1 = "1CAB7D9FTRFKIJDY"; //API Thingspeak Prototipe
String apiKey1 = "S1VN6AN4R1QT4QMD"; //API Thingspeak Portable

/*String wid = "20827bd2c71d5d59";
String apiKey2 = "bea9f057d1af19cfdd8ba31bb7b367f9"*/;// Jerukagung Climate

String wid = "e28208f1755c7f97";
String apiKey2 = "151d25fa6b39a591ea9651f1a110e2ad";// Jerukagung Meteorologi

uint16_t DataThingspeakPeriod = 59000;  //1 Menit
uint32_t DataThingspeakNext = 0;
uint32_t DataWeathercloudPeriod = 300000;
uint32_t DataWeathercloudNext = 0;
uint16_t DataFirebasePeriod = 30000;
uint32_t DataFirebaseNext = 0;
uint16_t DataPeriod = 10000;  //
uint32_t DataNext = 0;
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}
void FirebaseInit() {
  configTime(0, 0, ntpServer); //NTP Server
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  config.max_token_generation_retry = 5;
  Firebase.begin(&config, &auth);
  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: "); Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";
}
void setup() {
  Serial.begin(115200);
  Serial.println("SHT31 and BMP280 test");
  if (!sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Could not find a valid SHT31 sensor, check wiring!");
    while (1);
  }

  if (!bmp.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
  //ThingSpeak.begin(client);
  analogReadResolution(12);
  BlynkEdgent.begin();
  FirebaseInit();
}


void data() {
  temperature = sht31.readTemperature(); //- 0.4
  humidity = sht31.readHumidity();
  pressure = bmp.readPressure() / 100.0F;
  
  double calc = log(humidity / 100.0F) + ((17.625F * temperature) / (243.04F + temperature));
  dewpoint = (243.04F * calc / (17.625F - calc));
  
  #define c1 -8.78469475556
  #define c2 1.61139411
  #define c3 2.33854883889
  #define c4 -0.14611605
  #define c5 -0.012308094
  #define c6 -0.0164248277778
  #define c7 0.002211732
  #define c8 0.00072546
  #define c9 -0.000003582
  heat = c1 + (c2 * temperature) + (c3 * humidity) + (c4 * temperature * humidity) + (c5 * sq(temperature)) + (c6 * sq(humidity)) + (c7 * sq(temperature) * humidity) + (c8 * temperature * sq(humidity)) + (c9 * sq(temperature) * sq(humidity));  
  
  rssi = (WiFi.RSSI());
  ram = (ESP.getFreeHeap());
  sensorValue = analogRead(analogInPin);
  float voltage = (((sensorValue * 3.3) / 4095) * 2 + calibration); //multiply by two as voltage divider network is 100K & 100K Resistor
 
  bat_percentage = mapfloat(voltage, 2.8, 4.2, 0, 100); //2.8V as Battery Cut off Voltage & 4.2V as Maximum Voltage
  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, pressure);
  Blynk.virtualWrite(V3, dewpoint);
  Blynk.virtualWrite(V8, voltage);
  Blynk.virtualWrite(V9, rssi);
  Blynk.virtualWrite(V10, ram);
}
void dataFirebase() {
  // Send new readings to database
    timestamp = getTime();
    Serial.println (timestamp);

    parentPath = databasePath + "/" + String(timestamp);
    //Suhu
    json.set(temperaturePath.c_str(),  String(temperature));
    json.set(humidityPath.c_str(), String(humidity));
    json.set(dewpointPath.c_str(), String(dewpoint));
    json.set(heatindexPath.c_str(), String(heat));
    json.set(pressurePath.c_str(),  String(pressure));
    json.set(ramPath.c_str(),  String(ram));
    json.set(timePath.c_str(),  String(timestamp));

    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
}
WiFiClient client;
HTTPClient http;
void dataThingspeak() {
  //Specify content-type header
  //http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  //Data to send with HTTP POST
  String url1 = "http://api.thingspeak.com/update?api_key=" + apiKey1;
  url1 += "&field1=" + String(temperature);
  url1 += "&field2=" + String(humidity);
  url1 += "&field3=" + String(pressure);
  url1 += "&field4=" + String(dewpoint);
  url1 += "&field5=" + String(heat);
  // Send HTTP POST request
  http.begin(client, url1);
  int httpResponseCode = http.POST(url1);

  //Serial.print("HTTP Response code: ");
  //Serial.println(httpResponseCode);

  // Free resources
  http.end();
}
void dataWeathercloud() {
  WiFiClient client;
  HTTPClient http;
  String url2 = "http://api.weathercloud.net/set/wid/" + wid + "/key/" + apiKey2;
  url2 += "/temp/" + String(temperature*10);
  url2 += "/hum/" + String(humidity);
  url2 += "/bar/" + String(pressure*10);
  url2 += "/dew/" + String(dewpoint*10);
  url2 += "/heat/" + String(heat*10);
  url2 += "/chill/" + String(dewpoint*10);
  url2 += "/wspd/" + String(0);
  url2 += "/wspdhi/" + String(0);
  url2 += "/wdir/" + String(0);
  url2 += "/rain/" + String(0*10);
  url2 += "/rainrate/" + String(0*10);
  url2 += "/solarrad/" + String(0*10);
  url2 += "/et/" + String(0*10);
  url2 += "/uvi/" + String(0*10);  
     
  
  http.begin(client, url2);
  int httpResponseCode = http.POST(url2);
  
  http.end();
}

void loop() {
  BlynkEdgent.run();
  if (DataNext <= millis()) {
    data();
    DataNext = millis() + DataPeriod;
  }
  if (Firebase.ready() && (DataFirebaseNext <= millis())) {
    dataFirebase();
    DataFirebaseNext = millis() + DataFirebasePeriod;
  }
  if (DataThingspeakNext <= millis()) {
    dataThingspeak();
    DataThingspeakNext = millis() + DataThingspeakPeriod;
  }
  if (DataWeathercloudNext <= millis()) {
    dataWeathercloud();
    DataWeathercloudNext = millis() + DataWeathercloudPeriod;
  }
}
