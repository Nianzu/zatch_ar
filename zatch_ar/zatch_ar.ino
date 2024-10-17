
#include <SPI.h>
#include <TFT_eSPI.h>
#include "I2C_BM8563.h"
#define USE_TFT_ESPI_LIBRARY
#include "lv_xiao_round_screen.h"

// TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft);
I2C_BM8563 rtc(I2C_BM8563_DEFAULT_ADDRESS, Wire);

#define NUM_ADC_SAMPLE 20
#define BATTERY_DEFICIT_VOL 1850
#define BATTERY_FULL_VOL 2450 


// https://rgbcolorpicker.com/565
#define WHITE 0xFFFF
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define YELLOW 0xFFE0
#define GREY 0x4a49
#define DARK_GREY 0x3186
#define TEXT_COLOR 0xFFFF
#define SCALE0 0xC655
#define SCALE1 0x5DEE

#define DEG2RAD 0.0174532925

// TL_DATUM = 0 = Top left
// TC_DATUM = 1 = Top centre
// TR_DATUM = 2 = Top right
// ML_DATUM = 3 = Middle left
// MC_DATUM = 4 = Middle centre
// MR_DATUM = 5 = Middle right
// BL_DATUM = 6 = Bottom left
// BC_DATUM = 7 = Bottom centre
// BR_DATUM = 8 = Bottom right

uint32_t runTime = -99999;
double center_x = 120;
double center_y = 120;
int currentScreen = 0;
lv_coord_t touchX, touchY;
uint32_t primary_color = WHITE;

void setup(void)
{
  tft.begin();
  Serial.begin(9600);

  // Start I2C and RTC
  Wire.begin();
  rtc.begin();

  pinMode(TOUCH_INT, INPUT_PULLUP);

  tft.setRotation(0);
  tft.fillScreen(BLACK);

  img.setColorDepth(16);
}

double currentTime = 0;
I2C_BM8563_TimeTypeDef timeStruct;

void loop()
{
  if (millis() - runTime >= 0L)
  {
    runTime = millis();
    switch (currentScreen)
    {
    case 0:
      if (chsc6x_is_pressed())
      {
        currentScreen = 1;
        tft.fillScreen(BLACK);
        delay(500);
      }
      else
      {
        
        img.createSprite(240, 240);
        img.fillScreen(TFT_TRANSPARENT);

        int16_t textWidth = img.textWidth("000");   // Estimate the maximum width for a 3-digit number
        int16_t textHeight = 24;  // Adjust based on text size
        img.fillRect(center_x - (textWidth/2), center_y, textWidth, textHeight, BLACK);
        img.setTextSize(2);
        img.drawCentreString(String(battery_level_percent())+"%" ,center_x,center_y,1);
        img.pushSprite(0, 0, TFT_TRANSPARENT);
        img.deleteSprite();

        rtc.getTime(&timeStruct);
        drawClock(timeStruct.hours * 60 + timeStruct.minutes + timeStruct.seconds / 60.0);
      }
      break;
    case 1:
      if (chsc6x_is_pressed())
      {
        chsc6x_get_xy(&touchX, &touchY);
        tft.fillScreen(BLACK);
        delay(500);
        if (touchX < center_x && touchY < center_y) // Top Left
        {
          currentScreen = 0;
        }
        else if (touchX >= center_x && touchY < center_y) // Top Right
        {
          primary_color = RED;
        }
        else if (touchX >= center_x && touchY >= center_y) // BOT Right
        {
          primary_color = BLUE;
        }
        else if (touchX < center_x && touchY >= center_y) // BOT LEFT
        {
          primary_color = WHITE;
        }
      }
      else
      {
        img.createSprite(240, 240);
        img.fillScreen(TFT_TRANSPARENT);
        int arrow_x = 40;
        int arrow_y = 65;
        img.fillTriangle(arrow_x, arrow_y, arrow_x + 15, arrow_y + 10, arrow_x + 15, arrow_y - 10, primary_color);
        img.fillRect(arrow_x + 15, arrow_y - 2, 15, 5, primary_color);
        img.fillCircle(180,60,10,RED);
        img.fillCircle(180,180,10,BLUE);
        img.fillCircle(60,180,10,WHITE);
        img.pushSprite(0, 0, TFT_TRANSPARENT);
        img.deleteSprite();
      }
      break;
    }

    yield();
  }
}

int32_t battery_level_percent(void)
{
  int32_t mvolts = 0;
  for(int8_t i=0; i<NUM_ADC_SAMPLE; i++){
    mvolts += analogReadMilliVolts(D0);
  }
  mvolts /= NUM_ADC_SAMPLE;
  int32_t level = (mvolts - BATTERY_DEFICIT_VOL) * 100 / (BATTERY_FULL_VOL-BATTERY_DEFICIT_VOL); // 1850 ~ 2100
  level = (level<0) ? 0 : ((level>100) ? 100 : level); 
  return level;
}

void drawClock(double minutes)
{
  int x = 0;
  int y = 0;

  img.createSprite(240, 240);
  img.fillScreen(TFT_TRANSPARENT);

  // Draw the background
  img.drawArc(center_x, center_y, 117, 90, 0, 360, BLACK, BLACK);

  // Draw the hour ticks
  for (int i = 0; i < 12; i++)
  {
    int a = i * 30;
    int dx = 116 * cos((a - 90) * DEG2RAD) + center_x;
    int dy = 116 * sin((a - 90) * DEG2RAD) + center_y;
    img.fillCircle(dx, dy, 2, DARK_GREY);
  }

  // Draw the tracks
  img.drawArc(center_x, center_y, 112, 110, 0, 360, DARK_GREY, BLACK);
  img.drawArc(center_x, center_y, 97, 95, 0, 360, DARK_GREY, BLACK);

  // Draw the minute hand
  double minute_hand_angle = ((fmod(minutes, 60.0) * 6) - 90) * DEG2RAD;
  x = 111 * cos(minute_hand_angle) + center_x;
  y = 111 * sin(minute_hand_angle) + center_y;
  img.fillCircle(x, y, 3, primary_color);

  // Draw the hour hand
  double hour_hand_angle = ((minutes * 360 / 720) - 90) * DEG2RAD;
  x = 96 * cos(hour_hand_angle) + center_x;
  y = 96 * sin(hour_hand_angle) + center_y;
  img.fillCircle(x, y, 3, primary_color);

  img.pushSprite(0, 0, TFT_TRANSPARENT);
  img.deleteSprite();
}