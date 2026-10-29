#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_netif_sta_list.h>
#include <DNSServer.h>
#include "config.h"
#include "control.h"
#include "web_ui.h"
#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif
HardwareSerial bridge(1);
WebServer server(80);
DNSServer dns;
static uint32_t pageHits;
static CtParser parser;
static uint8_t snapshot[194],pendingFrame[CT_FRAME],events[32][16];
static bool haveState,eventValid[32],pending;
static uint8_t metricHeader[45],slotStats[8][40],doseRecords[24][48],openIntervals[8][20];
static bool haveMetrics,slotValid[8],doseValid[24],intervalValid[8];
static uint32_t doseReceived[24],metricPoll,historyGeneration;
static unsigned metricCursor;
static uint64_t little64(const uint8_t *p){uint64_t n=0;for(unsigned i=0;i<8;i++)n|=(uint64_t)p[i]<<(8*i);return n;}
static uint32_t metricGeneration(){return ct_u32(metricHeader+2);}
static void resetMetrics(){haveMetrics=false;memset(slotValid,0,sizeof slotValid);memset(doseValid,0,sizeof doseValid);memset(intervalValid,0,sizeof intervalValid);metricCursor=0;}

static uint32_t lastState,heartbeat,lastSend,operation,sequence,lastHistory;
static unsigned pendingSize,attempts,historyIndex,badframes;
static int outcome=-1;
static uint16_t resultRevision;
static String token;
static bool apConfigured;
static volatile uint32_t apStarts,apStops,stationJoins;
static void startAccessPoint(){
 WiFi.persistent(false);
 WiFi.mode(WIFI_SSID[0]?WIFI_AP_STA:WIFI_AP);
 WiFi.setSleep(false);
 apConfigured=WiFi.softAP("MedBox", "medbox-setup", 6, 0, 4);
 apConfigured=WiFi.softAPConfig(IPAddress(192,168,4,1),IPAddress(192,168,4,1),IPAddress(255,255,255,0))&&apConfigured;
 dns.stop();dns.start(53,"*",IPAddress(192,168,4,1));
 WiFi.setTxPower(WIFI_POWER_11dBm);
}

// USB-only maintenance transport to exercise the same HTTP routes without
// disconnecting the computer from its normal Wi-Fi. No separate control path.
static WiFiClient usbHttp;
static String usbLine;
static bool usbHttpActive;
static uint32_t usbHttpStarted;
static void usbConsole(){
 while(Serial.available()){
  char c=(char)Serial.read();if(c=='\r')continue;
  if(c!='\n'){if(usbLine.length()<600)usbLine+=c;else usbLine="";continue;}
  if(!usbHttpActive&&(usbLine.startsWith("GET /api/")||usbLine.startsWith("POST /api/"))){
   int space=usbLine.indexOf(' '),bodyStart=usbLine.indexOf(' ',space+1);
   String method=usbLine.substring(0,space),path=bodyStart<0?usbLine.substring(space+1):usbLine.substring(space+1,bodyStart);
   String body=bodyStart<0?"":usbLine.substring(bodyStart+1);
   if(usbHttp.connect(IPAddress(127,0,0,1),80)){
    usbHttp.print(method+" "+path+" HTTP/1.1\r\nHost: medbox\r\nConnection: close\r\nX-Medbox-Token: "+token+"\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: "+String(body.length())+"\r\n\r\n"+body);
    usbHttpActive=true;usbHttpStarted=millis();Serial.println("USBHTTP BEGIN");
   }else Serial.println("USBHTTP CONNECT FAILED");
  }
  usbLine="";
 }
 if(usbHttpActive){
  uint8_t b[256];int n=usbHttp.available();if(n>256)n=256;if(n>0){n=usbHttp.read(b,n);if(n>0)Serial.write(b,n);}
  if((!usbHttp.connected()&&!usbHttp.available())||(uint32_t)(millis()-usbHttpStarted)>5000){usbHttp.stop();usbHttpActive=false;Serial.println("\nUSBHTTP END");}
 }
}
static bool fresh(){return haveState&&(uint32_t)(millis()-lastState)<4000;}
static uint16_t revision(){return snapshot[4]|snapshot[5]<<8;}
static void sendFrame(uint8_t type,uint32_t seq,const uint8_t *data,unsigned n){
 uint8_t b[CT_FRAME];unsigned size=ct_frame(b,type,seq,data,n);bridge.write(b,size);
}
static String quote(const char *p,unsigned max){
 String s="\"";for(unsigned i=0;i<max&&p[i];i++){char c=p[i];if(c=='"'||c=='\\')s+='\\';if((uint8_t)c>=32)s+=c;}return s+'"';
}
static void json(int code,const String &s){server.sendHeader("Cache-Control","no-store");server.send(code,"application/json",s);}
static bool number(const String &s,unsigned limit,unsigned &out){
 if(!s.length()||s.length()>10)return false;uint64_t v=0;
 for(unsigned i=0;i<s.length();i++){if(s[i]<'0'||s[i]>'9')return false;v=v*10+(s[i]-'0');if(v>limit)return false;}
 out=(unsigned)v;return true;
}
static bool authorize(){
 if(server.header("X-Medbox-Token")!=token){json(403,"{\"error\":\"Reload this page before editing\"}");return false;}
 if(!fresh()){json(503,"{\"error\":\"STM32 offline; changes disabled\"}");return false;}
 if(pending){json(409,"{\"error\":\"A change is still being confirmed\"}");return false;}
 unsigned expected;if(!number(server.arg("revision"),65535,expected)||expected!=revision()){json(409,"{\"error\":\"Configuration changed. Reload the editor\"}");return false;}
 return true;
}
static void command(uint8_t type,uint8_t *p,unsigned n){
 p[0]=snapshot[4];p[1]=snapshot[5];operation=++sequence;if(!operation)operation=++sequence;
 pendingSize=ct_frame(pendingFrame,type,operation,p,n);bridge.write(pendingFrame,pendingSize);
 pending=true;attempts=1;lastSend=millis();outcome=-1;
 json(202,String("{\"operation\":")+operation+'}');
}
static void state(){
 String s="{\"online\":";s+=fresh()?"true":"false";s+=",\"token\":"+quote(token.c_str(),32);
 s+=",\"ageMs\":"+String(haveState?(uint32_t)(millis()-lastState):0)+",\"revision\":"+String(revision());
 if(haveState){
  s+=",\"epoch\":"+String(ct_u32(snapshot))+",\"flags\":"+String(snapshot[6])+",\"active\":"+String(snapshot[7]);
  s+=",\"sound\":"+String(snapshot[8])+",\"brightness\":"+String(snapshot[9])+",\"duration\":"+String(snapshot[10]|snapshot[11]<<8);
  s+=",\"uptime\":"+String(ct_u32(snapshot+180))+",\"rtcErrors\":"+String(ct_u32(snapshot+184))+",\"oledErrors\":"+String(ct_u32(snapshot+188))+",\"clockProgress\":"+String(snapshot[193]);
  s+=",\"slots\":[";
  for(unsigned i=0;i<8;i++){const uint8_t *p=snapshot+12+21*i;if(i)s+=',';
   s+="{\"name\":"+quote((const char*)p,16)+",\"hour\":"+String(p[17])+",\"minute\":"+String(p[18])+",\"days\":"+String(p[19])+",\"enabled\":"+String(p[20])+'}';}
  s+=']';
 }
 s+=",\"history\":[";bool first=true;
 if(haveState)for(unsigned i=0;i<snapshot[192]&&i<32;i++)if(eventValid[i]){const uint8_t *p=events[i];if(!first)s+=',';first=false;
  s+="{\"type\":"+String(p[1])+",\"mask\":"+String(p[2])+",\"dated\":"+String(p[3])+",\"epoch\":"+String(ct_u32(p+4))+",\"due\":"+String(ct_u32(p+8))+",\"uptime\":"+String(ct_u32(p+12))+'}';}
 s+="],\"operation\":"+String(operation)+",\"pending\":"+(pending?"true":"false")+",\"result\":"+String(outcome)+",\"resultRevision\":"+String(resultRevision)+",\"badFrames\":"+String(badframes)+'}';json(200,s);
}
static void metrics(){
 String s="{\"available\":";s+=haveMetrics?"true":"false";s+=",\"online\":";s+=fresh()?"true":"false";
 if(haveMetrics){
  s+=",\"generation\":"+String(metricGeneration())+",\"recordCount\":"+String(metricHeader[6])+",\"intervalCount\":"+String(metricHeader[7])+",\"trackingSince\":"+String(ct_u32(metricHeader+8))+",\"totalOpens\":"+String(ct_u32(metricHeader+12))+",\"extraOpens\":"+String(ct_u32(metricHeader+16))+",\"boundary\":"+String(ct_u32(metricHeader+20))+",\"intervalOpens\":"+String(ct_u32(metricHeader+24))+",\"intervalExtra\":"+String(ct_u32(metricHeader+28))+",\"intervalPartial\":"+String(metricHeader[32])+",\"untrustedOpens\":"+String(ct_u32(metricHeader+33))+",\"recordsEvicted\":"+String(ct_u32(metricHeader+37));
  s+=",\"slots\":[";
  for(unsigned i=0;i<8;i++){if(i)s+=',';if(!slotValid[i]){s+="null";continue;}const uint8_t *p=slotStats[i];
   s+="{\"slot\":"+String(i)+",\"responseTotalMs\":"+String((double)little64(p),0)+",\"ringTotalMs\":"+String((double)little64(p+8),0)+",\"due\":"+String(ct_u32(p+16))+",\"opened\":"+String(ct_u32(p+20))+",\"taken\":"+String(ct_u32(p+24))+",\"notTaken\":"+String(ct_u32(p+28))+",\"responseCount\":"+String(ct_u32(p+32))+",\"ringCount\":"+String(ct_u32(p+36))+"}";}
  s+=']';
 }s+='}';json(200,s);
}
static void doses(){
 unsigned offset=0,limit=8;
 if((server.hasArg("offset")&&!number(server.arg("offset"),23,offset))||(server.hasArg("limit")&&!number(server.arg("limit"),8,limit))||!limit){json(400,"{\"error\":\"Invalid page\"}");return;}
 String s="{\"generation\":"+String(metricGeneration())+",\"available\":"+(haveMetrics?"true":"false")+",\"records\":[";bool first=true;unsigned missing=0;
 if(haveMetrics)for(unsigned i=offset;i<offset+limit&&i<metricHeader[6];i++){
  if(!doseValid[i]){missing++;continue;}const uint8_t *p=doseRecords[i];if(!first)s+=',';first=false;
  uint32_t ring=ct_u32(p+16);if((p[43]&4)&&fresh()){uint32_t elapsed=millis()-doseReceived[i];ring=0xffffffffu-ring<elapsed?0xffffffffu:ring+elapsed;}
  s+="{\"id\":"+String(ct_u32(p))+",\"due\":"+String(ct_u32(p+4))+",\"opened\":"+String(ct_u32(p+8))+",\"responseMs\":"+String(ct_u32(p+12))+",\"ringMs\":"+String(ring)+",\"reportedAt\":"+String(ct_u32(p+20))+",\"name\":"+quote((const char*)p+24,16)+",\"slot\":"+String(p[41])+",\"report\":"+String(p[42])+",\"flags\":"+String(p[43])+",\"intervalOpens\":"+String(p[44]|p[45]<<8)+",\"intervalExtra\":"+String(p[46]|p[47]<<8)+"}";
 }
 s+="],\"missing\":"+String(missing)+"}";json(200,s);
}
static void intervals(){
 String s="{\"generation\":"+String(metricGeneration())+",\"intervals\":[";bool first=true;unsigned missing=0;
 if(haveMetrics)for(unsigned i=0;i<metricHeader[7];i++){if(!intervalValid[i]){missing++;continue;}const uint8_t *p=openIntervals[i];if(!first)s+=',';first=false;
  s+="{\"start\":"+String(ct_u32(p))+",\"end\":"+String(ct_u32(p+4))+",\"opens\":"+String(ct_u32(p+8))+",\"extra\":"+String(ct_u32(p+12))+",\"partial\":"+String(ct_u32(p+16))+"}";}
 s+="],\"missing\":"+String(missing)+"}";json(200,s);
}
static void report(){
 if(!authorize())return;unsigned id,value,previous;
 if(!number(server.arg("id"),0xffffffffu,id)||!id||!number(server.arg("report"),2,value)||!number(server.arg("previous"),2,previous)){json(400,"{\"error\":\"Invalid report\"}");return;}
 uint8_t p[8]={0};ct_put32(p+2,id);p[6]=(uint8_t)value;p[7]=(uint8_t)previous;command(CT_REPORT,p,8);
}
static void slot(){
 if(!authorize())return;unsigned index,hour,minute,days,enabled;
 String name=server.arg("name");name.trim();name.toUpperCase();
 bool valid=name.length()>0&&name.length()<=16;
 for(unsigned i=0;i<name.length();i++)if(name[i]!=' '&&(name[i]<'A'||name[i]>'Z')&&(name[i]<'0'||name[i]>'9'))valid=false;
 if(!valid||!number(server.arg("slot"),7,index)||!number(server.arg("hour"),23,hour)||!number(server.arg("minute"),59,minute)||!number(server.arg("days"),127,days)||!days||!number(server.arg("enabled"),1,enabled)){json(400,"{\"error\":\"Use a 1-16 character name, valid time, and at least one weekday\"}");return;}
 uint8_t p[24]={0};p[2]=(uint8_t)index;memcpy(p+3,name.c_str(),name.length());p[20]=(uint8_t)hour;p[21]=(uint8_t)minute;p[22]=(uint8_t)days;p[23]=(uint8_t)enabled;command(CT_SLOT,p,sizeof p);
}
static void settings(){
 if(!authorize())return;unsigned sound,brightness,duration;
 if(!number(server.arg("sound"),1,sound)||!number(server.arg("brightness"),10,brightness)||!brightness||!number(server.arg("duration"),2000,duration)||duration<100||duration%100){json(400,"{\"error\":\"Invalid settings\"}");return;}
 uint8_t p[6]={0,0,(uint8_t)sound,(uint8_t)brightness,(uint8_t)duration,(uint8_t)(duration>>8)};command(CT_SETTINGS,p,6);
}
static void clockSet(){if(!authorize())return;unsigned epoch;
 if(!number(server.arg("epoch"),3155759999u,epoch)){json(400,"{\"error\":\"Clock must be in years 2000-2099\"}");return;}
 uint8_t p[6]={0};ct_put32(p+2,epoch);command(CT_CLOCK,p,6);
}
void setup(){
 Serial.begin(115200);bridge.setRxBufferSize(1024);bridge.begin(BRIDGE_BAUD,SERIAL_8N1,BRIDGE_RX,BRIDGE_TX);
 sequence=esp_random();char key[17];snprintf(key,sizeof key,"%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random());token=key;
 WiFi.onEvent([](WiFiEvent_t e){if(e==ARDUINO_EVENT_WIFI_AP_START)apStarts++;if(e==ARDUINO_EVENT_WIFI_AP_STOP)apStops++;if(e==ARDUINO_EVENT_WIFI_AP_STACONNECTED)stationJoins++;});
 startAccessPoint();if(WIFI_SSID[0])WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
 const char *headers[]={"X-Medbox-Token"};server.collectHeaders(headers,1);
 auto dashboard=[](){pageHits++;server.sendHeader("Cache-Control","no-store");server.sendHeader("Content-Security-Policy","default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; frame-ancestors 'none'; object-src 'none'");server.send_P(200,"text/html; charset=utf-8",WEB_UI);};
 server.on("/",HTTP_GET,dashboard);
 server.on("/generate_204",HTTP_GET,dashboard);server.on("/hotspot-detect.html",HTTP_GET,dashboard);server.on("/connecttest.txt",HTTP_GET,dashboard);

 server.on("/api/network",HTTP_GET,[]{
  wifi_config_t config={};esp_err_t result=esp_wifi_get_config(WIFI_IF_AP,&config);
  String s="{\"configured\":";s+=apConfigured?"true":"false";
  s+=",\"mode\":"+String((unsigned)WiFi.getMode())+",\"ssid\":"+quote((const char*)config.ap.ssid,32)+",\"hidden\":"+String(config.ap.ssid_hidden)+",\"channel\":"+String(config.ap.channel)+",\"clients\":"+String(WiFi.softAPgetStationNum())+",\"starts\":"+String(apStarts)+",\"stops\":"+String(apStops)+",\"joins\":"+String(stationJoins)+",\"configError\":"+String(result)+",\"txPowerQuarterDbm\":"+String((int)WiFi.getTxPower())+",\"ip\":"+quote(WiFi.softAPIP().toString().c_str(),32)+",\"heap\":"+String(ESP.getFreeHeap())+"}";
  s.remove(s.length()-1);
  s+=",\"pageHits\":"+String(pageHits)+",\"stations\":[";
  wifi_sta_list_t stations={};esp_netif_sta_list_t addresses={};esp_wifi_ap_get_sta_list(&stations);esp_netif_get_sta_list(&stations,&addresses);
  for(int i=0;i<addresses.num;i++){if(i)s+=',';s+="{\"ip\":"+quote(IPAddress(addresses.sta[i].ip.addr).toString().c_str(),32)+",\"rssi\":"+String((int)stations.sta[i].rssi)+"}";}
  s+="]}";json(200,s);
 });
 server.on("/api/analytics",HTTP_GET,metrics);server.on("/api/doses",HTTP_GET,doses);server.on("/api/intervals",HTTP_GET,intervals);server.on("/api/report",HTTP_POST,report);
 server.on("/api/state",HTTP_GET,state);server.on("/api/slot",HTTP_POST,slot);server.on("/api/settings",HTTP_POST,settings);server.on("/api/clock",HTTP_POST,clockSet);
 server.on("/api/test",HTTP_POST,[]{if(authorize()){uint8_t p[2]={0};command(CT_TEST,p,2);}});
 server.onNotFound([]{if(server.method()==HTTP_GET&&!server.uri().startsWith("/api/")){server.sendHeader("Location","http://192.168.4.1/");server.send(302,"text/plain","Open http://192.168.4.1/");}else json(404,"{\"error\":\"Not found\"}");});server.begin();
 Serial.printf("MedBox: join WiFi MedBox, password medbox-setup, open http://%s\n",WiFi.softAPIP().toString().c_str());
}
void loop(){
 uint32_t now=millis();while(bridge.available()){
  int r=ct_feed(&parser,(uint8_t)bridge.read(),now);if(r<0)badframes++;if(r<=0)continue;
  unsigned type=parser.bytes[3],n=parser.bytes[4];uint32_t seq=ct_u32(parser.bytes+5);const uint8_t *p=parser.bytes+9;
  if(type==CT_STATE&&n==sizeof snapshot){
   bool restart=haveState&&ct_u32(p+180)<ct_u32(snapshot+180);
   if(restart)resetMetrics();
   if(!haveState||restart||p[192]!=snapshot[192]){memset(eventValid,0,sizeof eventValid);historyIndex=0;}
   memcpy(snapshot,p,n);haveState=true;lastState=now;
  }else if(type==CT_RESULT&&n==3&&pending&&seq==operation){outcome=p[0];resultRevision=p[1]|p[2]<<8;pending=false;}
  else if(type==CT_EVENT&&(n==16||n==20)&&p[0]<32&&(!haveMetrics||(n==20&&ct_u32(p+16)==historyGeneration))){memcpy(events[p[0]],p,16);eventValid[p[0]]=true;}
  else if(type==CT_ANALYTICS&&n>=6){
   if(p[0]==0&&n==45&&p[6]<=24&&p[7]<=8){
    uint32_t gen=ct_u32(p+2),history=ct_u32(p+41);
    if(!haveMetrics||gen!=metricGeneration()){memset(slotValid,0,sizeof slotValid);memset(doseValid,0,sizeof doseValid);memset(intervalValid,0,sizeof intervalValid);metricCursor=0;}
    if(history!=historyGeneration){memset(eventValid,0,sizeof eventValid);historyIndex=0;historyGeneration=history;}
    memcpy(metricHeader,p,45);haveMetrics=true;
   }else if(haveMetrics&&ct_u32(p+2)==metricGeneration()){
    if(p[0]==1&&p[1]<8&&n==46){memcpy(slotStats[p[1]],p+6,40);slotValid[p[1]]=true;}
    if(p[0]==2&&p[1]<24&&n==54){memcpy(doseRecords[p[1]],p+6,48);doseValid[p[1]]=true;doseReceived[p[1]]=now;}
    if(p[0]==3&&p[1]<8&&n==26){memcpy(openIntervals[p[1]],p+6,20);intervalValid[p[1]]=true;}
   }
  }
 }
 if((uint32_t)(now-heartbeat)>=1000){heartbeat=now;sendFrame(CT_HELLO,0,nullptr,0);}
 if(pending&&(uint32_t)(now-lastSend)>=2000){
  if(attempts>=5){pending=false;outcome=6;}else{bridge.write(pendingFrame,pendingSize);lastSend=now;attempts++;}
 }
 if(fresh()&&!pending&&snapshot[192]&&(uint32_t)(now-lastHistory)>=120){lastHistory=now;uint8_t b[5]={(uint8_t)(historyIndex++%snapshot[192])};ct_put32(b+1,historyGeneration);sendFrame(CT_HISTORY,0,b,haveMetrics?5:1);}
 if(fresh()&&!pending&&(uint32_t)(now-metricPoll)>=70){metricPoll=now;
  if(!haveMetrics)sendFrame(CT_ANALYTICS,0,nullptr,0);
  else{unsigned cursor=metricCursor++%40,mode=cursor<8?1:cursor<32?2:3,index=cursor<8?cursor:cursor<32?cursor-8:cursor-32;
   if((mode==1)||(mode==2&&index<metricHeader[6])||(mode==3&&index<metricHeader[7])){uint8_t b[6]={(uint8_t)mode,(uint8_t)index};ct_put32(b+2,metricGeneration());sendFrame(CT_ANALYTICS,0,b,6);}}
 }
 static uint32_t apCheck;
 if((uint32_t)(now-apCheck)>10000){apCheck=now;if(!apConfigured||!(WiFi.getMode()&WIFI_AP))startAccessPoint();}
 dns.processNextRequest();usbConsole();server.handleClient();delay(1);
}
