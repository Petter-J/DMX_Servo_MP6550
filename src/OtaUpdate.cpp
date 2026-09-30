#include "OtaUpdate.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>

// --------------------------------------------------
// OTA
// --------------------------------------------------

static WebServer otaServer(80);

static bool gOtaActive = false;
static bool restartPending = false;
static uint32_t restartAtMs = 0;

static const char *OTA_SSID = "DMX_Servo_MP6550_OTA";
static const char *OTA_PASS = "dmxservo123";

// --------------------------------------------------
// WEBBSIDA
// --------------------------------------------------

static const char OTA_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="sv">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>DMX Servo MP6550 OTA</title>

<style>
* { box-sizing: border-box; }

body {
    margin: 0;
    padding: 20px;
    font-family: Arial, Helvetica, sans-serif;
    background: #111827;
    color: #f9fafb;
}

.card {
    max-width: 520px;
    margin: 40px auto;
    padding: 28px;
    background: #1f2937;
    border-radius: 18px;
    box-shadow: 0 15px 40px rgba(0,0,0,0.35);
}

h1 {
    margin-top: 0;
    margin-bottom: 5px;
    font-size: 28px;
}

.subtitle {
    color: #9ca3af;
    margin-bottom: 25px;
}

.status {
    background: #111827;
    padding: 14px;
    border-radius: 10px;
    margin-bottom: 20px;
    line-height: 1.6;
}

.ok {
    color: #86efac;
    font-weight: bold;
}

input[type=file] {
    width: 100%;
    padding: 14px;
    margin-bottom: 18px;
    background: #374151;
    color: white;
    border: 1px solid #4b5563;
    border-radius: 10px;
}

button {
    width: 100%;
    padding: 14px;
    font-size: 17px;
    font-weight: bold;
    border: none;
    border-radius: 10px;
    cursor: pointer;
    background: #2563eb;
    color: white;
}

button:disabled { opacity: 0.5; }

.progressBox {
    width: 100%;
    height: 20px;
    margin-top: 22px;
    background: #374151;
    border-radius: 10px;
    overflow: hidden;
}

.progress {
    width: 0%;
    height: 100%;
    background: #22c55e;
}

#percent {
    text-align: center;
    margin-top: 8px;
    font-weight: bold;
}

#message {
    min-height: 24px;
    margin-top: 18px;
    text-align: center;
}

.footer {
    margin-top: 25px;
    text-align: center;
    font-size: 12px;
    color: #6b7280;
}
</style>
</head>

<body>
<div class="card">

<h1>DMX Servo MP6550</h1>
<div class="subtitle">Firmware Update</div>

<div class="status">
OTA mode: <span class="ok">ACTIVE</span><br>
Motor output: <span class="ok">DISABLED</span><br>
WiFi: DMX_Servo_MP6550_OTA<br>
IP: 192.168.4.1
</div>

<input type="file" id="firmware" accept=".bin">
<button id="uploadButton" onclick="uploadFirmware()">Upload firmware</button>

<div class="progressBox">
    <div id="progress" class="progress"></div>
</div>

<div id="percent">0%</div>
<div id="message">Select firmware.bin</div>
<div class="footer">ESP32 OTA Update</div>

</div>

<script>
function uploadFirmware()
{
    const fileInput = document.getElementById("firmware");
    const button = document.getElementById("uploadButton");
    const progress = document.getElementById("progress");
    const percent = document.getElementById("percent");
    const message = document.getElementById("message");

    if (fileInput.files.length === 0)
    {
        message.innerHTML = "Select a .bin file first";
        return;
    }

    const file = fileInput.files[0];
    const formData = new FormData();
    formData.append("update", file);

    const xhr = new XMLHttpRequest();

    button.disabled = true;
    message.innerHTML = "Uploading...";

    xhr.upload.onprogress = function(event)
    {
        if (event.lengthComputable)
        {
            const value = Math.round(event.loaded / event.total * 100);

            progress.style.width = value + "%";
            percent.innerHTML = value + "%";
        }
    };

    xhr.onload = function()
    {
        if (xhr.status === 200)
        {
            progress.style.width = "100%";
            percent.innerHTML = "100%";
            message.innerHTML = "Update complete - restarting...";
        }
        else
        {
            message.innerHTML = "Update failed";
            button.disabled = false;
        }
    };

    xhr.onerror = function()
    {
        message.innerHTML = "Connection error";
        button.disabled = false;
    };

    xhr.open("POST", "/update", true);
    xhr.send(formData);
}
</script>

</body>
</html>
)rawliteral";

// --------------------------------------------------
// START OTA
// --------------------------------------------------

void ota_begin()
{
    if (gOtaActive)
        return;

    WiFi.mode(WIFI_AP_STA);
    delay(200);

    if (!WiFi.softAP(OTA_SSID, OTA_PASS))
        return;

    // HUVUDSIDA
    otaServer.on("/", HTTP_GET, []()
                 { otaServer.send_P(200, "text/html", OTA_PAGE); });

    // UPDATE
    otaServer.on(
        "/update",
        HTTP_POST,
        []()
        {
            if (Update.hasError())
            {
                otaServer.send(500, "text/plain", "Update failed");
                return;
            }

            otaServer.send(200, "text/plain", "Update OK");

            restartPending = true;
            restartAtMs = millis() + 1500;
        },
        []()
        {
            HTTPUpload &upload = otaServer.upload();

            if (upload.status == UPLOAD_FILE_START)
                Update.begin(UPDATE_SIZE_UNKNOWN);

            else if (upload.status == UPLOAD_FILE_WRITE)
                Update.write(upload.buf, upload.currentSize);

            else if (upload.status == UPLOAD_FILE_END)
                Update.end(true);

            else if (upload.status == UPLOAD_FILE_ABORTED)
                Update.end();
        });

    otaServer.begin();
    gOtaActive = true;
}

// --------------------------------------------------
// HANDLE
// --------------------------------------------------

void ota_handle()
{
    if (!gOtaActive)
        return;

    otaServer.handleClient();

    if (restartPending && (int32_t)(millis() - restartAtMs) >= 0)
    {
        delay(100);
        ESP.restart();
    }
}

// --------------------------------------------------
// STATUS
// --------------------------------------------------

bool ota_is_active()
{
    return gOtaActive;
}