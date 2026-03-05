#include "web_server.h"
#include <WiFi.h>
#include <WebServer.h>
#include "esp_log.h"
#include "lovense.h"

static const char* TAG = "web_server";

static const char* WIFI_SSID = "DefCon2";
static const char* WIFI_PASSWORD = "1911lufthansa1607";

static WebServer server(80);

static const char APP_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>LVS-Gateway Controller</title>
  <style>
    :root {
      --bg:#1a1a2e; --card:#16213e; --accent:#e94560;
      --accent-glow:rgba(233,69,96,0.4); --text:#eaeaea;
      --text-dim:#8a8a9a; --success:#4ecca3; --danger:#e94560;
    }
    *{margin:0;padding:0;box-sizing:border-box}
    body{
      font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;
      background:var(--bg);color:var(--text);
      min-height:100vh;display:flex;align-items:center;
      justify-content:center;padding:20px;
    }
    .container{
      background:var(--card);border-radius:24px;
      padding:40px 32px;width:100%;max-width:400px;
      box-shadow:0 20px 60px rgba(0,0,0,0.3);
    }
    h1{text-align:center;font-size:1.5rem;font-weight:600;margin-bottom:8px}
    .subtitle{text-align:center;color:var(--text-dim);font-size:0.85rem;margin-bottom:32px}
    .status-bar{
      display:flex;align-items:center;justify-content:center;gap:10px;
      margin-bottom:28px;padding:12px;
      background:rgba(78,204,163,0.1);border:1px solid rgba(78,204,163,0.3);
      border-radius:12px;
    }
    .status-dot{width:10px;height:10px;border-radius:50%;background:var(--success);box-shadow:0 0 8px rgba(78,204,163,0.5)}
    .status-text{font-size:0.85rem;color:var(--success)}
    .value-display{text-align:center;margin-bottom:24px}
    .value-number{
      font-size:4rem;font-weight:700;line-height:1;
      background:linear-gradient(135deg,var(--accent),#ff6b8a);
      -webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;
    }
    .value-label{font-size:0.8rem;color:var(--text-dim);margin-top:4px;letter-spacing:1px;text-transform:uppercase}
    .speed-display{text-align:center;font-size:0.9rem;color:var(--text-dim);margin-bottom:28px}
    .speed-display span{color:var(--text);font-weight:600}
    .slider-wrapper{padding:0 4px;margin-bottom:16px}
    input[type="range"]{
      -webkit-appearance:none;appearance:none;width:100%;height:8px;
      border-radius:4px;background:rgba(255,255,255,0.1);outline:none;
    }
    input[type="range"]::-webkit-slider-thumb{
      -webkit-appearance:none;appearance:none;width:32px;height:32px;
      border-radius:50%;background:var(--accent);cursor:pointer;
      box-shadow:0 4px 12px var(--accent-glow);
    }
    input[type="range"]::-moz-range-thumb{
      width:32px;height:32px;border-radius:50%;background:var(--accent);
      cursor:pointer;border:none;box-shadow:0 4px 12px var(--accent-glow);
    }
    .slider-labels{display:flex;justify-content:space-between;padding:0 4px;margin-bottom:24px}
    .slider-labels span{font-size:0.75rem;color:var(--text-dim)}
    .btn-stop{
      display:block;width:100%;padding:14px;
      background:rgba(233,69,96,0.15);color:var(--accent);
      border:1px solid rgba(233,69,96,0.3);border-radius:12px;
      font-size:0.95rem;font-weight:600;cursor:pointer;
    }
    .btn-stop:hover{background:rgba(233,69,96,0.25)}
    .log-section{margin-top:24px;padding-top:20px;border-top:1px solid rgba(255,255,255,0.06)}
    .log-header{font-size:0.75rem;color:var(--text-dim);text-transform:uppercase;letter-spacing:1px;margin-bottom:8px}
    .log{
      background:rgba(0,0,0,0.3);border-radius:10px;padding:12px;
      max-height:120px;overflow-y:auto;
      font-family:'SF Mono','Fira Code',monospace;font-size:0.75rem;
      line-height:1.6;color:var(--text-dim);
    }
    .log .sent{color:var(--accent)}
    .log .ok{color:var(--success)}
    .log .error{color:#ff6b6b}
  </style>
</head>
<body>
  <div class="container">
    <h1>LVS-Gateway</h1>
    <p class="subtitle">Vibration Controller</p>
    <div class="status-bar">
      <div class="status-dot"></div>
      <span class="status-text">Direkt verbunden via WiFi</span>
    </div>
    <div class="value-display">
      <div class="value-number" id="valueDisplay">0</div>
      <div class="value-label">Intensitaet</div>
    </div>
    <div class="speed-display">Vibration Speed: <span id="speedDisplay">0.00</span></div>
    <div class="slider-wrapper">
      <input type="range" id="slider" min="0" max="3" value="0" step="1" oninput="onSliderChange(this.value)">
    </div>
    <div class="slider-labels"><span>Aus</span><span>0.3</span><span>0.5</span><span>0.8</span></div>
    <button class="btn-stop" onclick="sendStop()">Stopp</button>
    <div class="log-section">
      <div class="log-header">Protokoll</div>
      <div class="log" id="log"></div>
    </div>
  </div>
  <script>
    var sendTimeout=null;
    var speedMap=[0,0.3,0.5,0.8];
    var vibrateMap=[0,3,5,8];
    var labelMap=['Aus','Niedrig','Mittel','Hoch'];
    function onSliderChange(value){
      var v=parseInt(value);
      var speed=speedMap[v];
      document.getElementById('valueDisplay').textContent=labelMap[v];
      document.getElementById('speedDisplay').textContent=speed.toFixed(1);
      clearTimeout(sendTimeout);
      sendTimeout=setTimeout(function(){sendValue(v);},100);
    }
    function sendStop(){
      document.getElementById('slider').value=0;
      document.getElementById('valueDisplay').textContent='Aus';
      document.getElementById('speedDisplay').textContent='0.0';
      sendValue(0);
    }
    function sendValue(level){
      var vibVal=vibrateMap[level];
      var xhr=new XMLHttpRequest();
      xhr.open('GET','/api/vibrate?v='+vibVal,true);
      xhr.onload=function(){
        if(xhr.status===200){
          addLog('Gesendet: '+labelMap[level]+' (Speed '+speedMap[level].toFixed(1)+')','ok');
        }else{
          addLog('Fehler: HTTP '+xhr.status,'error');
        }
      };
      xhr.onerror=function(){addLog('Verbindungsfehler','error');};
      xhr.send();
    }
    function addLog(message,type){
      type=type||'';
      var logEl=document.getElementById('log');
      var line=document.createElement('div');
      line.className=type;
      var time=new Date().toLocaleTimeString('de-DE');
      line.textContent='['+time+'] '+message;
      logEl.appendChild(line);
      logEl.scrollTop=logEl.scrollHeight;
    }
    addLog('Bereit - Slider bewegen zum Steuern','ok');
  </script>
</body>
</html>)rawliteral";

// Forward declaration
extern std::string generate_response(const std::string& command);

void handleRoot() {
  server.send_P(200, "text/html", APP_HTML);
}

void handleVibrate() {
  if (!server.hasArg("v")) {
    server.send(400, "text/plain", "Missing parameter v");
    return;
  }

  int value = server.arg("v").toInt();
  if (value < 0) value = 0;
  if (value > 10) value = 10;

  std::string command;
  if (value == 0) {
    command = "PowerOff;";
  } else {
    command = "Vibrate:" + std::to_string(value) + ";";
  }

  std::string response = generate_response(command);
  Serial.printf("[web] %s -> %s\n", command.c_str(), response.c_str());

  server.send(200, "text/plain", "OK");
}

void web_server_init() {
  ESP_LOGI(TAG, "Connecting to WiFi: %s", WIFI_SSID);
  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    ESP_LOGI(TAG, "WiFi connected! IP: %s", WiFi.localIP().toString().c_str());
    Serial.printf("WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.println("Open in browser: http://" + WiFi.localIP().toString());
  } else {
    ESP_LOGE(TAG, "WiFi connection failed!");
    Serial.println("\nWiFi connection failed!");
    return;
  }

  server.on("/", handleRoot);
  server.on("/api/vibrate", handleVibrate);
  server.begin();
  ESP_LOGI(TAG, "Web server started on port 80");
  Serial.println("Web server started on port 80");
}

void web_server_loop() {
  server.handleClient();
}
