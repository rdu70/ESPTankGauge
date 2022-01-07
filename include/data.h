#ifndef _DATA_H
#define _DATA_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ESPNtpClient.h>

#include "config.h"

#define FQDN_LEN 64

// Data Class
class Data {
  public:

    int Wifi_RSSI;
    unsigned long Uptime;
    bool HomieConnected;
    StaticJsonDocument<2048> doc;
    int History[12][10];

    struct ConfigStruct {
      char mqtt_srv_host[FQDN_LEN];
      char ntp_srv_host[FQDN_LEN];
    } CFG;

    struct DateStruct {
      int dayofyear;
      int day;
      int month;
      int year;
    } DT;

    struct TankLevelStruct {
        int cm;
        int l;
        int pct;
        bool err;
        bool Updated;
        bool SendMQTT;
      } TL;

    Data();

    void setTL(int cm, int l, int pct);
    void setTLerr();
    bool isUpdatedTL();
    void dspUptime(char *TmpSt);

    // Configuration management
    void LoadConfig();
    void SaveConfig();

    // Read/Write history file (json format)
    void ReadHistory();
    void WriteHistory();

  private:
    File f;
    char fname[64];
    char TmpSt[32];

    const char *SetWithDefault(const char *Value, const char *DefaultValue);
 
};

#endif  /* _DATA_H */