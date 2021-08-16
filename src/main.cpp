#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

#include <DS3232RTC.h>
#include <GxEPD2_BW.h>
#include <Wire.h>

#include "DSEG7_Classic_Bold_25.h"

#define CS 5
#define DC 10
#define RESET 9
#define BUSY 19

DS3232RTC RTC(false);
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(CS, DC, RESET, BUSY));

bool connect_WiFi()
{
  int WIFI_CONFIGURED;

  if (WL_CONNECT_FAILED == WiFi.begin())
  { //WiFi not setup, you can also use hard coded credentials with WiFi.begin(SSID,PASS);
    WIFI_CONFIGURED = false;
  }
  else
  {
    if (WL_CONNECTED == WiFi.waitForConnectResult())
    { //attempt to connect for 10s
      WIFI_CONFIGURED = true;
    }
    else
    { //connection failed, time out
      WIFI_CONFIGURED = false;
      //turn off radios
      WiFi.mode(WIFI_OFF);
      btStop();
    }

    IPAddress ip;
    ip = WiFi.localIP();
    Serial.println(ip);
  }
  return WIFI_CONFIGURED;
}

void print_2_digits(int number)
{
  if (number >= 0 && number < 10)
  {
    Serial.write('0');
  }
  Serial.print(number);
}

void print_tm_elements(tmElements_t tm)
{
  Serial.write(" ");
  print_2_digits(tm.Hour);
  Serial.write(':');
  print_2_digits(tm.Minute);
  Serial.write(':');
  print_2_digits(tm.Second);
  Serial.print(" ");
  Serial.print(tm.Day);
  Serial.write('/');
  Serial.print(tm.Month);
  Serial.write('/');
  Serial.print(tmYearToCalendar(tm.Year));
  Serial.println();
}

void print_time(struct tm &timeinfo)
{
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void setup_time()
{
  configTime(0, 0, "pool.ntp.org");
  RTC.squareWave(SQWAVE_NONE); //disable square wave output

  struct tm timeinfo;
  if (getLocalTime(&timeinfo))
  {
    Serial.write("setting time: ");
    print_time(timeinfo);
    time_t t = mktime(&timeinfo);
    RTC.set(t);
  }
}

void setup()
{
  Wire.begin(SDA, SCL);

  Serial.begin(921600, SERIAL_8N1, 3, 1);
  Serial.println("Setup");

  display.init(0, false); //_initial_refresh to false to prevent full update on init
  display.setFullWindow();
  display.fillScreen(GxEPD_BLACK);
  display.setTextColor(GxEPD_WHITE);
  display.display(false);

  connect_WiFi();
  setup_time();

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  Serial.println("Done setup");
  Serial.println("");
}

bool get_local_time(struct tm &timeinfo)
{
  return getLocalTime(&timeinfo);
}

bool get_rtc_time(struct tm &timeinfo)
{
  tmElements_t tm;
  RTC.read(tm);
  time_t t = makeTime(tm);
  timeinfo = *gmtime(&t);
  return true;
}

void draw_time(struct tm &timeinfo)
{
  if (timeinfo.tm_hour < 10)
  {
    display.print("0");
  }
  display.print(timeinfo.tm_hour);
  display.print(":");
  if (timeinfo.tm_min < 10)
  {
    display.print("0");
  }
  display.print(timeinfo.tm_min);
  display.print(":");
  if (timeinfo.tm_sec < 10)
  {
    display.print("0");
  }
  display.println(timeinfo.tm_sec);
}

void loop()
{
  Serial.println("updating...");

  display.init(0, false); //_initial_refresh to false to prevent full update on init
  display.setFullWindow();

  display.fillScreen(GxEPD_BLACK);
  display.setTextColor(GxEPD_WHITE);

  display.setFont(&DSEG7_Classic_Bold_25);

  struct tm timeinfo;
  if (get_local_time(timeinfo))
  {
    Serial.write("local time: ");
    print_time(timeinfo);

    display.setCursor(5, 53 + 5);
    display.print("L ");
    draw_time(timeinfo);
  }

  if (get_rtc_time(timeinfo))
  {
    Serial.write("rtc time: ");
    print_time(timeinfo);

    display.setCursor(5, 53 + 5 + 25 + 5);
    display.print("R ");
    draw_time(timeinfo);
  }

  display.display(true);
  Serial.println("");

  delay(10 * 1000);
}
