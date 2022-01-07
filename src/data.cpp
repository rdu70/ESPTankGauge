#include "data.h"

Data::Data() {
  Wifi_RSSI = -100;
  Uptime = 0;

  TL.Updated = false;
  TL.cm = 0;
  TL.l = 0;
  TL.pct = 0;
  TL.err = false;
  TL.SendMQTT = false;

  DT.day = 0;  DT.month = 0;  DT.year = 0;
  DT.dayofyear = 0;

  memset(&CFG, 0, sizeof(CFG));
  memset(&History, 0, sizeof(History));
}

void Data::setTL(int cm, int l, int pct) {
  Data::TL.Updated = false;
  Data::TL.cm = cm;
  Data::TL.l = l;
  Data::TL.pct = pct;
  Data::TL.err = false;
  Data::TL.Updated = true;
  Data::TL.SendMQTT = true;
}

void Data::setTLerr() {
  Data::TL.err = true;
  Data::TL.Updated = true;
}

bool Data::isUpdatedTL() {
  Data::TL.Updated = false;
  return Data::TL.Updated;
}

void Data::dspUptime(char *TmpSt) {
  int uptime = Data::Uptime;
  int days = uptime / 86400;
  int hours = (uptime - 86400 * days) / 3600;
  int minutes = (uptime - 86400 * days - 3600 * hours) / 60;
  int seconds = (uptime - 86400 * days - 3600 * hours - 60 * minutes);
  sprintf(TmpSt, "%id %02ih %02im %02is", days, hours, minutes, seconds);
}

const char *Data::SetWithDefault(const char *Value, const char *DefaultValue) {
  if (strlen(Value) > 0) return Value;
  else return DefaultValue;
}

void Data::LoadConfig() {
  bool configreaded = false;
  
  // Set defaults if cfg not found
  strcpy(CFG.mqtt_srv_host, MQTT_IP);
  strcpy(CFG.ntp_srv_host, "pool.ntp.org");

  // Read config
  File cfg = LittleFS.open(CFG_FILE, "r");
  if (cfg) {
    doc.clear();
    DeserializationError error = deserializeJson(doc, cfg);
    if (!error) configreaded = true;
    cfg.close();
  }

  strlcpy(CFG.mqtt_srv_host, configreaded ? SetWithDefault(doc["mqtt_srv_host"].as<char *>(), MQTT_IP) : MQTT_IP, sizeof(CFG.mqtt_srv_host));
  strlcpy(CFG.ntp_srv_host, configreaded ? SetWithDefault(doc["ntp_srv_host"].as<char *>(), "pool.ntp.org") : "pool.ntp.org", sizeof(CFG.mqtt_srv_host));

  if (!configreaded) {
    SaveConfig();
  }
}

void Data::SaveConfig() {
  File cfg = LittleFS.open(CFG_FILE, "w");
  if (cfg) {
    doc.clear();
    doc["mqtt_srv_host"] = CFG.mqtt_srv_host;
    doc["ntp_srv_host"] = CFG.ntp_srv_host;
    serializeJson(doc, cfg);
    cfg.close();
  }
}

void Data::ReadHistory() {
  int days[] = { 31, 28, 31, 30, 31, 30, 
               31, 31, 30, 31, 30, 31 }; 

  strcpy(TmpSt, NTP.getTimeDateString());
  int year = (TmpSt[6]-48)*1000+(TmpSt[7]-48)*100+(TmpSt[8]-48)*10+(TmpSt[9]-48); 
  int month = (TmpSt[3]-48)*10 + (TmpSt[4]-48); 
  //int day = (TmpSt[0]-48)*10 + (TmpSt[1]-48);
  int readmonth = month;
  int readyear = year;
  memset(&History, 0, sizeof(History));
  for (int i = 0; i<11; i++){
    sprintf(fname, "/%04i%02i.json", readyear, readmonth);
    f = LittleFS.open(fname ,"r");
    if (f) {
      doc.clear();
      DeserializationError error = deserializeJson(doc, f);
      if (!error) {
        for (int d = 1; d < days[readmonth-1]; d++) {
          sprintf(TmpSt, "%i", d);
          int value = doc[TmpSt];
          if (value > 0) History[11-i][(d-1) / 3] = value;
          yield();
        }
      }
    }
    f.close();

    readmonth--;
    if (readmonth <= 0) {
      readmonth += 12;
      readyear--;
    }
  }
}

void Data::WriteHistory() {

  doc.clear();
  sprintf(fname, "/%04i%02i.json", DT.year, DT.month);
  f = LittleFS.open(fname ,"r");
  if (f) {
    DeserializationError error = deserializeJson(doc, f);
    if (!error) {
    }
  }
  f.close();

  sprintf(TmpSt, "%i", DT.day);
  doc[TmpSt] = TL.l;
  f = LittleFS.open(fname ,"w");
  serializeJson(doc, f);
  f.close();     
}