#include <Arduino.h>
#include <Wire.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#include <ESPNtpClient.h>
#include <LittleFS.h>

#include "config.h"
#include "data.h"
#include "dsp.h"
#include "homie.h"
#include "httpsrv.h"
#include "ota.h"

Data data;
HTTPSrv MyWebSrv;
HomieClient MyHomieClient;

unsigned long ms_time;
unsigned long ms_ntp;
unsigned long ms_wifi;
unsigned long ms_serial;
unsigned long ms_sensor;
unsigned long ms_dsp;
unsigned long ms_debounce;
unsigned long ms_starthomie;
bool homie_started;

bool btn_pressed;
bool btn_edge;

int screen;
int lastscreen;
int Color;
int saver;

int lastday;

bool sensor_valid;
int SensorLevel;
int SensorMean[SENSOR_AVG];
int ReadErrorCount;
bool FirstRead = true;

bool TankLevel_updated;
bool TankLevel_valid;
float TankLevel_cm;
float TankLevel_cm_last;
float TankLevel_cm_avg;
int TankLevel_l;
int TankLevel_pct;

bool update_dsp;
bool update_mqtt;

char Txt [255];
int rssi;
int rssi_last;

int days[] = { 31, 28, 31, 30, 31, 30, 
               31, 31, 30, 31, 30, 31 }; 
const char *MonthsLetter[12] = {"J", "F", "M", "A", "M", "J", "J", "A", "S", "O", "N", "D"};
  
// Function to return the day number 
// of the year for the given date
// Format dd/mm/yyyy 
int dayOfYear(int day, int month, int year) 
{ 
  // If current year is a leap year and the date 
  // given is after the 28th of February then 
  // it must include the 29th February 
  if (month > 2 && year % 4 == 0 
      && (year % 100 != 0 || year % 400 == 0)) ++day; 

  // Add the days in the previous months 
  if (month > 1) for (int m = 0; m<month-1; m++) day += days[m];
  return day; 
} 


int read_sensor(){
  // Start ultrasonic probe
  Wire.beginTransmission(112);
  Wire.write(byte(81));
  Wire.endTransmission();

  // Wait until done (65ms per constructor)
  delay(70);

  // Read result
  int Reading = -1;
  Wire.requestFrom(112, 2);
  if (2 <= Wire.available()){
    // Read I2C
    Reading = Wire.read();
    Reading = Reading << 8;
    Reading |= Wire.read();
  }
  
  if (Reading < SENSOR_MIN) Reading = -1;
  else if (Reading > SENSOR_MAX) Reading = -1;
  else Reading = TANK_REF - Reading;

  return Reading;
}

float convert_capacity(float Reading){
  
  float TankLevel = (Reading) * TANK_L_PER_CM;
  if (TankLevel < 0) TankLevel = 0;
  if (TankLevel > TANK_CAPACITY) TankLevel = TANK_CAPACITY;
  if (Reading < 0) TankLevel = -1;

  return TankLevel;
}


void dsp_screen(){
  bool screenchanged = (screen != lastscreen);   // Is screen number changed ?
  if (screenchanged) saver = DSP_SAVER_TIME;     // reload saver for next time
  switch (screen){
    case 0 : {   // screen saver
      if (screenchanged){
        dsp_clear();
        dsp_backlight(20);
        dsp_setfont(0);   
      }
      int x = 50;
      if (TankLevel_valid) {
        sprintf(Txt, "%i", TankLevel_l);
        if (TankLevel_l < 1000) x = 80;
        if (TankLevel_l < 100) x = 105;        
      } else {
        x = 80;
        strcpy(Txt, "----");
      }
      dsp_clear();
      dsp_setfont(123);
      dsp_colorrgb(random(128)+92, random(128)+92, random(128)+92);
      dsp_print(random(0, x), random(32,128), Txt);    
    } break;

    case 1 : {   // main screen
      if (screenchanged){
        dsp_clear();
        dsp_backlight(90);
        dsp_setfont(0);
        dsp_color(DSP_COL_WHITE);
        dsp_print(5,10, "CAPACITE CUVE FIOUL");
        update_dsp = true;
      }

      if ((!TankLevel_valid) && (!sensor_valid)) {
        dsp_setfont(18);
        dsp_color(DSP_COL_RED);
        dsp_print(5, 40, " Err. mesure ");
        dsp_color(DSP_COL_BLACK);
        dsp_rectangle(0, 41, 120, 85, true);
        dsp_setfont(123);
        dsp_color(DSP_COL_WHITE);
        dsp_print(25, 85, "----");
        data.setTLerr();
      }

      if ((TankLevel_valid) && (update_dsp)) {
      
        if (TankLevel_l >= 0) {
          dsp_setfont(18);
          sprintf(Txt, "%3i cm %3i %%", int(TankLevel_cm), TankLevel_pct);
          dsp_color(DSP_COL_WHITE);
          dsp_print(5, 40, Txt);
          int TL_int = int(TankLevel_l);
          sprintf(Txt, "%i ", TL_int);
          if (TankLevel_pct <= 10) Color = DSP_COL_RED;
          else if (TankLevel_pct > 20) Color = DSP_COL_GREEN;
          else Color = DSP_COL_ORANGE;
          int X = 5;
          if (TL_int < 1000) X = 20;
          if (TL_int < 100) X = 40; 
          if (TL_int < 10) X = 55;
          dsp_color(DSP_COL_BLACK);
          dsp_rectangle(0, 41, 120, 85, true);
          dsp_setfont(123);
          dsp_color(Color);
          dsp_print(X, 85, Txt);
          dsp_gauge2(135, 20, TankLevel_pct);
          data.setTL(TankLevel_cm_avg, TankLevel_l, TankLevel_pct);
        }
        update_dsp = false;
      }
    } break;

    case 2 : {   // graph screen
      if (screenchanged){
        dsp_clear();
        dsp_backlight(90);
        dsp_setfont(0);
        dsp_color(DSP_COL_WHITE);
        dsp_print(5,10, " HIST. NIVEAU CUVE");
        const int ox = 15;
        const int oy = 112;
        const int scaley = 100;
        dsp_line(ox, oy, ox, oy - scaley);
        dsp_line(ox, oy, ox + 120, oy);
        dsp_setfont(6);
        for (int m = 0; m < 12; m++) { 
          dsp_line(ox + 10 * m, oy, ox + 10 * m, oy + 4);
          sprintf(Txt, "%s", MonthsLetter[(m + data.DT.month)%12]);
          dsp_print(ox + 10 * m + 2, oy + 8, Txt);
        }
        for (int i = 0; i < 5; i++) dsp_line(ox, oy - (i * scaley / 4), ox - 5 + (i%2), oy - (i * scaley / 4));
        // Do calculation
        data.ReadHistory();
        // Display graph
        dsp_color(DSP_COL_GREEN);
        for (int m = 0; m < 12; m++)
          for (int d = 0; d < 10; d++) {
            int x = ox + 1 + 10 * m + d;
            int dy = data.History[m][d] * scaley / TANK_CAPACITY;
            dsp_line(x, oy - 1, x, oy - 1 - dy);
          }
     }
    } break;

    case 3 : {   // debug screen
      if (screenchanged){
        dsp_clear();
        dsp_backlight(90);
        dsp_setfont(0);
        dsp_color(DSP_COL_WHITE);
        dsp_print(5,10, "===== I N F O =====");
      }
        dsp_color(DSP_COL_WHITE);
        dsp_setfont(6);
        int y = 20; int dy = 8;
        char St[50]; data.dspUptime(St);
        sprintf(Txt, "Release: %s  ", RELEASE);
        dsp_print(0, y, Txt); y += dy + 4;
        sprintf(Txt, "Uptime : %s  ", St);
        dsp_print(0, y, Txt); y += dy;
        sprintf(Txt, "NTPtime: %s  ", (NTP.syncStatus() == 0) ? NTP.getTimeDateString() : "Not synced           ");
        dsp_print(0, y, Txt); y += dy;
        sprintf(Txt, "DayOfYr: %i (%04i%02i%02i)  ", data.DT.dayofyear, data.DT.year, data.DT.month, data.DT.day);
        dsp_print(0, y, Txt); y += dy + 4;
        sprintf(Txt, "Sensor : Level:%3i cm   Mean:%4.1f cm  ", SensorLevel, TankLevel_cm_avg);
        dsp_print(0, y, Txt); y += dy;
        sprintf(Txt, "Sensor : Valid:%5s (%i)      ", sensor_valid ? "True":"False", ReadErrorCount);
        dsp_print(0, y, Txt); y += dy;
        sprintf(Txt, "Level  : %4.1f cm  %3i %%  %4i l  ", TankLevel_cm, TankLevel_pct, TankLevel_l);
        dsp_print(0, y, Txt); y += dy + 4;
        sprintf(Txt, "WiFi   : %-4s  RSSI:%i  ", WiFi.isConnected() ? "Up":"Down", WiFi.RSSI());
        dsp_print(0, y, Txt); y += dy;
        sprintf(Txt, "         IP:%s   ", WiFi.localIP().toString().c_str());
        dsp_print(0, y, Txt); y += dy + 4;
        sprintf(Txt, "Homie  : %s  ", data.HomieConnected ? "Up":"Down");
        dsp_print(0, y, Txt); y += dy;
        sprintf(Txt, "         %s      ", data.TL.SendMQTT ? "Queued":"Queue empty");
        dsp_print(0, y, Txt); y += dy;
    } break;
  }
  
  // Status common to some screen except Debug and Screensaver
  if ((screen != 0) && (screen != 3)) {
    if (WiFi.status() == WL_CONNECTED){
      rssi = WiFi.RSSI();
      if (rssi != rssi_last){
        sprintf(Txt, "WiFi:UP  IP:%s  RSSI:%i  ", WiFi.localIP().toString().c_str(), rssi);
        dsp_setfont(6);
        dsp_color(DSP_COL_GREEN);
        dsp_print(10, 128, Txt);
        rssi_last = rssi;
      }
    }
    else{
      sprintf(Txt, "WiFi:Not connected                 ");
      dsp_setfont(6);
      dsp_color(DSP_COL_RED);
      dsp_print(10, 128, Txt);
      rssi_last = 9999;
    }
  }

  lastscreen = screen;

}

void setup() {
  // timer variables init
  ms_dsp = millis();
  ms_time = ms_dsp;
  ms_ntp = ms_time;
  ms_sensor = ms_dsp;
  ms_serial = ms_dsp;
  ms_wifi = ms_dsp;
  ms_debounce = ms_dsp;
  ms_starthomie = ms_dsp;

  // GPIO setup
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(BTN_PINGND, OUTPUT_OPEN_DRAIN);
  digitalWrite(BTN_PINGND, LOW);
  btn_pressed = false;
  btn_edge = false;

  // Start serial port
  Serial.begin(115200);

  // Start filesystem
  LittleFS.begin();
  if (!digitalRead(BTN_PIN)){
    Serial.println("Formating flash filesystem");
    LittleFS.format();
  }

  // Load config from filesystem
  data.LoadConfig();

  // Start I2C
  Wire.begin();

  // Start WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PSK);

  // Wait for display to start and init
  delay(500);
  dsp_init();
  screen = 1;
  lastscreen = -1;
  dsp_clear();

  // Init general vars
  TankLevel_valid = false;
  TankLevel_cm_last = 99999;
  TankLevel_updated = false;
  rssi_last = 9999;
  ReadErrorCount = 0;
  sensor_valid = false;
  saver = DSP_SAVER_TIME;
 
  update_dsp = false;
  update_mqtt = false;

  // Init web server
  MyWebSrv.setup(&data);
  MyWebSrv.begin();

  // Init MQTT Homie client
  homie_started = false;

  // Init OTA
  OTAsetup();
  OTAbegin();

  // Init NTP client
  NTP.setTimeZone(TZ_Europe_Brussels);
  NTP.begin(data.CFG.ntp_srv_host);
  lastday = -1;
 }

void loop() {
  // Get current timer
  int ms = millis();

  // Update uptime every second
  if ((ms - ms_time) > 1000) {
    ms_time += 1000;
    data.Uptime++;
  }

  // Every minutes if ntp available
  if (((ms - ms_ntp) > 60001) && (NTP.syncStatus() == syncd)) {
     ms_ntp = ms;
    strcpy(Txt, NTP.getTimeDateString());
    int year = (Txt[6]-48)*1000+(Txt[7]-48)*100+(Txt[8]-48)*10+(Txt[9]-48); 
    int month = (Txt[3]-48)*10 + (Txt[4]-48); 
    int day = (Txt[0]-48)*10 + (Txt[1]-48);
    //int hour = (Txt[11]-48)*10 + (Txt[12]-48);
    //int min = (Txt[14]-48)*10 + (Txt[15]-48);
    data.DT.dayofyear = dayOfYear(day, month, year);
    if (data.DT.dayofyear != lastday) {
      data.WriteHistory();
    }
   
    lastday = data.DT.dayofyear;
    data.DT.year = year;
    data.DT.month = month; 
    data.DT.day = day;

    yield();
  }


  // Manage button
  if (!digitalRead(BTN_PIN)){
    btn_pressed = true;
    ms_debounce = ms;
  } 
  if ((digitalRead(BTN_PIN)) && (ms - ms_debounce) > 50){
    btn_pressed = false;
    btn_edge = false;
  }
  if (btn_pressed && !btn_edge){
    btn_edge = true;
    screen ++;
    if (screen > 3) screen = 1;
  }

  // Manage sensor and calculations
  if ((ms-ms_sensor) > 1001){
    ms_sensor = ms;

    // Read sensor and update error counter
    SensorLevel = read_sensor();
    if ((SensorLevel) == -1) ReadErrorCount++;
    else ReadErrorCount = 0;
    sensor_valid = (ReadErrorCount < 10);
    if (ReadErrorCount > 60){
      TankLevel_valid = false;
      data.setTLerr();
    }

    // If first correct reading, init the mean array
    if ((FirstRead) && (SensorLevel >= 0)){
      FirstRead = false;
      for (int i = 0; i<SENSOR_AVG; i++) SensorMean[i]=SensorLevel;
    }

    // if subsequent correct readings, update mean array
    if ((!FirstRead) && (SensorLevel >= 0)){
      for (int i = SENSOR_AVG-1; i>0; i--) SensorMean[i]=SensorMean[i-1];
      SensorMean[0]=SensorLevel;
      // Calc new average
      TankLevel_cm_avg = 0;
      for (int i = 0; i<SENSOR_AVG; i++) TankLevel_cm_avg+=SensorMean[i];
      TankLevel_cm_avg /= SENSOR_AVG;
      //Serial.println(TankLevel_cm_avg);

      if (TankLevel_cm_avg < TankLevel_cm) {  // Tank level has decreased
        TankLevel_cm = TankLevel_cm_avg;
        TankLevel_cm_last = TankLevel_cm;
        TankLevel_updated = true;
        //Serial.println("Tank decrease");
      }
      if (TankLevel_cm_avg > TankLevel_cm + 3) {  // Tank level has increased by at least 3cm
        TankLevel_cm = TankLevel_cm_avg;
        TankLevel_cm_last = TankLevel_cm;
        TankLevel_updated = true;
        //Serial.println("Tank High increase");
      }
      else if (TankLevel_cm_avg > TankLevel_cm + 0.5) {  // Tank level has slow increased
        TankLevel_cm = TankLevel_cm * .999 + TankLevel_cm_avg *.001;  // Very slowly increase measured level
        if (TankLevel_cm_last + 0.9 < TankLevel_cm) {  // if level continue to increase by at least 0.9cm, update result
          TankLevel_cm_last = TankLevel_cm;
          TankLevel_updated = true;
          //Serial.println("Tank Low increase");
        }
      }
    }

    if (TankLevel_updated)
    {
      TankLevel_l = convert_capacity(TankLevel_cm_avg);
      TankLevel_pct = int(TankLevel_l * 100 / TANK_CAPACITY);
      if (TankLevel_pct > 100) TankLevel_pct = 100;
      if (TankLevel_pct < 0) TankLevel_pct = 0;
      TankLevel_valid = true;
      update_dsp = true;
      data.setTL(int (TankLevel_cm_avg), TankLevel_l, TankLevel_pct);
      TankLevel_updated = false;
    }
  }

  yield();

  // Manage serial port input
  if (Serial.available()) {
      char Ch = Serial.read();
      if (Ch == '0') screen = 0;
      if (Ch == '1') screen = 1;
      if (Ch == '2') screen = 2;
      if (Ch == '3') screen = 3;
      saver = DSP_SAVER_TIME;
  }

  // Manage serial port output
  if ((ms - ms_serial) > 10050){
    ms_serial = ms;
    sprintf(Txt, "Level: %4.1f cm  %4i l  %3i %%", TankLevel_cm, TankLevel_l, TankLevel_pct);
    Serial.println(Txt);
    yield();
  }

  // Manage WiFi status every 10 seconds
  if ((ms - ms_wifi) > 9900){
    ms_wifi = ms;
    if (WiFi.status() == WL_CONNECTED){
      rssi = WiFi.RSSI();
      sprintf(Txt, "WiFi:UP  IP:%s  RSSI:%i  ", WiFi.localIP().toString().c_str(), rssi);
      data.Wifi_RSSI = rssi;
    }
    else{
      Serial.println("WiFi:Not connected");
      data.Wifi_RSSI = -100;
    }
  }

  // Manage display
  if ((ms - ms_dsp) > 950){
    ms_dsp = ms;
    if ((screen != 0) && (saver-- == 1)) screen = 0;    // start screen saver
    dsp_screen();
    yield();
  }

  // Manage homie mqtt client - Start client after 30s to have valid reading
 
  if ((!homie_started) && (!FirstRead) && ((ms - ms_starthomie) > 30000)){
    MyHomieClient.setup(&data);
    homie_started = true;
  }
  if (homie_started) MyHomieClient.handle();

 
  // Handle web server and OTA
  MyWebSrv.handle();
  yield();
  OTAhandle();
}