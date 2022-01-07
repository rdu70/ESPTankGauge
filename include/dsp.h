#ifndef _DSP_H
#define _DSP_H

#include <Arduino.h>
#include "config.h"

void dsp_init(){
  Serial1.begin(9600);
/*
  Serial1.print("SB"); // Switch to 38400 Baud
  Serial1.write(0x00);
  Serial1.write(0x00);
  Serial1.write(0x96);
  Serial1.write(0x00);
  delay(100);
  Serial1.flush();
  Serial1.updateBaudRate(38400);
*/
  Serial1.print("DC");   // Display config off
  Serial1.write(0x00);
  Serial1.print("DSS");  // Start Screen off
  Serial1.write(0x00);
}

void dsp_clear(){
  Serial1.print("CL");   // Clear screen
}

void dsp_setfont(int font){
  Serial1.print("SF");
  Serial1.write(font);
}

void dsp_color(int col){
  Serial1.print("SC");
  Serial1.write(col);
}

void dsp_colorrgb(int r, int g , int b){
  Serial1.print("ESC");
  Serial1.write(r>>2);
  Serial1.write(g>>2);
  Serial1.write(b>>2);
}

void dsp_print(int x, int y, char *St){
  Serial1.print("ETP");
  Serial1.write(x);
  Serial1.write(y);
  Serial1.print("TT");
  Serial1.print(St);
  Serial1.write(0x00);
  Serial1.write(0x0a);
  Serial1.write(0x0d);
  yield();
}

void dsp_rectangle(int x1, int y1, int x2, int y2, bool filled){
  if (filled) Serial1.print("FR"); else Serial1.print("DR");
  Serial1.write(x1);
  Serial1.write(y1);
  Serial1.write(x2);
  Serial1.write(y2);
  yield();
}

void dsp_line(int x1, int y1, int x2, int y2){
  Serial1.print("LN");
  Serial1.write(x1);
  Serial1.write(y1);
  Serial1.write(x2);
  Serial1.write(y2);
  yield();
}

void dsp_lineto(int x, int y){
  Serial1.print("LT");
  Serial1.write(x);
  Serial1.write(y);
}

void dsp_gauge(int x, int y, int pct){
  // (x, y) = upper left corner
  // size : w=20, h=80
  int step = 20;
  dsp_color(DSP_COL_WHITE);
  for (int i = 0; i<5; i++){
    dsp_line(x, y+step*i, x+5, y+step*i);
  }
  dsp_line(x, y, x, y+step*4);
  int col = DSP_COL_WHITE;
  if (pct < 10) col = DSP_COL_RED;
  dsp_color(DSP_COL_BLACK);
  dsp_rectangle(x+10, y, x+20, y+4*step, true);
  dsp_color(col);
  dsp_rectangle(x+10, y+4*step*(100-pct)/100, x+20, y+4*step, true);
}

void dsp_gauge2(int x, int y, int pct){
  // (x, y) = upper left corner
  // size : w=20, h=80
  // Red = 8 (10%) ; Orange = 16 (20%)
  int Color;
  int yy = y+80-(80*pct/100);
  dsp_color(DSP_COL_WHITE);
  dsp_line(x, y, x+5, y);
  dsp_lineto(x+5, y+74);
  dsp_line(x, y+20, x+5, y+20);
  dsp_line(x, y+40, x+5, y+40);
  dsp_line(x, y+60, x+5, y+60);
  dsp_color(DSP_COL_ORANGE);
  dsp_rectangle(x+5, y+64, x+4, y+71, false);
  dsp_color(DSP_COL_RED);
  dsp_rectangle(x+5, y+72, x+4, y+80, false);
  dsp_line(x, y+80, x+5, y+80);
  if (yy < y+64) Color = DSP_COL_WHITE;
  else if (yy >= y+72) Color = DSP_COL_RED;
  else Color = DSP_COL_ORANGE;
  dsp_color(Color);
  for (int xx = 10; xx<20; xx++){
    dsp_line(x+xx, yy, x+xx, y+80);
  }
  dsp_colorrgb(64, 64, 64);
  if (yy > y) for (int xx = 10; xx<20; xx++){
    if (xx == 12) dsp_colorrgb(128, 128, 128);
    dsp_line(x+xx, yy-1, x+xx, y);
  }
  dsp_color(DSP_COL_BLACK);
  dsp_line(x+10, y+20, x+15, y+20);
  dsp_line(x+10, y+40, x+15, y+40);
  dsp_line(x+10, y+60, x+15, y+60);
}

void dsp_backlight(int pct){
  Serial1.print("BL");
  Serial1.write(pct);
}


#endif  /* _DSP_H */