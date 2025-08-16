#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WiFi AP Credentials
const char* ap_ssid = "StoreRoomMonitor";
const char* ap_pass = "storeroom123";

// Pin Definitions (unchanged)
#define DHTPIN 2
#define PIR_PIN 27
#define MQ135_PIN 35
#define MQ2_PIN 34
#define RELAY1_PIN 17 // Fan1 - Manual (Web button, active-low)
#define RELAY2_PIN 5  // Fan2 - Auto (Motion, active-low)
#define LED1_PIN 4
#define LED2_PIN 16
#define MOTOR_PWM_PIN 18    // ENA pin of L298N
#define MOTOR_IN1_PIN 19    // IN1 of L298N
#define MOTOR_IN2_PIN 21    // IN2 of L298N

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const int MQ135_CLEAN_AIR = 100;
const int MQ2_CLEAN_AIR = 150;

// State Variables
bool fan1State = false;
bool ledState = false;
int fan3Speed = 0;
bool motionDetected = false;

// Timing Variables
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 2000; // 2 seconds

// Sensor Readings
float temperature = 0.0;
float humidity = 0.0;
int airQuality = 0;
int gasLevel = 0;

// Web Server
WebServer server(80);

// Motor control by temperature using analogWrite PWM
void controlMotorByTemp(float temperature) {
  if (isnan(temperature)) {
    analogWrite(MOTOR_PWM_PIN, 0);
    digitalWrite(MOTOR_IN1_PIN, LOW);
    digitalWrite(MOTOR_IN2_PIN, LOW);
    fan3Speed = 0;
    return;
  }
  if (temperature < 32.0) {
    analogWrite(MOTOR_PWM_PIN, 0);
    digitalWrite(MOTOR_IN1_PIN, LOW);
    digitalWrite(MOTOR_IN2_PIN, LOW);
    fan3Speed = 0;
  } else {
    digitalWrite(MOTOR_IN1_PIN, HIGH);
    digitalWrite(MOTOR_IN2_PIN, LOW);
    int motorSpeed = (temperature >= 40.0) ? 255 : map((int)temperature, 32, 40, 50, 255);
    motorSpeed = constrain(motorSpeed, 50, 255);
    analogWrite(MOTOR_PWM_PIN, motorSpeed);
    fan3Speed = map(motorSpeed, 50, 255, 0, 100);
  }
}

// Modern HTML/CSS + AJAX Hot Reload
String htmlPage() {
  String page = "<!DOCTYPE html><html><head><title>Store Room Monitor</title>";
  page += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  page += R"(
    <style>
      body{margin:0;font-family:'Segoe UI',Arial,sans-serif;background:#f6f8fb;}
      .container{max-width:420px;margin:30px auto 0 auto;padding:20px;background:#fff;border-radius:16px;box-shadow:0 4px 24px rgba(0,0,0,0.06);}
      h2{font-weight:700;text-align:center;}
      .grid{display:grid;grid-template-columns:1fr 1fr;gap:14px;margin:22px 0;}
      .card{background:#f1f3f6;border-radius:10px;padding:18px 12px;text-align:center;box-shadow:0 2px 8px rgba(0,0,0,0.05);}
      .label{font-size:1rem;color:#888;margin-bottom:6px;}
      .value{font-size:1.22rem;font-weight:600;}
      .status{display:inline-block;padding:3px 14px;border-radius:12px;font-size:0.97rem;font-weight:500;}
      .on{background:#bae6ba;color:#237c35;}
      .off{background:#f9caca;color:#b71c1c;}
      .button-row{display:flex;justify-content:center;gap:22px;margin:20px 0;}
      .btn{display:inline-flex;align-items:center;justify-content:center;background:#e1e8ef;color:#2e4960;border:none;border-radius:12px;font-size:1rem;padding:10px 30px;font-weight:600;box-shadow:0 1px 3px #d1d7df;cursor:pointer;transition:background .17s;}
      .btn:active{background:#d1d7df;}
      .btn.fan{color:#0069d6;}
      .btn.led{color:#e8a700;}
      .icon{margin-right:8px;font-size:1.2em;}
      @media(max-width:520px){.container{max-width:98vw;padding:10px;}.card{padding:12px 4px;}.btn{padding:10px 14px;font-size:0.94rem;}}
    </style>
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.3/font/bootstrap-icons.css">
    <script>
      function updateData(){
        fetch('/data').then(r=>r.json()).then(d=>{
          document.getElementById('temp').innerHTML = d.temperature + " &deg;C";
          document.getElementById('hum').innerHTML = d.humidity + " %";
          document.getElementById('air').innerHTML = d.airQuality + " %";
          document.getElementById('gas').innerHTML = d.gasLevel + " %";
          document.getElementById('motion').className = "status " + (d.motionDetected ? "on":"off");
          document.getElementById('motion').innerHTML = d.motionDetected ? "Detected":"None";
          document.getElementById('fan2').className = "status " + (d.motionDetected ? "on":"off");
          document.getElementById('fan2').innerHTML = d.motionDetected ? "ON":"OFF";
          document.getElementById('fan3speed').innerHTML = d.fan3Speed + " %";
          document.getElementById('led').className = "status " + (d.ledState ? "on":"off");
          document.getElementById('led').innerHTML = d.ledState ? "ON":"OFF";
          document.getElementById('manualfanstat').className = "status " + (d.fan1State ? "on":"off");
          document.getElementById('manualfanstat').innerHTML = d.fan1State ? "ON":"OFF";
          document.getElementById('fanbtn').innerHTML = (d.fan1State ? "Turn OFF Fan":"Turn ON Fan");
          document.getElementById('ledbtn').innerHTML = (d.ledState ? "Turn OFF Light":"Turn ON Light");
        });
      }
      setInterval(updateData, 2000); // Every 2s
      window.onload=updateData;
    </script>
  )";
  page += "</head><body><div class='container'>";
  page += "<h2>Store Room Monitor</h2>";

  page += "<div class='grid'>";
  page += "<div class='card'><span class='label'>Temperature</span><div class='value' id='temp'>...</div></div>";
  page += "<div class='card'><span class='label'>Humidity</span><div class='value' id='hum'>...</div></div>";
  page += "<div class='card'><span class='label'>Air Quality</span><div class='value' id='air'>...</div></div>";
  page += "<div class='card'><span class='label'>Gas Level</span><div class='value' id='gas'>...</div></div>";
  page += "<div class='card'><span class='label'>Motion</span><div id='motion' class='status'>...</div></div>";
  page += "<div class='card'><span class='label'>Fan2 (Auto)</span><div id='fan2' class='status'>...</div></div>";
  page += "<div class='card'><span class='label'>Fan3 Speed</span><div class='value' id='fan3speed'>...</div></div>";
  page += "<div class='card'><span class='label'>LED</span><div id='led' class='status'>...</div></div>";
  page += "</div>"; // grid

  page += "<div class='button-row'>";
  page += "<form action='/fan' method='POST'><button class='btn fan' type='submit'><span class='icon'><i class='bi bi-fan'></i></span><span id='fanbtn'>Turn ON Fan</span></button></form>";
  page += "<form action='/led' method='POST'><button class='btn led' type='submit'><span class='icon'><i class='bi bi-lightbulb'></i></span><span id='ledbtn'>Turn ON Light</span></button></form>";
  page += "</div>";

  page += "<div style='text-align:center;font-size:0.97rem;color:#bbb;margin-top:18px;'>Manual Fan status: <span id='manualfanstat' class='status'>...</span></div>";
  page += "</div></body></html>";
  return page;
}

// Data endpoint for AJAX
String jsonData() {
  String json = "{";
  json += "\"temperature\":" + String(temperature,1) + ",";
  json += "\"humidity\":" + String(humidity,1) + ",";
  json += "\"airQuality\":" + String(airQuality) + ",";
  json += "\"gasLevel\":" + String(gasLevel) + ",";
  json += "\"motionDetected\":" + String(motionDetected ? "true":"false") + ",";
  json += "\"fan1State\":" + String(fan1State ? "true":"false") + ",";
  json += "\"ledState\":" + String(ledState ? "true":"false") + ",";
  json += "\"fan3Speed\":" + String(fan3Speed);
  json += "}";
  return json;
}

void handleRoot() {
  server.send(200, "text/html", htmlPage());
}
void handleFan() {
  fan1State = !fan1State;
  digitalWrite(RELAY1_PIN, fan1State ? LOW : HIGH); // active-low
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}
void handleLed() {
  ledState = !ledState;
  digitalWrite(LED1_PIN, ledState);
  digitalWrite(LED2_PIN, ledState);
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}
void handleData() {
  server.send(200, "application/json", jsonData());
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(MQ135_PIN, INPUT);
  pinMode(MQ2_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(MOTOR_IN1_PIN, OUTPUT);
  pinMode(MOTOR_IN2_PIN, OUTPUT);
  pinMode(MOTOR_PWM_PIN, OUTPUT);

  digitalWrite(MOTOR_IN1_PIN, LOW);
  digitalWrite(MOTOR_IN2_PIN, LOW);
  analogWrite(MOTOR_PWM_PIN, 0);

  digitalWrite(RELAY1_PIN, HIGH); // Fan1 OFF (active-low)
  digitalWrite(RELAY2_PIN, HIGH); // Fan2 OFF (active-low)
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);

  // OLED Init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED init failed!");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("🍟 Store Room Monitor");
  display.display();
  delay(1000);

  // WiFi Access Point Mode
  WiFi.softAP(ap_ssid, ap_pass);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("Access Point IP: ");
  Serial.println(myIP);

  // Web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/fan", HTTP_POST, handleFan);
  server.on("/led", HTTP_POST, handleLed);
  server.on("/data", HTTP_GET, handleData); // AJAX endpoint
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();

  unsigned long now = millis();
  if (now - lastUpdate >= updateInterval) {
    lastUpdate = now;

    // Sensor Readings
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("❌ DHT read failed!");
      temperature = 0.0;
      humidity = 0.0;
    }

    int mq135Value = analogRead(MQ135_PIN);
    airQuality = map(constrain(mq135Value, MQ135_CLEAN_AIR, 4095),
                     MQ135_CLEAN_AIR, 4095, 100, 0) - 35;

    int mq2Value = analogRead(MQ2_PIN);
    gasLevel = map(constrain(mq2Value, MQ2_CLEAN_AIR, 4095),
                   MQ2_CLEAN_AIR, 4095, 0, 100);

    motionDetected = digitalRead(PIR_PIN);

    // Fan2 (Auto by Motion)
    digitalWrite(RELAY2_PIN, motionDetected ? LOW : HIGH);

    // Fan3 (Motor controlled by temperature)
    controlMotorByTemp(temperature);

    // OLED Display All Data
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.printf("Temp: %.1f C\n", temperature);
    display.printf("Hum : %.1f %%\n", humidity);
    display.printf("Air : %d %%\n", airQuality);
    display.printf("Gas : %d %%\n", gasLevel);
    display.printf("Motion: %s\n", motionDetected ? "YES" : "NO");
    display.printf("Fan1 Manual: %s\n", fan1State ? "ON" : "OFF");
    display.printf("Fan2 Auto  : %s\n", motionDetected ? "ON" : "OFF");
    display.printf("Fan3 PWM: %d%%\n", fan3Speed);
    display.printf("LED: %s\n", ledState ? "ON" : "OFF");

    display.display();
  }
}