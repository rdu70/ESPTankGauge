#ifndef _HOMIE_H
#define _HOMIE_H

#include <Arduino.h>

#include "config.h"
#include "data.h"

#include <WiFiClient.h>
#include "LeifHomieLib.h"



class HomieClient {
  public:
    void setup(Data *datastore);
    void begin();
    void handle();

    Data *data;

  private:
    int lastms;
    int refresh;
    HomieDevice homie;
    HomieProperty * pPropEnvLevel_cm=NULL;
    HomieProperty * pPropEnvLevel_l=NULL;
    HomieProperty * pPropEnvLevel_pct=NULL;

};

#endif  /* _HOMIE_H */