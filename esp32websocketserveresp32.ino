#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "Redmi10";
const char* password = "1234abcd";
const int ledPins[] = {2, 15, 4, 5}; // Pins for four LEDs
const int numLEDs = sizeof(ledPins) / sizeof(ledPins[0]);

WebSocketsServer webSocket = WebSocketsServer(81);
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Toggle Buttons</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 10px;
            background-color: black;
            color: white;
        }
        .created-by {
            color: red;
            font-size: 16px;
            margin-bottom: 10px;
        }
        .container {
            display: flex;
            flex-direction: column;
            align-items: center;
        }
        .status {
            font-size: 20px;
            margin-bottom: 10px;
        }
        .switch {
            display: inline-block;
            position: relative;
            width: 60px;
            height: 34px;
        }
        .switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }
        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #ccc;
            transition: .4s;
        }
        .slider:before {
            position: absolute;
            content: "";
            height: 26px;
            width: 26px;
            left: 4px;
            bottom: 4px;
            background-color: white;
            transition: .4s;
        }
        input:checked + .slider {
            background-color: #4caf50;
        }
        input:checked + .slider:before {
            transform: translateX(26px);
        }
        .slider.round {
            border-radius: 34px;
        }
        .slider.round:before {
            border-radius: 50%;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1 class="created-by">Created by Yarana IoT Guru</h1>
        <div class="status" id="status1">OFF</div>
        <label class="switch">
            <input type="checkbox" class="toggleButton" id="toggleButton1">
            <span class="slider round"></span>
        </label>
        <div class="status" id="status2">OFF</div>
        <label class="switch">
            <input type="checkbox" class="toggleButton" id="toggleButton2">
            <span class="slider round"></span>
        </label>
        <div class="status" id="status3">OFF</div>
        <label class="switch">
            <input type="checkbox" class="toggleButton" id="toggleButton3">
            <span class="slider round"></span>
        </label>
        <div class="status" id="status4">OFF</div>
        <label class="switch">
            <input type="checkbox" class="toggleButton" id="toggleButton4">
            <span class="slider round"></span>
        </label>
    </div>
    <script>
        var ws = new WebSocket('ws://' + window.location.hostname + ':81/');
        var buttons = document.querySelectorAll('.toggleButton');
        var statuses = [document.getElementById('status1'), document.getElementById('status2'), document.getElementById('status3'), document.getElementById('status4')];
        
        buttons.forEach((button, index) => {
            button.addEventListener('change', function() {
                var state = button.checked ? '1' : '0';
                ws.send(state + index); // Send the state along with the LED index
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

)rawliteral";

void setup() {
  for (int i = 0; i < numLEDs; i++) {
    pinMode(ledPins[i], OUTPUT);
  }
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println("IP Address: " + WiFi.localIP().toString());

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  server.begin();
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    int index = payload[length - 1] - '0'; // Convert char to int
    if (index >= 0 && index < numLEDs) {
      if (payload[0] == '1') {
        digitalWrite(ledPins[index], HIGH);
      } else {
        digitalWrite(ledPins[index], LOW);
      }
    }
  }
}

void loop() {
  webSocket.loop();
}
