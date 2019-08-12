#include "WiFiClientSecure.h"
#include "ssl_client.h"
#include "esp_bt.h"


#include "esp_camera.h"

#define DEBUG 1
const char* ssid = "";
const char* password = "";
const char* token = "";

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  65        /* Time ESP32 will go to sleep (in seconds) */
#define MAX_WIFI_CONNECTION_LOOP 100
// ------------------------------------------------
// define the number of bytes you want to access
// ------------------------------------------------
#define EEPROM_SIZE 1

#define SERVER ""
#define URL "/"
#define SECURE 0
#define BOUNDARY     "--------------------------133747188241686651551404"
#define PORT 8081
#define TIMEOUT 8000
#define BUFFERSIZE 500

// ------------------------------------------------
// Pin definition for CAMERA_MODEL_AI_THINKER
// ------------------------------------------------
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#if defined(DEBUG) && DEBUG > 0
 #define DEBUG_PRINT(mess) Serial.print(mess)
 #define DEBUG_PRINTLN(mess) Serial.println(mess)
#else
 #define DEBUG_PRINT(fmt, args...) /* Don't do anything in release builds */
 #define DEBUG_PRINTLN(fmt, args...) /* Don't do anything in release builds */
#endif



// *********************************************************
// * Format HTTP Header
// *********************************************************
String header(String token, size_t length)
{
  String  data;
  data =  F("POST ");
  data += String(URL);
  data += F(" HTTP/1.1\r\n");
  data += F("cache-control: no-cache\r\n");
  data += F("Content-Type: multipart/form-data; boundary=");
  data += BOUNDARY;
  data += "\r\n";
  data += F("User-Agent: PostmanRuntime/6.4.1\r\n");
  data += F("Accept: */*\r\n");
  data += F("Host: ");
  data += SERVER;
  data += F("\r\n");
  data += F("Authorization: ");
  data += F("MTQzODQ6ZnpjaHVyaHVmYWU7YXplYXpl\r\n");
  data += F("Connection: keep-alive\r\n");
  data += F("content-length: ");
  data += String(length);
  data += "\r\n";
  data += "\r\n";
  return (data);
}
// *********************************************************
// * Format HTTP Body
// *********************************************************
String body(String content , String message)
{
  String data;
  data = "--";
  data += BOUNDARY;
  data += F("\r\n");
  if (content == "imageFile")
  {
    data += F("Content-Disposition: form-data; name=\"imageFile\"; filename=\"picture.jpg\"\r\n");
    data += F("Content-Type: image/jpeg\r\n");
    data += F("\r\n");
  }
  else
  {
    data += "Content-Disposition: form-data; name=\"" + content + "\"\r\n";
    data += "\r\n";
    data += message;
    data += "\r\n";
  }
  return (data);
}

// *****************************************************************
// * Send image using HTTP (or HTTPS) protocol
// ***************************************************************** 
String sendImage(String token, String message,uint8_t *data_pic, size_t size_pic)
{
    // ------------------------------------------------
  // Initialize header
  // ------------------------------------------------


  String bodyPic =  body("imageFile", message);
  String bodyEnd =  String("--") + BOUNDARY + String("--\r\n");
  size_t allLen =  bodyPic.length() + size_pic + bodyEnd.length() ;//+ bodyTxt.length() ;
  String headerTxt =  header(token, allLen);
 
  // ------------------------------------------------
  // Init HTTP connection
  // ------------------------------------------------
#if defined(SECURE) && SECURE > 0    
  WiFiClientSecure client;
#else
  WiFiClient client;
#endif
  DEBUG_PRINTLN("EAP"+String(heap_caps_get_free_size(MALLOC_CAP_8BIT)));
  int err = client.connect(SERVER, PORT);
  if (!err)
  {
    DEBUG_PRINT("Connection Failed\n");
    DEBUG_PRINT("Server : "+ String(SERVER) + ":"+String(PORT)+"\n");
    DEBUG_PRINT("Error Code : " + String(err)+"\n");
    return ("connection failed");
  }

  // Send headers 
  client.print(headerTxt  + bodyPic);
  
  // ------------------------------------------------
  // client.write does not work for big packet
  // So split the data
  // ------------------------------------------------
  int i = 0;
  int si = 0;
  int tried = 0;
  while (i<size_pic) {
      if ((i+BUFFERSIZE)>size_pic)
        si = size_pic-i;
      else
        si = BUFFERSIZE;
      err=0;tried=0;

      // For some unknown reasons, there is a lot of packet loss
      // So, make 3 tries for each packet
      // Seems to be OK for most of the cases
      while ((err!=si) && (tried<3)) {
        if (tried>0)
          delay(200);
         err = client.write(data_pic+i,si);
         tried++;
      }

      DEBUG_PRINT(String(i)+"/"+String(si)+":"+String(err)+"->"+String(tried)+"\n");

      i+=BUFFERSIZE;
    };
  
  // Send end of connection
  client.print("\r\n"+bodyEnd);
  // Get response
  long tOut = millis() + TIMEOUT;
  String serverRes = "";
  while (client.connected() && tOut > millis()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      DEBUG_PRINTLN("headers received");
      break;
    }
  }
  // if there are incoming bytes available
  // from the server, read them and print them:
  while (client.available() && tOut > millis()) {
    serverRes += char(client.read());
  }

  DEBUG_PRINT(serverRes);
  return serverRes;
}

void setup() {
  // ------------------------------------------------
  // Open Serial
  // ------------------------------------------------
  String res;
  int tts = TIME_TO_SLEEP;
  psramInit();

  #if defined(DEBUG) && DEBUG > 0
  Serial.begin(115200);
  delay(1000); //Take some time to open up the Serial Monitor
  #endif

  // ------------------------------------------------
  // Camera configuration for AI Thinker ESP 32
  // ------------------------------------------------
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.jpeg_quality = 15;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  //sensor_t * s = esp_camera_sensor_get();
  //s->set_vflip(s, 1);//flip it back
  //s->set_brightness(s, 1);//up the blightness just a bit
  //s->set_saturation(s, -2);//lower the saturation

  // ------------------------------------------------
  // Init Camera
  // ------------------------------------------------
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    DEBUG_PRINT("Camera init failed with error "+String(err));
    return;
  }

  camera_fb_t * fb = NULL;

  // ------------------------------------------------
  // Take Picture with Camera
  // ------------------------------------------------
  fb = esp_camera_fb_get();
  if (!fb) {
    DEBUG_PRINT("Camera capture failed");

  } else {
    // ------------------------------------------------
    // Wifi Connection
    // ------------------------------------------------
    WiFi.begin(ssid, password);
    int cnt = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      DEBUG_PRINT('.');
      if (cnt++ > MAX_WIFI_CONNECTION_LOOP )
      {
          esp_sleep_enable_timer_wakeup(60 * uS_TO_S_FACTOR);
          DEBUG_PRINTLN("\nFAILED TO CONNECT WIFI : Setup ESP32 to sleep for every 60 Seconds");
         esp_deep_sleep_start();

      }
 }


    DEBUG_PRINTLN("Connected");
    DEBUG_PRINT("Ip Address : ");
    DEBUG_PRINTLN(WiFi.localIP());
    DEBUG_PRINT("MAC Address : ");
    DEBUG_PRINTLN(WiFi.macAddress());
    DEBUG_PRINT("Sending image to server, size : ");
    DEBUG_PRINTLN(String(fb->len));
    delay(500);
    
    // ------------------------------------------------
    // Send image through HTTP protocol
    // ------------------------------------------------
    res = sendImage("asdw", "A54S89EF5", fb->buf, fb->len);
    DEBUG_PRINTLN("Return :"+res);
    esp_camera_fb_return(fb);


    int index = res.indexOf("TTS:");
    if (index>-1) {
      String ret = res.substring(index+4);
      for (index=0; index<ret.length();index++)
        if (ret.charAt(index)<'0' && ret.charAt(index) > '9') {
          ret.remove(index);
          break;
        }
      DEBUG_PRINTLN("SLEEPING :" + ret);
      tts = ret.toInt();
      if (tts<60)
        tts = TIME_TO_SLEEP;
    }

  }

  // ------------------------------------------------
  // Deep Sleep
  // ------------------------------------------------
  esp_sleep_enable_timer_wakeup(tts * uS_TO_S_FACTOR);
  DEBUG_PRINTLN("Setup ESP32 to sleep for every " + String(tts) + " Seconds");
  esp_deep_sleep_start();

}



// *********************************************************
// * Loop never used (setup to deep sleep)
// *********************************************************
void loop( ) {
}
