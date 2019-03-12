#include <Arduino.h>
#include <driver/dac.h>
#include <M5Stack.h>
#include <OmronD6T.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <string>

#define JST 3600* 9

 const char* wifi_ssid = "<SSID名>";
 const char* wifi_pass = "<SSIDパスワード>";

// IFTTT Webhook URL
const char* webhook_url = "https://maker.ifttt.com/trigger/<Webhook イベント>/with/key/<Webhook キー>";

struct tm detect_time; // 検知時刻を格納するtm構造体
char format_time[20];  // 出力文字格納用

OmronD6T sensor;

HTTPClient http;

int color[7];
const int range_min = 25;
const int range_max = 35;

uint16_t rgbToColor(uint8_t red, uint8_t green, uint8_t blue)
{
  return ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
}

int getRangeColor(float temperature)
{
  float range = range_max - range_min;
  float block = range / 7;

  if (temperature < range_min)
  {
    return 0;
  }
  else if (temperature < (range_min + block))
  {
    return 1;
  }
  else if (temperature < (range_min + (block * 2)))
  {
    return 2;
  }
  else if (temperature < (range_min + (block * 3)))
  {
    return 3;
  }
  else if (temperature < (range_min + (block * 4)))
  {
    return 4;
  }
  else if (temperature < (range_min + (block * 5)))
  {
    return 5;
  }
  else
  {
    return 6;
  }
}

void initColormap() {
  color[0] = rgbToColor(76, 134, 228);
  color[1] = rgbToColor(127, 178, 248);
  color[2] = rgbToColor(177, 208, 245);
  color[3] = rgbToColor(221, 220, 219);
  color[4] = rgbToColor(252, 194, 173);
  color[5] = rgbToColor(255, 151, 125);
  color[6] = rgbToColor(234, 94, 78);
}

void send_webhook(String event_type, String event_place, String event_time)
{
  Serial.println("send_webhook() called.");
  String url = String(webhook_url);
  url.concat("?value1=");
  url.concat(event_type);
  url.concat("&value2=");
  url.concat(event_place);
  url.concat("&value3=");
  url.concat(event_time);
  Serial.print("url:");
  Serial.println(url);
  http.begin(url);
  int httpStatusCode = http.GET();
  Serial.print("httpStatusCode:");
  Serial.println(httpStatusCode);
  String body = http.getString();
  Serial.print("Response Body: ");
  Serial.println(body);
  if(httpStatusCode == 200) {
    Serial.println("send webhook succeed");
  }
}

void setup() {
  M5.begin();
  dac_output_disable(DAC_CHANNEL_1);
  Serial.begin(115200);
  delay(1000);

  WiFi.begin(wifi_ssid, wifi_pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    M5.Lcd.printf(".");
  }
  
  M5.Lcd.println("\nwifi connect ok");

  initColormap();

  // 時間設定
  configTime( JST, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
}

void loop() {
  int upsell = 0; // セル
  int detect_flg = 0; // 検知フラグ
  
  sensor.scanTemp();

  int16_t rectw = (M5.Lcd.width() / 4);
  int16_t recth = (M5.Lcd.height() / 4);
  
  Serial.println("-------------------------");
  int x, y;
  for (y = 0; y < 4; y++) {
    
    for (x = 0; x < 4; x++) {
      float temperature = sensor.temp[x][y];
      int rangecolor = getRangeColor(temperature);
      Serial.print("temperature:");
      Serial.print(sensor.temp[x][y]);
      Serial.print(" rangecolor:");
      Serial.println(rangecolor);
      
      // 温度を検知して変化したセルをカウント
      if (rangecolor > 1) {
        upsell++;
      }

      M5.Lcd.fillRect((rectw * x), (recth * y), rectw, recth, color[rangecolor]);
    }
    
    Serial.println("");

    if (upsell > 2 && detect_flg != 1) {
      // 検出フラグをたてる
      detect_flg = 1;
    }
    
  }
  
  if (detect_flg == 1) {    
    Serial.println("誰かいる");

    // 検知した日時を取得
    getLocalTime(&detect_time);
    sprintf(format_time, "%04d/%02d/%02d-%02d:%02d:%02d",
    detect_time.tm_year + 1900, detect_time.tm_mon + 1, detect_time.tm_mday,
    detect_time.tm_hour, detect_time.tm_min, detect_time.tm_sec);
    
    // IFTTTのWebhookに投げる
    send_webhook("HumanDetection!!", "OwnSeat", String(format_time));
  } else {
    Serial.println("誰もいない");
  }
  
  delay(3000);
}
