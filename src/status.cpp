#include "status.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include "bambuddy_client.h"
#include "ota.h"
#include "settings.h"
#include "tag_reader.h"

namespace status {

namespace {

const uint32_t CHECK_INTERVAL_MS = 60000;

Api api = Api::Checking;
String apiDetail = "not checked yet";
uint32_t apiCheckedAt = 0;
bool apiChecked = false;
bool checkRequested = true;
String lastScan = "none yet";
uint32_t lastScanAt = 0;

const char* apiName(Api a) {
  switch (a) {
    case Api::NotConfigured: return "not_configured";
    case Api::Checking: return "checking";
    case Api::Connected: return "connected";
    case Api::AuthFailed: return "auth_failed";
    case Api::Unreachable: return "unreachable";
    case Api::Error: return "error";
  }
  return "error";
}

void check() {
  checkRequested = false;
  if (!settings::wifiConnected()) return;
  String detail;
  const int code = bambuddy::ping(detail);
  Api result;
  if (code == 0) {
    result = Api::NotConfigured;
  } else if (code == 404 || code == 200) {
    result = Api::Connected;
  } else if (code == 401 || code == 403) {
    result = Api::AuthFailed;
  } else if (code < 0) {
    result = Api::Unreachable;
  } else {
    result = Api::Error;
  }
  reportApi(result, detail);
}

}  // namespace

void loop() {
  if (!settings::wifiConnected()) return;
  if (checkRequested || !apiChecked || millis() - apiCheckedAt >= CHECK_INTERVAL_MS) check();
}

void requestCheck() {
  checkRequested = true;
  api = Api::Checking;
  apiDetail = "checking";
}

void reportApi(Api a, const String& detail) {
  api = a;
  apiDetail = detail;
  apiChecked = true;
  apiCheckedAt = millis();
}

void reportScan(const String& text) {
  lastScan = text;
  lastScanAt = millis();
}

String json(const char* firmwareVersion) {
  JsonDocument doc;
  const uint32_t now = millis();
  JsonObject b = doc["bambuddy"].to<JsonObject>();
  b["state"] = apiName(api);
  b["detail"] = apiDetail;
  b["url"] = settings::current.bambuddyUrl;
  b["api_key_set"] = settings::current.apiKey.length() > 0;
  if (apiChecked) b["checked_s_ago"] = (now - apiCheckedAt) / 1000;
  doc["reader_ok"] = tag_reader::ok();
  JsonObject s = doc["last_scan"].to<JsonObject>();
  s["text"] = lastScan;
  if (lastScanAt) s["s_ago"] = (now - lastScanAt) / 1000;
  JsonObject w = doc["wifi"].to<JsonObject>();
  w["ssid"] = WiFi.SSID();
  w["rssi"] = WiFi.RSSI();
  w["ip"] = WiFi.localIP().toString();
  doc["firmware"] = firmwareVersion;
  String otaState, otaLatest, otaDetail;
  int otaPct = 0;
  ota::describe(otaState, otaLatest, otaPct, otaDetail);
  JsonObject u = doc["update"].to<JsonObject>();
  u["state"] = otaState;
  u["latest"] = otaLatest;
  u["progress"] = otaPct;
  u["detail"] = otaDetail;
  doc["uptime_s"] = now / 1000;
  String out;
  serializeJson(doc, out);
  return out;
}

const char STATUS_WIDGET_HTML[] = R"HTML(
<div id="sst" style="text-align:left;border:1px solid #8884;border-radius:6px;padding:10px 12px;margin:10px 0;line-height:1.7">Loading status&hellip;</div>
<script>
(function(){
var C={connected:['#1a9e4b','Connected'],auth_failed:['#d33','API key rejected'],unreachable:['#d33','Unreachable'],error:['#d33','Error'],checking:['#c80','Checking&hellip;'],not_configured:['#888','Not configured']};
function ago(s){return s==null?'':(s<60?s+' s':Math.floor(s/60)+' min')+' ago'}
function esc(t){return String(t).replace(/[&<>"]/g,function(c){return'&#'+c.charCodeAt(0)+';'})}
function dot(c){return '<span style="display:inline-block;width:10px;height:10px;border-radius:50%;background:'+c+';margin-right:6px"></span>'}
function post(u){fetch(u,{method:'POST'}).then(function(){setTimeout(function(){load(0)},500)})}
function upd(u){
if(u.state=='available')return '&mdash; update <b>'+esc(u.latest)+'</b> available <a href="#" id="sstu">install</a>';
if(u.state=='installing')return '&mdash; installing '+esc(u.latest)+': '+u.progress+'%';
if(u.state=='done')return '&mdash; '+esc(u.detail);
if(u.state=='checking')return '&mdash; checking for updates&hellip;';
if(u.state=='up_to_date')return '&mdash; up to date <a href="#" id="sstc">check again</a>';
if(u.state=='failed')return '&mdash; <span style="color:#d33">'+esc(u.detail)+'</span> '+(u.latest&&u.latest!=''?'<a href="#" id="sstu">retry install</a> ':'')+'<a href="#" id="sstc">check again</a>';
return '<a href="#" id="sstc">check for updates</a>'}
function load(r){fetch('/status.json'+(r?'?refresh=1':'')).then(function(x){return x.json()}).then(function(s){
var b=s.bambuddy,c=C[b.state]||C.error;
document.getElementById('sst').innerHTML=
'<b>Bambuddy</b>: '+dot(c[0])+(/auth_failed|unreachable|error/.test(b.state)?esc(b.detail.charAt(0).toUpperCase()+b.detail.slice(1)):c[1])+
(b.checked_s_ago!=null?' <small>('+ago(b.checked_s_ago)+')</small>':'')+
' <a href="#" id="sstr">check now</a><br>'+
'<small>'+(b.url?esc(b.url):'no URL set')+(b.api_key_set?', API key set':', no API key')+'</small><br>'+
'<b>Reader</b>: '+dot(s.reader_ok?'#1a9e4b':'#d33')+(s.reader_ok?'RC522 OK':'RC522 not responding, check wiring')+'<br>'+
'<b>Last scan</b>: '+esc(s.last_scan.text)+(s.last_scan.s_ago!=null?' <small>('+ago(s.last_scan.s_ago)+')</small>':'')+'<br>'+
'<b>Firmware</b>: '+esc(s.firmware)+' '+upd(s.update)+'<br>'+
'<small>WiFi '+esc(s.wifi.ssid)+' ('+s.wifi.rssi+' dBm)</small>';
document.getElementById('sstr').onclick=function(e){e.preventDefault();load(1)};
var bi=document.getElementById('sstu');if(bi)bi.onclick=function(e){e.preventDefault();if(confirm('Install firmware '+s.update.latest+'? The scanner restarts when done.'))post('/ota/install')};
var bc=document.getElementById('sstc');if(bc)bc.onclick=function(e){e.preventDefault();post('/ota/check')};
}).catch(function(){})}
load(0);setInterval(function(){load(0)},3000);
})();
</script>
)HTML";

}  // namespace status
