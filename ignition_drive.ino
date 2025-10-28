#include <Arduino.h>
#include <WiFi.h>
#include <pgmspace.h>
#include <math.h>
#include <esp_https_server.h>
#include <esp_http_server.h>

#include "certs.h"  // your PEMs

/******** Geometry & timing ********/
#ifndef TRACK_LEN_MM
#define TRACK_LEN_MM   75
#endif
const float SOFT_MIN_MM = 0.0f;
const float SOFT_MAX_MM = (float)TRACK_LEN_MM;

const int   STEPS_PER_REV   = 20;
const float LEAD_MM_PER_REV = 0.5f;
const int   STEPS_PER_MM    = STEPS_PER_REV / LEAD_MM_PER_REV;   // 40

const int STEP_HIGH_US = 60;
const int MIN_DWELL_US = 600;

const int HOMING_DELAY_US = 1000;
const int SAFE_START_MM   = 10;
const int MAX_HOME_MM     = 100;
const long MAX_HOME_STEPS = (long)MAX_HOME_MM * STEPS_PER_MM;

const int   CENTER_DWELL_US = 1200;
const float LAND_MARGIN      = 0.3f;

static inline uint32_t max_u32(uint32_t a, uint32_t b){ return a>b?a:b; }

/******** Motion layer ********/
class StepperAxis {
public:
  StepperAxis(int stepPin,int dirPin,int enPin,int endPin,int forwardDirLevel,const char* name)
  : STEP(stepPin),DIR(dirPin),EN(enPin),ES(endPin),
    FWD(forwardDirLevel),REV(forwardDirLevel==HIGH?LOW:HIGH),label(name){}
  void begin(){ pinMode(STEP,OUTPUT); pinMode(DIR,OUTPUT); pinMode(EN,OUTPUT); pinMode(ES,INPUT_PULLUP);
    digitalWrite(EN,HIGH); digitalWrite(STEP,LOW); delay(50); enable(); }
  void enable(){ digitalWrite(EN,LOW); }
  void setDirection(bool fwd){ digitalWrite(DIR,fwd?FWD:REV); }
  float mm() const { return (float)posSteps/(float)STEPS_PER_MM; }
  void pulseFast(){ digitalWrite(STEP,HIGH); delayMicroseconds(STEP_HIGH_US); digitalWrite(STEP,LOW); delayMicroseconds(STEP_HIGH_US); }
  bool stepForward(){ if(posSteps >= (long)(SOFT_MAX_MM*STEPS_PER_MM)) return false; setDirection(true); pulseFast(); posSteps++; return true; }
  bool stepReverse(){ if(posSteps <= (long)(SOFT_MIN_MM*STEPS_PER_MM)) return false; setDirection(false); pulseFast(); posSteps--; return true; }
  void moveMM(float dmm){ if(!dmm) return; long n = lroundf(fabsf(dmm)*STEPS_PER_MM); bool fwd=dmm>0;
    for(long i=0;i<n;i++){ if(fwd){ if(!stepForward()) break; } else { if(!stepReverse()) break; } delayMicroseconds(150);} }
  void moveToMM(float t){ moveMM(t - mm()); }
  bool home(){
    if(endstopPressed()){ int guard=3*STEPS_PER_MM; while(endstopPressed()&&guard-->0){ setDirection(true); pulseFast(); posSteps++; delayMicroseconds(900);} delay(30);}
    setDirection(false); long cnt=0; while(!endstopPressed()){ pulseFast(); posSteps--; delayMicroseconds(HOMING_DELAY_US);
      if(++cnt>MAX_HOME_STEPS){ Serial.printf("[%s] Home guard hit\n",label); return false; } }
    setDirection(true); { int guard=3*STEPS_PER_MM; while(endstopPressed()&&guard-->0){ pulseFast(); posSteps++; delayMicroseconds(900); }
      int extra=(int)(0.5f*STEPS_PER_MM); for(int i=0;i<extra;i++){ pulseFast(); posSteps++; delayMicroseconds(900);} }
    posSteps=0; Serial.printf("[%s] Home=0mm\n",label); return true;
  }
  void moveToMM_slow(float tgt,int dwell_us=CENTER_DWELL_US){
    if(tgt<SOFT_MIN_MM) tgt=SOFT_MIN_MM; if(tgt>SOFT_MAX_MM) tgt=SOFT_MAX_MM; long T=lroundf(tgt*STEPS_PER_MM);
    bool fwd = (T>posSteps); setDirection(fwd); long N=labs(T-posSteps);
    for(long i=0;i<N;i++){ pulseFast(); posSteps+= fwd?1:-1; delayMicroseconds(dwell_us); }
  }
private:
  const int STEP,DIR,EN,ES; const int FWD,REV; const char* label; long posSteps=0;
  bool endstopPressed(){ const int N=5; int c=0; for(int i=0;i<N;i++){ if(digitalRead(ES)==LOW) c++; delay(1);} return c>N/2; }
};

// Pins (adjust if needed)
StepperAxis axisX(1,2,3,4,LOW,"X");
StepperAxis axisY(9,8,7,44,LOW,"Y");

// Coordinator
struct Coordinator {
  float rx_steps=0.0f, ry_steps=0.0f;
  void moveVectorMM(float dx,float dy,uint32_t dt_ms){
    float x0=axisX.mm(), y0=axisY.mm();
    float xt=constrain(x0+dx,SOFT_MIN_MM,SOFT_MAX_MM);
    float yt=constrain(y0+dy,SOFT_MIN_MM,SOFT_MAX_MM);
    dx=xt-x0; dy=yt-y0;
    float sx_f = dx*STEPS_PER_MM + rx_steps;
    float sy_f = dy*STEPS_PER_MM + ry_steps;
    long stepsX=lroundf(fabsf(sx_f)), stepsY=lroundf(fabsf(sy_f));
    if(stepsX==0 && stepsY==0){ rx_steps=sx_f; ry_steps=sy_f; delay(dt_ms); return; }
    long N = (stepsX>stepsY)?stepsX:stepsY;
    bool xf=(sx_f>=0), yf=(sy_f>=0); axisX.setDirection(xf); axisY.setDirection(yf);
    float incx=(stepsX>0)?(float)stepsX/(float)N:0.0f, incy=(stepsY>0)?(float)stepsY/(float)N:0.0f;
    float ex=0.0f, ey=0.0f;
    uint32_t per = (N>0)? max_u32((uint32_t)MIN_DWELL_US, (uint32_t)((dt_ms*1000UL)/(uint32_t)N)) : (uint32_t)MIN_DWELL_US;
    for(long i=0;i<N;i++){
      ex+=incx; if(ex>=1.0f){ ex-=1.0f; if(xf) (void)axisX.stepForward(); else (void)axisX.stepReverse(); }
      ey+=incy; if(ey>=1.0f){ ey-=1.0f; if(yf) (void)axisY.stepForward(); else (void)axisY.stepReverse(); }
      delayMicroseconds(per);
    }
    float x1=axisX.mm(), y1=axisY.mm();
    float ax=(x1-x0)*STEPS_PER_MM, ay=(y1-y0)*STEPS_PER_MM;
    rx_steps = sx_f - ax; ry_steps = sy_f - ay;
  }
  void movePolar(float v_mm_s, float theta_rad, uint32_t dt_ms){
    float dt_s=dt_ms/1000.0f;
    moveVectorMM(v_mm_s*cosf(theta_rad)*dt_s, v_mm_s*sinf(theta_rad)*dt_s, dt_ms);
  }
  void resetResiduals(){ rx_steps=0.0f; ry_steps=0.0f; }
} ctrl;

/******** Centering helpers ********/
void lineToXY_slow(float x,float y,int dwell_us=CENTER_DWELL_US){
  x=constrain(x,SOFT_MIN_MM,SOFT_MAX_MM); y=constrain(y,SOFT_MIN_MM,SOFT_MAX_MM);
  float x0=axisX.mm(), y0=axisY.mm(); float dx=x-x0, dy=y-y0;
  long sx=lroundf(dx*STEPS_PER_MM), sy=lroundf(dy*STEPS_PER_MM);
  long ax=labs(sx), ay=labs(sy), N=(ax>ay)?ax:ay; if(!N) return;
  bool xf=(sx>=0), yf=(sy>=0); float ex=0, ey=0, incx=(ax>0)?(float)ax/N:0, incy=(ay>0)?(float)ay/N:0;
  axisX.setDirection(xf); axisY.setDirection(yf);
  for(long i=0;i<N;i++){
    ex+=incx; if(ex>=1.0f){ ex-=1.0f; if(xf) (void)axisX.stepForward(); else (void)axisX.stepReverse(); }
    ey+=incy; if(ey>=1.0f){ ey-=1.0f; if(yf) (void)axisY.stepForward(); else (void)axisY.stepReverse(); }
    delayMicroseconds(dwell_us);
  }
}
void centerAxesReliably(float center_mm = TRACK_LEN_MM * 0.5f){
  axisX.enable(); axisY.enable();
  const float SEAT_MM=5.0f;
  axisX.moveToMM_slow(min(SOFT_MAX_MM-LAND_MARGIN, axisX.mm()+SEAT_MM));
  axisY.moveToMM_slow(min(SOFT_MAX_MM-LAND_MARGIN, axisY.mm()+SEAT_MM));
  const float FAR = SOFT_MAX_MM - LAND_MARGIN;
  axisX.moveToMM_slow(FAR); axisY.moveToMM_slow(FAR);
  lineToXY_slow(center_mm, center_mm);
  axisX.moveToMM_slow(center_mm); axisY.moveToMM_slow(center_mm);
}

/******** Tilt mapping ********/
volatile float lastGamma = NAN, lastBeta = NAN;
volatile bool  gotData   = false;
volatile bool  ignition_on = false;   // NEW ignition flag

volatile float theta_cmd = 0.0f;  // rad
volatile float v_mm_s    = 0.0f;  // mm/s

#ifndef MAX_TURN_RATE
#define MAX_TURN_RATE    0.8f
#endif
#ifndef MAX_ACCEL
#define MAX_ACCEL        8.0f
#endif
#ifndef MAX_DECEL
#define MAX_DECEL        8.0f
#endif
#ifndef V_MAX
#define V_MAX            15.0f
#endif
#ifndef V_MIN
#define V_MIN            0.0f
#endif
#ifndef BETA_DEADBAND
#define BETA_DEADBAND    2.0f
#endif
#ifndef GAMMA_DEADBAND
#define GAMMA_DEADBAND   2.0f
#endif

static const float BETA_MIN=-40.0f,BETA_MAX=40.0f, GAMMA_MIN=-70.0f,GAMMA_MAX=-30.0f;

static inline float clampf(float v,float lo,float hi){ return v<lo?lo:(v>hi?hi:v); }
static float fmap_clamped(float x,float in_min,float in_max,float out_min,float out_max){
  if(x<=in_min) return out_min; if(x>=in_max) return out_max;
  float t=(x-in_min)/(in_max-in_min); return out_min + t*(out_max-out_min);
}
static float apply_deadband_symmetric(float x,float db,float in_min,float in_max){
  if(fabsf(x)<=db) return 0.0f;
  if(x>0){ float span=in_max-db; return (x-db)*(in_max/span); }
  float span=in_min+db; return (x+db)*(in_min/span);
}
static void tiltToRates_mm(float beta,float gamma,float &dtheta_dt,float &dv_dt){
  float beta_db=apply_deadband_symmetric(beta,BETA_DEADBAND,BETA_MIN,BETA_MAX);
  dtheta_dt=fmap_clamped(beta_db,BETA_MIN,BETA_MAX,-MAX_TURN_RATE,+MAX_TURN_RATE);
  if(fabsf(gamma+50.0f)<=GAMMA_DEADBAND) gamma=-50.0f;
  dv_dt=fmap_clamped(gamma,GAMMA_MIN,GAMMA_MAX,-MAX_DECEL,+MAX_ACCEL);
}

/******** HTTPS + WSS ********/
static const char* AP_SSID="ESP32-Tilt";
static const char* AP_PASS="tilt12345";

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Tilt (WSS)</title>
<style>
:root{color-scheme:light dark}
body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Helvetica,Arial,sans-serif;margin:2rem}
.card{max-width:560px;padding:1rem;border:1px solid #ddd;border-radius:12px;box-shadow:0 2px 12px rgba(0,0,0,.06)}
button{padding:.6rem 1rem;border:0;border-radius:10px;cursor:pointer;margin-right:.5rem}
#ign{background:#c62828;color:#fff}
#ign.on{background:#2e7d32}
.mono{font-family:ui-monospace,Consolas,Monaco,monospace}
</style>
<div class="card">
  <h1>Phone Tilt → ESP32 (HTTPS + WSS)</h1>
  <p>Status: <span id="ws">disconnected</span></p>
  <p class="mono">γ: <span id="g">–</span>° &nbsp; β: <span id="b">–</span>°</p>
  <p>
    <button id="perm">Start sensors</button>
    <button id="ign" class="">Ignition: OFF</button>
  </p>
  <p style="margin-top:.5rem;font-size:.9rem;opacity:.75">iOS needs HTTPS and a user gesture. Accept the certificate if prompted.</p>
</div>
<script>
let ws, latest={g:NaN,b:NaN}, ignition=0;
function connectWS(){
  ws = new WebSocket(`wss://${location.hostname}:8443/ws`);
  const el = document.getElementById('ws');
  ws.onopen  = ()=> el.textContent='connected';
  ws.onclose = ()=> el.textContent='disconnected';
  ws.onerror = ()=> el.textContent='error';
}
async function ensurePerm(){
  try{
    if(typeof DeviceMotionEvent!=='undefined' && typeof DeviceMotionEvent.requestPermission==='function'){
      return (await DeviceMotionEvent.requestPermission())==='granted';
    }else if(typeof DeviceOrientationEvent!=='undefined' && typeof DeviceOrientationEvent.requestPermission==='function'){
      return (await DeviceOrientationEvent.requestPermission())==='granted';
    }
    return true;
  }catch(e){return false;}
}
function startSensors(){
  window.addEventListener('deviceorientation', e=>{
    latest.g = (typeof e.gamma==='number')?e.gamma:NaN;
    latest.b = (typeof e.beta ==='number')?e.beta :NaN;
    document.getElementById('g').textContent = Number.isFinite(latest.g)?latest.g.toFixed(2):'NaN';
    document.getElementById('b').textContent = Number.isFinite(latest.b)?latest.b.toFixed(2):'NaN';
  }, true);
  setInterval(()=>{
    if(ws && ws.readyState===1 && Number.isFinite(latest.g) && Number.isFinite(latest.b)){
      ws.send(`g:${latest.g.toFixed(4)},b:${latest.b.toFixed(4)},ign:${ignition}`);
    }
  }, 50);
}
document.getElementById('perm').addEventListener('click', async ()=>{
  if(!(await ensurePerm())){ alert('Motion permission denied.'); return; }
  if(!ws || ws.readyState!==1) connectWS();
  startSensors();
});
document.getElementById('ign').addEventListener('click', ()=>{
  ignition = ignition ? 0 : 1;
  const btn = document.getElementById('ign');
  if(ignition){ btn.textContent='Ignition: ON'; btn.classList.add('on'); }
  else        { btn.textContent='Ignition: OFF'; btn.classList.remove('on'); }
  // Send an immediate state update
  if(ws && ws.readyState===1){
    const g = Number.isFinite(latest.g)?latest.g.toFixed(4):'NaN';
    const b = Number.isFinite(latest.b)?latest.b.toFixed(4):'NaN';
    ws.send(`g:${g},b:${b},ign:${ignition}`);
  }
});
</script>
)HTML";

httpd_handle_t srv_https=nullptr, srv_http=nullptr;

static esp_err_t index_get_handler(httpd_req_t *req){
  httpd_resp_set_type(req,"text/html");
  httpd_resp_set_hdr(req,"Cache-Control","no-store");
  httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}
static esp_err_t ws_handler(httpd_req_t *req){
  if(req->method==HTTP_GET) return ESP_OK;
  httpd_ws_frame_t f={}; f.type=HTTPD_WS_TYPE_TEXT;
  if(httpd_ws_recv_frame(req,&f,0)!=ESP_OK) return ESP_FAIL;
  if(f.len>4096) return ESP_FAIL;
  uint8_t* payload=(uint8_t*)malloc(f.len+1); if(!payload) return ESP_ERR_NO_MEM;
  f.payload=payload;
  if(httpd_ws_recv_frame(req,&f,f.len)!=ESP_OK){ free(payload); return ESP_FAIL; }
  payload[f.len]=0;
  const char* c=(const char*)payload;
  const char* gpos=strstr(c,"g:"); const char* bpos=strstr(c,"b:"); const char* ipos=strstr(c,"ign:");
  if(gpos && bpos){
    float g=strtof(gpos+2,nullptr); float b=strtof(bpos+2,nullptr);
    if(isfinite(g)&&isfinite(b)){ lastGamma=g; lastBeta=b; gotData=true; }
  }
  if(ipos){
    int ign = strtol(ipos+4,nullptr,10);
    ignition_on = (ign!=0);
  }
  free(payload);
  return ESP_OK;
}
static esp_err_t http80_index(httpd_req_t *req){
  httpd_resp_set_type(req,"text/html");
  IPAddress ip=WiFi.softAPIP();
  String body=String("<!doctype html><meta charset='utf-8'><title>ESP32 tilt – HTTP</title><h1>ESP32 running</h1><p>Open <a href='https://")
              + ip.toString() + ":8443/'>https://" + ip.toString() + ":8443/</a></p>";
  httpd_resp_send(req, body.c_str(), body.length());
  return ESP_OK;
}
void start_http_helper(){
  httpd_config_t cfg=HTTPD_DEFAULT_CONFIG();
  cfg.server_port=80; cfg.lru_purge_enable=true;
  if(httpd_start(&srv_http,&cfg)==ESP_OK){
    httpd_uri_t root={}; root.uri="/"; root.method=HTTP_GET; root.handler=http80_index;
    httpd_register_uri_handler(srv_http,&root);
    Serial.println("[HTTP] helper up on :80");
  }
}
bool start_https_wss_server_8443(){
  const unsigned char* cert_ptr=(const unsigned char*)SERVER_CERT_PEM;
  const unsigned char* key_ptr =(const unsigned char*)SERVER_KEY_PEM;
  size_t cert_len=strlen_P((PGM_P)SERVER_CERT_PEM)+1;
  size_t key_len =strlen_P((PGM_P)SERVER_KEY_PEM)+1;
  httpd_ssl_config_t conf=HTTPD_SSL_CONFIG_DEFAULT();
  conf.port_secure=8443; conf.transport_mode=HTTPD_SSL_TRANSPORT_SECURE;
  conf.servercert=cert_ptr; conf.servercert_len=cert_len;
  conf.prvtkey_pem=key_ptr; conf.prvtkey_len=key_len;
  conf.httpd.stack_size=12288; conf.httpd.max_open_sockets=4;
  conf.httpd.lru_purge_enable=true; conf.httpd.recv_wait_timeout=10; conf.httpd.send_wait_timeout=10;
  if(httpd_ssl_start(&srv_https,&conf)!=ESP_OK){ Serial.println("[HTTPS] start failed"); return false; }
  httpd_uri_t root={}; root.uri="/"; root.method=HTTP_GET; root.handler=index_get_handler; httpd_register_uri_handler(srv_https,&root);
  httpd_uri_t ws={}; ws.uri="/ws"; ws.method=HTTP_GET; ws.handler=ws_handler; ws.is_websocket=true; ws.handle_ws_control_frames=true;
  httpd_register_uri_handler(srv_https,&ws);
  Serial.println("[HTTPS/WSS] listening on :8443 (/, /ws)");
  return true;
}

/******** Setup / Loop (velocity mode) ********/
unsigned long lastPrintMs=0, lastPhysUpdateMs=0, lastCmdMs=0;
static const uint32_t CMD_PERIOD_MS = 80;

void setup(){
  Serial.begin(115200); delay(200);
  WiFi.mode(WIFI_AP); WiFi.softAP(AP_SSID,AP_PASS); WiFi.setSleep(false);
  IPAddress ip=WiFi.softAPIP();
  Serial.printf("[WiFi] AP up. http://%s  →  https://%s:8443/\n", ip.toString().c_str(), ip.toString().c_str());
  start_https_wss_server_8443(); start_http_helper();

  axisX.begin(); axisY.begin();
  axisX.moveMM(+SAFE_START_MM); axisY.moveMM(+SAFE_START_MM);
  bool okX=axisX.home(), okY=axisY.home(); if(!(okX&&okY)) Serial.println("Homing failed — check wiring/dir/switch.");
  centerAxesReliably(TRACK_LEN_MM * 0.5f);

  lastPhysUpdateMs = lastCmdMs = millis();
}

void loop(){
  unsigned long now=millis();
  if(lastPhysUpdateMs==0) lastPhysUpdateMs=now;
  float dt=(now - lastPhysUpdateMs)/1000.0f; lastPhysUpdateMs=now;

  float dtheta_dt=0.0f, dv_dt=0.0f;
  if(gotData && isfinite(lastBeta) && isfinite(lastGamma)){
    float beta = clampf(lastBeta,  -40.0f, +40.0f);
    float gamma= clampf(lastGamma, -70.0f, -30.0f);
    tiltToRates_mm(beta, gamma, dtheta_dt, dv_dt);
  }

  // Steering can still change; speed is locked to 0 when ignition is OFF
  theta_cmd += dtheta_dt * dt;
  if(theta_cmd >  PI) theta_cmd -= 2.0f*PI;
  if(theta_cmd < -PI) theta_cmd += 2.0f*PI;

  if(ignition_on){
    v_mm_s += dv_dt * dt;
    v_mm_s  = clampf(v_mm_s, V_MIN, V_MAX);
  }else{
    v_mm_s = 0.0f;    // hold still when ignition OFF
  }

  if(now - lastCmdMs >= CMD_PERIOD_MS){
    lastCmdMs = now;
    ctrl.movePolar(v_mm_s, theta_cmd, CMD_PERIOD_MS);
  }

  if(now - lastPrintMs >= 1000){
    lastPrintMs = now;
    float px=axisX.mm(), py=axisY.mm();
    Serial.printf("IGN=%d | β=%.1f° γ=%.1f° | θ=%.2f rad  v=%.2f mm/s | pos=(%.1f, %.1f) mm\n",
                  ignition_on?1:0, lastBeta, lastGamma, theta_cmd, v_mm_s, px, py);
  }
}
