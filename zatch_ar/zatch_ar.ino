
#include <SPI.h>
#include <TFT_eSPI.h>
#include "I2C_BM8563.h"
#define USE_TFT_ESPI_LIBRARY
#include "lv_xiao_round_screen.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <WiFi.h>

// TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft);
I2C_BM8563 rtc(I2C_BM8563_DEFAULT_ADDRESS, Wire);

const char *ntpServer = "time.cloudflare.com";
const char *ssid = "ST";
const char *password = "test1234";
// TimerHandle_t hp_timer;

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
#define TEAL 0x063f
#define DARK_GREY 0x3186
#define TEXT_COLOR 0xFFFF
#define SCALE0 0xC655
#define SCALE1 0x5DEE

#define DEG2RAD 0.0174532925

#define BATLVLFORHP 25

// TL_DATUM = 0 = Top left
// TC_DATUM = 1 = Top centre
// TR_DATUM = 2 = Top right
// ML_DATUM = 3 = Middle left
// MC_DATUM = 4 = Middle centre
// MR_DATUM = 5 = Middle right
// BL_DATUM = 6 = Bottom left
// BC_DATUM = 7 = Bottom centre
// BR_DATUM = 8 = Bottom right

double center_x = 120;
double center_y = 120;
lv_coord_t touchX, touchY;
uint32_t primary_color = WHITE;

double currentTime = 0;
I2C_BM8563_TimeTypeDef timeStruct;

int currentBattery = 0;

bool is_high_power = false;

void state_machine_task(void *pvParameters)
{
  int currentScreen = 0;
  while (1)
  {
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
        rtc.getTime(&timeStruct);
        printf("%d H %d m %d s\n", timeStruct.hours, timeStruct.minutes, timeStruct.seconds);
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
        img.fillCircle(180, 60, 10, RED);
        img.fillCircle(180, 180, 10, BLUE);
        img.fillCircle(60, 180, 10, WHITE);
        img.pushSprite(0, 0, TFT_TRANSPARENT);
        img.deleteSprite();
      }
      break;
    }

    yield();
  }
}

void high_power_task(void *pvParameters)
{
  int state = 0;
  while (1)
  {
    switch (state)
    {
    case 0: // Low power
      if (currentBattery > BATLVLFORHP)
      {
        state = 1;
        is_high_power = true;
      }
      break;
    case 1: // Go to high power
      state = 2;
      break;
    case 2: // Hight power
      if (currentBattery < BATLVLFORHP)
      {
        state = 3;
      }
      break;
    case 3: // Go to low power
      state = 0;
      is_high_power = false;
      break;
    }

    // if (currentBattery < BATLVLFORHP)
    // {
    //   xTimerStop(hp_timer, 0);
    // }
    // // Connect to an access point
    // Serial.print("Connecting to Wi-Fi ");
    // WiFi.begin(ssid, password);
    // Serial.print(" At this point ");
    // while (WiFi.status() != WL_CONNECTED)
    // {
    //   delay(500);
    //   Serial.print(".");
    // }
    // Serial.println(" CONNECTED");

    // // Set ntp time to local
    // configTime(6 * 3600, 0, ntpServer);

    // // Get local time
    // struct tm timeInfo;
    // if (getLocalTime(&timeInfo))
    // {
    //   printf("Got local time");
    //   // Set RTC time
    //   I2C_BM8563_TimeTypeDef timeStruct;
    //   timeStruct.hours = timeInfo.tm_hour;
    //   timeStruct.minutes = timeInfo.tm_min;
    //   timeStruct.seconds = timeInfo.tm_sec;
    //   rtc.setTime(&timeStruct);

    //   // Set RTC Date
    //   I2C_BM8563_DateTypeDef dateStruct;
    //   dateStruct.weekDay = timeInfo.tm_wday;
    //   dateStruct.month = timeInfo.tm_mon + 1;
    //   dateStruct.date = timeInfo.tm_mday;
    //   dateStruct.year = timeInfo.tm_year + 1900;
    //   rtc.setDate(&dateStruct);
    // }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void battery_level_percent(TimerHandle_t xTimer)
{
  int32_t mvolts = 0;
  for (int8_t i = 0; i < NUM_ADC_SAMPLE; i++)
  {
    mvolts += analogReadMilliVolts(D0);
  }
  mvolts /= NUM_ADC_SAMPLE;
  int32_t level = (mvolts - BATTERY_DEFICIT_VOL) * 100 / (BATTERY_FULL_VOL - BATTERY_DEFICIT_VOL); // 1850 ~ 2100
  level = (level < 0) ? 0 : ((level > 100) ? 100 : level);
  int prev_Battery = currentBattery;
  currentBattery = level;

  // if (currentBattery > BATLVLFORHP && prev_Battery < BATLVLFORHP)
  // {
  //   printf("Starting HP task\n");
  //   xTimerStart(hp_timer, 0);
  // }
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
  img.fillCircle(x, y, 5, primary_color);

  // Draw the battery level
  int16_t textWidth = img.textWidth("000"); // Estimate the maximum width for a 3-digit number
  int16_t textHeight = 24;                  // Adjust based on text size
  img.fillRect(center_x - (textWidth / 2), 180, textWidth, textHeight, BLACK);
  img.setTextSize(2);
  img.drawCentreString(String(currentBattery) + "%", center_x, 180, 1);

  // Update the high power indicator
  img.drawSmoothArc(center_x, center_y, 72, 65, 140, 220, is_high_power ? TEAL : BLACK, BLACK, true);

  img.pushSprite(0, 0, TFT_TRANSPARENT);
  img.deleteSprite();
}

void setup()
{
  // WiFi.mode(WIFI_STA);
  // WiFi.disconnect();

  tft.begin();
  Serial.begin(9600);

  // Start I2C and RTC
  Wire.begin();
  rtc.begin();

  pinMode(TOUCH_INT, INPUT_PULLUP);

  tft.setRotation(0);
  tft.fillScreen(BLACK);

  img.setColorDepth(16);

  xTaskCreate(&state_machine_task, "state_machine_task", 4096, NULL, 5, NULL);
  xTimerStart(xTimerCreate("battery_monitor", pdMS_TO_TICKS(1000), pdTRUE, NULL, &battery_level_percent), 0);
  xTaskCreate(&high_power_task, "high_power_task", 4096, NULL, 5, NULL);
  // hp_timer = xTimerCreate("high_power", pdMS_TO_TICKS(10000), pdTRUE, NULL, &high_power_timer);

  vTaskDelete(NULL);
}
void loop()
{
  vTaskDelete(NULL);
}