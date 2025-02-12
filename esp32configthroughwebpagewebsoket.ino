/// Youtube Video Link : - https://youtu.be/frt7MSAPuXU

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// Replace with your network credentials
const char* default_ssid = "YaranaFiber5g";
const char* default_password = "Yarana@7052";

// Create an instance of the server
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
Preferences preferences;

// Pin definitions
const int bootButtonPin = 0; // GPIO0 is typically used for the boot button on ESP32
const int ledPin = 2; // GPIO2 typically has an onboard LED

// Variables to hold network credentials
String selectedSSID = "";
String enteredPassword = "";

// Variables for boot button monitoring
bool resetToDefault = false;
long startTime = 0;

// Function to handle WebSocket events
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.println("WebSocket client connected");
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.println("WebSocket client disconnected");
    }
}

// Function to handle the root path
void handleRoot(AsyncWebServerRequest *request) {
    String html = R"rawliteral(
    <html>
    <head>
        <title>WiFi Networks</title>
        <style>
            body { font-family: Arial, sans-serif; }
            table { width: 100%; border-collapse: collapse; }
            th, td { padding: 10px; text-align: left; border-bottom: 1px solid #ddd; }
            th { background-color: #f2f2f2; }
            tr:hover { background-color: #f5f5f5; }
            .container { width: 80%; margin: auto; padding: 20px; }
            .form-container { margin-top: 20px; }
            .form-container input[type="text"], .form-container input[type="password"] {
                width: 100%; padding: 12px; margin: 8px 0; box-sizing: border-box;
            }
            .form-container input[type="submit"] {
                width: 100%; background-color: #4CAF50; color: white; padding: 14px 20px;
                margin: 8px 0; border: none; cursor: pointer;
            }
            .form-container input[type="submit"]:hover { background-color: #45a049; }
        </style>
        <script>
            var ws;

            function initWebSocket() {
                ws = new WebSocket('ws://' + window.location.hostname + '/ws');
                ws.onmessage = function(event) {
                    var wifiList = JSON.parse(event.data);
                    updateWiFiTable(wifiList);
                };
            }

            function updateWiFiTable(wifiList) {
                var table = document.getElementById('wifiTable');
                table.innerHTML = `
                    <tr>
                        <th>Nr</th>
                        <th>SSID</th>
                        <th>RSSI</th>
                        <th>Channel</th>
                        <th>Encryption</th>
                    </tr>
                `;
                wifiList.forEach((network, index) => {
                    var row = table.insertRow();
                    row.insertCell(0).innerHTML = index + 1;
                    row.insertCell(1).innerHTML = `<a href="#" onclick="connectToSSID('${network.ssid}')">${network.ssid}</a>`;
                    row.insertCell(2).innerHTML = network.rssi;
                    row.insertCell(3).innerHTML = network.channel;
                    row.insertCell(4).innerHTML = network.encryption;
                });
            }

            function connectToSSID(ssid) {
                document.getElementById('ssid').value = ssid;
                document.getElementById('connect-form').style.display = 'block';
            }

            window.onload = function() {
                initWebSocket();
            };
        </script>
    </head>
    <body>
        <div class="container">
            <h1>WiFi Networks</h1>
            <table id="wifiTable">
                <tr>
                    <th>Nr</th>
                    <th>SSID</th>
                    <th>RSSI</th>
                    <th>Channel</th>
                    <th>Encryption</th>
                </tr>
            </table>
            <div class="form-container" id="connect-form" style="display: none;">
                <h2>Connect to WiFi</h2>
                <form action="/submit" method="post">
                    <input type="hidden" name="ssid" id="ssid">
                    <label for="password">Password:</label>
                    <input type="password" name="password" id="password">
                    <input type="submit" value="Connect">
                </form>
            </div>
        </div>
    </body>
    </html>
    )rawliteral";
    request->send(200, "text/html", html);
}

// Function to handle the submit path
void handleSubmit(AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
        selectedSSID = request->getParam("ssid", true)->value();
        enteredPassword = request->getParam("password", true)->value();

        // Save the credentials in NVS
        preferences.putString("ssid", selectedSSID);
        preferences.putString("password", enteredPassword);

        // Connect to the selected network
        WiFi.begin(selectedSSID.c_str(), enteredPassword.c_str());
        while (WiFi.status() != WL_CONNECTED) {
            digitalWrite(ledPin, !digitalRead(ledPin)); // Toggle LED
            delay(500);
            Serial.println("Connecting to WiFi...");
        }
        digitalWrite(ledPin, LOW); // Turn off LED after connection
        Serial.println("Connected to WiFi");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());

        request->send(200, "text/html", "<html><body><h1>Connected to " + selectedSSID + "</h1><p>IP Address: " + WiFi.localIP().toString() + "</p></body></html>");
    } else {
        request->send(400, "text/html", "<html><body><h1>Bad Request</h1></body></html>");
    }
}

// Function to send the WiFi scan result via WebSocket
void sendWiFiScanResult() {
    int n = WiFi.scanNetworks();
    DynamicJsonDocument doc(2048); // Increased the size of the document
    JsonArray networks = doc.to<JsonArray>();
    for (int i = 0; i < n; ++i) {
        JsonObject network = networks.createNestedObject();
        network["ssid"] = WiFi.SSID(i);
        network["rssi"] = WiFi.RSSI(i);
        network["channel"] = WiFi.channel(i);
        network["encryption"] = encryptionType(WiFi.encryptionType(i));
    }
    String json;
    serializeJson(doc, json);
    ws.textAll(json);
}

// Function to get encryption type as a string
String encryptionType(wifi_auth_mode_t encryptionType) {
    switch (encryptionType) {
        case WIFI_AUTH_OPEN:
            return "Open";
        case WIFI_AUTH_WEP:
            return "WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WPA";
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA+WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "WPA2-EAP";
        case WIFI_AUTH_WPA3_PSK:
            return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return "WPA2+WPA3";
        case WIFI_AUTH_WAPI_PSK:
            return "WAPI";
        default:
            return "Unknown";
    }
}
/// Youtube Video Link : - https://youtu.be/frt7MSAPuXU
// Task to monitor the boot button
void buttonMonitorTask(void* parameter) {
    while (true) {
        if (digitalRead(bootButtonPin) == LOW) {
            if (startTime == 0) {
                startTime = millis();
            } else if (millis() - startTime > 2000) { // Changed to 2 seconds
                resetToDefault = true;
                preferences.clear(); // Clear preferences
                preferences.end(); // End preferences to reset to default
                ESP.restart(); // Restart ESP to apply changes
            }
        } else {
            startTime = 0;
        }
        delay(100);
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW); // Ensure LED is off initially
    preferences.begin("wifi", false);

    // Check if the boot button is pressed
    pinMode(bootButtonPin, INPUT_PULLUP);

    // Start a task to handle the button press
    xTaskCreate(buttonMonitorTask, "ButtonMonitor", 2048, NULL, 1, NULL);

    delay(3000); // Allow time for button task to set resetToDefault

    String savedSSID = preferences.getString("ssid", "");
    String savedPassword = preferences.getString("password", "");

    if (resetToDefault) {
        Serial.println("Resetting to default Wi-Fi credentials...");
        WiFi.begin(default_ssid, default_password);
    } else if (savedSSID != "" && savedPassword != "") {
        Serial.println("Connecting to saved Wi-Fi credentials...");
        WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    } else {
        Serial.println("Connecting to default Wi-Fi credentials...");
        WiFi.begin(default_ssid, default_password);
    }

    while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(ledPin, !digitalRead(ledPin)); // Toggle LED
        delay(500);
        Serial.println("Connecting to WiFi...");
    }
    digitalWrite(ledPin, LOW); // Turn off LED after connection

    Serial.println("Connected to WiFi");

    // Print the IP address of the server
    Serial.print("Server IP Address: ");
    Serial.println(WiFi.localIP());

    // Define the web server routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/submit", HTTP_POST, handleSubmit);

    // Add WebSocket event handler
    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);

    server.begin();
    Serial.println("HTTP server started");

    // Set an interval to perform WiFi scans and send the results
    xTaskCreate([](void*) {
        while (true) {
            sendWiFiScanResult();
            delay(10000); // Scan every 10 seconds
        }
    }, "WiFiScanner", 4096, NULL, 1, NULL);
}

void loop() {
    ws.cleanupClients();
}
/// Youtube Video Link : - https://youtu.be/frt7MSAPuXU
