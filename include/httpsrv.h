#ifndef _HTTPSRV_H
#define _HTTPSRV_H

#include <Arduino.h>

#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

#include "data.h"

#define BS_GREEN 1
#define BS_YELLOW 2
#define BS_RED 3
#define BS_GREY 4
#define BS_BLUE 5

#define PAGE_ROOT 0
#define PAGE_HISTORY 1
#define PAGE_UPLOAD 2
#define PAGE_CONFIG 3

class HTTPSrv {
  public:
    void setup(Data *datastore);
    void begin();
    void handle();

    Data *data;


  private:
    void handleRoot();
    void handleConfig();
    void handleSave();
    void handle200();
    void handleGetUpload();
    void handleFileUpload();
    void handleNotFound();
    void handleHistory();

    String HTML_Header(int page, bool Bootstrap, bool Chartjs);
    void HTML_Footer(String message);

    String getContentType(String filename);
    bool handleFileRead(String path);

    String bootstrapColor(int Color);
    String bool2RowColor(boolean value);
    String int2RowColor(int value, int low, int warn);
    String float2RowColor(float value, float warn, float error);

    ESP8266WebServer *server;
    File fsUploadFile;

};

#endif  /* _HTTPSRV_H */