#include <SPI.h>
#include <Ethernet.h>
#include <WebSocketsServer.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // Change for multiple devices
IPAddress ip(192, 168, 1, 50);  // Static IP
EthernetServer server(80);       // Web Server on Port 80
WebSocketsServer webSocket(81);  // WebSocket Server on Port 81

const int ledPins[] = {2, 3, 4, 5}; // LED control pins
const int numLEDs = sizeof(ledPins) / sizeof(ledPins[0]);

void setup() {
    Serial.begin(115200);

    // Initialize Ethernet
    Ethernet.begin(mac, ip);
    Serial.print("IP Address: ");
    Serial.println(Ethernet.localIP());

    // Start Servers
    server.begin();
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    // Initialize LED Pins
    for (int i = 0; i < numLEDs; i++) {
        pinMode(ledPins[i], OUTPUT);
        digitalWrite(ledPins[i], LOW);
    }
}

void loop() {
    EthernetClient client = server.available();
    if (client) {
        handleClient(client);
    }
    webSocket.loop();
}

// Handle WebSocket Events
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
        int index = payload[length - 1] - '0'; // Extract LED index
        if (index >= 0 && index < numLEDs) {
            if (payload[0] == '1') {
                digitalWrite(ledPins[index], HIGH);
            } else {
                digitalWrite(ledPins[index], LOW);
            }
        }
    }
}

// Serve Web Page to Client
void handleClient(EthernetClient &client) {
    boolean currentLineIsBlank = true;
    while (client.connected()) {
        if (client.available()) {
            char c = client.read();
            Serial.write(c);
            if (c == '\n' && currentLineIsBlank) {
                sendWebPage(client);
                break;
            }
            if (c == '\n') {
                currentLineIsBlank = true;
            } else if (c != '\r') {
                currentLineIsBlank = false;
            }
        }
    }
    delay(1);
    client.stop();
}

// Send Web Page to Client
void sendWebPage(EthernetClient &client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println(R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>LED Control</title>
    <style>
        body { font-family: Arial; background-color: black; color: white; text-align: center; }
        .switch { display: inline-block; width: 60px; height: 34px; position: relative; }
        .switch input { opacity: 0; width: 0; height: 0; }
        .slider { position: absolute; cursor: pointer; width: 100%; height: 100%; background-color: #ccc; transition: .4s; }
        .slider:before { content: ""; position: absolute; width: 26px; height: 26px; left: 4px; bottom: 4px; background-color: white; transition: .4s; }
        input:checked + .slider { background-color: #4caf50; }
        input:checked + .slider:before { transform: translateX(26px); }
        .slider.round { border-radius: 34px; }
        .slider.round:before { border-radius: 50%; }
    </style>
</head>
<body>
    <h1>W5500 WebSocket LED Control</h1>
    <div class="status" id="status1">LED 1: OFF</div>
    <label class="switch"><input type="checkbox" class="toggleButton" id="toggleButton1"><span class="slider round"></span></label>
    <div class="status" id="status2">LED 2: OFF</div>
    <label class="switch"><input type="checkbox" class="toggleButton" id="toggleButton2"><span class="slider round"></span></label>
    <div class="status" id="status3">LED 3: OFF</div>
    <label class="switch"><input type="checkbox" class="toggleButton" id="toggleButton3"><span class="slider round"></span></label>
    <div class="status" id="status4">LED 4: OFF</div>
    <label class="switch"><input type="checkbox" class="toggleButton" id="toggleButton4"><span class="slider round"></span></label>

    <script>
        var ws = new WebSocket('ws://' + window.location.hostname + ':81/');
        var buttons = document.querySelectorAll('.toggleButton');
        var statuses = [document.getElementById('status1'), document.getElementById('status2'), document.getElementById('status3'), document.getElementById('status4')];

        buttons.forEach((button, index) => {
            button.addEventListener('change', function() {
                var state = button.checked ? '1' : '0';
                ws.send(state + index);
            });
        });

        ws.onmessage = function(event) {
            var data = event.data;
            var state = data.substring(0, 1);
            var index = parseInt(data.substring(1));
            statuses[index].textContent = state === '1' ? 'ON' : 'OFF';
        };
    </script>
</body>
</html>
)rawliteral");
}
