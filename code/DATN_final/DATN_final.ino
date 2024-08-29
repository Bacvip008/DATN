#include <math.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30100_PulseOximeter.h"
#include <WiFi.h>
#include "RTClib.h"
#include <Arduino.h>
#include "FS.h"
#include <LittleFS.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/SDHelper.h>
#include "icons.h"

#define USER_EMAIL "datn@gmail.com"
#define USER_PASSWORD "12345678"

#define API_KEY "AIzaSyC7P9qNRa0hBBQxncWe_OoiZQ0r4xu562c"
#define STORAGE_BUCKET_ID "datn-f2cad.appspot.com"
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool taskCompleted = false;  //task cua firebase

#define POX_REPORTING_PERIOD_MS 1000

#define FORMAT_LITTLEFS_IF_FAILED true

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  // Obtained from I2C Scanner. Look for the program in Github.
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String SDT1 = "0386610288";
String SDT2 = "0989235843";

RTC_DS3231 rtc;

char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

int alarmOption = 0;

int nguongSpO2 = 93;

int gio = 0;
int phut = 0;
int giay = 0;
int ngay = 0;
int thang = 0;
int nam = 0;

int baothuc_gio = 69;
int baothuc_phut = 69;
int baothuc_giay = 0;

PulseOximeter pox;
uint32_t poxLastReport = 0;
uint32_t prevMillis = 0;
int sp = 0;
int hr = 0;

int hr_standard = 90;
int temp_hr[3] = { 0 };
int hr_history[3] = { 0 };
int hr_index = 0;
int hr_after_filter = 0;
int sp_history[3] = { 0 };
int sp_index = 0;

int tatBuzzer = 1;

int option = 1;
int cursor = 1;
int manhinh = 1;
int xoa = 1;
int xoa1 = 1;

const int pinOkBtn = 13;
const int pinUpBtn = 14;
const int pinDownBtn = 12;
int counter = 0;
int OKnumber = 0;

//các biến và khai báo
const char *ssid = "Hehe";
const char *password = "12345678";
int attempt = 0;
int wifi_timeout = 15;
int wifi_timeout2 = 5;
int mode = 0;
const unsigned long delay15p = 15000 * 60;  //*60
const unsigned long delay1p = 1000 * 60;
float thoigian = 0;
unsigned long previoustime = 0;
unsigned long rightnow = 0;

unsigned long int resetTime = 0;
bool secondLoop = 0;

const int buzzerPin = 23;

class BtnClass {
public:
  const int debounceThres = 50;  // Debouncing threshold in miliseconds
  bool lastSteadyState = LOW;
  bool lastBounceState = LOW;
  int buttonPin;
  bool isPressed;
  bool currentState;
  int *counter;
  int modifier = 0;


  unsigned long int lastDebounceTime;

  BtnClass(int pin, int number, int *count) {
    buttonPin = pin;    // assigns the button pin
    modifier = number;  // what value is added to the counter
    counter = count;    // store counter variable address so this class can change the counter variable.
  }



  // Member function to add a value to the counter.
  void addCounter() {
    if (manhinh == 1) {
      *counter += modifier;
      if (*counter < 0)  // limits the counter so it cannot be a negative number.
        *counter = 2;
      else if (*counter > 2)
        *counter = 0;
    } else if (manhinh == 2) {
      if (OKnumber == 2) {
        *counter += modifier;
        if (*counter < 3)  // limits the counter so it cannot be a negative number.
          *counter = 5;
        else if (*counter > 5)
          *counter = 3;
      } else if (OKnumber == 3 && alarmOption == 1) {
        *counter += modifier;
        if (*counter < 6)  // limits the counter so it cannot be a negative number.
          *counter = 30;
        else if (*counter > 30)
          *counter = 6;
      } else if (OKnumber == 3 && alarmOption == 2) {
        *counter += modifier;
        if (*counter < 30)  // limits the counter so it cannot be a negative number.
          *counter = 43;
        else if (*counter > 43)
          *counter = 31;
      }
    } else if (manhinh == 3) {
      *counter += modifier;
      if (*counter < 43)  // limits the counter so it cannot be a negative number.
        *counter = 48;
      else if (*counter > 48)
        *counter = 43;
    }
  }



  // Member function to always check the button.
  void buttonCheck() {
    // read the current state of the button
    currentState = digitalRead(buttonPin);

    // Button debouncing
    // For more information on debouncing, check ESP32.io
    if (millis() - lastDebounceTime > debounceThres) {
      if (lastSteadyState == HIGH && currentState == LOW) {
        isPressed = true;
        addCounter();  // Only adds the counter when button is pressed (after the debouncing)
        noTone(buzzerPin);
      } else if (lastSteadyState == LOW && currentState == HIGH) {
        isPressed = false;
      }

      lastSteadyState = currentState;
    }

    if (currentState != lastBounceState) {
      lastDebounceTime = millis();
      lastBounceState = currentState;
    }
  }
};

BtnClass upButton(pinUpBtn, 1, &counter);
BtnClass downButton(pinDownBtn, -1, &counter);

class OKBtnClass {
public:
  const int debounceThres = 50;  // Debouncing threshold in miliseconds
  bool lastSteadyState = LOW;
  bool lastBounceState = LOW;
  int buttonPin;
  bool isPressed;
  bool currentState;
  int *OKnumber;
  int modifier = 0;
  unsigned long int lastDebounceTime;

  OKBtnClass(int pin, int number, int *OKnum) {
    buttonPin = pin;    // assigns the button pin
    modifier = number;  // what value is added to the counter
    OKnumber = OKnum;   // Assign address to OKnumber
  }

  void addOKnumber() {
    if (manhinh == 1 or manhinh == 4 or manhinh == 3) {
      *OKnumber += modifier;
      if (*OKnumber > 1)
        *OKnumber = 0;
    } else if (manhinh == 2) {
      *OKnumber += modifier;
      if (*OKnumber > 3)
        *OKnumber = 2;
      else if (*OKnumber < 2)
        *OKnumber = 3;
    }
  }
  // Member function to always check the button.
  void buttonCheck() {
    // read the current state of the button
    currentState = digitalRead(buttonPin);

    // Button debouncing
    // For more information on debouncing, check ESP32.io
    if (millis() - lastDebounceTime > debounceThres) {
      if (lastSteadyState == HIGH && currentState == LOW) {
        isPressed = true;
        addOKnumber();
        noTone(buzzerPin);
      } else if (lastSteadyState == LOW && currentState == HIGH) {
        isPressed = false;
      }

      lastSteadyState = currentState;
    }

    if (currentState != lastBounceState) {
      lastDebounceTime = millis();
      lastBounceState = currentState;
    }
  }
};

OKBtnClass OKButton(pinOkBtn, 1, &OKnumber);

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);

  //setup cho màn hình
  initialScreen();

  //setup cho max30100
  initialMax30100();

  //setup cho wifi
  initialWifi();

  //setup cho sim
  initialSim();

  //setup cho nút
  pinMode(pinUpBtn, INPUT_PULLUP);
  pinMode(pinDownBtn, INPUT_PULLUP);
  pinMode(pinOkBtn, INPUT_PULLUP);

  //setup cho rtc
  initialRTC();
  //rtc.adjust(DateTime(2024, 6, 28, 20, 59, 0));
  //setup cho file
  initialFile();

  //setup cho firebase
  if (WiFi.status() == WL_CONNECTED)
    initialFirebase();

  pox.begin();
}

void loop() {
  BuzzerTimer();
  uploadFile();
  pox.update();
  rtcModule();
  baothuc();
  switch (mode) {
    case 0:
      // Serial.println("Offline mode");
      displaymode();
      upButton.buttonCheck();
      downButton.buttonCheck();
      OKButton.buttonCheck();
      bothButton();
      SpO2Alert();
      batbuzzer();
      pickoption();
      chooseOption();
      screen();
      if (rightnow == 0) {
        rightnow = millis();
      }

      // Serial.println(millis() - rightnow);
      if (millis() - rightnow >= delay15p) {
        Serial.println("try to reconnecting to wifi");
        WiFi.begin(ssid, password);
        delay(1500);

        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("\nConnected!");
          Serial.print("IP Address: ");
          Serial.println(WiFi.localIP());
          Serial.println("wifi connected!");
          Serial.println("Changing to online mode!");
          initialFirebase();
          mode = 1;
          pox.begin();
          rightnow = 0;
        } else {
          Serial.println("There's no wifi!");
          pox.begin();
          rightnow = 0;
        }
      }


      break;
    case 1:
      // Serial.println("Online mode");
      displaymode();
      upButton.buttonCheck();
      downButton.buttonCheck();
      OKButton.buttonCheck();
      bothButton();
      SpO2Alert();
      batbuzzer();
      pickoption();
      chooseOption();
      screen();
      if (WiFi.status() != WL_CONNECTED) {
        mode = 0;
        Serial.print("Wifi disconnected, changing to offline mode\n");
        pox.begin();
      }
      break;
  }
  //debug only

  Serial.print(" counter: ");
  Serial.print(counter);

  Serial.print(" OKnumber: ");
  Serial.println(OKnumber);



  // For debugging the buttons only
  //Serial.printf("UpButtonState:%d,DownButtonState:%d,UpIsPressed:%d,DownIsPressed:%d,OKButtonState:%d,OKIsPressed:%d\n", upButton.currentState, downButton.currentState, upButton.isPressed, downButton.isPressed,OKButton.currentState, OKButton.isPressed);
  // Serial.printf("millis():%d,resettime():%d\n", millis(), resetTime);
}

void rtcModule() {
  DateTime now = rtc.now();
  gio = now.hour();
  phut = now.minute();
  giay = now.second();
  ngay = now.day();
  thang = now.month();
  nam = now.year();
}

// Function to make the OLED displays content centered with X and Y offset
void displayCenter(String text, int X, int Y) {
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;

  display.getTextBounds(text, 0, 0, &x1, &y1, &width, &height);

  display.setCursor((SCREEN_WIDTH - width) / 2 + X, (SCREEN_HEIGHT - height) / 2 + Y);
  display.println(text);
}

// Function to show a RESET display to the OLED
void KhanCap() {
  display.clearDisplay();
  display.setTextSize(2);
  tone(buzzerPin, 2000);
  displayCenter("EMERGENCY\n    CALL", 0, 0);
  goidien(SDT1);
  guitinnhan(SDT2, "Canh bao khan cap");
  display.display();
  display.setTextSize(1);
  delay(2000);
  pox.begin();
  display.clearDisplay();  // delay to not receive any input from buttons
}

void splashScreen() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3);
  displayCenter("HEALTH\n   CARE", 0, 0);
  display.display();
}

void batbuzzer() {
  if (upButton.isPressed && downButton.isPressed) {
    tone(buzzerPin, 2000);
  }
}

void bothButton() {
  if (upButton.isPressed && downButton.isPressed) {
    // Timing for 2 seconds
    if (millis() - resetTime > 1000) {
      // if already passed the 2 second mark
      if (secondLoop) {
        counter = 0;
        KhanCap();
      }
      secondLoop = true;
      resetTime = millis();
    }
  } else {
    secondLoop = false;
  }
}

void goidien(String sdtnhan) {

  Serial2.print("ATD");
  delay(200);
  Serial2.print(SDT1);
  delay(200);
  Serial2.println(";");
}

void guitinnhan(String sdtnhan, String noidung) {

  Serial2.print("AT+CMGS=\"");
  delay(200);
  Serial2.print(sdtnhan);
  delay(200);
  Serial2.println("\"");
  delay(200);
  Serial2.print(noidung);
  delay(200);
  Serial2.write(26);
  delay(200);
}

void uploadFile() {
  if (gio == 23 && phut == 59 && giay == 59) {
    if (Firebase.ready() && !taskCompleted) {
      taskCompleted = true;

      Serial.println("\nUpload file...\n");
      // MIME type should be valid to avoid the download problem.
      // The file systems for flash and SD/SDMMC can be changed in FirebaseFS.h.
      if (!Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID /* Firebase Storage bucket id */, "data.csv" /* path to local file */, mem_storage_type_flash /* memory storage type, mem_storage_type_flash and mem_storage_type_sd */, String(ngay) + "-" + String(thang) + "-" + String(nam) + "/data.csv" /* path of remote file stored in the bucket */, "file/csv" /* mime type */, fcsUploadCallback /* callback function */))
        Serial.println(fbdo.errorReason());
      pox.begin();
    } else {
      //deleteFile(LittleFS, "/data.csv");
      writeFile(LittleFS, "/data.csv", "Date, Time, Heartrate, SpO2 \r");
      pox.begin();
    }
  }
}

void fcsUploadCallback(FCS_UploadStatusInfo info) {
  if (info.status == firebase_fcs_upload_status_init) {
    Serial.printf("Uploading file %s (%d) to %s\n", info.localFileName.c_str(), info.fileSize, info.remoteFileName.c_str());
  } else if (info.status == firebase_fcs_upload_status_upload) {
    Serial.printf("Uploaded %d%s, Elapsed time %d ms\n", (int)info.progress, "%", info.elapsedTime);
  } else if (info.status == firebase_fcs_upload_status_complete) {
    Serial.println("Upload completed\n");
    FileMetaInfo meta = fbdo.metaData();
    Serial.printf("Name: %s\n", meta.name.c_str());
    Serial.printf("Bucket: %s\n", meta.bucket.c_str());
    Serial.printf("contentType: %s\n", meta.contentType.c_str());
    Serial.printf("Size: %d\n", meta.size);
    Serial.printf("Generation: %lu\n", meta.generation);
    Serial.printf("Metageneration: %lu\n", meta.metageneration);
    Serial.printf("ETag: %s\n", meta.etag.c_str());
    Serial.printf("CRC32: %s\n", meta.crc32.c_str());
    Serial.printf("Tokens: %s\n", meta.downloadTokens.c_str());
    Serial.printf("Download URL: %s\n\n", fbdo.downloadURL().c_str());
  } else if (info.status == firebase_fcs_upload_status_error) {
    Serial.printf("Upload failed, %s\n", info.errorMsg.c_str());
  }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char *path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char *path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  Serial.println("- read from file:");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {

  File file = fs.open(path, FILE_APPEND);
  if (hr_history[0] == hr_history[1] && hr_history[0] == hr_history[2])
    pox.begin();
  file.print(message);
  pox.update();
  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\r\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("- file renamed");
  } else {
    Serial.println("- rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}

void initialFirebase() {
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;  // see addons/TokenHelper.h

  // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
  Firebase.reconnectNetwork(true);

  // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
  // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  Firebase.begin(&config, &auth);

  /* Assign upload buffer size in byte */
  // Data to be uploaded will send as multiple chunks with this size, to compromise between speed and memory used for buffering.
  // The memory from external SRAM/PSRAM will not use in the TCP client internal tx buffer.
  config.fcs.upload_buffer_size = 512;
}

void initialWifi() {
  Serial.print("Connecting to Wi-Fi network: ");
  Serial.println(ssid);

  // Attempt connection with timeout and proper error handling

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED && attempt < wifi_timeout) {
    delay(500);  // Introduce a delay between attempts
    Serial.print(".");
    attempt++;
  }

  // Handle connection status
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    mode = 1;
  } else {
    Serial.println("\nFailed to connect after retries.");
    mode = 0;
  }
}

void initialScreen() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  splashScreen();
  delay(1000);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
}

void initialSim() {
  Serial2.print("AT\r\n");    // Gui lenh AT test SIM
  Serial2.print("ATE0\r\n");  // OFF rep
  Serial2.print("AT+CSCS=\"GSM\"\r\n");
  Serial2.print("AT+CMGF=1\r\n");
  Serial2.print("AT+CNMI=2,2,0,0,0\r\n");
  Serial2.print("AT+CMGDA=\"DEL ALL\"\r\n");
  Serial2.print("AT+CLIP=1\r\n");
  Serial2.print("AT&W\r\n");
}

void initialRTC() {
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
}

void initialFile() {
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    Serial.println("LittleFS Mount Failed");
    return;
  }
  writeFile(LittleFS, "/data.csv", "Date, Time, Heartrate, SpO2 \r");  //Create and write a new file in the root directory
  //appendFile(LittleFS, "/data.csv", (String(n) + "," + String(n+20) + "\r\n").c_str());
  readFile(LittleFS, "/data.csv");
  Serial.println("Test complete");
}

void initialMax30100() {
  Serial.print("Initializing pulse oximeter..");

  if (!pox.begin()) {
    Serial.println("FAILED");
    for (;;)
      ;
  } else {
    Serial.println("SUCCESS");
  }

  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

void onBeatDetected() {
  Serial.println("Beat!");
  //display.drawBitmap(24,40,heartss,15,15,1);
  display.display();
}

void cambientim() {
  pox.update();

  if (millis() - poxLastReport > POX_REPORTING_PERIOD_MS) {
    hr = round(pox.getHeartRate());
    //sp = round(pox.getSpO2());
    sp = pox.getSpO2();
    if (hr != 0 && sp != 0) {
      pox.update();
      appendFile(LittleFS, "/data.csv", (String(ngay) + "/" + String(thang) + "/" + String(nam) + "," + String(gio) + ":" + String(phut) + ":" + String(giay) + "," + String(hr) + "," + String(sp) + "\r\n").c_str());
      pox.update();
    }
    hr_history[hr_index] = hr;
    hr_index = (hr_index + 1) % 3;
    if (sp != 0 && sp >= 88) {
      sp_history[sp_index] = sp;
      sp_index = (sp_index + 1) % 3;
    }
    // Serial.print("Heart rate:");
    // Serial.print(hr);
    // Serial.print("bpm / SpO2:");
    // Serial.print(sp);
    // Serial.println("%");

    poxLastReport = millis();
  }
}

void menu() {
  display.setCursor(20, 24);
  display.println(F("Bao thuc"));
  display.setCursor(20, 32);
  display.println(F("Nguong SpO2"));
  display.setCursor(20, 40);
  display.println(F("Thong tin"));
  // display.setCursor(20, 24);
  // display.println(F("option 4"));
  display.display();
}

void hienthithongso() {
  cambientim();
  display.setCursor(0, 0);
  display.println(F("Nhip tim: "));
  display.fillRect(60, 0, 18, 8, SSD1306_BLACK);
  display.setCursor(60, 0);
  display.print(hr);  // Hiển thị số đếm
  // Xóa kí tự cũ trước khi ghi số mới
  // display.fillRect(0, 8, SCREEN_WIDTH, 8, SSD1306_BLACK);
  display.fillRect(30, 8, 60, 8, SSD1306_BLACK);
  display.setCursor(0, 8);
  display.print(F("SpO2: "));
  display.print(sp);  // Hiển thị số đếm
  display.display();
}

void clear() {
  if (xoa == 1) {
    // Serial.println("man hinh se duoc xoa");
    display.clearDisplay();
    xoa = 0;
  }
}

void clear2() {
  if (xoa1 == 1) {
    Serial.println("man hinh se duoc xoa");
    display.clearDisplay();
    xoa1 = 0;
  }
}

void displaymode() {
  display.fillRect(0, 56, 500, 8, SSD1306_BLACK);
  display.setCursor(0, 56);
  //display.print("hh:mm");
  display.print(gio);
  display.print(":");
  display.print(phut);
  display.setCursor(62, 56);
  //display.print("dd/mm/yyyy");
  display.print(ngay);
  display.print("/");
  display.print(thang);
  display.print("/");
  display.print(nam);
  display.display();
  switch (mode) {
    case 0:
      display.fillRect(85, 0, 40, 8, SSD1306_BLACK);
      display.setCursor(85, 0);
      display.print("offline");
      break;
    case 1:
      display.fillRect(85, 0, 40, 8, SSD1306_BLACK);
      display.setCursor(87, 0);
      display.print("online");
      break;
  }
}

void pickoption() {
  if (manhinh == 1 && OKnumber == 0) {  //biến khống chế vào mode
    switch (counter) {
      case 0:
        {
          xoa = 1;
          menu();
          display.setCursor(0, 24);
          display.println(F(">"));
          display.fillRect(0, 32, 8, 8, SSD1306_BLACK);
          display.fillRect(0, 40, 8, 8, SSD1306_BLACK);
          // display.fillRect(0, 24, 30, 8, SSD1306_BLACK);
          // Serial.println("1");
          break;
        }
      case 1:
        {
          xoa = 1;
          menu();
          display.setCursor(0, 32);
          display.println(F(">"));
          display.fillRect(0, 24, 8, 8, SSD1306_BLACK);
          display.fillRect(0, 40, 8, 8, SSD1306_BLACK);
          // display.fillRect(0, 24, 30, 8, SSD1306_BLACK);
          // Serial.println("2");
          break;
        }
      case 2:
        {
          xoa = 1;
          menu();
          display.setCursor(0, 40);
          display.println(F(">"));
          display.fillRect(0, 24, 8, 8, SSD1306_BLACK);
          display.fillRect(0, 32, 8, 8, SSD1306_BLACK);
          // display.fillRect(0, 24, 30, 8, SSD1306_BLACK);
          // Serial.println("3");
          break;
        }
        // case 3:
        //   {
        //     xoa = 1;
        //     menu();
        //     display.setCursor(0, 24);
        //     display.println(F(">"));
        //     display.fillRect(0, 8, 30, 8, SSD1306_BLACK);
        //     display.fillRect(0, 16, 30, 8, SSD1306_BLACK);
        //     display.fillRect(0, 0, 30, 8, SSD1306_BLACK);
        //     // Serial.println("4");
        //     break;
        //   }
    }
  }
}

void chooseOption() {
  if (OKnumber == 0) {
    manhinh = 1;
    xoa = 1;
    clear2();

  } else if (OKnumber == 1) {
    switch (counter) {
      case 0:
        if (OKnumber == 1) {
          OKnumber == 2;
        }
        manhinh = 2;
        break;

      case 1:
        manhinh = 3;
        break;

      case 2:
        manhinh = 4;
        break;
    }
  }
}

void turnOnBuzzer() {
  tone(buzzerPin, 1000);
  tatBuzzer = 0;
}

void BuzzerTimer() {
  if (tatBuzzer == 0) {
    tatBuzzer = millis();
  }

  if (millis() - tatBuzzer >= delay1p) {
    Serial.println("woooooo");
    noTone(buzzerPin);
    tatBuzzer = 0;
  }
}

void baothuc() {
  if (baothuc_gio == gio && baothuc_phut == phut && baothuc_giay == giay) {
    Serial.println("gio an toi roi gio an toi roi");
    turnOnBuzzer();
  }
}

void alarmChange() {
  if (OKnumber == 3 && counter == 3)
    alarmOption = 1;
  else if (OKnumber == 3 && counter == 4)
    alarmOption = 2;
  else if (OKnumber == 3 && counter == 5) {
    alarmOption = 0;
    OKnumber = 0;
  }


  if (OKnumber == 3 && alarmOption == 1) {
    switch (counter) {
      case 6:
        baothuc_gio = 1;
        break;

      case 7:
        baothuc_gio = 2;
        break;

      case 8:
        baothuc_gio = 3;
        break;

      case 9:
        baothuc_gio = 4;
        break;

      case 10:
        baothuc_gio = 5;
        break;

      case 11:
        baothuc_gio = 6;
        break;

      case 12:
        baothuc_gio = 7;
        break;

      case 13:
        baothuc_gio = 8;
        break;

      case 14:
        baothuc_gio = 9;
        break;

      case 15:
        baothuc_gio = 10;
        break;

      case 16:
        baothuc_gio = 11;
        break;

      case 17:
        baothuc_gio = 12;
        break;

      case 18:
        baothuc_gio = 13;
        break;

      case 19:
        baothuc_gio = 14;
        break;

      case 20:
        baothuc_gio = 15;
        break;

      case 21:
        baothuc_gio = 16;
        break;

      case 22:
        baothuc_gio = 17;
        break;

      case 23:
        baothuc_gio = 18;
        break;

      case 24:
        baothuc_gio = 19;
        break;

      case 25:
        baothuc_gio = 20;
        break;

      case 26:
        baothuc_gio = 21;
        break;

      case 27:
        baothuc_gio = 22;
        break;

      case 28:
        baothuc_gio = 23;
        break;

      case 29:
        baothuc_gio = 24;
        break;

      case 30:
        baothuc_gio = 0;
        break;
    }
  }

  if (OKnumber == 3 && alarmOption == 2) {
    switch (counter) {
      case 31:
        baothuc_phut = 0;
        break;

      case 32:
        baothuc_phut = 5;
        break;

      case 33:
        baothuc_phut = 10;
        break;

      case 34:
        baothuc_phut = 15;
        break;

      case 35:
        baothuc_phut = 20;
        break;

      case 36:
        baothuc_phut = 25;
        break;

      case 37:
        baothuc_phut = 30;
        break;

      case 38:
        baothuc_phut = 35;
        break;

      case 39:
        baothuc_phut = 40;
        break;

      case 40:
        baothuc_phut = 45;
        break;

      case 41:
        baothuc_phut = 50;
        break;

      case 42:
        baothuc_phut = 55;
        break;

      case 43:
        baothuc_phut = 60;
        break;
    }
  }
}

void SpO2Alert() {
  if (sp != 0 && sp >= 88) {
    if (sp_history[0] <= nguongSpO2 && sp_history[1] <= nguongSpO2 && sp_history[2] <= nguongSpO2 && sp_history[0] != 0 && sp_history[1] != 0 && sp_history[2] != 0) {
      counter = 0;
      SpO2khancap();
      sp_history[0] = 0;
      sp_history[1] = 0;
      sp_history[2] = 0;
      pox.update();
    }
  }
}

void SpO2khancap() {
  Serial.println("canh bao spo2");
  pox.update();
  tone(buzzerPin, 2000);
  guitinnhan(SDT2,"SpO2 qua thap");
  goidien(SDT1);
  pox.begin();
}

void SpO2config() {
  if (OKnumber == 1 && manhinh == 3) {
    switch (counter) {
      case 43:
        nguongSpO2 = 95;
        break;

      case 44:
        nguongSpO2 = 94;
        break;

      case 45:
        nguongSpO2 = 93;
        break;

      case 46:
        nguongSpO2 = 92;
        break;

      case 47:
        nguongSpO2 = 91;
        break;

      case 48:
        nguongSpO2 = 90;
        break;
    }
  }
}

void hienthiSpO2() {
  display.setTextSize(2);
  display.fillRect(0, 24, 200, 30, SSD1306_BLACK);
  displayCenter(String(nguongSpO2).c_str(), 0, 0);
  display.setTextSize(1);
}

void hienThiAlarm() {
  if (counter == 0 && OKnumber == 1) {
    display.setTextSize(1);
    display.setCursor(0, 8);
    display.print("press OK again");
  }
  if (OKnumber == 2) {
    display.fillRect(0, 8, 100, 20, SSD1306_BLACK);
  }
  if (counter == 3) {
    display.setTextSize(2);
    display.fillRect(0, 8, 100, 20, SSD1306_BLACK);
    display.fillRect(100, 24, 20, 20, SSD1306_BLACK);
    display.setCursor(12, 24);
    display.print(">");
    display.setTextSize(1);
  } else if (counter == 4) {
    display.fillRect(0, 8, 100, 20, SSD1306_BLACK);
    display.fillRect(12, 24, 20, 20, SSD1306_BLACK);
    display.setCursor(100, 24);
    display.print("<");
    display.setTextSize(1);
  } else if (counter == 5) {
    display.fillRect(0, 8, 100, 20, SSD1306_BLACK);
    display.setTextSize(1);
    display.setCursor(2, 47);
    display.print(">");
  }
}

void screen() {
  switch (manhinh) {
    case 1:
      hienthithongso();
      menu();
      break;

    case 2:
      clear();
      alarmChange();
      // Serial.println("chuyen sang man hinh option 1");
      pox.update();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("Bao thuc");
      display.setTextSize(2);
      display.fillRect(0, 24, 200, 30, SSD1306_BLACK);
      hienThiAlarm();
      display.setTextSize(2);
      //display.setCursor(30,24);
      //display.print(baothuc_gio);
      displayCenter((String(baothuc_gio) + ":" + String(baothuc_phut)).c_str(), 0, 0);
      display.setTextSize(1);
      display.setCursor(10, 47);
      display.print("Back");
      display.display();
      xoa1 = 1;
      break;

    case 3:
      clear();
      // Serial.println("chuyen sang man hinh option 2");
      pox.update();
      display.setCursor(0, 0);
      display.println("Nguong SpO2");
      hienthiSpO2();
      SpO2config();
      display.display();
      xoa1 = 1;
      break;

    case 4:

      clear();
      // Serial.println("chuyen sang man hinh option 3");
      pox.update();
      display.setCursor(0, 0);
      display.println("Thong tin");
      display.setCursor(36, 20);
      display.println("Max30100");
      display.setCursor(37, 30);
      display.println("SSD1306");
      display.setCursor(45, 40);
      display.println("ESP32");
      // display.setCursor(0, 30);
      // display.print("thiet bi theo doi suc khoe di dong");
      display.display();
      xoa1 = 1;
      break;
  }
}