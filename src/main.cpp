#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEAdvertising.h>

#define LED_PIN 2

const char* wifi_ssid = "GMpro";
const char* wifi_password = "Sangkur87";

WebServer server(80);
bool spamming = false;
bool scanning = false;
bool random_mac = true;
int current_device = 0;
unsigned long interval_ms = 1000;
TaskHandle_t spamTaskHandle = NULL;
TaskHandle_t scanTaskHandle = NULL;

// Scan results
struct ScanResult {
    String name;
    String addr;
    int rssi;
    uint32_t timestamp;
};
std::vector<ScanResult> scanResults;

// BLE Scan callback
class MyScanCallbacks : public NimBLEScanCallbacks {
public:
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        ScanResult r;
        r.name = advertisedDevice->getName().c_str();
        r.addr = advertisedDevice->getAddress().toString().c_str();
        r.rssi = advertisedDevice->getRSSI();
        r.timestamp = millis();
        scanResults.push_back(r);
        Serial.printf("Scan: %s (%s) %d dBm\n", r.name.c_str(), r.addr.c_str(), r.rssi);
    }
};

void setup_wifi();
void handleRoot();
void handleStart();
void handleStop();
void handleStatus();
void handleDevices();
void handleScan();

void setup() {
    Serial.begin(115200);
    delay(1000);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    Serial.println("\n===========================");
    Serial.println("  ESP32 BLE Spam - Web Controller v1.3");
    Serial.println("===========================");
    Serial.println();
    
    setup_wifi();
    
    Serial.println("\nInitializing BLE...");
    BLEDevice::init("ESP32-BLE");
    delay(500);
    Serial.println("BLE ready!");
    
    server.on("/", HTTP_GET, handleRoot);
    server.on("/start", HTTP_GET, handleStart);
    server.on("/stop", HTTP_GET, handleStop);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/devices", HTTP_GET, handleDevices);
    server.on("/scan", HTTP_GET, handleScan);
    server.begin();
    
    Serial.println("\nServer started. Ready to spam!");
}

void setup_wifi() {
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
        Serial.println("\nWiFi connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWiFi failed! Starting AP...");
        WiFi.mode(WIFI_AP);
        WiFi.softAP("BLE-Spam", "12345678");
        Serial.print("AP IP: ");
        Serial.println(WiFi.softAPIP());
    }
    digitalWrite(LED_PIN, HIGH);
}

void loop() {
    server.handleClient();
    
    if (spamming) {
        digitalWrite(LED_PIN, (millis() / 100) % 2 ? HIGH : LOW);
    } else if (scanning) {
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
    
    String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>ESP32 BLE Spam</title>";
    html += "<style>* {margin:0;padding:0;box-sizing:border-box} body {font-family:sans-serif;background:#1a1a2e;color:#fff;min-height:100vh;padding:20px} .container {max-width:500px;margin:0 auto} .header {text-align:center;padding:20px 0} .header h1 {color:#e94560;margin-bottom:10px} .card {background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.1);border-radius:10px;padding:15px;margin:10px 0} .status {text-align:center;padding:15px;border-radius:10px;font-weight:bold;margin:10px 0} .status.off {background:#c0392b} .status.on {background:#27ae60;animation:pulse 1s infinite} @keyframes pulse {0%,100%{opacity:1} 50%{opacity:0.7}} .btn {display:block;width:100%;padding:12px;border:none;border-radius:8px;font-size:16px;font-weight:bold;cursor:pointer;margin:8px 0} .btn-start {background:#27ae60;color:#fff} .btn-stop {background:#c0392b;color:#fff} .btn-scan {background:#3498db;color:#fff}</style>";
    html += "</head><body><div class='container'><div class='header'><h1>ESP32 BLE Spam</h1><p>" + ipStr + "</p></div>";
    html += "<div class='card'><div id='status' class='status off'>STOPPED</div>";
    html += "<label>Device: <select id='device'><option value='0'>AirPods Pro</option><option value='1'>iPhone</option><option value='2'>iPad</option><option value='3'>MacBook</option><option value='4'>Samsung Buds</option><option value='5'>Galaxy Watch</option><option value='6'>Windows Phone</option><option value='7'>Android Auto</option></select></label><br>";
    html += "<label>Interval (ms): <input type='number' id='interval' value='1000' min='100' max='5000'></label><br>";
    html += "<label><input type='checkbox' id='randomMac' checked> Random MAC</label><br>";
    html += "<button class='btn btn-start' onclick='startSpam()'>START SPAM</button>";
    html += "<button class='btn btn-stop' onclick='stopSpam()'>STOP SPAM</button>";
    html += "<button class='btn btn-scan' onclick='scanBLE()'>SCAN BLE</button>";
    html += "</div>";
    html += "<script>";
    html += "let isSpamming=false;function startSpam(){let d=document.getElementById('device').value;let i=document.getElementById('interval').value;let m=document.getElementById('randomMac').checked?1:0;fetch('/start?device='+d+'&interval='+i+'&mac='+m).then(()=>{document.getElementById('status').className='status on';document.getElementById('status').textContent='SPAMMING';isSpamming=true})};";
    html += "function stopSpam(){fetch('/stop').then(()=>{document.getElementById('status').className='status off';document.getElementById('status').textContent='STOPPED';isSpamming=false})";
    html += "function scanBLE(){fetch('/scan').then(()=>{setTimeout(()=>{fetch('/scan').then(()=>{}),3000)})";
    html += "</script></body></html>";
    
    server.send(200, "text/html", html);
}

void handleStart() {
    if (server.hasArg("device")) current_device = server.arg("device").toInt();
    if (server.hasArg("interval")) interval_ms = server.arg("interval").toInt();
    if (server.hasArg("mac") == "0") random_mac = false;
    else random_mac = true;
    
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
                        Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
                    }
                    
                    BLEAdvertisementData adv;
                    if (current_device == 0) adv.setName("AirPods Pro");
                    else if (current_device == 1) adv.setName("iPhone");
                    else if (current_device == 2) adv.setName("iPad");
                    else if (current_device == 3) adv.setName("MacBook");
                    else if (current_device == 4) adv.setName("Samsung Buds");
                    else if (current_device == 5) adv.setName("Galaxy Watch");
                    else if (current_device == 6) adv.setName("Windows Phone");
                    else adv.setName("Android Auto");
                    
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
        Serial.println("Spam task created");
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

void handleDevices() {
    String json = "[{"id":0,"name":"AirPods Pro"},{"id":1,"name":"iPhone"},{"id":2,"name":"iPad"},{"id":3,"name":"MacBook"},{"id":4,"name":"Samsung Buds"},{"id":5,"name":"Galaxy Watch"},{"id":6,"name":"Windows Phone"},{"id":7,"name":"Android Auto"}]";
    server.send(200, "application/json", json);
}

void handleScan() {
    if (!scanning) {
        scanning = true;
        scanResults.clear();
        Serial.println("Starting scan task...");
        xTaskCreatePinnedToCore(
            [](void* param) {
                Serial.println("Scan task started");
                NimBLEScan* pScan = BLEDevice::getScan();
                if (pScan) {
                    pScan->setScanCallbacks(new MyScanCallbacks());
                    pScan->setActiveScan(true);
                    pScan->setInterval(100);
                    pScan->setWindow(99);
                    pScan->start(5, false);
                    Serial.println("Scan complete");
                }
                scanning = false;
                Serial.println("Scan task exiting");
                vTaskDelete(NULL);
            },
            "BLE-Scan",
            8192,
            NULL,
            1,
            &scanTaskHandle,
            0
        );
    }
    server.send(200, "text/plain", "Scan started");
}
