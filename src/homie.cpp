#include "homie.h"

void HomieClient::setup(Data *datastore) {
    char TmpSt[255];

    lastms = millis();

    data = datastore;

    refresh = HOMIE_REFRESH;

    homie.strFriendlyName="Fioul Tank";
    homie.strID="fioultank";
    homie.strID.toLowerCase();

    homie.strMqttServerIP=data->CFG.mqtt_srv_host;
    homie.strMqttUserName=MQTT_USER;
    homie.strMqttPassword=MQTT_PASS;

    HomieNode * pEnvNode=homie.NewNode();

    pEnvNode->strID="level";
    pEnvNode->strFriendlyName="Fioul Tank Level";
//		pNode->strType="customtype";

    HomieProperty * pProp;


    pPropEnvLevel_cm=pProp=pEnvNode->NewProperty();
    pProp->strFriendlyName="Niveau (cm)";
    pProp->strID="level_cm";
    pProp->SetUnit("cm");
    pProp->SetRetained(true);
    pProp->datatype=homieInt;
    pProp->strFormat="0:125";
    sprintf(TmpSt, "%i", data->TL.cm);
    pProp->SetValue(String(TmpSt));

    pPropEnvLevel_l=pProp=pEnvNode->NewProperty();
    pProp->strFriendlyName="Niveau (l)";
    pProp->strID="level_l";
    pProp->SetUnit("l");
    pProp->SetRetained(true);
    pProp->datatype=homieInt;
    pProp->strFormat="0:2500";
    sprintf(TmpSt, "%i", data->TL.l);
    pProp->SetValue(String(TmpSt));

    pPropEnvLevel_pct=pProp=pEnvNode->NewProperty();
    pProp->strFriendlyName="Niveau (pct)";
    pProp->strID="level_pct";
    pProp->SetUnit("%");
    pProp->SetRetained(true);
    pProp->datatype=homieInt;
    pProp->strFormat="0:100";
    sprintf(TmpSt, "%i", data->TL.pct);
    pProp->SetValue(String(TmpSt));

	homie.Init();
}

void HomieClient::handle() {
    char TmpSt[255];
    homie.Loop();
    data->HomieConnected = homie.IsConnected();

    if ((millis() - lastms) > HOMIE_UPDATE*1000) {
        if (refresh > 0) refresh--;
        if(data->HomieConnected)
        {
            lastms = millis();
            // Send data if new data available or refresh forced time elapsed
            if ((data->TL.SendMQTT) || (refresh == 0)) {
                data->TL.SendMQTT = false;
                refresh = HOMIE_REFRESH;
                sprintf(TmpSt, "%i", data->TL.cm);
                pPropEnvLevel_cm->SetValue(String(TmpSt));
                sprintf(TmpSt, "%i", data->TL.l);
                pPropEnvLevel_l->SetValue(String(TmpSt));
                sprintf(TmpSt, "%i", data->TL.pct);
                pPropEnvLevel_pct->SetValue(String(TmpSt));
            }
       }
    }
}

