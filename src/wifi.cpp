#include "decls.h"
//WiFi routines
// NTP handler
// OTA window

volatile int wifiState;

bool wifiStartScan = false;
volatile bool RTCSet = false;
volatile bool OTAprog = false;
volatile bool wifiStarted = false;
volatile bool wifiScan = false; //Begin scan?
volatile bool wifiNeedPass = false; //Scanning?

void setupOTA(const char* host, const char* pass);
void initMDNS();
void NTPBegin();
void sendNTPpacket(const char* address);
void wlanNotFound();
void wifiTerminalBegin();

void setupWifi() {
#ifdef CONFIG_MBEDTLS_THREADING_C
  setupTLS();
#endif
  //Manual Wifi
  WiFi.mode(WIFI_STA);
  String hostname(HOSTNAME);
  hostname += String((unsigned long)ESP.getEfuseMac(), HEX);
  WiFi.hostname(hostname);
#ifdef USE_OTA 
  initMDNS(); 
  setupOTA(MDNS_NAME, OTA_PASSWORD);
  wlanConnect();
#endif
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.wifi_sta_disconnected.reason);
    if (info.wifi_sta_disconnected.reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT) 
      wifiNeedPass = true;
    else if (info.wifi_sta_disconnected.reason == WIFI_REASON_NO_AP_FOUND)
      wlanNotFound();
  }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

/*  WiFiEventId_t eventID = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
    serial.print("> WiFi lost connection. Reason: ");
    printReason(info.disconnected.reason);
    if (info.disconnected.reason == 15) setPasswordVisibility(true, false);
  }, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);*/
}


const char* wifiReasons[] = {
  "None",
  "Unspecified",
  "Auth Expire",
  "Auth Leave",
  "Assoc Expire",
  "Assoc TooMany",
  "Not Authed",
  "Not Assoced",
  "Assoc Leave",
  "Assoc Not Authed",
  "Disassoc Pwrcap Bad",
  "Disassoc Supchan Bad",
  "None",
  "IE Invalid",
  "MIC Failure",
  "4Way Handshake Timeout",
  "Group Key Update Timeout",
  "IE In 4Way Differs",
  "Group Cipher Invalid",
  "Pairwise Cipher Invalid",
  "AKMP Invalid",
  "Unsupp RSN IE Version",
  "Invalid RSN IE Cap",
  "802.1x Auth Failed",
  "Cipher Suite Rejected",
  "Beacon Timeout",
  "No AP Found",
  "Auth Fail",
  "Assoc Fail",
  "Handshake Timeout"
};

void printReason(int reason) {
  if (reason >= 200) reason -= 175;
  if (reason < 0 || reason > 29) reason = 0;
  serial.println(wifiReasons[reason]);
}

//just try to connect to a given network
void wlanConnect(uint8_t network){
  if (strcmp(settings->networks[network].ssid, "<Empty>") == 0) wlanNotFound();
  else {
    Serial.printf("Trying network SSID: %s\n", settings->networks[network].ssid);
    WiFi.begin(settings->networks[network].ssid, settings->networks[network].password);
  }
}

//Start the WIFI network and connect to AP
int currentNetwork;
void wlanConnect() {
  if (WiFi.status() != WL_CONNECTED) {
    currentNetwork = settings->currentNetwork;
    wlanConnect(currentNetwork);
  }
}

//Called if AP not found - try next network
void wlanNotFound() {
  if (++currentNetwork > 3) currentNetwork = 0;
  if (currentNetwork == settings->currentNetwork) {
    //Tried all of the networks
    info(NOW, 0, LV_SYMBOL_WIFI " Please choose a WIFI network");
  }
  else wlanConnect(currentNetwork);
}

//Called from WiFi started section in wifiHandle()
//Update the current network if different from saved network
void wlanUpdateNetwork() {
  if (WiFi.status() == WL_CONNECTED && currentNetwork != settings->currentNetwork) {
    settings->currentNetwork = currentNetwork;
    writeSettings();
  }
}

bool wifiDoingScan = false;
void wifiBeginScan() {
  if (wifiDoingScan) return;
  if (WiFi.status() != WL_CONNECTED) WiFi.disconnect();
  wifiScan = true;
  wifiScanMessage("Scanning.. Please wait..");
  Serial.println("Scanning for WIFI networks...");
}

//Get sig strength in range 0-100%
uint8_t wifiStrength() {
  uint8_t signal = 0;
  if (WiFi.status() == WL_CONNECTED) {
    int dBm = WiFi.RSSI();
    if (dBm <= -100) signal = 0;
    else if (dBm >= -50) signal = 100;
    else signal = 2 * (dBm + 100);
  }
  return signal;  
}

//Async scan crashes -- why?
// use sync scan for now I guess
//#define ASYNC_SCAN

//Called from loop()
void wifiHandle() {
  static unsigned long lastms = 0;
  if (wifiScan) {
#ifdef ASYNC_SCAN
    serial.println("* Begin wifi scan");
    WiFi.scanNetworks(true);
    wifiDoingScan = true;
#else
    int scanStatus = WiFi.scanNetworks();
    if (scanStatus == 0) wifiScanMessage("No Networks.");
    else for (size_t i = 0; i < scanStatus; i++)
      wifiScanEntry(i + 1, WiFi.SSID(i).c_str());
#endif
    wifiScan = false;
  }
#ifdef ASYNC_SCAN
  if (wifiDoingScan) {
    delay(5);
    int16_t WiFiScanStatus = WiFi.scanComplete();
    if (WiFiScanStatus < 0) { // it is busy scanning or got an error
      if (WiFiScanStatus == WIFI_SCAN_FAILED) {
        serial.println("* WiFi Scan has failed. Starting again.");
        wifiScan = true;
      }
      // other option is status WIFI_SCAN_RUNNING - just wait.
    } else { // Found Zero or more Wireless Networks     
      serial.printf("* Scan done: %d networks found", WiFiScanStatus);
      if (!WiFiScanStatus) wifiScanEntry(0, NULL);
      else for (int i = 0; i < WiFiScanStatus; ++i)
        wifiScanEntry(i + 1, WiFi.SSID(i).c_str());
      WiFi.scanDelete();
      wifiDoingScan = false;
    }
  }  
#endif
  int state = WiFi.status();
  if (state > 6) state = 6;
  if (state != wifiState) {
    wifiState = state;
    setWifiStatus(wifiState);
    if (state == 1) //No SSID
      WiFi.disconnect();
    if (state == WL_CONNECTED && !wifiStarted) {
      serial.printf("> WiFi Got IP: %s\r\n", WiFi.localIP().toString().c_str());
      wifiStarted = true;
      //Begin WiFi based services here
#ifdef MONITOR_PORT
      wifiTerminalBegin();
#endif
      weatherBegin();
      NTPBegin();
      wlanUpdateNetwork();
    }
  }  

  //Signal strength display
#ifndef USE_OTA
  if (settings->mode == MODE_WEB || settings->mode == MODE_FTP || settings->mode == MODE_POD || settings->mode == MODE_DLNA) {
#endif
    if (millis() > lastms) {
      lastms = millis() + 1000;
      if (WiFi.status() != WL_CONNECTED) {
        setSigStrengthLbl(LV_SYMBOL_WIFI " ?");          
      } else {
        uint8_t signal = wifiStrength();
        char s[25] = {0};
        snprintf(s, 24, LV_SYMBOL_WIFI " %d%%", signal);
        setSigStrengthLbl(s);    
      }
    }
#ifndef USE_OTA
  }
#endif

  //Wifi needs password - set by event from wifi thread
  if (wifiNeedPass) {
    wifiNeedPass = false;
    setPasswordVisibility(true, false);
  }
}

void wifiDisconnect() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  wifiStarted = false;
}

//-----------------------------------
//TCP Serial emulation
// Allows remote TCP connections for monitor
// Platformio.ini example: monitor_port = socket://deskradio.local:4444

#ifdef MONITOR_PORT

#define TERM_CLIENTS 10
int terminalPort = 4444;
WiFiServer terminalServer(terminalPort);
WiFiClient *terminalClients[TERM_CLIENTS] = { NULL };
char* terminalBuffer;
int terminalBufferHead = 0;
int terminalBufferTail = 0;
#define TERMINAL_BUFFER 8192

//Called from mySerial::write()
void wifiTerminalWrite(uint8_t c) {
  terminalBuffer[terminalBufferHead] = c;     //Put at the head
  terminalBufferHead = (terminalBufferHead + 1) % TERMINAL_BUFFER;    //Move the head along
  if (terminalBufferTail == terminalBufferHead)                       //Hit the tail?
    terminalBufferTail = (terminalBufferTail + 1) % TERMINAL_BUFFER;  //Move the tail along too
}

void wifiTerminalHandle() {
  // Check if a new client has connected
  WiFiClient newClient = terminalServer.available();
  bool connected = false;
  if (newClient) {
    // Find the first unused space
    for (int i = 0 ; i < TERM_CLIENTS ; i++) {
      if (terminalClients[i] == NULL) {
        terminalClients[i] = new WiFiClient(newClient);
        break;
      }
    }
    serial.printf("> Terminal Connected: %s\r\n", newClient.remoteIP().toString().c_str());
  }
  //Delete disconnected clients
  for (int i = 0 ; i < TERM_CLIENTS ; i++) {
    if (terminalClients[i] != NULL) {
      if (!terminalClients[i]->connected()) {
        serial.println("> Terminal Disconnected.");
        terminalClients[i]->stop();
        delete terminalClients[i];
        terminalClients[i] = NULL;          
      } else connected = true; 
    }
  }
  //Dump out the contents of the buffer
  if (connected) { 
    while (terminalBufferTail != terminalBufferHead) {
      //Send to any connected clients
      for (int i=0 ; i<TERM_CLIENTS ; ++i) {
        if (terminalClients[i] != NULL) { 
          terminalClients[i]->write(terminalBuffer[terminalBufferTail]);
        }
      }
      terminalBufferTail = (terminalBufferTail + 1) % TERMINAL_BUFFER;
    }
  }
}

//Called early from setup() to prepare the buffer
void wifiTerminalSetup() {
  terminalBuffer = (char*)ps_malloc(TERMINAL_BUFFER + 1);
  terminalBufferHead = terminalBufferTail = 0;
}

//Called from wifiHandle() on WiFi first connect
void wifiTerminalBegin() {
  terminalServer.begin();
  //server.setNoDelay(true); // Sends small packets immediately without consolidating a number of small packets into one big packet
}

#endif
//-----------------------------------------------------------------------
//URL encoding

String urlencode(String url) {
  uint16_t len = url.length();
  String enc = "";
  char c;
  char code0;
  char code1;
  for(int i = 0; i < len; i++) {
    c = url[i];
    if(isalnum(c)) enc += c;
    else {
      code1 = (c & 0xf) + '0';
      if((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if(c > 9) code0 = c - 10 + 'A';
      enc += String('%') + code0 + code1;
    }
  }
  return enc;
}

//----------------------------------------------------------------------
// TLS Thread safety?

#ifdef CONFIG_MBEDTLS_THREADING_C
int mutexCount = 0;

void mutex_init( mbedtls_threading_mutex_t * mutex ) {
  mutex->mutex = xSemaphoreCreateMutex();
  if( mutex->mutex != NULL )  {
    mutex->is_valid = 1;
    mutexCount = 0;
  } else {
    mutex->is_valid = 0;
    errorContinue(3, "Failed to initialize mbedTLS mutex!");
  }
}

/**
* @brief Implementation of mbedtls_mutex_free for thread-safety.
*
*/
void mutex_free( mbedtls_threading_mutex_t * mutex ) {
  if( mutex->is_valid == 1 ) {
    vSemaphoreDelete( mutex->mutex );
    mutex->is_valid = 0;
  }
}


/**
* @brief Implementation of mbedtls_mutex_lock for thread-safety.
*
* @return 0 if successful, MBEDTLS_ERR_THREADING_MUTEX_ERROR if timeout,
* MBEDTLS_ERR_THREADING_BAD_INPUT_DATA if the mutex is not valid.
*/
int mutex_lock( mbedtls_threading_mutex_t * mutex ) {
  int ret = MBEDTLS_ERR_THREADING_BAD_INPUT_DATA;
  if( mutex->is_valid == 1 ) {
    if (mutexCount == 0) {
      if(xSemaphoreTake( mutex->mutex, portMAX_DELAY ) ) ret = 0;
      else ret = MBEDTLS_ERR_THREADING_MUTEX_ERROR;
    } else ret = 0; //already locked
    mutexCount++;
  }
  return ret;
}

/**
* @brief Implementation of mbedtls_mutex_unlock for thread-safety.
*
* @return 0 if successful, MBEDTLS_ERR_THREADING_MUTEX_ERROR if timeout,
* MBEDTLS_ERR_THREADING_BAD_INPUT_DATA if the mutex is not valid.
*/
int mutex_unlock( mbedtls_threading_mutex_t * mutex ) {
  int ret = MBEDTLS_ERR_THREADING_BAD_INPUT_DATA;
  if (mutexCount == 0) ret = MBEDTLS_ERR_THREADING_MUTEX_ERROR;
  else if (--mutexCount == 0) {
    if( mutex->is_valid == 1 ) {
      if( xSemaphoreGive( mutex->mutex ) ) ret = 0;
      else ret = MBEDTLS_ERR_THREADING_MUTEX_ERROR;
    }
  } else ret = 0; //Still locked
  return ret;
}

//Called early from beginWiFi()
void setupTLS() {
  mbedtls_threading_set_alt(mutex_init, mutex_free, mutex_lock, mutex_unlock);
}

#endif

//-----------------------------------------
//ESP32 glibc RTC
// Mini non-blocking NTP client

#define NTP_PACKET_SIZE   48 // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
WiFiUDP ntpUdp;
time_t NTPretry = 0;

void initTimezone() {
  setenv("TZ", settings->tz, 1);
  tzset();
}

void setTime(time_t now) {
  struct timeval tv;
	tv.tv_sec = now;  // epoch time (seconds)
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);
}

void setTime(int sc, int mn, int hr, int dy, int mt, int yr, bool dstComp) {
  struct tm t = {};
  t.tm_year = yr - 1900;
  t.tm_mon = mt - 1;
  t.tm_mday = dy;
  t.tm_hour = hr;
  t.tm_min = mn;
  t.tm_sec = sc;
  time_t timeSinceEpoch = mktime(&t);
  if (dstComp && t.tm_isdst) timeSinceEpoch -= 3600;
  setTime(timeSinceEpoch);
}

time_t getTime() {
  time_t now;
  time(&now);  
  return now;
}

String getTime(String format){
  time_t tt = getTime(); 
  tm *mt = localtime(&tt);  
	char s[128], c[128];
	format.toCharArray(c, 127);
	strftime(s, 127, c, mt);
	return String(s);
}

//Call from setup()
void initNTP() {
  ntpUdp.begin(8888);
  initTimezone();
}

//Called from wifiHandle() when first connected
void NTPBegin() {
  if (!RTCSet) NTPretry = millis() + 1000;  //First update - 1 second after connect
}

//Force NTP update now
void NTPUpdate() {
  NTPretry = millis();
  RTCSet = false;
}

//Call from loop()
void NTPHandle() {
  static time_t timer;
  static time_t timeout = 0;
  static bool ntpWaitResponse = false;

  //If time not set, request time from NTP to set RTC
  if (!RTCSet && WiFi.status() == WL_CONNECTED && NTPretry && millis() > NTPretry) {
    serial.print("> Get Time: ");
    sendNTPpacket(TIMESERVER);
    NTPretry = millis() + 100000;     //100 seconds until retry
    timeout = millis() + 1500;    //1.5 seconds to respond
    ntpWaitResponse = true;
  }

  //If time requested, check for response
  if (ntpWaitResponse && timer < millis()) {
    timer = millis() + 100;       //0.1 seconds between checks
    int size = ntpUdp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      ntpUdp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      setTime(secsSince1900 - 2208988800UL);
      
      //Time is now set
      serial.println(getTime("%a %d/%m/%y %H:%M:%S"));
      RTCSet = true;
      ntpWaitResponse = false;
    }

    //If no response by timeout, bail and wait for retry
    if (timeout < millis()) {
      serial.println("Failed! Retrying..");
      ntpWaitResponse = false;
    }
  }
}

// send an NTP request to the time server at the given address
void sendNTPpacket(const char* address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  ntpUdp.beginPacket(address, 123); //NTP requests are to port 123
  ntpUdp.write(packetBuffer, NTP_PACKET_SIZE);
  ntpUdp.endPacket();
}

int getHour(bool tfh) { 
  time_t tt = getTime(); 
  tm *mt = localtime(&tt); 
	if (tfh) return mt->tm_hour;
	int hour = mt->tm_hour;
	if (hour > 12) return hour - 12;
	if (hour == 0) return 12;
	return hour;
}
int getMinute() { time_t tt = getTime(); tm *mt = localtime(&tt); return mt->tm_min; }
int getDay() { time_t tt = getTime(); tm *mt = localtime(&tt); return mt->tm_mday; }
int getMonth() { time_t tt = getTime(); tm *mt = localtime(&tt); return mt->tm_mon; }

unsigned long utf() { return getTime(); }


//----------------------------------------------
// OTA update

#ifdef USE_OTA
#include <ESPmDNS.h>

// Initialize mDNS
void initMDNS() {
  if (!MDNS.begin(MDNS_NAME)) {   // Set the hostname to "woofer.local"
    serial.print(LV_SYMBOL_WARNING " Error setting up MDNS responder!");
    while(1) {
      delay(1000);
    }
  }
  serial.println("> mDNS started: '" MDNS_NAME ".local'");
}

lv_obj_t * otaWin;
lv_obj_t * otaBar;
lv_obj_t * otaTxt;
lv_obj_t * otaPct;
static lv_style_t style_ota;

static void otaEvent(lv_event_t * event);

void createOTAwin(const char *type) {
  if (otaWin == NULL) {
    //Style   
    lv_style_init(&style_ota);
    lv_style_set_text_font(&style_ota, &lv_font_montserrat_14);
    lv_style_set_bg_opa(&style_ota, LV_OPA_100);
    lv_style_set_border_width(&style_ota, 3);
#ifdef THEME_BLUE
    lv_style_set_border_color(&style_ota, lv_palette_main(LV_PALETTE_BLUE));
#else     
    lv_style_set_border_color(&style_ota, lv_color_hex(0xFF7F50));
#endif
    lv_style_set_bg_color(&style_ota, lv_color_hex(0x0));
    //Window
    otaWin = lv_win_create(lv_scr_act(), 24);
    lv_win_add_title(otaWin, "OTA Programming");                        /*Set the title*/
    lv_obj_add_style(otaWin, &style_ota, LV_PART_MAIN);
    lv_obj_set_size(otaWin, 180, 90);
    lv_obj_align(otaWin, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t * close_btn = lv_win_add_btn(otaWin, LV_SYMBOL_CLOSE, 22);           /*Add close button and use built-in close action*/
    lv_obj_add_event_cb(close_btn, otaEvent, LV_EVENT_CLICKED, NULL);
    //Contents
    lv_obj_t * body = lv_win_get_content(otaWin);
    otaTxt = lv_label_create(body);
    lv_obj_set_pos(otaTxt, 10, 6);
    otaBar = lv_bar_create(body);
    lv_obj_set_size(otaBar, 110, 16);
    lv_obj_align_to(otaBar, otaTxt, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);
    otaPct = lv_label_create(body);
    lv_obj_align_to(otaPct, otaBar, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
  }
  lv_label_set_text(otaTxt, type);
  lv_bar_set_value(otaBar, 0, LV_ANIM_OFF);      
  lv_label_set_text(otaPct, "0%");  
}

void setupOTA(const char* host, const char* pass) {
  ArduinoOTA.setHostname(host);  
  ArduinoOTA.setPassword(pass);
  ArduinoOTA.setMdnsEnabled(false); //Do our own mDNS  
  ArduinoOTA
    .onStart([]() {
      screenSaverInteraction();
      webradioStop();
      clearProgLbl();
      updateSTMOLabel(STMO_NONE);
      sdSongFinished();
      closePopup(false);
      hideWebControls();
      setFunctionLbl(LV_SYMBOL_COPY " Firmware Update..");
      
      OTAprog = true;
      const char* type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "Update Sketch";
      else // U_SPIFFS
        type = "Update Filesystem";
      createOTAwin(type);
      lv_task_handler(); /* let the GUI do its work */  
    })
    .onEnd([]() {
      lv_label_set_text(otaTxt, "Done. Rebooting..");
      lv_task_handler(); /* let the GUI do its work */  
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      int pct = progress / (total / 100);
      if (otaBar) lv_bar_set_value(otaBar, pct, LV_ANIM_OFF);      
      if (otaPct) lv_label_set_text(otaPct, ((String)pct + "%").c_str());
      lv_task_handler(); /* let the GUI do its work */
      wifiHandle();  
    })
    .onError([](ota_error_t error) {
      const char *err = "?";
      if (error == OTA_AUTH_ERROR) err = "Auth Failed";
      else if (error == OTA_BEGIN_ERROR) err = "Begin Failed";
      else if (error == OTA_CONNECT_ERROR) err = "Connect Failed";
      else if (error == OTA_RECEIVE_ERROR) err = "Receive Failed";
      else if (error == OTA_END_ERROR) err = "End Failed";
      if (otaTxt) lv_label_set_text(otaTxt, err);
      lv_task_handler(); /* let the GUI do its work */  
    });
  ArduinoOTA.begin();
  serial.println("> OTA Programming enabled.");
}

//Close button clicked in OTA window
static void otaEvent(lv_event_t * event)
{
  lv_obj_del(otaWin);
  otaWin = NULL;       
  otaBar = NULL;       
  otaTxt = NULL;       
}

#endif