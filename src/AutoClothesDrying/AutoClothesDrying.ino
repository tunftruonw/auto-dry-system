#include <ArduinoJson.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <Servo.h>
#include <WiFiClientSecure.h>

// --- WIFI CONFIGURATION ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* Gemini_Token = "YOUR_GEMINI_API_KEY";
const char* Gemini_Max_Tokens = "100";
String res = "";

// --- PIN DEFINITIONS ---
#define DHTPIN D6
#define LIGHT_PIN D5  // Optional: Light Sensor (Analog)
#define DHTTYPE DHT11
#define RAIN_PIN A0    // Rain Sensor (Analog)
#define BUTTON_PIN D7  // Manual Push Button
#define SERVO_PIN D8   // Servo Motor Control Pin

// --- OBJECT INITIALIZATION ---
DHT dht(DHTPIN, DHTTYPE);
Servo myServo;
ESP8266WebServer server(80);
int servoPos = 0;  // Current servo position (0: Indoor, 180: Outdoor)

// --- GLOBAL VARIABLES ---
bool isOutdoor = false;  // Rack status (false: Indoor, true: Outdoor)
bool isRaining = false;
float temp, hum;
int rainVal;
int lightVal;
unsigned long lastUpdate = 0;
bool isButtonPressed = false;  // Flag to check if button is pressed

// --- CONTROL FUNCTIONS ---
void moveLaundry(bool outdoor) {
  if (outdoor) {
    myServo.write(180);  // Rotate to outdoor position
    isOutdoor = true;
    Serial.println("Action: Moving OUTDOOR");
  } else {
    myServo.write(0);  // Rotate to indoor position
    isOutdoor = false;
    Serial.println("Action: Moving INDOOR");
  }
}

// --- WEB INTERFACE (HTML) ---
void handleRoot() {
  String html = R"raw(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Smart Laundry</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; font-family: 'Segoe UI', sans-serif; }
        body { background: radial-gradient(circle at top, #0b1a33, #020b1a); color: white; display: flex; justify-content: center; padding: 40px 15px; min-height: 100vh; }
        .container { width: 420px; max-width: 100%; }
        .badge { text-align: center; color: #2dd4bf; border: 1px solid rgba(255, 255, 255, 0.1); padding: 8px 18px; border-radius: 20px; display: block; width: max-content; margin: auto; font-size: 13px; letter-spacing: 1px; }
        h1 { text-align: center; margin-top: 20px; font-size: 36px; }
        .subtitle { text-align: center; color: #94a3b8; margin-bottom: 35px; }
        .cards { display: grid; grid-template-columns: repeat(3, 1fr); gap: 15px; }
        .card { padding: 20px; border-radius: 18px; text-align: center; backdrop-filter: blur(10px); border: 1px solid rgba(255, 255, 255, 0.08); }
        .temp { background: linear-gradient(135deg, #5b2c06, #1e293b); }
        .hum { background: linear-gradient(135deg, #0f2a44, #1e293b); }
        .rain { background: linear-gradient(135deg, #4b3a06, #1e293b); }
        .icon { font-size: 26px; margin-bottom: 8px; }
        .value { font-size: 18px; font-weight: 600; }
        .label { font-size: 10px; color: #94a3b8; letter-spacing: 1px; margin-top: 3px; }
        .status { margin-top: 25px; padding: 25px; border-radius: 18px; background: linear-gradient(135deg, #1f2937, #0f172a); border: 1px solid rgba(255, 255, 255, 0.08); text-align: center; }
        .status small { color: #94a3b8; letter-spacing: 1px; }
        .status h2 { margin-top: 8px; font-size: 20px; }
        .buttons { display: grid; grid-template-columns: 1fr 1fr; gap: 18px; margin-top: 25px; }
        button { padding: 20px; border-radius: 18px; font-size: 16px; border: none; cursor: pointer; color: white; font-weight: bold; transition: 0.3s; }
        button:active { transform: scale(0.95); }
        .open { background: linear-gradient(135deg, #34d399, #10b981); box-shadow: 0 0 20px rgba(16, 185, 129, 0.3); }
        .close { background: linear-gradient(135deg, #f87171, #ef4444); box-shadow: 0 0 20px rgba(239, 68, 68, 0.3); }
    </style>
</head>
<body>
    <div class="container">
        <div class="badge">LIVE MONITORING</div>
        <h1>Smart Laundry</h1>
        <p class="subtitle">Automated drying system</p>
        <div class="cards">
            <div class="card temp">
                <div class="icon">&#127777;</div>
                <div class="value"><span id="tempVal">--</span>&deg;C</div>
                <div class="label">TEMPERATURE</div>
            </div>
            <div class="card hum">
                <div class="icon">&#128167;</div>
                <div class="value"><span id="humVal">--</span>%</div>
                <div class="label">HUMIDITY</div>
            </div>
            <div class="card rain">
                <div id="rainIcon" class="icon">&#127774;</div>
                <div id="rainVal" class="value">--</div>
                <div class="label">WEATHER</div>
            </div>
        </div>

        <div class="status">
            <small>CURRENT POSITION</small>
            <h2 id="posText">--</h2>
        </div>

        <div class="buttons">
            <button class="open" onclick="controlLaundry('open')">Dry Clothes</button>
            <button class="close" onclick="controlLaundry('close')">Retract</button>
        </div>
    </div>

    <script>
        function controlLaundry(action) {
            fetch('/' + action)
                .then(response => {
                    if(response.ok) {
                        console.log("Command " + action + " sent");
                        setTimeout(updateData, 500); // Update data shortly after command
                    }
                })
                .catch(err => console.error('Command error:', err));
        }

        function updateData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById("tempVal").innerText = data.temp;
                    document.getElementById("humVal").innerText = data.hum;
                    document.getElementById("rainVal").innerText = data.rain;
                    document.getElementById("posText").innerText = data.pos;
                    
                    const rainIcon = document.getElementById("rainIcon");
                    if(data.rain === "Raining") {
                        rainIcon.innerHTML = "&#127783;";
                    } else {
                        rainIcon.innerHTML = "&#127774;";
                    }
                })
                .catch(err => console.error('Fetch error:', err));
        }

        updateData();
        setInterval(updateData, 2000);
    </script>
</body>
</html>
)raw";
  server.send(200, "text/html", html);
}

void IRAM_ATTR buttonHandle() {
  isButtonPressed = true;
  Serial.println("Button pressed");
}

void handleData() {
  String json = "{";
  json += "\"temp\":" + String(temp, 1) + ",";
  json += "\"hum\":" + String(hum, 1) + ",";
  json += "\"rain\":\"" + String(isRaining ? "Raining" : "Clear") + "\",";
  json += "\"pos\":\"" + String(isOutdoor ? "Outdoor - Drying" : "Indoor - Retracted") + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  myServo.attach(SERVO_PIN);
  myServo.write(servoPos);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RAIN_PIN, INPUT);
  pinMode(LIGHT_PIN, INPUT);

  attachInterrupt(BUTTON_PIN, buttonHandle, FALLING);

  // Default starting position
  moveLaundry(false);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP Address: " + WiFi.localIP().toString());

  if (MDNS.begin("auto-clothes-drying")) {
    Serial.println("mDNS started! Visit: http://auto-clothes-drying.local");
  }

  MDNS.addService("http", "tcp", 80);

  server.on("/", handleRoot);

  server.on("/data", handleData);
  server.on("/open", []() {
    moveLaundry(true);
    server.send(200, "text/plain", "OK");  // Just send "OK" instead of redirecting
  });

  server.on("/close", []() {
    moveLaundry(false);
    server.send(200, "text/plain", "OK");  // Just send "OK" instead of redirecting
  });

  server.begin();
}

void loop() {
  if (isButtonPressed) {
    handleButtonPress();
    isButtonPressed = false;
    delay(10);
  }

  server.handleClient();  // Handle web requests

  // Sensor update and Auto-logic every 20 seconds
  if (millis() - lastUpdate > 20000) {
    temp = dht.readTemperature() - 8;
    hum = dht.readHumidity();
    rainVal = analogRead(RAIN_PIN);
    lightVal = digitalRead(LIGHT_PIN);
    lastUpdate = millis();

    Serial.println("Rain Value: " + String(rainVal));
    Serial.println("Light Value: " + String(lightVal));

    // --- AUTOMATIC LOGIC ---
    // If rain detected or high humidity -> Auto retract
    if (rainVal < 900) {
      if (isOutdoor) {
        if (!isRaining) {
          if (callGemini(rainVal, temp, hum, lightVal)) {
            moveLaundry(false);
            isRaining = true;
            Serial.println("Rain detected! Gemini confirmed: Moving indoor.");
          } else {
            Serial.println("Weather looks suspicious, but Gemini says it's okay.");
          }
        }
      }
    } else {
      if (isRaining) {
        moveLaundry(true);
        isRaining = false;
        Serial.println("Weather is clear now. Moving laundry back outdoor.");
      }
    }
  }
  MDNS.update();
}

void handleButtonPress() {
  // If the button is pressed, open the medicine box and send a notification
  if (!isOutdoor) {
    moveLaundry(true);
  } else {
    moveLaundry(false);
  }
}

bool callGemini(int rainVal, float temp, float hum, int lightVal) {
  bool status = false;

  Serial.println("Gemini on duty!, with a rain sensor value of " + String(rainVal) + ", temperature of " + String(temp) + "°C, humidity of " + String(hum) + "%, and light sensor value of " + String(lightVal));

  res = "Given the current weather conditions with a rain sensor value of " + String(rainVal) + ", temperature of " + String(temp) + "°C, humidity of " + String(hum) + "%, and light sensor value of " + String(lightVal) + " should I move the laundry rack outdoor for drying? Please answer with a simple 'Yes' or 'No'.";
  //  and compare with the realtime weather data in Da Nang, Viet Nam,
  int len = res.length();
  res = res.substring(0, (len - 1));
  res = "\"" + res + "\"";

  WiFiClientSecure client;
  client.setInsecure();  // Use for testing purposes only
  HTTPClient http;

  if (http.begin(client, "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=" + (String)Gemini_Token)) {  // HTTPS
    http.addHeader("Content-Type", "application/json");
    String payload = String("{\"contents\": [{\"parts\":[{\"text\":" + res + "}]}],\"generationConfig\": {\"maxOutputTokens\": " + (String)Gemini_Max_Tokens + "}}");

    int httpCode = http.POST(payload);

    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      String payload = http.getString();

      DynamicJsonDocument doc(2048);
      deserializeJson(doc, payload);
      String Answer = doc["candidates"][0]["content"]["parts"][0]["text"];

      // For Filtering out Special Characters, WhiteSpaces and NewLine Characters
      Answer.trim();
      String filteredAnswer = "";
      for (size_t i = 0; i < Answer.length(); i++) {
        char c = Answer[i];
        if (isalnum(c) || isspace(c)) {
          filteredAnswer += c;
        } else {
          filteredAnswer += ' ';
        }
      }
      Answer = filteredAnswer;

      String lowerAnswer = Answer;
      lowerAnswer.toLowerCase();

      if (lowerAnswer.indexOf("yes") >= 0) {
        status = false;
      } else if (lowerAnswer.indexOf("no") >= 0) {
        status = true;
      }

      Serial.println(Answer);
    } else {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
  res = "";
  return status;
}