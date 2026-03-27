#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_log.h"

static const char *TAG = "APP";

int current_bpm = 75;

// ---------------- BPM SIMULATION ----------------
void bpm_task(void *pv)
{
    while (1)
    {
        int change = (rand() % 11) - 5;
        current_bpm += change;

        if (current_bpm > 120) current_bpm = 120;
        if (current_bpm < 50) current_bpm = 50;

        ESP_LOGI(TAG, "BPM: %d", current_bpm);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// ---------------- HTTP HANDLERS ----------------

// MAIN PAGE (NO SPIFFS)
esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");

    const char *html =
    "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>Medico</title>"

    "<style>"
    "body{margin:0;font-family:Inter,Arial;background:#f4f6fb;color:#111;}"

    ".layout{display:grid;grid-template-columns:80px 2fr 1fr;height:100vh;}"

    /* SIDEBAR */
    ".sidebar{background:white;border-right:1px solid #eee;display:flex;flex-direction:column;align-items:center;padding:10px;gap:20px;}"
    ".logo{font-weight:bold;color:#6c5ce7;}"
    ".nav{width:40px;height:40px;border-radius:10px;background:#f0f2ff;display:flex;align-items:center;justify-content:center;}"

    /* MAIN */
    ".main{padding:20px;overflow:auto;}"
    ".card{background:white;border-radius:16px;padding:20px;margin-bottom:20px;box-shadow:0 5px 20px rgba(0,0,0,0.05);}"

    ".bpm{font-size:60px;font-weight:bold;color:#6c5ce7;}"
    ".status{color:#666;margin-top:5px;}"

    "canvas{width:100%;height:200px;background:#020617;border-radius:10px;}"

    ".metrics{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:10px;}"
    ".metric{background:#f8f9ff;padding:10px;border-radius:10px;}"

    /* RIGHT PANEL */
    ".right{padding:20px;background:#fff;border-left:1px solid #eee;overflow:auto;}"

    ".list-item{padding:10px;border-bottom:1px solid #eee;font-size:14px;}"

    "</style></head><body>"

    "<div class='layout'>"

    /* SIDEBAR */
    "<div class='sidebar'>"
    "<div class='logo'>M</div>"
    "<div class='nav'>🏠</div>"
    "<div class='nav'>💓</div>"
    "<div class='nav'>📊</div>"
    "<div class='nav'>⚙️</div>"
    "</div>"

    /* MAIN */
    "<div class='main'>"

    "<div class='card'>"
    "<h2>Overview Of Your Health</h2>"
    "<div class='bpm' id='bpm'>--</div>"
    "<div class='status' id='status'>Checking...</div>"
    "<div id='advice'></div>"
    "</div>"

    "<div class='card'>"
    "<h3>Live ECG</h3>"
    "<canvas id='ecg'></canvas>"
    "</div>"

    "<div class='metrics'>"
    "<div class='metric'>BP: <b id='bp'>--</b></div>"
    "<div class='metric'>Oxygen: <b id='oxy'>--%</b></div>"
    "<div class='metric'>Stress: <b id='stress'>--%</b></div>"
    "<div class='metric'>Temp: <b id='temp'>--°C</b></div>"
    "</div>"

    "</div>"

    /* RIGHT PANEL */
    "<div class='right'>"

    "<div class='card'>"
    "<h3>Medical Checkup History</h3>"
    "<div class='list-item'>Running - 140 Cal</div>"
    "<div class='list-item'>Cycling - 120 Cal</div>"
    "<div class='list-item'>Swimming - 150 Cal</div>"
    "<div class='list-item'>Yoga - 80 Cal</div>"
    "</div>"

    "<div class='card'>"
    "<h3>Heart Info</h3>"
    "<p>Normal BPM: 60–100</p>"
    "<p>Low BPM: rest or fatigue</p>"
    "<p>High BPM: stress or activity</p>"
    "<p>Maintain hydration and calm breathing.</p>"
    "</div>"

    "</div>"

    "</div>"

    "<script>"

    "let color='#6c5ce7';"
    "const canvas=document.getElementById('ecg');"
    "const ctx=canvas.getContext('2d');"
    "canvas.width=800; canvas.height=200;"

    "let offset=0;"

    "function drawECG(){"
    "ctx.fillStyle='#020617';"
    "ctx.fillRect(0,0,canvas.width,canvas.height);"

    "ctx.strokeStyle=color;"
    "ctx.lineWidth=2;"
    "ctx.beginPath();"

    "for(let x=0;x<canvas.width;x++){"
    "let y=100+Math.sin((x+offset)/10)*5;"

    "if(x%80===0) y=60;"
    "if(x%80===5) y=140;"

    "ctx.lineTo(x,y);"
    "}"

    "ctx.stroke();"
    "offset+=5;"
    "}"

    "function updateStatus(bpm){"
    "let s='',a='';"
    "if(bpm<60){s='LOW';color='#3b82f6';a='Increase activity';}"
    "else if(bpm>100){s='HIGH';color='#ef4444';a='Relax and breathe';}"
    "else{s='NORMAL';color='#22c55e';a='Stable condition';}"

    "document.getElementById('status').innerText=s;"
    "document.getElementById('advice').innerText=a;"
    "}"

    "function simulateExtras(bpm){"
    "document.getElementById('bp').innerText=(100+bpm/2)+'/'+(60+bpm/3);"
    "document.getElementById('oxy').innerText=(95+Math.random()*5).toFixed(0);"
    "document.getElementById('stress').innerText=(bpm-50);"
    "document.getElementById('temp').innerText=(36+Math.random()).toFixed(1);"
    "}"

    "function fetchBPM(){"
    "fetch('/bpm').then(r=>r.text()).then(d=>{"
    "let bpm=parseInt(d);"
    "document.getElementById('bpm').innerText=bpm;"
    "updateStatus(bpm);"
    "simulateExtras(bpm);"
    "});"
    "}"

    "setInterval(()=>{fetchBPM();drawECG();},1000);"
    "fetchBPM();"

    "</script>"

    "</body></html>";

    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}
// BPM API
esp_err_t bpm_handler(httpd_req_t *req)
{
    char resp[10];
    sprintf(resp, "%d", current_bpm);
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Redirect unknown URLs
esp_err_t not_found_handler(httpd_req_t *req, httpd_err_code_t err)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// ---------------- WIFI ----------------
void wifi_init()
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t ap_config = {
        .ap = {
            .ssid = "HealthMonitor",
            .ssid_len = strlen("HealthMonitor"),
            .password = "12345678",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        }
    };

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();

    esp_log_level_set("wifi", ESP_LOG_WARN);
}

// ---------------- SERVER ----------------
void start_server()
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_start(&server, &config);

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler
    };

    httpd_uri_t bpm = {
        .uri = "/bpm",
        .method = HTTP_GET,
        .handler = bpm_handler
    };

    // Optional: allow /index.html
    httpd_uri_t index_file = {
        .uri = "/index.html",
        .method = HTTP_GET,
        .handler = root_handler
    };

    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &bpm);
    httpd_register_uri_handler(server, &index_file);

    httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, not_found_handler);
}

// ---------------- MAIN ----------------
void app_main()
{
    nvs_flash_init();
    srand(time(NULL));

    wifi_init();
    start_server();

    xTaskCreate(bpm_task, "bpm_task", 2048, NULL, 1, NULL);

    ESP_LOGI(TAG, "Connect WiFi: HealthMonitor");
    ESP_LOGI(TAG, "Open: http://192.168.4.1");
}
