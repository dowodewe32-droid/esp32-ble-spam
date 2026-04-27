#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <NimBLEDevice.h>
#include <NimBLEAdvertising.h>

#define LED_PIN 2

const char* wifi_ssid = "GMpro";
const char* wifi_password = "Sangkur87";

WebServer server(80);
bool spamming = false;
bool random_mac = true;
int current_device = 0;
unsigned long interval_ms = 1000;
TaskHandle_t spamTaskHandle = NULL;

void handleRoot();
void handleStart();
void handleStop();
void handleStatus();

void setup() {
    Serial.begin(115200);
    delay(1000);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.println();
    Serial.println("===========================");
    Serial.println("  ESP32 BLE Spam v1.5");
    Serial.println("===========================");
    Serial.println();
    Serial.print("Connecting to WiFi: ");
    Serial.println(wifi_ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println("WiFi connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println();
        Serial.println("WiFi failed! Starting AP...");
        WiFi.mode(WIFI_AP);
        WiFi.softAP("BLE-Spam", "12345678");
        delay(200);
        Serial.print("AP IP: ");
        Serial.println(WiFi.softAPIP());
    }
    digitalWrite(LED_PIN, HIGH);

    Serial.println("Init BLE...");
    BLEDevice::init("ESP32");
    Serial.println("BLE ready!");

    server.on("/", HTTP_GET, handleRoot);
    server.on("/start", HTTP_GET, handleStart);
    server.on("/stop", HTTP_GET, handleStop);
    server.on("/status", HTTP_GET, handleStatus);
    server.begin();

    Serial.println("Server started!");
}
