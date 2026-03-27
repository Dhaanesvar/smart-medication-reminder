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
    "<title>Health Monitor</title>"

    "<style>"
    "body{margin:0;background:#020617;color:#e2e8f0;font-family:sans-serif;}"
    ".grid{display:grid;grid-template-columns:2fr 1fr;height:100vh;}"

    ".left{padding:20px;}"
    ".right{background:#020617;padding:20px;border-left:1px solid #1e293b;}"

    ".card{background:#020617;border:1px solid #1e293b;padding:20px;border-radius:12px;margin-bottom:20px;}"

    ".bpm{font-size:80px;font-weight:bold;}"
    ".pulse{width:15px;height:15px;border-radius:50%;display:inline-block;margin-left:10px;}"

    ".metrics{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:20px;}"
    ".metric{background:#020617;border:1px solid #1e293b;padding:10px;border-radius:10px;}"

    "canvas{width:100%;height:200px;background:black;border-radius:10px;}"
    "</style></head><body>"

    "<div class='grid'>"

    "<div class='left'>"

    "<div class='card'>"
    "<h2>Heart Monitor</h2>"
    "<div class='bpm' id='bpm'>-- <span id='pulse' class='pulse'></span></div>"
    "<div id='status'>Initializing...</div>"
    "<div id='advice'></div>"
    "</div>"

    "<div class='card'>"
    "<canvas id='ecg'></canvas>"
    "</div>"

    "<div class='metrics'>"
    "<div class='metric'>BP: <span id='bp'>--</span></div>"
    "<div class='metric'>Oxygen: <span id='oxy'>--</span>%</div>"
    "<div class='metric'>Stress: <span id='stress'>--</span>%</div>"
    "<div class='metric'>Temp: <span id='temp'>--</span>°C</div>"
    "</div>"

    "</div>"

    "<div class='right'>"
    "<h2>Heart Insights</h2>"
    "<p>Normal BPM: 60–100</p>"
    "<p>Low BPM: Resting or fatigue</p>"
    "<p>High BPM: Stress or activity</p>"
    "<h3>Advice</h3>"
    "<p>Maintain hydration, exercise, and calm breathing.</p>"
    "</div>"

    "</div>"

    "<script>"
    "const canvas=document.getElementById('ecg');"
    "const ctx=canvas.getContext('2d');"
    "canvas.width=800; canvas.height=200;"

    "let x=0;"

    "function drawECG(bpm){"
    "ctx.fillStyle='black';"
    "ctx.fillRect(0,0,canvas.width,canvas.height);"

    "ctx.strokeStyle=color;"
    "ctx.lineWidth=2;"
    "ctx.beginPath();"

    "for(let i=0;i<canvas.width;i++){"
    "let y=100+Math.sin((i+x)/10)*5;"

    "if(i%50===0){y=50;}"
    "if(i%50===5){y=150;}"

    "ctx.lineTo(i,y);"
    "}"

    "ctx.stroke();"
    "x+=5;"
    "}"

    "let color='#22c55e';"

    "function updateStatus(bpm){"
    "let status='',advice='';"

    "if(bpm<60){"
    "status='LOW'; color='#3b82f6'; advice='Increase activity';"
    "}else if(bpm>100){"
    "status='HIGH'; color='#ef4444'; advice='Relax and breathe';"
    "}else{"
    "status='NORMAL'; color='#22c55e'; advice='Stable condition';"
    "}"

    "document.getElementById('status').innerText=status;"
    "document.getElementById('advice').innerText=advice;"
    "document.getElementById('pulse').style.background=color;"
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
    "document.getElementById('bpm').innerHTML=bpm+' <span id=\"pulse\" class=\"pulse\"></span>';"
    "updateStatus(bpm);"
    "simulateExtras(bpm);"
    "});"
    "}"

    "setInterval(()=>{fetchBPM();drawECG();},1000);"
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
