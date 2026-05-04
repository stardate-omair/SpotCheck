#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>
#include <time.h>

#define TRIG_PIN 5
#define ECHO_PIN 18
#define PIR_PIN 19
#define SERVO_PIN 13
#define GREEN_LED 25
#define RED_LED 26
#define BUZZER_PIN 23

const char* ssid = "NAWAZ";
const char* password = "Ansha@22";

#define CLOSED_HOUR_START 22
#define CLOSED_HOUR_END 6

Servo barrier;
LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);

// State
bool slotOccupied = false;
bool intruderAlert = false;
String lastEvent = "None";
String lastEventTime = "--:--:--";
int entryCount = 0;
int intrusionCount = 0;
std::vector<String> eventLog;

long readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  return duration * 0.034 / 2;
}

bool isClosedHours() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return false;
  int hour = timeinfo.tm_hour;
  return (hour >= CLOSED_HOUR_START || hour < CLOSED_HOUR_END);
}

String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "N/A";
  char buf[20];
  strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
  return String(buf);
}

void logEvent(String event) {
  String ts = getTimestamp();
  String entry = "[" + ts + "] " + event;
  Serial.println(entry);
  eventLog.push_back(entry);
  if (eventLog.size() > 20) eventLog.erase(eventLog.begin());
  lastEvent = event;
  lastEventTime = ts;
}

void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <meta http-equiv='refresh' content='2'>
  <title>SpotCheck Dashboard</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { font-family: 'Segoe UI', sans-serif; background: #0f172a; color: #e2e8f0; min-height: 100vh; padding: 20px; }
    h1 { text-align: center; font-size: 1.5em; margin-bottom: 20px; color: #38bdf8; letter-spacing: 2px; }
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; max-width: 600px; margin: 0 auto; }
    .card { background: #1e293b; border-radius: 12px; padding: 20px; text-align: center; }
    .card h2 { font-size: 0.8em; color: #94a3b8; text-transform: uppercase; letter-spacing: 1px; margin-bottom: 10px; }
    .status { font-size: 1.4em; font-weight: bold; }
    .occupied { color: #f87171; }
    .free { color: #4ade80; }
    .alert { color: #fb923c; }
    .safe { color: #4ade80; }
    .count { font-size: 2em; font-weight: bold; color: #38bdf8; }
    .log { grid-column: 1 / -1; text-align: left; }
    .log-entry { font-size: 0.8em; color: #94a3b8; padding: 4px 0; border-bottom: 1px solid #334155; }
    .footer { text-align: center; margin-top: 20px; font-size: 0.7em; color: #475569; }
  </style>
</head>
<body>
  <h1>SpotCheck</h1>
  <div class='grid'>
    <div class='card'>
      <h2>Slot Status</h2>
      <div class='status )" + String(slotOccupied ? "occupied'>OCCUPIED" : "free'>FREE") + R"(</div>
    </div>
    <div class='card'>
      <h2>Intrusion</h2>
      <div class='status )" + String(intruderAlert ? "alert'>ALERT 🚨" : "safe'>Clear ✅") + R"(</div>
    </div>
    <div class='card'>
      <h2>Total Entries</h2>
      <div class='count'>)" + String(entryCount) + R"(</div>
    </div>
    <div class='card'>
      <h2>Intrusions</h2>
      <div class='count'>)" + String(intrusionCount) + R"(</div>
    </div>
    <div class='card log'>
      <h2>Event Log</h2>)";

  for (int i = eventLog.size() - 1; i >= 0; i--) {
    html += "<div class='log-entry'>" + eventLog[i] + "</div>";
  }

  html += R"(
    </div>
  </div>
  <div class='footer'>Auto-refreshes every 2 seconds · SpotCheck v3.0</div>
</body>
</html>)";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  barrier.attach(SERVO_PIN);
  barrier.write(0);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi ");
  lcd.setCursor(0, 1);
  lcd.print("Please wait...  ");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("Dashboard: http://");
  Serial.println(WiFi.localIP());

  configTime(3 * 3600, 0, "pool.ntp.org");

  server.on("/", handleRoot);
  server.begin();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SpotCheck v3.0");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());

  Serial.println("Calibrating PIR, wait 30 seconds...");
  delay(30000);
  lcd.clear();
  logEvent("System started");
}

bool wasOccupied = false;
bool wasMotion = false;

void loop() {
  server.handleClient();

  long distance = readDistance();
  int motion = digitalRead(PIR_PIN);
  slotOccupied = distance < 15;
  bool closed = isClosedHours();

  if (slotOccupied && !wasOccupied) { logEvent("Vehicle ENTERED slot"); entryCount++; }
  if (!slotOccupied && wasOccupied) logEvent("Vehicle EXITED slot");
  wasOccupied = slotOccupied;

  barrier.write(slotOccupied ? 0 : 90);
  digitalWrite(GREEN_LED, !slotOccupied);
  digitalWrite(RED_LED, slotOccupied);

  lcd.setCursor(0, 0);
  lcd.print(slotOccupied ? "Slot: OCCUPIED  " : "Slot: FREE      ");
  lcd.setCursor(0, 1);

  if (motion && closed) {
    if (!wasMotion) { logEvent("INTRUSION DETECTED"); intrusionCount++; }
    intruderAlert = true;
    lcd.print("INTRUDER ALERT! ");
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
  } else {
    intruderAlert = false;
    lcd.print(closed ? "Closed Hours    " : "PIR: Clear      ");
    digitalWrite(BUZZER_PIN, LOW);
  }

  wasMotion = motion;
  delay(500);
}