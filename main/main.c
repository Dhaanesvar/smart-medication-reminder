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
"<title>Medico Pro X</title>"

"<style>"
"body{margin:0;font-family:Segoe UI,Arial;background:#f4f7fb;color:#1a1a1a;}"

".layout{display:grid;grid-template-columns:90px 2fr 1fr;height:100vh;}"

/* SIDEBAR */
".sidebar{background:#fff;border-right:1px solid #e6e8ee;display:flex;flex-direction:column;align-items:center;padding:15px;gap:20px;}"
".logo{font-weight:700;color:#6c5ce7;font-size:18px;}"
".nav{width:60px;height:42px;background:#eef0ff;border-radius:12px;display:flex;align-items:center;justify-content:center;cursor:pointer;font-size:12px;}"

/* MAIN */
".main{padding:20px;overflow:auto;}"
".card{background:#fff;border-radius:16px;padding:20px;margin-bottom:20px;box-shadow:0 10px 25px rgba(0,0,0,0.05);}"
".bpm{font-size:64px;font-weight:700;color:#6c5ce7;}"
".status{font-size:16px;color:#555;}"
".advice{font-size:14px;color:#777;margin-top:5px;}"

".metrics{display:grid;grid-template-columns:1fr 1fr;gap:12px;}"
".metric{background:#f7f8ff;padding:12px;border-radius:12px;}"

"canvas{width:100%;height:220px;background:#020617;border-radius:12px;}"

/* RIGHT PANEL */
".right{padding:20px;background:#fff;border-left:1px solid #e6e8ee;overflow:auto;}"
"table{width:100%;border-collapse:collapse;}"
"th,td{padding:8px;border-bottom:1px solid #eee;font-size:14px;text-align:left;}"

".info{font-size:14px;line-height:1.6;color:#555;}"
"</style></head><body>"

"<div class='layout'>"

/* SIDEBAR */
"<div class='sidebar'>"

"<div class='logo'>MED</div>"

"<div style='font-size:12px;color:#666;text-align:center;'>"
"Patient ID<br><b>HM-1024</b><br><br>"
"Age<br><b>29</b><br><br>"
"Status<br><b id='zone'>Normal</b>"
"</div>"

"<div class='nav' onclick=\"showView('pulse')\">Pulse</div>"
"<div class='nav' onclick=\"showView('stats')\">Stats</div>"

"<div style='margin-top:auto;font-size:11px;color:#999;text-align:center;'>"
"Live Monitoring Active"
"</div>"

"</div>"

/* MAIN */
"<div class='main'>"

/* PULSE VIEW */
"<div id='pulseView'>"

"<div class='card'>"
"<h3>Patient Heart Overview</h3>"
"<div class='bpm' id='bpm'>--</div>"
"<div class='status' id='status'>Analyzing...</div>"
"<div class='advice' id='advice'></div>"
"</div>"

"<div class='card'>"
"<h3>Real-Time ECG Monitor</h3>"
"<canvas id='ecg'></canvas>"
"</div>"

"<div class='metrics'>"
"<div class='metric'>Blood Pressure<br><b id='bp'>--</b></div>"
"<div class='metric'>Oxygen Saturation<br><b id='oxy'>--%</b></div>"
"<div class='metric'>Stress Index<br><b id='stress'>--</b></div>"
"<div class='metric'>Body Temperature<br><b id='temp'>--</b></div>"
"</div>"

"</div>"

/* STATS VIEW */
"<div id='statsView' style='display:none;'>"

"<div class='card'>"
"<h3>Advanced Cardiac Metrics</h3>"

"<div class='metrics'>"
"<div class='metric'>Resting BPM<br><b>72</b></div>"
"<div class='metric'>HRV<br><b id='hrv'>-- ms</b></div>"
"<div class='metric'>MAP<br><b id='map'>--</b></div>"
"<div class='metric'>Cardiac Output<br><b id='co'>-- L/min</b></div>"
"</div>"

"</div>"

/* NEW: TREND GRAPH */
"<div class='card'>"
"<h3>Heart Rate Trend</h3>"
"<canvas id='trend'></canvas>"
"</div>"

/* NEW: RISK PANEL */
"<div class='card'>"
"<h3>Risk Classification</h3>"
"<div id='risk' style='font-size:14px;color:#555;'>Analyzing...</div>"
"</div>"

"</div>"

/* RIGHT PANEL */
"<div class='right'>"

/* HISTORY TABLE */
"<div class='card'>"
"<h3>Medical Activity Log</h3>"
"<table>"
"<tr><th>Activity</th><th>Date</th><th>Calories</th></tr>"
"<tr><td>Running</td><td>14 Apr</td><td>140</td></tr>"
"<tr><td>Cycling</td><td>12 Apr</td><td>120</td></tr>"
"<tr><td>Swimming</td><td>10 Apr</td><td>150</td></tr>"
"<tr><td>Yoga</td><td>9 Apr</td><td>80</td></tr>"
"</table>"
"</div>"

/* CLINICAL INTERPRETATION */
"<div class='card'>"
"<h3>Clinical Interpretation</h3>"
"<div class='info' id='interpret'>Waiting for data...</div>"
"</div>"

/* HEART EDUCATION */
"<div class='card'>"
"<h3>Cardiovascular Insight</h3>"
"<div class='info'>"
"The heart pumps blood throughout the body delivering oxygen and nutrients. "
"Normal resting heart rate ranges from 60 to 100 BPM. Sustained high rates may indicate stress, "
"while low rates can reflect rest or underlying conditions. Maintaining cardiovascular health "
"requires balanced activity, hydration, and controlled stress levels."
"</div>"
"</div>"

"</div>"

"</div>"

/* SCRIPT */
"<script>"

"let currentBPM=75;"
"let canvas=document.getElementById('ecg');"
"let ctx=canvas.getContext('2d');"
"canvas.width=800;canvas.height=220;"
"let offset=0;"
"let color='#6c5ce7';"

"let trendCanvas=document.createElement('canvas');"
"trendCanvas=document.getElementById('trend');"
"let tctx=trendCanvas.getContext('2d');"
"trendCanvas.width=800; trendCanvas.height=200;"

"let history=[];"

"function drawTrend(){"
"tctx.fillStyle='#020617';"
"tctx.fillRect(0,0,trendCanvas.width,trendCanvas.height);"

"tctx.strokeStyle='#22c55e';"
"tctx.beginPath();"

"for(let i=0;i<history.length;i++){"
"let x=i*10;"
"let y=150-history[i];"
"tctx.lineTo(x,y);"
"}"

"tctx.stroke();"
"}"

/* VIEW SWITCH */
"function showView(v){"
"document.getElementById('pulseView').style.display='none';"
"document.getElementById('statsView').style.display='none';"
"if(v==='pulse'){document.getElementById('pulseView').style.display='block';}"
"else{document.getElementById('statsView').style.display='block';}"
"}"

/* ECG */
"function drawECG(bpm){"
"ctx.fillStyle='#020617';ctx.fillRect(0,0,canvas.width,canvas.height);"
"ctx.strokeStyle=color;ctx.beginPath();"

"let base=110;"
"let cycle=60000/bpm;"
"let scale=canvas.width/2000;"

"for(let x=0;x<canvas.width;x++){"
"let t=(x+offset)/scale;"
"let p=t%cycle;"
"let y=base;"

"if(p>80&&p<140){y-=5*Math.sin((p-80)/60*Math.PI);}"
"if(p>180&&p<200){y+=8;}"
"if(p>200&&p<220){y-=35;}"
"if(p>220&&p<240){y+=15;}"
"if(p>300&&p<380){y-=10*Math.sin((p-300)/80*Math.PI);}"

"ctx.lineTo(x,y);}"
"ctx.stroke();offset+=5;}"
 
/* STATUS */
"function updateStatus(bpm){"
"let txt='';"
"if(bpm<60){color='#3b82f6';txt='Bradycardia detected. Recommend light activity.';}"
"else if(bpm>100){color='#ef4444';txt='Elevated heart rate. Suggest rest and hydration.';}"
"else{color='#22c55e';txt='Heart rate within normal resting range.';}"

"document.getElementById('status').innerText=txt;"
"document.getElementById('advice').innerText='Continuous monitoring active.';"
"document.getElementById('interpret').innerText=txt;"
"}"

/* CALC */
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

/* store history */
"history.push(currentBPM);"
"if(history.length>60) history.shift();"

"drawTrend();"

/* risk classification */
"let risk='';"
"if(currentBPM<60){risk='Low heart rate (Bradycardia risk)';}"
"else if(currentBPM>100){risk='Elevated heart rate (Tachycardia risk)';}"
"else{risk='Within normal physiological range';}"

"document.getElementById('risk').innerText=risk;"
"document.getElementById('zone').innerText=risk.split(' ')[0];"

"});}"
 
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
