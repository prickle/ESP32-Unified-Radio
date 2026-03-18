#pragma once

//-----------------------------------------------------------------------
//Personality definitions

//Coming in from platfomio.ini:
//Check at least one device selected
#ifndef SUPERWOOFER
#ifndef WORKRADIO
#ifndef WORKRADIO2
#ifndef MINIRADIO
#ifndef PANASONIC
#error At least one device must be defined!
#endif
#endif
#endif
#endif
#endif

//================================
//Super Woofer on WT32-SC01
#ifdef SUPERWOOFER
// HOSTNAME for OTA update
#define HOSTNAME  "ESP-WOOFER-"
#define MDNS_NAME "woofer"        //"woofer.local"
#define RADIONAME "Hitachi Super Woofer"

//WT32-SC01+
//ESP32S3 Settings:
//USB CDC on boot: Enabled
//Flash size 8mb
//Jtag: Integrated USB
//Partition: 8mb with SPIFFS
//PSRAM: QSPI PSRAM
//Programmer: ESPTool

#define LVGL_BUFF_SIZE  48                   //even number please..
#define NUM_PRESETS 16
#define PRESET_HEIGHT 45
//We have got an SD Card slot
#define SDPLAYER
//And a battery charger
#define BATTERYMON
//Using this LCD
#define TFT_ST7796  //480x320
//#define VUMETER
#define FFTMETER
#endif

//================================
//Work Radio on ESP32-S3
#ifdef WORKRADIO
// HOSTNAME for OTA update
#define HOSTNAME  "WORKRADIO-"
#define MDNS_NAME "workradio"        //"woofer.local"
#define RADIONAME "Workradio on S3"
//WT32-SC01+
//ESP32S3 Settings:
//USB CDC on boot: Enabled
//Flash size 8mb
//Jtag: Integrated USB
//Partition: 8mb with SPIFFS
//PSRAM: QSPI PSRAM
//Programmer: ESPTool

#define LVGL_BUFF_SIZE  48                   //even number please..
#define NUM_PRESETS 16
#define PRESET_HEIGHT 45
//We have got an SD Card slot
//#define SDPLAYER
//And a battery charger
//#define BATTERYMON
//Using this LCD
#define TFT_ST7796  //480x320
#define VUMETER
#define LOG_REDIRECT
#endif

//================================
//Work Radio on ESP32
#ifdef WORKRADIO2
//Also may need to enable //    #define AAC_ENABLE_SBR  // needs additional 60KB DRAM,
// in ESP32-audioI2S/src/aac_decoder/aac_decoder.h
// and may also need to play with delays in audio task
#define HOSTNAME  "WORKRADIO-"
#define MDNS_NAME "workradio"        //"woofer.local"
#define RADIONAME "Nick's Work Radio on ESP32"
//ESP32-WROVER
//4M Flash, 4M PSRAM
//Partition: No OTA
#define LVGL_BUFF_SIZE  40                   //even number please..
#define NUM_PRESETS 12
#define PRESET_HEIGHT 40
//We have got two scroll wheels
#define TWINSCROLL
//and a Monkeyboard DAB radio module
#define MONKEYBOARD
#define DAB_SERIAL    Serial2
#define FMSEEK
//3-band EQ
#define EQUALIZER
//Bluetooth (experimental)
//#define BLUETOOTH

//The workradio is jumper-wired, so for convenience - full pin mappings below..
//I2S audio
#define I2S_DOUT      15
#define I2S_BCLK      12 
#define I2S_LRC       13 
//Control inputs
#define INP_Q1A       34 
#define INP_Q1B       26 
#define INP_B1C       25 
#define INP_Q2A       35 
#define INP_Q2B       39 
#define INP_B2C       36 
//Touch screen
#define TP_SDA        4
#define TP_SCL        21
#define TP_INT        33
//DAB serial
#define DAB_TXD       0  
#define DAB_RXD       22 
//LCD display
#define TFT_ILI9341_1
#define TFT_WIDTH     320
#define TFT_HEIGHT    240
#define TFT_BL        32 
#define TFT_MOSI      23 
#define TFT_MISO      19 
#define TFT_SCK       18 
#define TFT_CS        14 
#define TFT_DC        27 
#define TFT_RST       5  
//Cannot use for I/O! - PSRAM
#define PSRAM_1       16
#define PSRAM_2       17

#endif

//================================
//Work Radio on ESP32
#ifdef MINIRADIO
#define HOSTNAME  "DESKRADIO-"
#define MDNS_NAME "deskradio"        //"deskradio.local"
#define RADIONAME "Mini Desk Radio"
#define THEME_BLUE
//ESP32-WROVER
//4M Flash, 4M PSRAM
//Partition: No OTA
#define LVGL_BUFF_SIZE  40                   //even number please..
#define NUM_PRESETS 12
#define PRESET_HEIGHT 40

// also trying CONFIG_ESP_WIFI_RX_IRAM_OPT=Y //seems to minimise clicking but does not stop skipping
// try ESP32_WIFI_STATIC_RX_BUFFER_NUM=16  was 12
// try WIFI_RX_BA_WIN=16 was 12
// try -Os 

//Interface to Miniradio-buttons on Arduino Mini ATmega328
#define MINIBUTTONS
//3-band EQ
#define EQUALIZER
//Resistive touch
#define FUNKYTOUCH
//Uncomment to turn on WIFI persistently and enable OTA
#define USE_OTA
//Enable connections from Platformio - monitor_port = socket://deskradio.local:4444
#define MONITOR_PORT
//Capture logging messages from ESP-IDF
#define LOG_REDIRECT
//Bluetooth (experimental)
//#define BLUETOOTH

//Basic controls
#define TFT_BL        14
#define AMP_MUTE      27

//I2S audio
#define I2S_DOUT      15
#define I2S_BCLK      12
#define I2S_LRC       13

//Control inputs
#define INP_PWR       26
#define INP_RXD       25
#define INP_TXD       33

//LCD display
//Actually defined in TFT_eSPI
#define TFT_ILI9341_2
#define TFT_MOSI      23
#define TFT_MISO      19
#define TFT_SCK       18
#define TFT_TCS       22
#define TFT_CS         5
#define TFT_DC        21
#define TFT_RST       32

//Cannot use for I/O!
#define PSRAM_1       16
#define PSRAM_2       17

#endif

//================================
//Panasonic Communications Reciever with NXP6686 on WT32-SC01
#ifdef PANASONIC
#define HOSTNAME  "PANASONIC-"
#define MDNS_NAME "panasonic"        //"deskradio.local"
#define RADIONAME "Panasonic Multiband"
#define THEME_BLUE
//ESP32-WROVER
//4M Flash, 4M PSRAM
//Partition: With OTA
#define LVGL_BUFF_SIZE  40                   //even number please..
#define NUM_PRESETS 12
#define PRESET_HEIGHT 50

//NXP/TEF6686 Radio module
#define NXP6686
#define FMSEEK
#define SCANNER

//Twin scroll, vol/func style
#define PANASCROLL
//3-band EQ
#define EQUALIZER
#define FORCE_MONO
//Touch needs calibration routine
#define CALITOUCH
//Uncomment to turn on WIFI persistently and enable OTA
//#define USE_OTA
//Enable connections from Platformio - monitor_port = socket://deskradio.local:4444
//#define MONITOR_PORT
//Capture logging messages from ESP-IDF
#define LOG_REDIRECT
//Bluetooth (experimental)
//#define BLUETOOTH

//I2S audio
#define I2S_DOUT      5
#define I2S_BCLK      25
#define I2S_LRC       26

//Control inputs
#define INP_Q1A       36
#define INP_Q1B       4 
#define INP_B1C       27 
#define INP_Q2A       34 
#define INP_Q2B       35 

//LCD display
//Setup
#define TFT_ST7796_2
#define TFT_WIDTH     320
#define TFT_HEIGHT    480
#define TFT_ROTATION  3
//Pinout
#define TFT_MOSI      13
#define TFT_MISO      12
#define TFT_SCK       14
#define TFT_CS        15
#define TFT_DC        21
#define TFT_RST       22
#define TFT_BL        23

//I2C bus
#define I2C_SDA       18
#define I2C_SCL       19

//Touch screen
#define TP_INT        39 //(VN)

//Cannot use for I/O!
#define PSRAM_1       16
#define PSRAM_2       17

#endif



//-----------------------------------------------------------------------
//Project-wide includes

#include <Arduino.h>

//Uncomment to turn on DMA transfers to TFT
#define USE_DMA

#define LGFX_USE_V1         // set to use new version of library
#include <LovyanGFX.hpp>    // main library
#include <lvgl.h>     // Modified
#include <EEPROM.h>
#include <Ticker.h>
#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <ArduinoFTPClient.h> //Modified
#include <TJpg_Decoder.h>
#include <ADS1115.h>
#include <Adafruit_INA219.h>
#include <sha1.h>
#include <HTTPClient.h>
#include "SoapESP32.h"
#ifdef USE_OTA
#include <ArduinoOTA.h>
#endif
#ifdef MONKEYBOARD
#include <MonkeyBoard.h>
#endif
#include <Audio.h> //see repository at github "https://github.com/schreibfaul1/ESP32-audioI2S"
// 'AAC_ENABLE_SBR' can be deactivated in 'aac_decoder.h', saves ~ 60KB

#include "config.h"  //Configuration, constants and headers
#include "img.h"     //Image data

//---------------------------------------------------------------------------------
// Function declarations

//art.cpp
void findAlbumArt(const char* path);
void showAlbumArt(bool fullscreen);
void loadAlbumArt(const char* path);
void clearAlbumArt();
void artAction();
void initAlbumArt();
void artHandle();

//audio.cpp
void webradioSetup();
void webradioHandle();  
void connectToHost(const char * server, bool metadata);
void connectToFS(char * path, uint32_t resumePos);
void setDspVolume(uint8_t volume);
void setVSTone ( uint16_t rtone );
uint16_t formatVSTone(int8_t bass, int8_t mid, int8_t treb);
void webradioStop();
void setMonoOutput(bool mono);

//batt.cpp
#ifdef I2C_SDA
void i2cScan();
#endif
#ifdef BATTERYMON
void ADCHandle();
void ADCInit();
void resetAmpHours();
#endif

//browser.cpp
void createFileBrowserWindow(lv_obj_t * page);
boolean isPlaylistFile(const char* fileName);
boolean isKnownMusicFile(const char * filename);
boolean isJPGFile(const char* fileName);
void showBrowserSpinner(bool shown);
void showFilename(const char * path);
void setBrowserTitle(const char* path);
void setBrowserVisibility();
void browserClearSelections();
void sdPopulateBrowser(char *selectEntry);
void initBrowser();
void listWarning(lv_obj_t* list, const char* msg);
void browserListMenu(lv_event_t * event);

#ifdef BLUETOOTH
//bluetooth.cpp
void setupBluetooth();
#endif

//buttons.cpp
void prepareEncoders();
void encodersHandle();
void prepareButtons();
void buttonsHandle();

//dab.cpp
void startDab();
void startFM();
void startI2S();
void dabStop();
void dabFMSearch(uint8_t dir);
void setVolume(uint8_t volume);
void setStereoWide(bool wide);
void dabHandle();
void initDab();
void dabTuningAction(int freq);
void setStereoMode(bool stereo);

//dlna.cpp
void DLNASetup();
void createDlnaWindow(lv_obj_t * parent);
void setDlnaVisibility();
void DLNAHandle();
bool DLNAClientHandle();
void DLNAGetServers(bool refresh);
void dlnaSetMeta(String name);
void showDlnaMetadata();

//ftp.cpp
void ftpPopulateBrowser(char *selectEntry);
void ftpPlaylistRecurseDirectory(lv_fs_file_t * f, char * orig_path);
void ftpGoUp();
void ftpGoHome();
void FTPHandle();
void initFTP();

//lvgl.cpp
void initScreen();
void resetScreen();
void lv_obj_set_hidden(lv_obj_t * obj, bool isHidden);
void lv_obj_swap(lv_obj_t * obj1, lv_obj_t * obj2);
bool fs_err(lv_fs_res_t err, const char * msg);
uint32_t lv_fs_size(lv_fs_file_t * file);
void screenSaverInteraction();
void refreshDisplay();
void inputDriverInit();
void SDfileSystemInit();
void SPIFFSfileSystemInit();
void screenSaverHandle();
lv_color_t lv_col(uint16_t color);
void pumpLvgl();
void initSPIFFS();
#ifdef SDPLAYER
void initSDCard();
#endif
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);
void setBrightness(uint8_t bright);

//main.cpp
void createMainWindow(lv_obj_t * win);
void updateArtImg(lv_img_dsc_t * imgDsc, bool fullscreen);
void updateArtImg(const char * msg);
void setArtBar(int pct);
void VUHandle();
void showLoadSpinner();
#ifdef SDPLAYER
void showReloadBtn();
#endif
void hideWebControls();
void clearBufStat();
void setBattChgLbl(const char * txt);
void setMainVisibility();
void popup(char* prompt, void(*okFunction)(void), bool animated);
void closePopup(bool animated);
void setBufMeter(int val);
void setTimeBar(int pct);
void clearProgLbl();
void printBufStat(bool wrIsRunning, int wrCodec, int wrBitrate);
void setupInfoLabels();
void info(uint8_t field, int time, const char * format, ...);
void infoLabelHandle();
void updateFMDisplay(int freq);
void setDabImg(const void* src);
void setDabBar(int val, lv_anim_enable_t anim);
void setStaticLabels(bool isStatic);
void setSignalMeter(int value);

//nxp6686.cpp
void initNXP();
void NXPStartMW();
void NXPStartSW();
void NXPStartLW();
void NXPStartFM();
void NXPSetFrequency(uint32_t frequency);
void NXPChangeFrequency(bool up);
void NXPSearch(uint8_t dir);
void NXPStartI2S();
void NXPSetI2SRate(uint32_t rate);
void NXPHandle();
void NXPStop();
void NXPFineTune();
void NXPScan();
void NXPSetBandwidth(int index);
void NXPSetAGC(int index);
void NXPSetNoiseBlanker(int val);

//playlist.cpp
void createPlaylistWindow(lv_obj_t * parent);
int getPlaylistIndex();
void setPlaylistIndex(int index);
int getPlaylistCount();
void hidePlaylist(bool hide);
void removePlaylist(const char* path);
void appendFileName(char * path, const char * name, int maxlen);
void setPlaylistVisibility();
const char* getPlaylistPath();
void addPlaylistEntry(lv_fs_file_t * f, char * path);
void addPlaylistEntry(lv_fs_file_t* f, const char* name, const char* path);
void playlistRecurseDirectory(lv_fs_file_t * f, char * orig_path);
void resumePlaylist();
bool loadPlaylist(const char * file);
lv_obj_t * getPlaylistChild(int index);
const char* getPlaylistItem(int index);
void clearPlaylist();
void updatePlaylistHeader();
void shufflePlaylist();
void writePlaylist();
void writePlaylistToFp(lv_fs_file_t* f);

//podcast.cpp
void createPodSearchWindow(lv_obj_t * parent);
void hidePodSearchWindow(bool hide);
void createPodcastWindow(lv_obj_t * parent);
void showPodcastSpinner(bool shown);
void podListMove(int dir);
void podActivate();
void podEOF();
void setPodVisibility();
void clearPodcasts();
void podShowEpisodeInfo();
bool podClientHandle();
void podcastHandle();
const char* getPodcastName(); 
const char* getPodEpisodeName();
void initPodcast();
void readPodcasts();
void clearPodEpisodes();
void podStop();
void removePodcasts();
bool transferPodcasts();
void podPlayLatestEpisode(int id);

//presets.cpp
void createPresetsWindow(lv_obj_t * parent);
void updateUrlEditText();
void setPlaylistBtnVisibility();
void showPresets(bool yesno);
void writePodcasts();
void renamePreset(const char* oldname, const char* newname);
void deletePreset(const char* name);

//receiver.cpp
lv_obj_t* createReceiverWidget(lv_obj_t* parent);
void scanClear(int* freqTable);
void scanAdd(int frequency, int value);
void scanCursor(int frequency);
void scanBWOptions(const char* options, int selected);
void scanAGCOptions(const char* options);
void scanAGCSelected(int selected);

//screens.cpp
void errorContinue(int time, const char * message);
void errorHalt(const char * message);
void initTerminalLog();
void screenInit(void);
void keyboardShow(lv_obj_t * parent, lv_obj_t * editText, lv_event_cb_t cb);
void keyboardShow(lv_obj_t * parent, lv_obj_t * textarea);
void keyboardHide(bool animated, void (*doneAction)());
bool keyboardShowing();
void tabViewShowMain();
void closeModeList();
int activeTab();
void updateSTMOLabel(uint8_t mode);
void setSelectedMode(int mode);
void setRadioMode(int mode);
void timeHandle();
void terminalHandle();
void setSdcardLbl(const char * txt);
void setSigStrengthLbl(const char * txt);
void setStmoLbl(const char * txt);
void createTerminalWindow(lv_obj_t *win);
void createSysmonWindow(lv_obj_t *scr);
bool tabViewIsScrolling();
void removeFactoryBtn();
void listMenuClose(lv_event_t * event);
void listMenuClose(lv_obj_t * list, bool animated);
void listMenuClicked(lv_event_t * event);
lv_obj_t* createListMenu(lv_obj_t* button);
void listMenuActivate(lv_obj_t* menu);
void setupModes();
void setObjectVisibility();
void setFunctionLbl(const char* txt);

//sd.cpp
boolean playFile(const char *file);
void sdStop();
void sdSongFinished();
void playPlaylistFile();
void sdEOF();
void metadataSD();
void initPlayer();
#ifdef SDPLAYER
void sdReinit();
#endif

//search.cpp
void createSearchWindow(lv_obj_t * parent);
void hideSearchWindow(bool hide);
void searchHandle();
bool searchClientHandle();
void searchCloseDropdown();

//settings.cpp
void createSettingsWindow(lv_obj_t * page);
void initSettings();
void setDefaults();
void writeSettings();
void setSettingsVisibility();
void passwordHandle();
void setPasswordVisibility(bool visible, bool timed);
void wifiScanMessage(const char* message);
void setWifiStatus(int state);
void wifiScanEntry(int index, const char* entry);
void closeNetworkDropdown();
void setDabScanLabel(const char* buf);
void hideScanSpinner(bool hide);

//stations.cpp
void createStationsWindow(lv_obj_t * page);
int listGetSelect(lv_obj_t * list);
lv_obj_t* listGetSelectObj(lv_obj_t * list);
void listSetSelect(lv_obj_t * btn);
void listSetSelect(lv_obj_t* list, int index);
void listClearSelect(lv_obj_t* parent);
void setStationsVisibility();
void hideStationWindow(bool hide);
void clearStations();
int readWebStations();
bool stationInList(char * url);
lv_obj_t * addStationButton(const char* text, lv_event_cb_t callback, void* data, int datalen);
void webRadioListAction(lv_event_t * event);
void stationsChanged();
void removeWebStations();
bool transferWebStations();
const char* stationListName(char * url);
const char* stationListName(lv_obj_t * obj);
void listDoSelect();
void listMoveSelect(int offset);

//weather.cpp
lv_obj_t * createWeatherWidget(lv_obj_t * parent);
void weatherBegin();
void setupWeather();
void weatherHandle();
bool weatherClientHandle();
void createWeather();

//widgets.cpp
lv_obj_t* createVU(lv_obj_t* parent, int w, int h, uint16_t backCol);
void drawVU(int16_t l, int16_t r);
lv_obj_t* createWindSpd(lv_obj_t* parent, int w, int h);
void drawWindSpd(lv_obj_t* obj, float speed, float gust);
#ifdef FFTMETER
lv_obj_t* createFFT(lv_obj_t* parent, int w, int h, uint16_t backCol);
void drawFFT(lv_obj_t* obj, uint8_t* data);
void initFFT(void);
void execFFT(const int16_t* in);
uint32_t getFFT(size_t index);
#endif

//wifi.cpp
unsigned long utf();
void wifiHandle();
void setupWifi();
void initNTP();
void wlanConnect();
void wifiDisconnect();
void wifiBeginScan();
void NTPUpdate();
void NTPHandle();
void setTime(int sc, int mn, int hr, int dy, int mt, int yr, bool dstComp);
String getTime(String format);
int getHour(bool tfh);
int getMinute();
int getDay();
int getMonth();
void setupTLS();
String urlencode(String url);
void wifiTerminalSetup();
void wifiTerminalHandle();
void wifiTerminalWrite(uint8_t c);
