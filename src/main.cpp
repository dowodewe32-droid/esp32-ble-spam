#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <NimBLEScan.h>

#define LED_BUILTIN 2

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

struct ScanResult {
    String name;
    String addr;
    int rssi;
    uint32_t timestamp;
};
std::vector<ScanResult> scanResults;

const char* device_names[] = {
    "AirPods Pro", "AirPods", "AirPods Max", "Apple Watch", "Apple TV",
    "HomePod Mini", "iPhone", "iPad", "MacBook Pro", "Find My",
    "Share Menu", "NameDrop", "AirPlay", "Handoff", "AirDrop",
    "Samsung Buds", "Galaxy Watch", "Galaxy Tag", "Galaxy Phone",
    "Windows Phone", "Windows Device", "Chromebook", "Android Auto",
    "Smart Lock", "Google Pixel"
};
const int device_count = sizeof(device_names) / sizeof(device_names[0]);

void handleRoot();
void handleStart();
void handleStop();
void handleStatus();
void handleDevices();
void handleScan();
void handleScanResults();

void setup() {
    Serial.begin(115200);
    delay(500);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println();
    Serial.println(F("==========================================="));
    Serial.println(F("  ESP32 BLE Spam - Web Controller v1.1"));
    Serial.println(F("==========================================="));
    Serial.println();
    Serial.print(F("Attempting to connect to WiFi: "));
    Serial.println(wifi_ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println();
        Serial.println(F("========== WiFi CONNECTED =========="));
        Serial.print(F("SSID: "));
        Serial.println(WiFi.SSID());
        Serial.print(F("IP: "));
        Serial.println(WiFi.localIP());
        Serial.print(F("Web: http://"));
        Serial.println(WiFi.localIP());
        Serial.println(F("==================================="));
        digitalWrite(LED_BUILTIN, HIGH);
    } else {
        Serial.println();
        Serial.println(F("WiFi FAILED! Starting AP mode..."));
        WiFi.mode(WIFI_AP);
        WiFi.softAP("BLE-Spam", "12345678");
        delay(200);
        Serial.print(F("AP IP: "));
        Serial.println(WiFi.softAPIP());
        Serial.print(F("Web: http://"));
        Serial.println(WiFi.softAPIP());
        digitalWrite(LED_BUILTIN, HIGH);
    }

    BLEDevice::init("");

    server.on("/", HTTP_GET, handleRoot);
    server.on("/start", HTTP_GET, handleStart);
    server.on("/stop", HTTP_GET, handleStop);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/devices", HTTP_GET, handleDevices);
    server.on("/scan", HTTP_GET, handleScan);
    server.on("/scan_results", HTTP_GET, handleScanResults);
    server.begin();

    Serial.println();
    Serial.println(F("Server started. Ready to spam!"));
    Serial.println();
}

void loop() {
    server.handleClient();
    
    if (spamming) {
        digitalWrite(LED_BUILTIN, (millis() / 200) % 2 ? HIGH : LOW);
    } else if (scanning) {
        digitalWrite(LED_BUILTIN, (millis() / 100) % 2 ? HIGH : LOW);
    } else {
        digitalWrite(LED_BUILTIN, HIGH);
    }
}

void handleRoot() {
    String ipStr = WiFi.localIP().toString();
    if (ipStr == "0.0.0.0") {
        ipStr = WiFi.softAPIP().toString();
    }
    
    String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>ESP32 BLE Spam</title>";
    html += "<style>* {margin:0;padding:0;box-sizing:border-box} body {font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:linear-gradient(135deg,#1a1a2e,#16213e);color:#fff;min-height:100vh;padding:20px} .container {max-width:500px;margin:0 auto} .header {text-align:center;padding:30px 0} .header h1 {color:#e94560;font-size:28px;margin-bottom:10px} .header p {color:#888;font-size:14px} .card {background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.1);border-radius:16px;padding:24px;margin-bottom:16px} .status {text-align:center;padding:20px;border-radius:12px;font-size:20px;font-weight:bold;margin-bottom:20px} .status.off {background:linear-gradient(135deg,#c0392b,#e74c3c)} .status.on {background:linear-gradient(135deg,#27ae60,#2ecc71);animation:pulse 1s infinite} @keyframes pulse {0%,100%{opacity:1}50%{opacity:0.7}} .form-group {margin-bottom:16px} .form-group label {display:block;color:#888;margin-bottom:8px;font-size:13px;text-transform:uppercase} .form-group select,.form-group input {width:100%;padding:14px;border-radius:10px;border:1px solid rgba(255,255,255,0.1);background:rgba(0,0,0,0.3);color:#fff;font-size:16px} .btn {display:block;width:100%;padding:16px;border:none;border-radius:10px;font-size:18px;font-weight:bold;cursor:pointer;margin-top:10px} .btn-start {background:linear-gradient(135deg,#27ae60,#2ecc71);color:#fff} .btn-start:hover {transform:translateY(-2px)} .btn-stop {background:linear-gradient(135deg,#c0392b,#e74c3c);color:#fff} .btn-stop:hover {transform:translateY(-2px)} .btn-scan {background:linear-gradient(135deg,#3498db,#2980b9);color:#fff} .btn-scan:hover {transform:translateY(-2px)} .device-list {margin-top:20px;padding:15px;background:rgba(0,0,0,0.2);border-radius:10px;font-size:13px} .device-list h4 {color:#e94560;margin-bottom:10px} .device-list span {display:inline-block;padding:5px 10px;margin:3px;background:rgba(255,255,255,0.1);border-radius:5px;font-size:12px} .scan-results {margin-top:20px;padding:15px;background:rgba(0,0,0,0.2);border-radius:10px;max-height:300px;overflow-y:auto} .scan-results h4 {color:#3498db;margin-bottom:10px} .scan-item {padding:8px;border-bottom:1px solid rgba(255,255,255,0.1);font-size:12px} .scan-item:last-child {border-bottom:none} .rssi-good {color:#2ecc71} .rssi-ok {color:#f39c12} .rssi-bad {color:#e74c3c}</style>";
    html += "</head><body><div class='container'><div class='header'><h1>ESP32 BLE Spam</h1><p>Security Testing Tool | " + ipStr + "</p></div><div class='card'><div id='status' class='status off'>STOPPED</div><div class='form-group'><label>Target Device</label><select id='device'>";
    
    for (int i = 0; i < device_count; i++) {
        html += "<option value='" + String(i) + "'>" + String(device_names[i]) + "</option>";
    }
    
    html += "</select></div><div class='form-group'><label>Interval (ms)</label><input type='number' id='interval' value='1000' min='100' max='5000' step='100'></div><div class='form-group'><label><input type='checkbox' id='randomMac' checked> Random MAC Address</label></div><button class='btn btn-start' onclick='startSpam()'>START SPAM</button><button class='btn btn-stop' onclick='stopSpam()'>STOP SPAM</button><button class='btn btn-scan' onclick='scanBLE()'>SCAN BLE</button></div><div class='card'><div class='device-list'><h4>Available Devices (25)</h4>";
    
    for (int i = 0; i < device_count; i++) {
        html += "<span>" + String(device_names[i]) + "</span>";
    }
    
    html += "</div></div><div class='card'><div class='scan-results'><h4>Scan Results (<span id='scanCount'>0</span>)</h4><div id='scanList'></div></div></div><script>let isSpamming=false;let isScanning=false;function startSpam(){let d=document.getElementById('device').value;let i=document.getElementById('interval').value;let m=document.getElementById('randomMac').checked?1:0;fetch('/start?device='+d+'&interval='+i+'&mac='+m).then(r=>r.text()).then(()=>{document.getElementById('status').className='status on';document.getElementById('status').textContent='SPAMMING';isSpamming=true})});function stopSpam(){fetch('/stop').then(r=>r.text()).then(()=>{document.getElementById('status').className='status off';document.getElementById('status').textContent='STOPPED';isSpamming=false})});function scanBLE(){fetch('/scan').then(r=>r.text()).then(()=>{isScanning=true;setTimeout(updateScan,3000)})};function updateScan(){fetch('/scan_results').then(r=>r.json()).then(d=>{let html='';document.getElementById('scanCount').textContent=d.length;d.forEach(e=>{let rssiClass=e.rssi>-60?'rssi-good':e.rssi>-80?'rssi-ok':'rssi-bad';html+='<div class=\"scan-item\"><strong>'+e.name+'</strong><br>'+e.addr+' <span class=\"'+rssiClass+'\">'+e.rssi+' dBm</span> '+e.time+'s ago</div>'});document.getElementById('scanList').innerHTML=html;if(isScanning)setTimeout(updateScan,2000)});if(!isScanning)setTimeout(updateScan,2000)};function updateStatus(){fetch('/status').then(r=>r.json()).then(d=>{if(d.spamming!==isSpamming){isSpamming=d.spamming;document.getElementById('status').className=d.spamming?'status on':'status off';document.getElementById('status').textContent=d.spamming?'SPAMMING':'STOPPED'}})};setInterval(updateStatus,2000);</script></body></html>";
    
    server.send(200, "text/html", html);
}

void handleStart() {
    if (server.hasArg("device")) current_device = server.arg("device").toInt();
    if (server.hasArg("interval")) interval_ms = server.arg("interval").toInt();
    if (server.arg("mac") == "0") random_mac = false;
    else random_mac = true;

    if (!spamming) {
        spamming = true;
        xTaskCreatePinnedToCore(
            [](void* param) {
                uint8_t mac[6];
                while (spamming) {
                    if (random_mac) {
                        for (int i = 0; i < 6; i++) mac[i] = esp_random() & 0xFF;
                        esp_base_mac_addr_set(mac);
                        Serial.printf("Set random MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
                    }
                    
                    BLEAdvertisementData adv;
                    adv.setName(device_names[current_device]);
                    adv.setAppearance(0x0040);
                    
                    NimBLEAdvertising* pAdv = BLEDevice::getAdvertising();
                    pAdv->setAdvertisementData(adv);
                    pAdv->start();
                    Serial.println("Started advertising");
                    delay(80);
                    pAdv->stop();
                    Serial.println("Stopped advertising");
                    delay(interval_ms);
                }
                Serial.println("BLE spam task exiting");
                vTaskDelete(NULL);
            },
            "BLE-Spam",
            4096,
            NULL,
            1,
            &spamTaskHandle,
            0
        );
        
        Serial.println("Spam started: " + String(device_names[current_device]));
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
    String json = "{\"spamming\":" + String(spamming ? "true" : "false") + 
                 ",\"device\":" + String(current_device) + 
                 ",\"interval\":" + interval_ms + 
                 ",\"mac\":" + String(random_mac ? "true" : "false") + "}";
    server.send(200, "application/json", json);
}

void handleDevices() {
    String json = "[";
    for (int i = 0; i < device_count; i++) {
        if (i > 0) json += ",";
        json += "{\"id\":" + String(i) + ",\"name\":\"" + String(device_names[i]) + "\"}";
    }
    json += "]";
    server.send(200, "application/json", json);
}

void handleScan() {
    if (!scanning) {
        scanning = true;
        scanResults.clear();
        Serial.println("Starting BLE scan...");
        
        xTaskCreatePinnedToCore(
            [](void* param) {
                Serial.println("BLE scan started for 5 seconds");
                BLEScan* pScan = BLEDevice::getScan();
                pScan->setActiveScan(true);
                pScan->setInterval(100);
                pScan->setWindow(99);
                
                NimBLEScanResults results = pScan->start(5, false);
                int count = results.getCount();
                Serial.printf("Scan complete, found %d devices\n", count);
                
                for (int i = 0; i < count; i++) {
                    NimBLEAdvertisedDevice temp = results.getDevice(i);
                    ScanResult r;
                    r.name = temp.getName().c_str();
                    r.addr = temp.getAddress().toString().c_str();
                    r.rssi = temp.getRSSI();
                    r.timestamp = millis();
                    scanResults.push_back(r);
                    Serial.printf("  %s (%s) %d dBm\n", r.name.c_str(), r.addr.c_str(), r.rssi);
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

void handleScanResults() {
    String json = "[";
    uint32_t now = millis();
    for (size_t i = 0; i < scanResults.size(); i++) {
        if (i > 0) json += ",";
        uint32_t ago = (now - scanResults[i].timestamp) / 1000;
        json += "{\"name\":\"" + scanResults[i].name + "\",\"addr\":\"" + scanResults[i].addr + "\",\"rssi\":" + String(scanResults[i].rssi) + ",\"time\":" + String(ago) + "}";
    }
    json += "]";
    server.send(200, "application/json", json);
}
