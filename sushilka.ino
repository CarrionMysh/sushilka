#include <GyverEncoder.h>
#include <DallasTemperature.h>
#include <GyverPID.h>
#include <PIDtuner.h>
#include <PIDtuner2.h>
#include <OneWire.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>

#define pin_motor 10
#define pin_heater 11
#define ONE_WIRE_BUS 12
#define pin_buzz A5

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin


#define TS_MINX 150     //проверить что это такое
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940

#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);
DeviceAddress sensorAddress;

float temperature;

void setup() {
  Serial.begin(9600);
  tft.reset();
  uint16_t identifier = tft.readID();
  tft.begin(identifier);
  tft.fillScreen(BLACK);

  tft.setCursor(0,0);
  tft.setTextColor(MAGENTA); tft.setTextSize(1);

  noTone(pin_buzz);                         //приветственный писк
  tone(pin_buzz, 500, 50);
  delay(50);
  tone(pin_buzz, 800, 50);

  tft.print(F("Search sensor..."));
  sensor.begin();
  sensor.getAddress(sensorAddress, sensor.getDeviceCount());
  tft.print(F("found ")); tft.println(sensor.getDeviceCount(), DEC);

  for (uint8_t i = 0; i < 8; i++)
  {
    if (sensorAddress[i] < 16) tft.print("0");
    tft.print(sensorAddress[i], HEX);
  }

  tft.println();

  sensor.setResolution(9);

  pinMode(pin_motor, OUTPUT);
  digitalWrite(pin_motor,LOW);
  pinMode(pin_heater, OUTPUT);
  digitalWrite(pin_heater, LOW);
}

void loop() {
  get_temp();
  delay(1000);
}

void get_temp(){
  sensor.requestTemperatures();
  temperature = sensor.getTempC(sensorAddress);
  Serial.print("temp=");Serial.println(temperature);
}

void alarm(){

}
