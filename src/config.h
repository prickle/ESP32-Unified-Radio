#pragma once

#include <Arduino.h>

//We are in Factory Reset mode
extern bool factoryMode;

extern volatile bool RTCSet;
extern volatile bool OTAprog;
extern volatile bool wifiStarted;
extern volatile bool wifiScanning; //Scanning?
extern volatile bool ADCFound;
extern volatile bool SDFound;
extern volatile bool ShuntFound;
extern volatile bool touchFound;
extern volatile bool nxpFound;
extern bool startupError;

#ifdef MONKEYBOARD
extern volatile bool DABFound;
extern volatile bool DABActive;
extern uint8_t currentDabStatus;
extern long dabFrequency;
#endif

#ifdef MINIRADIO
extern bool powerOff;
#endif

//String storage
extern char webStationName[35];
extern char searchStationName[35];
#ifdef MONKEYBOARD
extern char fmStationName[35];
#endif
extern char * ftpCurrentDir;
extern char * sdFileName;
extern String metaArtist, metaAlbum, metaTitle, metaYear;
extern String dlnaMetaName, dlnaMetaAlbum;

///Length of paths
#define FTP_NAME_LENGTH  512

//Clock
#define TZ "AEST-10AEDT,M10.1.0/02:00:00,M4.1.0/03:00:00"
#define TIMESERVER "pool.ntp.org"

//Screensaver
extern bool ScreenSaverPending;
extern bool ScreenSaverActive;
#define SCREENSAVER_TIMEOUT 30000

//Audio
#ifdef MONKEYBOARD
#define VOLUME_STEPS 16
#else
#define VOLUME_STEPS 21
#endif
#define WR_STACK_SIZE  12000
extern TaskHandle_t radioTaskHandle;

#ifdef FFTMETER
#define FFT_SIZE 256
#define FFT_BARS  58
#endif


//Tab control
extern uint8_t mainWindowIndex;


//OTA
#define OTA_PASSWORD       "admin"

//Webstation Search - api.radio-browser.info
#define SEARCH_HOST        "de1.api.radio-browser.info"
#define SEARCH_URL         "http://" SEARCH_HOST "/json/stations/search?"
#define SEARCH_DEFAULTS    "&hidebroken=true&limit=50&order=name"//&bitrateMin=20"//&bitrateMax=128"

//Podcast Search - api.podcastindex.org
#define PODCAST_HOST       "api.podcastindex.org"
#define PODCAST_URL        "https://" PODCAST_HOST "/api/1.0/search/byterm?q="
#define PODCAST_DEFAULTS   "&val=any&max=59&aponly=false"
#define PODCAST_IDURL      "https://" PODCAST_HOST "/api/1.0/episodes/byfeedid?id="
#define PODCAST_IDRESULTS  "&max="
#define PODCAST_LATESTRES  "&newest"
#define PODCAST_LATEST     0
//API key
#define PODCAST_KEY        "TBM6PUVWQX3BYGSFSG3T"
#define PODCAST_SECRET     "kskKLdVsQ^ZBEbnkaZKvPJV$89H#zrwb6dBMMFS2"

//Open Weather Map - api.openweathermap.org
#define WEATHER_HOST       "api.openweathermap.org";
#define WEATHER_LOCATION   "Melbourne,au";
#define WEATHER_OWMKEY     "b69d7e9800e16452f3ad5e92e64dff08";
#define WEATHER_TIMEOUT    20000

//Battery monitoring
extern float battV;
extern float solarV;
extern float chargeI;
extern float dischargeI;
extern int8_t battPercent;
extern int8_t powerSource;
extern float inputPower;
extern float outputPower;
extern float battAmps;
extern int64_t mAseconds;
extern bool battCharging;           //when charging
//Voltage limits
#define FULL_BATT            12.59 //V
#define FLAT_BATT            9.0   //V    // 0% battery totally expired
//Power source
#define SOURCE_UNKNOWN  0
#define SOURCE_BATTERY  1
#define SOURCE_AC       2
#define SOURCE_SOLAR    3
#define SOURCE_USB      4
#define FT62XX_ADDR 0x38
#define ADS1_ADDR 0x48
#define INA_ADDR 0x40
#define NXP_ADDR 0x64
//ADC States
#define ADCSTATE_IDLE     0
#define ADCSTATE_BATTERY  1
#define ADCSTATE_CURRENT  2
#define ADCSTATE_SOLAR    3
#define ADCSTATE_MODE     4

//extern lv_font_t digital;

//NXP6686
#define NXPTABLE_FMIN 0
#define NXPTABLE_FMAX 1
#define NXPTABLE_STEP 2
#define NXPTABLE_FINE 3
#define NXPTABLE_MODE 4
#define NXPTABLE_SMIN 5
#define NXPTABLE_SMAX 6
#define NXPTABLE_TMIN 7
#define NXPTABLE_TMAX 8


//Shared styles
extern lv_style_t style_wp;
extern lv_style_t style_win;
extern lv_style_t style_transp;
extern lv_style_t style_halfopa;
extern lv_style_t style_ta;
extern lv_style_t style_list;
extern lv_style_t style_listsel;
extern lv_style_t style_groupbox;
extern lv_style_t style_digitalfont;
extern lv_style_t style_bigfont;
extern lv_style_t style_biggerfont;
extern lv_style_t style_biggestfont;
extern lv_style_t style_bigfont_orange;

//Main info labels
extern lv_obj_t * progNameLbl;
extern lv_obj_t * progNowLbl;
extern lv_obj_t * progTimeBar;
extern lv_obj_t * progTextLbl;

//The keyboard
extern lv_obj_t * keyBoard;

//Top-level shared objects
extern lv_obj_t * browserMainList;
extern lv_obj_t * tabView;
extern lv_obj_t * vuMeter;
extern lv_obj_t * fftMeter;
extern lv_obj_t * modeList;
extern lv_obj_t * stationsTab;
extern lv_obj_t * dabVolSlider;
extern lv_obj_t * dabStationList;
extern lv_obj_t * mainContainer;

//Storage names
#define SYSTEM_DRV_LETTER  "C:"
#define STORAGE_DRV_LETTER "D:"
#define WEBSTATIONS_FILE   "/webStations.m3u"
#define PLAYLIST_FILE      "/PlayList.m3u"
#define DLNALIST_FILE      "/DlnaList.m3u"
#define PODLIST_FILE       "/PodList.pod"
#define MUSIC_DIR          "/Music"

//Storage paths
#define WEBSTATIONS_PATH   SYSTEM_DRV_LETTER WEBSTATIONS_FILE
#define WEBSTATIONS_COPY   STORAGE_DRV_LETTER WEBSTATIONS_FILE
#define WEBSTATIONS_TEST   STORAGE_DRV_LETTER WEBSTATIONS_FILE
#define PODLIST_PATH       SYSTEM_DRV_LETTER PODLIST_FILE
#define PODLIST_COPY       STORAGE_DRV_LETTER PODLIST_FILE
#define MUSIC_PATH         STORAGE_DRV_LETTER MUSIC_DIR
#define SDPLAYLIST_PATH    STORAGE_DRV_LETTER PLAYLIST_FILE
#define FTPPLAYLIST_PATH   SYSTEM_DRV_LETTER PLAYLIST_FILE
#define DLNAPLAYLIST_PATH  SYSTEM_DRV_LETTER DLNALIST_FILE
#define FTPTEST_PATH       "E:/"


//settings->mode
extern uint8_t MODE_UNKNOWN, MODE_DAB, MODE_FM, MODE_FTP, MODE_WEB, MODE_SD, MODE_POD, MODE_DLNA, MODE_LINE, MODE_BT, MODE_NLW, MODE_NMW, MODE_NSW, MODE_NFM;
extern const char * modeString[14];
extern int totalModes;

//Stereo / Mono
#define STMO_NONE   0
#define STMO_MONO   1
#define STMO_STEREO 2
#define STMO_SLIDE  3

//Info fields
#define NAME  0
#define NOW   1
#define TEXT  2

//Weather forecast storage
#define MAX_DAYS 5
#define MAX_3HRS (MAX_DAYS * 8)

typedef struct OW_forecast {
  time_t   dt = { 0 };  // dt
  float    temp = { 0 };
  float    tmin = { 0 };
  float    tmax = { 0 };
  float    speed = { 0 };
  float    gust = { 0 };
  char   main[32];
  char   description[64];
  char   icon[16];

} OW_forecast;


//Pin defines
#ifdef SDPLAYER
// These pins for SD Card
#define SD_MISO      38
#define SD_MOSI      40
#define SD_SCK       39
#define SD_CS        41    //15  // or 5
#endif

//I2S audio
 
#ifdef SUPERWOOFER
//Internal mono DAC
#define I2S_INT_DOUT      37
#define I2S_INT_BCLK      36
#define I2S_INT_LRC       35

//External stereo DAC
#define I2S_EXT_DOUT      21
#define I2S_EXT_BCLK      13
#define I2S_EXT_LRC       12
#endif

#ifdef WORKRADIO
//External stereo DAC
#define I2S_DOUT      21
#define I2S_BCLK      9
#define I2S_LRC       11
#endif

//I2C Bus

#ifdef BATTERYMON
//External I2C Bus
#define I2C_SDA       10
#define I2C_SCL       11
#endif

#define LV_SYMBOL_DEGREE "\xc2\xb0"

//0xf002, 0xf03a, 0xf0a1, 0xf2c9, 0xf2ce, 0xf519, 0xf72e
//Extra symbols in size 20 font
#define LV_SYMBOL_SEARCH      "\xef\x80\x82"
#define LV_SYMBOL_LIST2       "\xef\x80\xba"
#define LV_SYMBOL_BULLHORN    "\xef\x82\xa1"
#define LV_SYMBOL_THERMOMETER "\xef\x8b\x89"
#define LV_SYMBOL_BROADCAST   "\xef\x94\x99"
#define LV_SYMBOL_WIND        "\xef\x9c\xae"


//main ftpFS class
class ftpFSFS : public FS {
public:
  ftpFSFS();
  ~ftpFSFS();
};
extern ftpFSFS ftpFS; 

//EEPROM settings structures
#define EE_MAGIC  0x45
#define EE_SIZE   1300

//presets
struct __attribute__ ((packed)) presetObject{
  char name[35] = "<Empty>";  
  uint8_t mode;
};

//wifi creds
struct __attribute__ ((packed)) networkObject{
  char ssid[28];
  char password[32];
};

//EEPROM settings structure
struct __attribute__ ((packed)) settingsObject{
  uint8_t id;
  uint8_t mode;
  uint8_t vsVolume;
  uint16_t vsTone;
  int8_t balance;
  uint8_t dabVolume;
  uint8_t dabEQ;
  char dabChannel[35];
  int32_t dabFM;
  uint16_t playlistIndex;
  char server[256];
  //Credentials
  uint8_t currentNetwork;
  networkObject networks[4];
  //Presets
  presetObject presets[16];
  //Daylight savings
  bool dst;
  //FTP Client
  uint32_t ftpAddress;
  uint16_t ftpPlaylistIndex;
  char ftpUser[32];
  char ftpPass[32];
  //Autosave
  bool autosave;
  //DLNA
  uint16_t dlnaPlaylistIndex;
  //Log
  uint8_t logLevel;
  //Misc
  bool mono;
  uint8_t brightness;
  //NXP6686
  uint16_t freqMW;
  uint16_t freqSW;
  uint16_t freqLW;
  uint8_t bwFM;
  uint8_t bwMW;
  uint8_t bwSW;
  uint8_t bwLW;
  uint8_t agcFM;
  uint8_t agcMW;
  uint8_t agcSW;
  uint8_t agcLW;
  char tz[64];
  bool wide;
};
//Pointer to settings object
extern settingsObject* settings;

//custom serial device also outputs to onscreen terminal
class mySerial : public Stream {
  public:
    mySerial():Stream(Serial){}
    int read(void) { return Serial.read(); }
    size_t write(uint8_t ch);
    int available(void) { return Serial.available(); }
    int peek(void) { return Serial.peek(); }
    void flush() { }
    bool InSetup = true;
};
extern mySerial serial;

extern SPIClass myspi;    //SD Card SPI Class

//Station search entry
typedef struct stationInfo {
  char* name = NULL;
  char* url = NULL;
  char* codec = NULL;
  int bitrate;
  stationInfo(const char* title, const char* link, const char* enc, int rate) : bitrate(rate) {
    if (title) { if ((name = (char*)ps_malloc(strlen(title) + 2))) strcpy(name, title); else { stationInfoErr(); return; } }
    if (link) { if ((url = (char*)ps_malloc(strlen(link) + 2))) strcpy(url, link); else { stationInfoErr(); return; } }
    if (enc) { if ((codec = (char*)ps_malloc(strlen(enc) + 2))) strcpy(codec, enc); else { stationInfoErr(); return; } }
  }
  stationInfo(stationInfo* s) : stationInfo(s->name, s->url, s->codec, s->bitrate) {}
  ~stationInfo() {
    if (name) free(name);
    if (url) free(url);
    if (codec) free(codec);
  }
  void stationInfoErr() { log_w("Out of memory"); }
} stationInfo;

//Podcast search entry
typedef struct podInfo {
  uint32_t id = 0;
  char* name = NULL;
  char* description = NULL;
  bool isEpisode = false;
  int32_t feedId = -1;
  int32_t lastUpdateTime = 0;
  char* url = NULL;
  podInfo(uint32_t podId, const char* title, const char* desc, bool episode, const char* link, int32_t fid, int32_t lut) : id(podId), isEpisode(episode), feedId(fid), lastUpdateTime(lut) {
    if (title) { if ((name = (char*)ps_malloc(strlen(title) + 2))) strcpy(name, title); else { podInfoErr(); return; } }
    if (desc) { if ((description = (char*)ps_malloc(strlen(desc) + 2))) strcpy(description, desc); else { podInfoErr(); return; } }
    if (link) { if ((url = (char*)ps_malloc(strlen(link) + 2))) strcpy(url, link); else { podInfoErr(); return; } }
  }
  podInfo(podInfo* p) : podInfo(p->id, p->name, p->description, p->isEpisode, p->url, p->feedId, p->lastUpdateTime) {}
  ~podInfo() {
    if (name) free(name);
    if (description) free(description);
    if (url) free(url);
  }
  void podInfoErr() { log_w("Out of memory"); }
} podInfo;
//Currently selected podcast, if any (NULL if not)
extern podInfo* currentPodcast; 

//Dlna server entry
typedef struct dlnaServer {
  int         serverIndex;
  char*       name = NULL;
  IPAddress   ip;
  dlnaServer(int index, soapServer_t* srv) : serverIndex(index) {
    if (srv) { if ((name = (char*)ps_malloc(strlen(srv->friendlyName.c_str()) + 2))) strcpy(name, srv->friendlyName.c_str()); else { dlnaServerErr(); return; } }
    ip = (uint32_t)srv->ip;
  }
  ~dlnaServer() {
    if (name) free(name);
  }
  void dlnaServerErr() { log_w("Out of memory"); }
} dlnaServer;

//Dlna object entry
typedef struct dlnaObject {
  char*       album = NULL;
  char*       artist = NULL;
  char*       name = NULL;
  char*       id = NULL;
  char*       uri = NULL;
  int         bitrate;
  eFileType   type;
  int         size;
  bool        isDir;
  int         port;
  IPAddress   ip;
  dlnaObject(soapObject_t* obj) {
    if (obj) {
      if ((album = (char*)ps_malloc(strlen(obj->album.c_str()) + 2))) strcpy(album, obj->album.c_str()); else { dlnaErr(); return; } 
      if ((artist = (char*)ps_malloc(strlen(obj->artist.c_str()) + 2))) strcpy(artist, obj->artist.c_str()); else { dlnaErr(); return; } 
      if ((name = (char*)ps_malloc(strlen(obj->name.c_str()) + 2))) strcpy(name, obj->name.c_str()); else { dlnaErr(); return; } 
      if ((id = (char*)ps_malloc(strlen(obj->id.c_str()) + 2))) strcpy(id, obj->id.c_str()); else { dlnaErr(); return; } 
      if ((uri = (char*)ps_malloc(strlen(obj->uri.c_str()) + 2))) strcpy(uri, obj->uri.c_str()); else { dlnaErr(); return; } 
      bitrate = obj->bitrate;
      type = obj->fileType;
      size = obj->size;
      isDir = obj->isDirectory;
      port = obj->downloadPort;
      ip = (uint32_t)obj->downloadIp;
    }
  }
  ~dlnaObject() {
    if (album) free(album);
    if (artist) free(artist);
    if (name) free(name);
    if (id) free(id);
    if (uri) free(uri);
  }
  void dlnaErr() { log_w("Out of memory"); }
} dlnaObject;

//dlna list entry
typedef struct dlnaInfo {
  dlnaServer* server = NULL;
  dlnaObject* object = NULL;
  uint8_t infoType;
  int index;
  dlnaInfo(uint8_t type, int i, soapServer_t* srv) : infoType(type), index(i) {
    if (srv) server = new dlnaServer(i, srv);
  }
  dlnaInfo(uint8_t type, int i, soapObject_t* obj) : infoType(type), index(i) {
    if (obj) object = new dlnaObject(obj);
  }
  ~dlnaInfo() {
    if (server) delete(server);
    if (object) delete(object);
  }
} dlnaInfo;

//Linked list of directory entries for DLNA
typedef struct dlnaDir {
  char* name = NULL;            //Directory name
  char* id = NULL;              //Directory ID
  dlnaDir* previous = NULL;     //Pointer to previous directory
  int server;                   //Index of server
  dlnaDir(dlnaDir* last, char* dirName, int serverId, const char* dirId) : previous(last), server(serverId) {
    if ((name = (char*)ps_malloc(strlen(dirName) + 2))) strcpy(name, dirName); else { dlnaDirErr(); return; } 
    if ((id = (char*)ps_malloc(strlen(dirId) + 2))) strcpy(id, dirId); else { dlnaDirErr(); return; } 
  }
  ~dlnaDir() {
    if (name) delete(name);
    if (id) delete(id);
  }
  void dlnaDirErr() { log_w("Out of memory"); }
} dlnaDir;


//RGB LED = 48
//Can't use pins 35, 36, 37


#ifdef TFT_ST7796
#define TFT_WIDTH   480
#define TFT_HEIGHT  320
//LovyanGFX Display Class Config
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7796  _panel_instance;  // ST7796UI
  lgfx::Bus_Parallel8 _bus_instance;    // MCU8080 8B
  lgfx::Light_PWM     _light_instance;
  lgfx::Touch_FT5x06  _touch_instance;
public:
  LGFX(void) {
    //Bus
    { auto cfg = _bus_instance.config();
      cfg.freq_write = 60000000;    
      cfg.pin_wr = 47;             
      cfg.pin_rd = -1;             
      cfg.pin_rs = 0;              
      // LCD data interface, 8bit MCU (8080)
      cfg.pin_d0 = 9;              
      cfg.pin_d1 = 46;             
      cfg.pin_d2 = 3;              
      cfg.pin_d3 = 8;              
      cfg.pin_d4 = 18;             
      cfg.pin_d5 = 17;             
      cfg.pin_d6 = 16;             
      cfg.pin_d7 = 15;             
      _bus_instance.config(cfg);   
      _panel_instance.setBus(&_bus_instance);      
    }
    //LCD Panel
    { auto cfg = _panel_instance.config();    
      cfg.pin_cs           =    -1;  
      cfg.pin_rst          =    4;  
      cfg.pin_busy         =    -1; 
      cfg.panel_width      =   320;
      cfg.panel_height     =   480;
      cfg.offset_x         =     0;
      cfg.offset_y         =     0;
      cfg.offset_rotation  =     0;
      cfg.dummy_read_pixel =     8;
      cfg.dummy_read_bits  =     1;
      cfg.readable         =  true;
      cfg.invert           = true;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       =  true;
      _panel_instance.config(cfg);
    }
    //Backlight
    { auto cfg = _light_instance.config();    
      cfg.pin_bl = 45;              
      cfg.invert = false;           
      cfg.freq   = 44100;           
      cfg.pwm_channel = 7;          
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);  
    }
    //Touch
    { auto cfg = _touch_instance.config();
      cfg.x_min      = 0;
      cfg.x_max      = 319;
      cfg.y_min      = 0;  
      cfg.y_max      = 479;
      cfg.pin_int    = 7;  
      cfg.bus_shared = true; 
      cfg.offset_rotation = 0;
      cfg.i2c_port = 1;
      cfg.i2c_addr = 0x38;
      cfg.pin_sda  = 6;   
      cfg.pin_scl  = 5;   
      cfg.freq = 400000;  
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);  
    }
    setPanel(&_panel_instance); 
  }
};
#endif

#ifdef TFT_ST7796_2
//LovyanGFX Display Class Config
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7796  _panel_instance;  // ST7796UI
  lgfx::Bus_SPI        _bus_instance;   // SPIバスのインスタンス
  lgfx::Light_PWM     _light_instance;
  lgfx::Touch_FT5x06  _touch_instance;
public:
  LGFX(void) {
    //Bus
    {
      auto cfg = _bus_instance.config();    // バス設定用の構造体を取得します。
      cfg.spi_host = SPI3_HOST;     // 使用するSPIを選択  ESP32-S2,C3 : SPI2_HOST or SPI3_HOST / ESP32 : VSPI_HOST or HSPI_HOST
      cfg.spi_mode = 0;             // SPI通信モードを設定 (0 ~ 3)
      cfg.freq_write = 60000000;    // 送信時のSPIクロック (最大80MHz, 80MHzを整数で割った値に丸められます)
      cfg.freq_read  = 16000000;    // 受信時のSPIクロック
      cfg.spi_3wire  = true;        // 受信をMOSIピンで行う場合はtrueを設定
      cfg.use_lock   = true;        // トランザクションロックを使用する場合はtrueを設定
      cfg.dma_channel = SPI_DMA_CH_AUTO; // 使用するDMAチャンネルを設定 (0=DMA不使用 / 1=1ch / 2=ch / SPI_DMA_CH_AUTO=自動設定)
      // ※ ESP-IDFバージョンアップに伴い、DMAチャンネルはSPI_DMA_CH_AUTO(自動設定)が推奨になりました。1ch,2chの指定は非推奨になります。
      cfg.pin_sclk = TFT_SCK;            // SPIのSCLKピン番号を設定
      cfg.pin_mosi = TFT_MOSI;            // SPIのMOSIピン番号を設定
      cfg.pin_miso = TFT_MISO;            // SPIのMISOピン番号を設定 (-1 = disable)
      cfg.pin_dc   = TFT_DC;            // SPIのD/Cピン番号を設定  (-1 = disable)
      _bus_instance.config(cfg);    // 設定値をバスに反映します。
      _panel_instance.setBus(&_bus_instance);      // バスをパネルにセットします。
    }

    { // 表示パネル制御の設定を行います。
      auto cfg = _panel_instance.config();    // 表示パネル設定用の構造体を取得します。
      cfg.pin_cs           =    TFT_CS;  // CSが接続されているピン番号   (-1 = disable)
      cfg.pin_rst          =    TFT_RST;  // RSTが接続されているピン番号  (-1 = disable)
      cfg.pin_busy         =    -1;  // BUSYが接続されているピン番号 (-1 = disable)
      cfg.panel_width      =   TFT_WIDTH;  // 実際に表示可能な幅
      cfg.panel_height     =   TFT_HEIGHT;  // 実際に表示可能な高さ
      cfg.offset_x         =     0;  // パネルのX方向オフセット量
      cfg.offset_y         =     0;  // パネルのY方向オフセット量
      cfg.offset_rotation  =    TFT_ROTATION;  // 回転方向の値のオフセット 0~7 (4~7は上下反転)
      cfg.dummy_read_pixel =     8;  // ピクセル読出し前のダミーリードのビット数
      cfg.dummy_read_bits  =     1;  // ピクセル以外のデータ読出し前のダミーリードのビット数
      cfg.readable         =  true;  // データ読出しが可能な場合 trueに設定
      cfg.invert           = false;  // パネルの明暗が反転してしまう場合 trueに設定
      cfg.rgb_order        = false;  // パネルの赤と青が入れ替わってしまう場合 trueに設定
      cfg.dlen_16bit       = false;  // 16bitパラレルやSPIでデータ長を16bit単位で送信するパネルの場合 trueに設定
      cfg.bus_shared       =  true;  // SDカードとバスを共有している場合 trueに設定(drawJpgFile等でバス制御を行います)
      _panel_instance.config(cfg);
    }
    //Backlight
    { auto cfg = _light_instance.config();    
      cfg.pin_bl = TFT_BL;              
      cfg.invert = false;           
      cfg.freq   = 44100;           
      cfg.pwm_channel = 7;          
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);  
    }
    //Touch
    { auto cfg = _touch_instance.config();
      cfg.x_min      = 0;
      cfg.x_max      = (TFT_WIDTH - 1);
      cfg.y_min      = 0;  
      cfg.y_max      = (TFT_HEIGHT - 1);
      cfg.pin_int    = TP_INT;  
      cfg.bus_shared = true; 
      cfg.offset_rotation = 0;
      cfg.i2c_port = 0;
      cfg.i2c_addr = 0x38;
      cfg.pin_sda  = I2C_SDA;   
      cfg.pin_scl  = I2C_SCL;
#ifdef NXP6686         
      cfg.freq = 100000;  
#else
      cfg.freq = 400000;  
#endif
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);  
    }
    setPanel(&_panel_instance); 
  }
};
#endif

#ifdef TFT_ILI9341_1
class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9341     _panel_instance;
  lgfx::Bus_SPI        _bus_instance;   // SPIバスのインスタンス
  lgfx::Light_PWM     _light_instance;
  lgfx::Touch_FT5x06           _touch_instance; // FT5206, FT5306, FT5406, FT6206, FT6236, FT6336, FT6436

public:
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();    // バス設定用の構造体を取得します。
      cfg.spi_host = SPI3_HOST;     // 使用するSPIを選択  ESP32-S2,C3 : SPI2_HOST or SPI3_HOST / ESP32 : VSPI_HOST or HSPI_HOST
      cfg.spi_mode = 0;             // SPI通信モードを設定 (0 ~ 3)
      cfg.freq_write = 60000000;    // 送信時のSPIクロック (最大80MHz, 80MHzを整数で割った値に丸められます)
      cfg.freq_read  = 16000000;    // 受信時のSPIクロック
      cfg.spi_3wire  = true;        // 受信をMOSIピンで行う場合はtrueを設定
      cfg.use_lock   = true;        // トランザクションロックを使用する場合はtrueを設定
      cfg.dma_channel = SPI_DMA_CH_AUTO; // 使用するDMAチャンネルを設定 (0=DMA不使用 / 1=1ch / 2=ch / SPI_DMA_CH_AUTO=自動設定)
      // ※ ESP-IDFバージョンアップに伴い、DMAチャンネルはSPI_DMA_CH_AUTO(自動設定)が推奨になりました。1ch,2chの指定は非推奨になります。
      cfg.pin_sclk = TFT_SCK;            // SPIのSCLKピン番号を設定
      cfg.pin_mosi = TFT_MOSI;            // SPIのMOSIピン番号を設定
      cfg.pin_miso = TFT_MISO;            // SPIのMISOピン番号を設定 (-1 = disable)
      cfg.pin_dc   = TFT_DC;            // SPIのD/Cピン番号を設定  (-1 = disable)
      _bus_instance.config(cfg);    // 設定値をバスに反映します。
      _panel_instance.setBus(&_bus_instance);      // バスをパネルにセットします。
    }

    { // 表示パネル制御の設定を行います。
      auto cfg = _panel_instance.config();    // 表示パネル設定用の構造体を取得します。
      cfg.pin_cs           =    TFT_CS;  // CSが接続されているピン番号   (-1 = disable)
      cfg.pin_rst          =    TFT_RST;  // RSTが接続されているピン番号  (-1 = disable)
      cfg.pin_busy         =    -1;  // BUSYが接続されているピン番号 (-1 = disable)
      cfg.panel_width      =   TFT_HEIGHT;  // 実際に表示可能な幅
      cfg.panel_height     =   TFT_WIDTH;  // 実際に表示可能な高さ
      cfg.offset_x         =     0;  // パネルのX方向オフセット量
      cfg.offset_y         =     0;  // パネルのY方向オフセット量
      cfg.offset_rotation  =     0;  // 回転方向の値のオフセット 0~7 (4~7は上下反転)
      cfg.dummy_read_pixel =     8;  // ピクセル読出し前のダミーリードのビット数
      cfg.dummy_read_bits  =     1;  // ピクセル以外のデータ読出し前のダミーリードのビット数
      cfg.readable         =  true;  // データ読出しが可能な場合 trueに設定
      cfg.invert           =  true;  // パネルの明暗が反転してしまう場合 trueに設定
      cfg.rgb_order        = false;  // パネルの赤と青が入れ替わってしまう場合 trueに設定
      cfg.dlen_16bit       = false;  // 16bitパラレルやSPIでデータ長を16bit単位で送信するパネルの場合 trueに設定
      cfg.bus_shared       =  true;  // SDカードとバスを共有している場合 trueに設定(drawJpgFile等でバス制御を行います)
      _panel_instance.config(cfg);
    }
    { // バックライト制御の設定を行います。（必要なければ削除）
      auto cfg = _light_instance.config();    // バックライト設定用の構造体を取得します。
      cfg.pin_bl = TFT_BL;              // バックライトが接続されているピン番号
      cfg.invert = false;           // バックライトの輝度を反転させる場合 true
      cfg.freq   = 44100;           // バックライトのPWM周波数
      cfg.pwm_channel = 7;          // 使用するPWMのチャンネル番号
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);  // バックライトをパネルにセットします。
    }
    { // タッチスクリーン制御の設定を行います。（必要なければ削除）
      auto cfg = _touch_instance.config();
      cfg.x_min      = 0;    // タッチスクリーンから得られる最小のX値(生の値)
      cfg.x_max      = 239;  // タッチスクリーンから得られる最大のX値(生の値)
      cfg.y_min      = 0;    // タッチスクリーンから得られる最小のY値(生の値)
      cfg.y_max      = 319;  // タッチスクリーンから得られる最大のY値(生の値)
      cfg.pin_int    = TP_INT;   // INTが接続されているピン番号
      cfg.bus_shared = true; // 画面と共通のバスを使用している場合 trueを設定
      cfg.offset_rotation = 2;// 表示とタッチの向きのが一致しない場合の調整 0~7の値で設定
      cfg.i2c_port = 0;      // 使用するI2Cを選択 (0 or 1)
      cfg.i2c_addr = 0x38;   // I2Cデバイスアドレス番号
      cfg.pin_sda  = TP_SDA;     // SDAが接続されているピン番号
      cfg.pin_scl  = TP_SCL;     // SCLが接続されているピン番号
      cfg.pin_rst  = -1;
      cfg.freq = 400000;     // I2Cクロックを設定
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);  // タッチスクリーンをパネルにセットします。
    }
    setPanel(&_panel_instance); // 使用するパネルをセットします。
  }
};

#endif

#ifdef TFT_ILI9341_2
class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9341     _panel_instance;
  lgfx::Bus_SPI        _bus_instance;   // SPIバスのインスタンス
  lgfx::Light_PWM     _light_instance;
  lgfx::Touch_XPT2046          _touch_instance;
public:
  LGFX(void)
  {
    { // バス制御の設定を行います。
      auto cfg = _bus_instance.config();    // バス設定用の構造体を取得します。
      cfg.spi_host = VSPI_HOST;     // 使用するSPIを選択  ESP32-S2,C3 : SPI2_HOST or SPI3_HOST / ESP32 : VSPI_HOST or HSPI_HOST
      // ※ ESP-IDFバージョンアップに伴い、VSPI_HOST , HSPI_HOSTの記述は非推奨になるため、エラーが出る場合は代わりにSPI2_HOST , SPI3_HOSTを使用してください。
      cfg.spi_mode = 0;             // SPI通信モードを設定 (0 ~ 3)
      cfg.freq_write = 40000000;    // 送信時のSPIクロック (最大80MHz, 80MHzを整数で割った値に丸められます)
      cfg.freq_read  = 16000000;    // 受信時のSPIクロック
      cfg.spi_3wire  = true;        // 受信をMOSIピンで行う場合はtrueを設定
      cfg.use_lock   = true;        // トランザクションロックを使用する場合はtrueを設定
      cfg.dma_channel = SPI_DMA_CH_AUTO; // 使用するDMAチャンネルを設定 (0=DMA不使用 / 1=1ch / 2=ch / SPI_DMA_CH_AUTO=自動設定)
      cfg.pin_sclk = TFT_SCK;            // SPIのSCLKピン番号を設定
      cfg.pin_mosi = TFT_MOSI;            // SPIのMOSIピン番号を設定
      cfg.pin_miso = TFT_MISO;            // SPIのMISOピン番号を設定 (-1 = disable)
      cfg.pin_dc   = TFT_DC;            // SPIのD/Cピン番号を設定  (-1 = disable)
      _bus_instance.config(cfg);    // 設定値をバスに反映します。
      _panel_instance.setBus(&_bus_instance);      // バスをパネルにセットします。
    }
    { // 表示パネル制御の設定を行います。
      auto cfg = _panel_instance.config();    // 表示パネル設定用の構造体を取得します。
      cfg.pin_cs           =    TFT_CS;  // CSが接続されているピン番号   (-1 = disable)
      cfg.pin_rst          =    TFT_RST;  // RSTが接続されているピン番号  (-1 = disable)
      cfg.pin_busy         =    -1;  // BUSYが接続されているピン番号 (-1 = disable)
      cfg.panel_width      =   240;  // 実際に表示可能な幅
      cfg.panel_height     =   320;  // 実際に表示可能な高さ
      cfg.offset_x         =     0;  // パネルのX方向オフセット量
      cfg.offset_y         =     0;  // パネルのY方向オフセット量
      cfg.offset_rotation  =     0;  // 回転方向の値のオフセット 0~7 (4~7は上下反転)
      cfg.dummy_read_pixel =     8;  // ピクセル読出し前のダミーリードのビット数
      cfg.dummy_read_bits  =     1;  // ピクセル以外のデータ読出し前のダミーリードのビット数
      cfg.readable         =  true;  // データ読出しが可能な場合 trueに設定
      cfg.invert           = false;  // パネルの明暗が反転してしまう場合 trueに設定
      cfg.rgb_order        = false;  // パネルの赤と青が入れ替わってしまう場合 trueに設定
      cfg.dlen_16bit       = false;  // 16bitパラレルやSPIでデータ長を16bit単位で送信するパネルの場合 trueに設定
      cfg.bus_shared       =  true;  // SDカードとバスを共有している場合 trueに設定(drawJpgFile等でバス制御を行います)
      _panel_instance.config(cfg);
    }
    { // バックライト制御の設定を行います。（必要なければ削除）
      auto cfg = _light_instance.config();    // バックライト設定用の構造体を取得します。
      cfg.pin_bl = TFT_BL;              // バックライトが接続されているピン番号
      cfg.invert = false;           // バックライトの輝度を反転させる場合 true
      cfg.freq   = 44100;           // バックライトのPWM周波数
      cfg.pwm_channel = 7;          // 使用するPWMのチャンネル番号
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);  // バックライトをパネルにセットします。
    }
    { // タッチスクリーン制御の設定を行います。（必要なければ削除）
      auto cfg = _touch_instance.config();
      cfg.x_min      = 0;    // タッチスクリーンから得られる最小のX値(生の値)
      cfg.x_max      = 239;  // タッチスクリーンから得られる最大のX値(生の値)
      cfg.y_min      = 0;    // タッチスクリーンから得られる最小のY値(生の値)
      cfg.y_max      = 319;  // タッチスクリーンから得られる最大のY値(生の値)
      cfg.pin_int    = -1;   // INTが接続されているピン番号
      cfg.bus_shared = true; // 画面と共通のバスを使用している場合 trueを設定
      cfg.offset_rotation = 0;// 表示とタッチの向きのが一致しない場合の調整 0~7の値で設定
      cfg.spi_host = VSPI_HOST;// 使用するSPIを選択 (HSPI_HOST or VSPI_HOST)
      cfg.freq = 1000000;     // SPIクロックを設定
      cfg.pin_sclk = TFT_SCK;     // SCLKが接続されているピン番号
      cfg.pin_mosi = TFT_MOSI;     // MOSIが接続されているピン番号
      cfg.pin_miso = TFT_MISO;     // MISOが接続されているピン番号
      cfg.pin_cs   = TFT_TCS;     //   CSが接続されているピン番号
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);  // タッチスクリーンをパネルにセットします。
    }
    setPanel(&_panel_instance); // 使用するパネルをセットします。
  }
};
#endif


//Colour names
#define LTBLUE    0xB6DF
#define LTTEAL    0xBF5F
#define LTGREEN   0xF7BF
#define LTCYAN    0xC7FF
#define LTRED     0x34FD
#define LTMAGENTA 0xFD5F
#define LTYELLOW  0xFFF8
#define LTORANGE  0xFE73
#define LTPINK    0xFDDF
#define LTPURPLE  0xCCFF
#define LTGREY    0xE71C

#define BLUE      0x001F
#define TEAL      0x0438
#define GREEN     0xE007
#define CYAN      0x07FF
#define RED       0x00F8
#define MAGENTA   0xF81F
#define YELLOW    0xE0FF
#define ORANGE    0xFC00
#define PINK      0xF81F
#define PURPLE    0x8010
#define GREY      0xC618
#define WHITE     0xFFFF
#define BLACK     0x0000
#define CHAR      0xE7E0

#define DKBLUE    0x000D
#define DKTEAL    0x020C
#define DKGREEN   0xE003
#define DKCYAN    0x03EF
#define DKRED     0x0060
#define DKMAGENTA 0x8008
#define DKYELLOW  0x8400
#define DKORANGE  0x8200
#define DKPINK    0x9009
#define DKPURPLE  0x4010
#define DKGREY    0x4A49
#define BACKG     0x0821
