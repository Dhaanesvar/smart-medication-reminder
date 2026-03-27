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
"<title>Medico Pro</title>"

"<style>"
"body{margin:0;font-family:Segoe UI,Arial;background:#f5f7fb;}"

".layout{display:grid;grid-template-columns:90px 2fr 1fr;height:100vh;}"

/* SIDEBAR */
".sidebar{background:#fff;border-right:1px solid #eee;display:flex;flex-direction:column;align-items:center;padding:15px;gap:20px;}"
".logo{font-weight:bold;color:#6c5ce7;}"
".nav{width:55px;height:40px;background:#eef0ff;border-radius:10px;display:flex;align-items:center;justify-content:center;cursor:pointer;}"

/* MAIN */
".main{padding:20px;overflow:auto;}"
".card{background:#fff;border-radius:15px;padding:20px;margin-bottom:20px;box-shadow:0 5px 15px rgba(0,0,0,0.05);}"
".bpm{font-size:60px;font-weight:bold;color:#6c5ce7;}"

".metrics{display:grid;grid-template-columns:1fr 1fr;gap:10px;}"
".metric{background:#f5f6ff;padding:10px;border-radius:10px;}"

"canvas{width:100%;height:200px;background:#020617;border-radius:10px;}"

/* RIGHT PANEL */
".right{padding:20px;background:#fff;border-left:1px solid #eee;overflow:auto;}"
"table{width:100%;border-collapse:collapse;}"
"th,td{padding:8px;border-bottom:1px solid #eee;font-size:14px;text-align:left;}"

"</style></head><body>"

"<div class='layout'>"

/* SIDEBAR */
"<div class='sidebar'>"
"<div class='logo'>MED</div>"
"<div class='nav' onclick=\"showView('pulse')\">Pulse</div>"
"<div class='nav' onclick=\"showView('stats')\">Stats</div>"
"</div>"

/* MAIN */
"<div class='main'>"

/* PULSE VIEW */
"<div id='pulseView'>"

"<div class='card'>"
"<h3>Overview</h3>"
"<div class='bpm' id='bpm'>--</div>"
"<div id='status'>Analyzing...</div>"
"<div id='advice'></div>"
"</div>"

"<div class='card'>"
"<h3>ECG Monitor</h3>"
"<canvas id='ecg'></canvas>"
"</div>"

"<div class='metrics'>"
"<div class='metric'>BP: <b id='bp'>--</b></div>"
"<div class='metric'>Oxygen: <b id='oxy'>--%</b></div>"
"<div class='metric'>Stress: <b id='stress'>--%</b></div>"
"<div class='metric'>Temp: <b id='temp'>--</b></div>"
"</div>"

"</div>"

/* STATS VIEW */
"<div id='statsView' style='display:none;'>"
"<div class='card'>"
"<h3>Cardiac Statistics</h3>"

"<div class='metrics'>"
"<div class='metric'>Resting BPM<br><b>72</b></div>"
"<div class='metric'>HRV<br><b id='hrv'>--</b></div>"
"<div class='metric'>MAP<br><b id='map'>--</b></div>"
"<div class='metric'>Cardiac Output<br><b id='co'>--</b></div>"
"</div>"

"</div>"
"</div>"

"</div>"

/* RIGHT PANEL */
"<div class='right'>"

"<div class='card'>"
"<h3>Medical History</h3>"
"<table>"
"<tr><th>Activity</th><th>Date</th><th>Cal</th></tr>"
"<tr><td>Running</td><td>14 Apr</td><td>140</td></tr>"
"<tr><td>Cycling</td><td>12 Apr</td><td>120</td></tr>"
"<tr><td>Swimming</td><td>10 Apr</td><td>150</td></tr>"
"<tr><td>Yoga</td><td>9 Apr</td><td>80</td></tr>"
"</table>"
"</div>"

"<div class='card'>"
"<h3>Heart Info</h3>"
"<p>Normal: 60–100 BPM</p>"
"<p>Low: fatigue</p>"
"<p>High: stress</p>"
"</div>"

"</div>"

"</div>"

/* SCRIPT */
"<script>"

"let currentBPM = 75;"
"let canvas=document.getElementById('ecg');"
"let ctx=canvas.getContext('2d');"
"canvas.width=800; canvas.height=200;"
"let offset=0;"
"let color='#6c5ce7';"

/* VIEW SWITCH */
"function showView(view){"
"document.getElementById('pulseView').style.display='none';"
"document.getElementById('statsView').style.display='none';"
"if(view==='pulse'){document.getElementById('pulseView').style.display='block';}"
"else{document.getElementById('statsView').style.display='block';}"
"}"

/* ECG PQRST */
"function drawECG(bpm){"
"ctx.fillStyle='#020617';"
"ctx.fillRect(0,0,canvas.width,canvas.height);"

"ctx.strokeStyle=color;"
"ctx.lineWidth=2;"
"ctx.beginPath();"

"let baseline=100;"
"let cycle = 60000 / bpm;"
"let scale = canvas.width / 2000;"

"for(let x=0;x<canvas.width;x++){"

"let t=(x+offset)/scale;"
"let p=t%cycle;"
"let y=baseline;"

/* P */
"if(p>80&&p<140){y-=5*Math.sin((p-80)/60*Math.PI);}"

/* Q */
"if(p>180&&p<200){y+=8;}"

/* R */
"if(p>200&&p<220){y-=35;}"

/* S */
"if(p>220&&p<240){y+=15;}"

/* T */
"if(p>300&&p<380){y-=10*Math.sin((p-300)/80*Math.PI);}"

"ctx.lineTo(x,y);"
"}"

"ctx.stroke();"
"offset+=5;"
"}"

/* STATUS */
"function updateStatus(bpm){"
"if(bpm<60){color='#3b82f6';document.getElementById('status').innerText='LOW';document.getElementById('advice').innerText='Increase activity';}"
"else if(bpm>100){color='#ef4444';document.getElementById('status').innerText='HIGH';document.getElementById('advice').innerText='Relax';}"
"else{color='#22c55e';document.getElementById('status').innerText='NORMAL';document.getElementById('advice').innerText='Stable';}"
"}"

/* MEDICAL CALC */
"function simulate(bpm){"
"let sys=90+(bpm*0.5);"
"let dia=60+(bpm*0.3);"
"document.getElementById('bp').innerText=Math.round(sys)+'/'+Math.round(dia);"
"document.getElementById('oxy').innerText=(97+Math.sin(Date.now()/5000)).toFixed(1);"
"document.getElementById('stress').innerText=Math.max(0,bpm-60);"
"document.getElementById('temp').innerText=(36.5+Math.sin(Date.now()/8000)*0.2).toFixed(1);"

"document.getElementById('hrv').innerText=(120-bpm);"
"document.getElementById('map').innerText=Math.round(dia+(sys-dia)/3);"
"document.getElementById('co').innerText=(bpm*0.07).toFixed(2);"
"}"

/* FETCH */
"function fetchBPM(){"
"fetch('/bpm').then(r=>r.text()).then(d=>{"
"currentBPM=parseInt(d);"
"document.getElementById('bpm').innerText=currentBPM;"
"updateStatus(currentBPM);"
"simulate(currentBPM);"
"});"
"}"

"setInterval(()=>{fetchBPM();drawECG(currentBPM);},1000);"
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
