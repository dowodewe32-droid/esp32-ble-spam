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

void loop() {
    server.handleClient();

    if (spamming) {
        digitalWrite(LED_PIN, (millis() / 200) % 2 ? HIGH : LOW);
    } else {
        digitalWrite(LED_PIN, HIGH);
    }
}

void handleRoot() {
    String ipStr = WiFI.localIP().toString();
    if (ipStr == "0.0.0.0") {
        ipStr = WiFI.softAPIP().toString();
    }

    String html = "<html><body>";
    html += "<h1>ESP32 BLE Spam</h1>";
    html += "<p>" + ipStr + "</p>";
    html += "<p id='status'>STOPPED</p>";
    html += "<button onclick='startSpam()'>START</button>";
    html += "<button onclick='stopSpam()'>STOP</button>";
    html += "<script>";
    html += "function startSpam(){fetch('/start').then(()=>{document.getElementById('status').innerHTML='SPAMMING'})";
    html += "function stopSpam(){fetch('/stop').then(()=>{document.getElementById('status').innerHTML='STOPPED'})";
    html += "</script></body></html>";

    server.send(200, "text/html", html);
}

void handleStart() {
    if (!spamming) {
        spamming = true;
        Serial.println("Starting spam task...");
        xTaskCreatePinnedToCore(
            [](void* param) {
                Serial.println("Spam task started");
                while (spamming) {
                    if (random_mac) {
                        uint8_t mac[6];
                        for (int i = 0; i < 6; i++) mac[i] = esp_random() & 0xFF;
                        esp_base_mac_addr_set(mac);
                    }

                    BLEAdvertisementData adv;
                    if (current_device == 0) adv.setName("AirPods Pro");
                    else adv.setName("ESP32 BLE");

                    NimBLEAdvertising* pAdv = BLEDevice::getAdvertising();
                    pAdv->setAdvertisementData(adv);
                    pAdv->start();
                    delay(80);
                    pAdv->stop();
                    delay(interval_ms);
                }
                Serial.println("Spam task exiting");
                vTaskDelete(NULL);
            },
            "BLE-Spam",
            4096,
            NULL,
            1,
            &spamTaskHandle,
            0
        );
    }
    server.send(200, "text/plain", "Started");
}

void handleStop() {
    spamming = false;
    if (spamTaskHandle) {
        vTaskDelete(spamTaskHandle);
        spamTaskHandle = NULL;
    }
    BLEDevice::getAdvertising()->stop();
    Serial.println("Spam stopped");
    server.send(200, "text/plain", "Stopped");
}

void handleStatus() {
    String json = "{"spamming":" + String(spamming ? "true" : "false") + "}";
    server.send(200, "application/json", json);
}
