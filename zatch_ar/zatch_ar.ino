//This will make the meter visually alert you when the reading crosses 45, with both the gauge and the displayed number turning red.
//
// TFT_eSPI.h Version TFT_eSPI@2.5.43, updated from https://github.com/Seeed-Projects/SeeedStudio_TFT_eSPI
// BSP is 3.0.5 
// Xiao ESP32C6 AOK :-)
// tested  10/14/24 by PJ Glasso
//
#include <SPI.h>
#include <TFT_eSPI.h>
#include "I2C_BM8563.h"

TFT_eSPI tft = TFT_eSPI(); 
TFT_eSprite img = TFT_eSprite(&tft);
I2C_BM8563 rtc(I2C_BM8563_DEFAULT_ADDRESS, Wire);

// https://rgbcolorpicker.com/565
#define WHITE       0xFFFF
#define BLACK       0x0000
#define BLUE        0x001F
#define RED         0xF800
#define GREEN       0x07E0
#define YELLOW      0xFFE0
#define GREY        0x4a49
#define DARK_GREY   0x3186
#define TEXT_COLOR  0xFFFF
#define SCALE0      0xC655
#define SCALE1      0x5DEE

#define DEG2RAD     0.0174532925

uint32_t runTime = -99999;
double center_x = 120;
double center_y = 120;

void setup(void) {
  tft.begin();
  Serial.begin(9600);

  // Start I2C and RTC
  Wire.begin();
  rtc.begin();

  tft.setRotation(0);
  tft.fillScreen(BLACK);


  img.setColorDepth(16);
}

double currentTime = 0;
I2C_BM8563_TimeTypeDef timeStruct;

void loop() {
  if (millis() - runTime >= 0L) {
    runTime = millis();

    rtc.getTime(&timeStruct);
    drawClock(timeStruct.hours * 60 + timeStruct.minutes + timeStruct.seconds/60.0);

    yield();  
  }
}

void drawClock(double minutes)
{
    int x = 0;
    int y = 0;

    img.createSprite(240, 240);

    // Draw the background
    img.drawArc(center_x, center_y, 117, 90, 0, 360, BLACK, BLACK);

    // Draw the hour ticks
    for (int i = 0; i < 12; i++)
    {
      int a = i*30;
      int dx = 116 * cos((a-90) * DEG2RAD) + center_x; 
      int dy = 116 * sin((a-90) * DEG2RAD) + center_y;
      img.fillCircle(dx, dy, 2, DARK_GREY);
    }

    // Draw the tracks
    img.drawArc(center_x, center_y, 112, 110, 0, 360, DARK_GREY, BLACK);
    img.drawArc(center_x, center_y, 97, 95, 0, 360, DARK_GREY, BLACK);

    // Draw the minute hand
    double minute_hand_angle = ((fmod(minutes,60.0)*6)-90) * DEG2RAD;
    x = 111 * cos(minute_hand_angle) + center_x; 
    y = 111 * sin(minute_hand_angle) + center_y;
    img.fillCircle(x, y, 3, WHITE);

    // Draw the hour hand
    double hour_hand_angle = ((minutes * 360 / 720) - 90) * DEG2RAD;
    x = 96 * cos(hour_hand_angle) + center_x; 
    y = 96 * sin(hour_hand_angle) + center_y;
    img.fillCircle(x, y, 3, WHITE);

    img.pushSprite(0, 0, TFT_TRANSPARENT);
    img.deleteSprite();

}