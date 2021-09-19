#include "GyverRelay.h"
#include <OneWire.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>
#include <EEPROM.h>

#define pla_temp_addr 1
#define petg_temp_addr 2
#define custom_temp_addr 3

#define pin_flow 12
#define pin_heater 10
#define ONE_WIRE_BUS 11
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

#define TS_MINX 150     //калибровка тача
#define TS_MINY 60
#define TS_MAXX 920
#define TS_MAXY 940

#define BLACK   0x0000
#define GRAY    0x8410
#define WHITE   0xFFFF
#define RED     0xF800
#define ORANGE  0xFA60
#define YELLOW  0xFFE0
#define GREEN   0x07E0
#define CYAN    0x07FF
#define AQUA    0x04FF
#define BLUE    0x001F
#define MAGENTA 0xF81F
#define PINK    0xF8FF
#define LIGHTBLUE   0xC6FF
#define LIGHTGREEN  0xC7F7

#define BUTTON_X 40
#define BUTTON_Y 100
#define BUTTON_W 60
#define BUTTON_H 30
#define BUTTON_SPACING_X 20
#define BUTTON_SPACING_Y 20
#define BUTTON_TEXTSIZE 2

#define MINPRESSURE 10
#define MAXPRESSURE 1000

#define wait_flow_for_start 2000
#define cooldown_temperature 40

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
OneWire  ds(11);
GyverRelay heater_on(REVERSE);

byte addr[8] = {0x28, 0x2A, 0x04, 0x95, 0xF0, 0x01, 0x3C, 0x4C };
float celsius;

byte pla_temp;
byte petg_temp;
float temperature;
byte target_temp;
byte custom_temp;
boolean flow, heater;
int pixel_x, pixel_y;
byte flagDallRead = 0.5;
unsigned long prevTime_scr_refr = 0, time_begin_flow;
const byte interval_scr_reft = 2;

Adafruit_GFX_Button pla_btn, petg_btn, custom_btn, start_btn, stop_btn, increase_btn_5, increase_btn_1, decrease_btn_5, decrease_btn_1;
Adafruit_GFX_Button option_btn, back_btn, pla_inc_btn, pla_dec_btn, petg_inc_btn, petg_dec_btn, ok_btn, debug_on_btn, debug_off_btn;

void setup() {
  Serial.begin(115200);
  heater_on.k = 0.7;
  heater_on.hysteresis = 2;
  heater_on.dT = 200;

  ds.reset();
  ds.write(0x1F); // точность 0,5гр = 1F; 0,25гр = 3F; 0,125гр = 5F; 0,0625гр = 7F

  pla_temp = EEPROM.read(pla_temp_addr);
  petg_temp = EEPROM.read(petg_temp_addr);
  custom_temp = EEPROM.read(custom_temp_addr);

  tft.reset();
  tft.begin(0x9341);
  tft.cp437(true);
  tft.fillScreen(BLACK);

  tft.setCursor(10, 0);
  tft.setTextColor(MAGENTA); tft.setTextSize(1);

  noTone(pin_buzz);                         //приветственный писк
  tone(pin_buzz, 500, 50);
  delay(50);
  tone(pin_buzz, 800, 50);

  pinMode(pin_flow, OUTPUT);
  digitalWrite(pin_flow, LOW);
  pinMode(pin_heater, OUTPUT);
  digitalWrite(pin_heater, LOW);

  custom_btn.initButton(&tft, 120, 145, 115, 45, BLUE, WHITE, BLUE, "Custom", 3);
  pla_btn.initButton(&tft, 60, 200, 90, 45, BLUE, WHITE, BLUE, "Pla", 3);
  petg_btn.initButton(&tft, 180, 200, 90, 45, BLUE, WHITE, BLUE, "PETG", 3);
  start_btn.initButton(&tft, 120, 270, 130, 50, RED, WHITE, BLACK, "START", 3);
  stop_btn.initButton(&tft, 120, 270, 220, 100, BLUE, WHITE, RED, "STOP", 3);
  increase_btn_5.initButton(&tft, 70, 145, 90, 45, GREEN, GREEN, BLACK, "+5", 3);
  increase_btn_1.initButton(&tft, 240 - 70, 145, 90, 45, GREEN, GREEN, BLACK, "+1", 3);
  decrease_btn_5.initButton(&tft, 70, 205, 90, 45, BLUE, BLUE, BLACK, "-5", 3);
  decrease_btn_1.initButton(&tft, 240 - 70, 205, 90, 45, BLUE, BLUE, BLACK, "-1", 3);
  option_btn.initButton(&tft, 190, 67, 60, 50, GRAY, BLACK, GRAY, "opt", 3);
  back_btn.initButton(&tft, 120, 90, 90, 45, GREEN, WHITE, BLUE, "Back", 3);
  pla_inc_btn.initButton(&tft, 170, 40, 40, 40, GREEN, GREEN, BLACK, "+", 2);
  pla_dec_btn.initButton(&tft, 220, 40, 40, 40, LIGHTBLUE, LIGHTBLUE, BLACK, "-", 2);
  petg_inc_btn.initButton(&tft, 170, 85, 40, 40, GREEN, GREEN, BLACK, "+", 2);
  petg_dec_btn.initButton(&tft, 220, 85, 40, 40, LIGHTBLUE, LIGHTBLUE, BLACK, "-", 2);
  ok_btn.initButton(&tft, 120, 270, 90, 45, GREEN, WHITE, BLUE, "Ok", 3);
  debug_on_btn.initButton(&tft, 60, 160, 90, 45, GREEN, BLACK, BLUE, "Debug on", 2);
  debug_off_btn.initButton(&tft, 180, 160, 90, 45, RED, WHITE, BLUE, " Debug off", 2);

  // start_scr();
}

void loop() {
  scr_one();
}

void beep() {
  noTone(pin_buzz);
  tone(pin_buzz, 1000, 10);
}

void beep_beep() {
  noTone(pin_buzz);
  tone(pin_buzz, 1000, 100);
  delay(200);
  tone(pin_buzz, 1000, 100);
  delay(200);
  // tone(pin_buzz, 1000, 100);
  // delay(200);
}

void beeep() {
  noTone(pin_buzz);
  tone (pin_buzz, 700, 100);
  delay (100);
}

bool Touch_getXY(void) {
  TSPoint p = ts.getPoint();
  bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
  // Serial.println(p.z);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  if (pressed) {
    pixel_x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
    pixel_y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0);
  }
  return pressed;
}

void update_button_scr_one() {
  while (true) {

    dallRead(flagDallRead * 1000);
    status();
    control();
    Touch_getXY();
    // Serial.print(pixel_x);Serial.print(" ");Serial.println(pixel_y);

    if (pla_btn.contains(pixel_x, pixel_y)) {
      pla_btn.press(true);
      petg_btn.press(false);
      custom_btn.press(false);
      pixel_x = 0; pixel_y = 0;
      target_temp = pla_temp;
      if (pla_btn.justPressed()) {
        beep();
        pla_btn.drawButton(false);
        petg_btn.drawButton(true);
        // custom_btn.drawButton(true);
      }
    }

    if (option_btn.contains(pixel_x, pixel_y)) {
      pla_btn.press(false);
      petg_btn.press(false);
      option_btn.press(true);
      pixel_x = 0; pixel_y = 0;
      if (option_btn.justPressed()) {
        beep_beep();
        option_btn.drawButton(false);
      }
      delay(100);
      break;
    }

    if (petg_btn.contains(pixel_x, pixel_y)) {
      petg_btn.press(true);
      pla_btn.press(false);
      custom_btn.press(false);
      target_temp = petg_temp;
      pixel_x = 0; pixel_y = 0;
      if (petg_btn.justPressed()) {
        beep();
        petg_btn.drawButton(false);
        pla_btn.drawButton(true);
        // custom_btn.drawButton(true);
      }
    }

    if (custom_btn.contains(pixel_x, pixel_y)) {
      petg_btn.press(false);
      pla_btn.press(false);
      custom_btn.press(true);
      pixel_x = 0; pixel_y = 0;
      if (custom_btn.justPressed()) {
        beep();
        petg_btn.drawButton(true);
        pla_btn.drawButton(true);
        custom_btn.drawButton(false);
      }
      delay(100);
      break;
    }
    if (start_btn.contains(pixel_x, pixel_y)) {
      start_btn.press(true);
      stop_btn.press(false);
      pixel_x = 0; pixel_y = 0;
      if (petg_btn.isPressed() || pla_btn.isPressed() && start_btn.justPressed()) {
        start_btn.drawButton(false);
        beep_beep();
        time_begin_flow = millis();
        break;
      }
      else if (start_btn.justPressed()) {
        start_btn.drawButton(true);
        beep();
        delay(100);
        start_btn.press(false);
      }
    }
  }
  return;
}

void update_button_custom() {
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(3);
  while (true) {
    control();
    Touch_getXY();

    if (increase_btn_1.contains(pixel_x, pixel_y)) {
      increase_btn_1.press(true);
      pixel_x = 0; pixel_y = 0;
      if (increase_btn_1.justPressed()) {
        increase_btn_1.drawButton(false);
        beep();
        custom_temp = custom_temp + 1;
        lim_temp();
        delay(100);
        increase_btn_1.drawButton(true);
      }
      tft.setCursor(160, 10);
      tft.setTextColor(WHITE, BLACK);
      tft.setTextSize(4);
      tft.print(custom_temp);

    }

    if (increase_btn_5.contains(pixel_x, pixel_y)) {
      increase_btn_5.press(true);
      pixel_x = 0; pixel_y = 0;
      if (increase_btn_5.justPressed()) {
        increase_btn_5.drawButton(false);
        beep();
        custom_temp = custom_temp + 5;
        lim_temp();
        delay(100);
        increase_btn_5.drawButton(true);
      }
      tft.setCursor(160, 10);
      tft.setTextColor(WHITE, BLACK);
      tft.setTextSize(4);
      tft.print(custom_temp);
    }

    if (decrease_btn_1.contains(pixel_x, pixel_y)) {
      decrease_btn_1.press(true);
      pixel_x = 0; pixel_y = 0;
      if (decrease_btn_1.justPressed()) {
        decrease_btn_1.drawButton(false);
        beep();
        custom_temp = custom_temp - 1;
        lim_temp();
        delay(100);
        decrease_btn_1.drawButton(true);
      }
      tft.setCursor(160, 10);
      tft.setTextColor(WHITE, BLACK);
      tft.setTextSize(4);
      tft.print(custom_temp);
    }

    if (decrease_btn_5.contains(pixel_x, pixel_y)) {
      decrease_btn_5.press(true);
      pixel_x = 0; pixel_y = 0;
      if (decrease_btn_5.justPressed()) {
        decrease_btn_5.drawButton(false);
        beep();
        custom_temp = custom_temp - 5;
        lim_temp();
        delay(100);
        decrease_btn_5.drawButton(true);
      }
      tft.setCursor(160, 10);
      tft.setTextColor(WHITE, BLACK);
      tft.setTextSize(4);
      tft.print(custom_temp);
    }

    if (back_btn.contains(pixel_x, pixel_y)) {
      back_btn.press(true);
      pixel_x = 0; pixel_y = 0;
      if (back_btn.justPressed()) {
        back_btn.drawButton(false);
        beeep();
      }
      EEPROM.update(custom_temp_addr, custom_temp);
      increase_btn_1.press(false);
      increase_btn_5.press(false);
      decrease_btn_1.press(false);
      decrease_btn_5.press(false);
      back_btn.press(false);
      break;
    }

    if (start_btn.contains(pixel_x, pixel_y)) {
      start_btn.press(true);
      stop_btn.press(false);
      pixel_x = 0; pixel_y = 0;
      target_temp = custom_temp;
      EEPROM.update(custom_temp_addr, custom_temp);
      if (start_btn.justPressed()) {
        start_btn.drawButton(false);
        beep_beep();
        time_begin_flow = millis();
      }
      increase_btn_1.press(false);
      increase_btn_5.press(false);
      decrease_btn_1.press(false);
      decrease_btn_5.press(false);
      break;
    }
    increase_btn_1.press(false);
    increase_btn_5.press(false);
    decrease_btn_1.press(false);
    decrease_btn_5.press(false);
  }
  return;
}

void update_button_stop() {

  while (true) {
    Touch_getXY();
    dallRead(flagDallRead * 1000);
    control();
    status();
    if (stop_btn.contains(pixel_x, pixel_y)) {
      stop_btn.press(true);
      start_btn.press(false);
      pla_btn.press(false);
      petg_btn.press(false);
      custom_btn.press(false);
      pixel_x = 0; pixel_y = 0;
      if (stop_btn.justPressed()) {
        stop_btn.drawButton(false);
        beep_beep();
      }
      break;
    }
  }
  return;
}

void update_button_option() {
  while (true) {
    Touch_getXY();
    control();

    if (pla_inc_btn.contains(pixel_x, pixel_y)) {
      pla_inc_btn.press(true);
      pixel_x = 0; pixel_y = 0;
      if (pla_inc_btn.justPressed()) {
        pla_inc_btn.drawButton(false);
        beep();
        pla_temp = pla_temp + 1;
        tft.setTextColor(ORANGE, BLACK);
        tft.setCursor(115, 35);
        tft.print(pla_temp);
        delay(100);
        pla_inc_btn.drawButton(true);
      }
      pla_inc_btn.press(false);
    }

    if (pla_dec_btn.contains(pixel_x, pixel_y)) {
      pla_dec_btn.press(true);
      pixel_x = 0; pixel_y = 0;
      if (pla_dec_btn.justPressed()) {
        pla_dec_btn.drawButton(false);
        beep();
        pla_temp = pla_temp - 1;
        tft.setTextColor(ORANGE, BLACK);
        tft.setCursor(115, 35);
        tft.print(pla_temp);
        delay(100);
        pla_dec_btn.drawButton(true);
      }
      pla_dec_btn.press(false);
    }

    if (petg_inc_btn.contains(pixel_x, pixel_y)) {
      petg_inc_btn.press(true);
      pixel_x = 0; pixel_y = 0;
      if (petg_inc_btn.justPressed()) {
        petg_inc_btn.drawButton(false);
        beep();
        petg_temp = petg_temp + 1;
        tft.setTextColor(ORANGE, BLACK);
        tft.setCursor(115, 80);
        tft.print(petg_temp);
        delay(100);
        petg_inc_btn.drawButton(true);
      }
      petg_inc_btn.press(false);
    }

    if (petg_dec_btn.contains(pixel_x, pixel_y)) {
      petg_dec_btn.press(true);
      pixel_x = 0; pixel_y = 0;
      if (petg_dec_btn.justPressed()) {
        petg_dec_btn.drawButton(false);
        beep();
        petg_temp = petg_temp - 1;
        tft.setTextColor(ORANGE, BLACK);
        tft.setCursor(115, 80);
        tft.print(petg_temp);
        delay(100);
        petg_dec_btn.drawButton(true);
      }
      petg_dec_btn.press(false);
    }

    if (ok_btn.contains(pixel_x, pixel_y)) {
      ok_btn.press(true);
      pixel_x = 0; pixel_y = 0;
      if (ok_btn.justPressed()) {
        ok_btn.drawButton(false);
        beep_beep();
        delay(100);
      }
      EEPROM.update(pla_temp_addr,pla_temp);
      EEPROM.update(petg_temp_addr,petg_temp);
      ok_btn.press(false);
      break;
    }

  }
}

void option_scr() {
  tft.fillScreen(BLACK);
  pla_inc_btn.drawButton(true);
  pla_dec_btn.drawButton(true);
  petg_inc_btn.drawButton(true);
  petg_dec_btn.drawButton(true);
  ok_btn.drawButton(true);
  tft.setTextSize(2);
  tft.setCursor(2, 35);
  tft.setTextColor(AQUA);
  tft.print(utf8rus("Темп.PLA"));
  tft.setCursor(2, 80);
  tft.print(utf8rus("Темп.PETG"));
  // tft.setCursor(2,120);
  // tft.print("dT");

  tft.setTextColor(ORANGE);
  tft.setCursor(115, 35);
  tft.print(pla_temp);
  tft.setCursor(115, 80);
  tft.print(petg_temp);

  update_button_option();
}

void custom_scr() {
  tft.fillScreen(BLACK);
  tft.setTextColor(CYAN); tft.setTextSize(2);
  tft.setCursor(0, 10);
  tft.print(utf8rus(F("  Установка")));
  tft.setCursor(0, 22);
  tft.print(utf8rus(F("температуры:")));
  tft.setCursor(160, 10);
  tft.setTextColor(WHITE);
  tft.setTextSize(4);
  tft.print(custom_temp);
  tft.print("\xB0");// tft.print("C");
  increase_btn_1.drawButton(true);
  increase_btn_5.drawButton(true);
  decrease_btn_1.drawButton(true);
  decrease_btn_5.drawButton(true);
  start_btn.drawButton(true);
  back_btn.drawButton(true);
  update_button_custom();
  // if (start_btn.press()) work_scr();
  custom_btn.press(false);
}

void scr_one() {
  tft.fillScreen(BLACK);
  tft.setTextColor(MAGENTA); tft.setTextSize(2);
  tft.setCursor(0, 10);
  tft.print(utf8rus(F("Т.тек:")));

  tft.setCursor(0, 50);
  tft.setTextColor(MAGENTA); tft.print("Flow");

  tft.setCursor(0, 70);
  tft.setTextColor(MAGENTA); tft.print("Heater");

  custom_btn.drawButton(true);
  pla_btn.drawButton(true);
  petg_btn.drawButton(true);
  start_btn.drawButton(true);
  option_btn.drawButton(true);

  update_button_scr_one();
  if (custom_btn.isPressed()) custom_scr();
  if (start_btn.isPressed()) work_scr();
  if (option_btn.isPressed()) {
    option_scr();
    option_btn.press(false);
  }
}

void work_scr() {
  tft.fillScreen(BLACK);
  tft.setTextColor(MAGENTA); tft.setTextSize(2);
  tft.setCursor(0, 10);
  tft.print(utf8rus(F("Т.тек:")));
  tft.setCursor(0, 50);
  tft.setTextColor(MAGENTA); tft.print("Flow");

  tft.setCursor(0, 70);
  tft.setTextColor(MAGENTA); tft.print("Heater");
  tft.setCursor(0,90);
  tft.setTextColor(BLUE); tft.print(utf8rus("Цель.темп: "));
  tft.setTextColor(LIGHTGREEN); tft.print(target_temp); tft.print("\xB0"); tft.print("C");

  stop_btn.drawButton(true);
  update_button_stop();
  start_btn.press(false);
}

void lim_temp() {
  if (custom_temp > 80) custom_temp = 80;
  if (custom_temp < 30) custom_temp = 30;
}

void status() {
  if (millis() - prevTime_scr_refr > (interval_scr_reft * 1000)) {
    // tft.setTextSize(2);
    // tft.setCursor(100, 10);

    tft.setCursor(80, 10);
    tft.setTextSize(4);

    if (temperature < 40) tft.setTextColor(GREEN, BLACK);
    else tft.setTextColor(RED, BLACK);
    tft.print(temperature, 1);tft.setTextSize(3); tft.print("\xB0"); tft.print("C");

    tft.setTextSize(2);
    tft.setCursor(100, 50);
    tft.setTextColor(CYAN, BLACK);
    if (flow) tft.print("ON ");
    else tft.print("OFF");

    tft.setCursor(100, 70);
    tft.setTextColor(CYAN, BLACK);
    if (heater) tft.print("ON ");
    else tft.print("OFF");

    prevTime_scr_refr = millis();
  }
}

void control() {
  heater_on.setpoint = target_temp;
  if (start_btn.isPressed()) {
    if ((millis() - time_begin_flow > wait_flow_for_start) && flow) {
      heater_on.input = temperature;
      if (heater_on.getResultTimer()) {
        heater = true;
        digitalWrite(pin_heater, HIGH);
      } else {
        heater = false;
        digitalWrite(pin_heater, LOW);
      }
    } else {
      digitalWrite(pin_flow, HIGH);
      flow = true;
    }
  }

if (temperature < 0) {
  tft.fillScreen(BLACK);
  tft.setTextSize(4);
  tft.setTextColor(RED);
  tft.print("ERROR");
  beep_beep();
  beep_beep();
}

  if (stop_btn.isPressed()) {
    digitalWrite(pin_heater, LOW);
    heater = false;
    if (temperature < cooldown_temperature) {
      digitalWrite(pin_flow, LOW);
      flow = false;
    }
  }

}

String utf8rus(String source) {
  int i, k;
  String target;
  unsigned char n;
  char m[2] = { '0', '\0' };

  k = source.length(); i = 0;

  while (i < k) {
    n = source[i]; i++;

    if (n >= 0xC0) {
      switch (n) {
        case 0xD0: {
            n = source[i]; i++;
            if (n == 0x81) {
              n = 0xA8;
              break;
            }
            if (n >= 0x90 && n <= 0xBF) n = n + 0x30;
            break;
          }
        case 0xD1: {
            n = source[i]; i++;
            if (n == 0x91) {
              n = 0xB8;
              break;
            }
            if (n >= 0x80 && n <= 0x8F) n = n + 0x70;
            break;
          }
      }
    }
    m[0] = n; target = target + String(m);
  }
  return target;
}

void dallRead(unsigned long interval) {
  static unsigned long prevTime = 0;
  if (millis() - prevTime > interval) { //Проверка заданного интервала
    static boolean flagDall = 0; //Признак операции
    prevTime = millis();
    flagDall = ! flagDall; //Инверсия признака
    if (flagDall) {
      ds.reset();
      ds.write(0xCC); //Обращение ко всем датчикам
      // ds.write(0x3F); // точность 0,5гр = 1F; 0,25гр = 3F; 0,125гр = 5F; 0,0625гр = 7F
      ds.write(0x44); //Команда на конвертацию
      // flagDallRead = 1; //Время возврата в секундах
    }
    else {
      int temp;
      ds.reset();
      ds.select(addr);
      ds.write(0xBE); //Считывание значения с датчика
      temp = (ds.read() | ds.read() << 8); //Принимаем два байта температуры
      temperature = (float)temp / 16.0;
      // flagDallRead = 2; //Время возврата в секундах
      Serial.println(temperature);
      status();
    }
  }
}
