#include "decls.h"
#ifdef BLUETOOTH

lv_obj_t* transportContainer;
lv_obj_t* backBtn;
lv_obj_t* rewBtn;
lv_obj_t* playBtn;
lv_obj_t* playBtnLbl;
lv_obj_t* stopBtn;
lv_obj_t* ffBtn;
lv_obj_t* forwardBtn;

static void backBtn_action(lv_event_t * event);
static void rewBtn_action(lv_event_t * event);
static void playBtn_action(lv_event_t * event);
static void stopBtn_action(lv_event_t * event);
static void ffBtn_action(lv_event_t * event);
static void forwardBtn_action(lv_event_t * event);

bool playButtonState = false;
int currentPlayStatus = ESP_AVRC_PLAYBACK_STOPPED;
int currentSampleRate = 0;
bool currentConnectionState = false;
volatile bool currentStackState = false;
bool gotMetadata = false;
uint32_t trackLength = 0;
uint32_t currentPlayPosition = 0;
bool showingTimeBar = false;
const char * rName = NULL;
esp_bd_addr_t peerAddress = {0};

void passcodeEntered(const char*);

//Bluetooth transport controls
lv_obj_t* createTransportWidget(lv_obj_t* parent) {
  transportContainer = lv_obj_create(parent);
  lv_obj_add_style(transportContainer, &style_groupbox, LV_PART_MAIN);
  lv_obj_clear_flag(transportContainer, LV_OBJ_FLAG_SCROLLABLE);
#if (TFT_WIDTH == 480)
  lv_obj_set_size(transportContainer, 460, 50);
#else
  lv_obj_set_size(transportContainer, 312, 50);
#endif

  backBtn = lv_btn_create(transportContainer);
  lv_obj_set_size(backBtn, 44, 34);
  lv_obj_add_style(backBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(backBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(backBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_set_pos(backBtn, 2, 0);
  lv_obj_add_event_cb(backBtn, backBtn_action, LV_EVENT_ALL, NULL);
  lv_obj_t * label = lv_label_create(backBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_PREV);

  rewBtn = lv_btn_create(transportContainer);
  lv_obj_set_size(rewBtn, 44, 34);
  lv_obj_add_style(rewBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(rewBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(rewBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_align_to(rewBtn, backBtn, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
  lv_obj_add_event_cb(rewBtn, rewBtn_action, LV_EVENT_ALL, NULL);
  label = lv_label_create(rewBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_LEFT);

  playBtn = lv_btn_create(transportContainer);
  lv_obj_set_size(playBtn, 44, 34);
  lv_obj_add_style(playBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(playBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(playBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_align_to(playBtn, rewBtn, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
  lv_obj_add_event_cb(playBtn, playBtn_action, LV_EVENT_ALL, NULL);
  playBtnLbl = lv_label_create(playBtn);
  lv_obj_align(playBtnLbl, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(playBtnLbl, LV_SYMBOL_PLAY);

  stopBtn = lv_btn_create(transportContainer);
  lv_obj_set_size(stopBtn, 44, 34);
  lv_obj_add_style(stopBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(stopBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(stopBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_align_to(stopBtn, playBtn, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
  lv_obj_add_event_cb(stopBtn, stopBtn_action, LV_EVENT_ALL, NULL);
  label = lv_label_create(stopBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_STOP);

  ffBtn = lv_btn_create(transportContainer);
  lv_obj_set_size(ffBtn, 44, 34);
  lv_obj_add_style(ffBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(ffBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(ffBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_align_to(ffBtn, stopBtn, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
  lv_obj_add_event_cb(ffBtn, ffBtn_action, LV_EVENT_ALL, NULL);
  label = lv_label_create(ffBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_RIGHT);

  forwardBtn = lv_btn_create(transportContainer);
  lv_obj_set_size(forwardBtn, 44, 34);
  lv_obj_add_style(forwardBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(forwardBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(forwardBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_align_to(forwardBtn, ffBtn, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
  lv_obj_add_event_cb(forwardBtn, forwardBtn_action, LV_EVENT_ALL, NULL);
  label = lv_label_create(forwardBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_NEXT);
  return transportContainer;  
}

static void backBtn_action(lv_event_t * event) {
  lv_event_code_t code = lv_event_get_code(event);
  if (code == LV_EVENT_PRESSED) passBluetooth(ESP_AVRC_PT_CMD_BACKWARD, ESP_AVRC_PT_CMD_STATE_PRESSED);
  if (code == LV_EVENT_RELEASED) passBluetooth(ESP_AVRC_PT_CMD_BACKWARD, ESP_AVRC_PT_CMD_STATE_RELEASED);
}

static void rewBtn_action(lv_event_t * event) {
  lv_event_code_t code = lv_event_get_code(event);
  if (code == LV_EVENT_PRESSED) passBluetooth(ESP_AVRC_PT_CMD_REWIND, ESP_AVRC_PT_CMD_STATE_PRESSED);
  if (code == LV_EVENT_RELEASED) passBluetooth(ESP_AVRC_PT_CMD_REWIND, ESP_AVRC_PT_CMD_STATE_RELEASED);
}

//Change play button icon from play to pause and back
void updatePlayButton() {
  bool playing = (currentPlayStatus == ESP_AVRC_PLAYBACK_PLAYING) || 
                 (currentPlayStatus == ESP_AVRC_PLAYBACK_FWD_SEEK) || 
                 (currentPlayStatus == ESP_AVRC_PLAYBACK_REV_SEEK);
  if (playButtonState != playing) {
    playButtonState = playing;
    if (playing) {
      lv_label_set_text(playBtnLbl, LV_SYMBOL_PAUSE);
    } else {
      lv_label_set_text(playBtnLbl, LV_SYMBOL_PLAY);
    }
  }
}

static void playBtn_action(lv_event_t * event) {
  lv_event_code_t code = lv_event_get_code(event);
  if (playButtonState) {
    if (code == LV_EVENT_PRESSED) passBluetooth(ESP_AVRC_PT_CMD_PAUSE, ESP_AVRC_PT_CMD_STATE_PRESSED);
    if (code == LV_EVENT_RELEASED) passBluetooth(ESP_AVRC_PT_CMD_PAUSE, ESP_AVRC_PT_CMD_STATE_RELEASED);
  } else {    
    if (code == LV_EVENT_PRESSED) passBluetooth(ESP_AVRC_PT_CMD_PLAY, ESP_AVRC_PT_CMD_STATE_PRESSED);
    if (code == LV_EVENT_RELEASED) passBluetooth(ESP_AVRC_PT_CMD_PLAY, ESP_AVRC_PT_CMD_STATE_RELEASED);
  }
}

static void stopBtn_action(lv_event_t * event) {
  lv_event_code_t code = lv_event_get_code(event);  
  if (code == LV_EVENT_PRESSED) passBluetooth(ESP_AVRC_PT_CMD_STOP, ESP_AVRC_PT_CMD_STATE_PRESSED);
  if (code == LV_EVENT_RELEASED) passBluetooth(ESP_AVRC_PT_CMD_STOP, ESP_AVRC_PT_CMD_STATE_RELEASED);
}

static void ffBtn_action(lv_event_t * event) {
  lv_event_code_t code = lv_event_get_code(event);  
  if (code == LV_EVENT_PRESSED) passBluetooth(ESP_AVRC_PT_CMD_FAST_FORWARD, ESP_AVRC_PT_CMD_STATE_PRESSED);
  if (code == LV_EVENT_RELEASED) passBluetooth(ESP_AVRC_PT_CMD_FAST_FORWARD, ESP_AVRC_PT_CMD_STATE_RELEASED);
}

static void forwardBtn_action(lv_event_t * event) {
  lv_event_code_t code = lv_event_get_code(event);  
  if (code == LV_EVENT_PRESSED) passBluetooth(ESP_AVRC_PT_CMD_FORWARD, ESP_AVRC_PT_CMD_STATE_PRESSED);
  if (code == LV_EVENT_RELEASED) passBluetooth(ESP_AVRC_PT_CMD_FORWARD, ESP_AVRC_PT_CMD_STATE_RELEASED);
}

bool isBtConnected() {
  return currentConnectionState;
}

bool isBtStarted() {
  return currentStackState;
}

//------------------------------------------------------------------------------------
// Passcode window - action required

static lv_obj_t * passcodeWindow;
static lv_obj_t * passcodeKeyboard;
static lv_timer_t * passcodeTimer = NULL;
#define PASSCODE_TIMEOUT 20000

static void passcodeWindow_action(lv_event_t * event);
static void passcodeKeyboard_action(lv_event_t * event);
static void passcodeTimer_action(lv_timer_t * timer);

//Construct the passcode window and pop it up
void createPasscodeWindow(lv_obj_t * page, int y, void(*okFunction)(const char*), bool animated) {
  if (passcodeWindow) {
    lv_obj_del(passcodeWindow);
    animated = false;
  }
  passcodeWindow = lv_win_create(page, 40);
  lv_obj_set_size(passcodeWindow, 208, 170);
  int width = lv_obj_get_content_width(page);
  int x = (width - 208) / 2;
  lv_obj_set_pos(passcodeWindow, x, y);
  lv_obj_add_style(passcodeWindow, &style_win, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(passcodeWindow, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_t * header = lv_win_get_header(passcodeWindow);
  lv_obj_t * title = lv_textarea_create(header);
  lv_textarea_set_one_line(title, true);
  lv_textarea_set_placeholder_text(title, "Passcode?");
  lv_obj_set_width(title, 80);
  lv_obj_add_style(title, &style_biggerfont, LV_PART_MAIN);
  lv_obj_add_style(header, &style_groupbox, LV_PART_MAIN);

  lv_obj_t * body = lv_win_get_content(passcodeWindow);
  passcodeKeyboard = lv_keyboard_create(body);
  lv_keyboard_set_mode(passcodeKeyboard, LV_KEYBOARD_MODE_NUMBER);
  lv_obj_set_size(passcodeKeyboard, lv_pct(100), lv_pct(100));
  lv_keyboard_set_textarea(passcodeKeyboard, title);
  lv_obj_add_event_cb(keyBoard, passcodeKeyboard_action, LV_EVENT_ALL, NULL);

  lv_obj_t * ok_btn = lv_win_add_btn(passcodeWindow, LV_SYMBOL_OK, 50);           /*Add close button and use built-in close action*/
  lv_obj_add_style(ok_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(ok_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(ok_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(ok_btn, passcodeWindow_action, LV_EVENT_CLICKED, (void*)okFunction);

  lv_obj_t * cancel_btn = lv_win_add_btn(passcodeWindow, LV_SYMBOL_CLOSE, 50);           /*Add close button and use built-in close action*/
  lv_obj_add_style(cancel_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(cancel_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(cancel_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(cancel_btn, passcodeWindow_action, LV_EVENT_CLICKED, NULL);
  if (animated) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, passcodeWindow);
    lv_anim_set_values(&a, LV_VER_RES, y);
#pragma GCC diagnostic push                         //Trouble at mill - pay no heed
#pragma GCC diagnostic ignored "-Wcast-function-type"
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
#pragma GCC diagnostic pop
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_time(&a, 500);
    lv_anim_start(&a);
  }
  passcodeTimer = lv_timer_create(passcodeTimer_action, PASSCODE_TIMEOUT, NULL);
}

//Trigger the passcode window popup
void getPasscode(void(*okFunction)(const char*), bool animated) {
#ifdef VUMETER
  createPasscodeWindow(mainWindow, 112, okFunction, animated); 
#else
  createPasscodeWindow(mainWindow, 0, okFunction, animated); 
#endif
}

void passcodeHiddenAction(lv_anim_t * a) {
  lv_obj_del(passcodeWindow);
  passcodeWindow = NULL;  
}

//Close the passcode window
void closePasscode(bool animated) {
  if (passcodeWindow == NULL) return;
  if (passcodeTimer) lv_timer_del(passcodeTimer);
  passcodeTimer = NULL;
  if (animated) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, passcodeWindow);
    lv_anim_set_values(&a, lv_obj_get_y(passcodeWindow), LV_VER_RES);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
#pragma GCC diagnostic pop
    lv_anim_set_ready_cb(&a, passcodeHiddenAction);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_set_time(&a, 500);
    lv_anim_start(&a);
  } else {
    lv_obj_del(passcodeWindow);
    passcodeWindow = NULL;  
  }
}

static void passcodeWindow_action(lv_event_t * event) {
  void (*okFunction)(const char*) = (void(*)(const char*))lv_event_get_user_data(event);
  if (okFunction) (*okFunction)(lv_textarea_get_text(lv_keyboard_get_textarea(passcodeKeyboard)));
  closePasscode(true);
}

static void passcodeTimer_action(lv_timer_t * timer) {
  closePasscode(true);
}

//Send the passcode reply
void passcodeEntered(const char* passcodeStr) {
  uint32_t code = atoi(passcodeStr);
  if (code > 0) {
    authBluetooth(code, true);
  }
}

static void passcodeKeyboard_action(lv_event_t * event) {
  uint32_t res = lv_event_get_code(event);
  if (passcodeTimer) lv_timer_reset(passcodeTimer);
  if(res == LV_EVENT_READY || res == LV_EVENT_CANCEL){
    closePasscode(true);
    if(res == LV_EVENT_READY)
      passcodeEntered(lv_textarea_get_text(lv_keyboard_get_textarea(passcodeKeyboard)));
  
  }
}

void updateTimeBar() {
  bool show = (trackLength > 0) && (currentPlayPosition > 0);
  if (show != showingTimeBar) {
    showingTimeBar = show;
    showTimeBar(show);
  }
  if (show) {
    uint8_t pct = (currentPlayPosition * 100.0) / trackLength;
    if (pct > 100) pct = 100;
    setTimeBar(pct);
  }
}

//------------------------------------------------------------------
//Bluetooth events
//From main thread context

#define BT_TXT_EVENT 0
#define BT_CONN_EVENT 1
#define BT_VOL_EVENT 2
#define BT_META_EVENT 3
#define BT_STATE_EVENT 4
#define BT_SR_EVENT 5
#define BT_CH_EVENT 6
#define BT_POS_EVENT 7
#define BT_RSSI_EVENT 8
#define BT_RNAME_EVENT 9
#define BT_PASSKEY_EVENT 10
#define BT_PASSREQ_EVENT 11
#define BT_AUTH_EVENT 12

//Called from audio message handler to deal with bluetooth related messages
void bluetoothMessage(uint32_t source, uint32_t val, const char* txt) {
  if (settings->mode != MODE_BT) return;
  if (source == BT_TXT_EVENT) {
    info(TEXT, 0, LV_SYMBOL_BLUETOOTH " %s", txt);
  }
  if (source == BT_AUTH_EVENT) {
    closePasscode(true);
    if (val == ESP_BT_STATUS_SUCCESS) {
      info(TEXT, 0, "Authentication success: %s\r\n", txt);
    } else info(TEXT, 0, "Authentication failed, status:%d\r\n", val);
  }
  else if (source == BT_CONN_EVENT) {
    currentConnectionState = val;
    if (val == ESP_A2D_CONNECTION_STATE_CONNECTING) {
      info(TEXT, 0, LV_SYMBOL_BLUETOOTH " Connecting..");
    }
    else if (val == ESP_A2D_CONNECTION_STATE_CONNECTED) {
      if (rName) {    
        info(TEXT, 0, LV_SYMBOL_BLUETOOTH " Connected to %s", rName);
      } 
      else info(TEXT, 0, LV_SYMBOL_BLUETOOTH " Connected");
      updateUrlEditText();
    }
    else if (val == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
      info(TEXT, 0, LV_SYMBOL_BLUETOOTH " Disconnected");
      info(NAME, 0, LV_SYMBOL_STOP " Stopped");
      info(NOW, 0, "");
      rName = NULL;
      gotMetadata = false;
      updateUrlEditText();
    }
  }
  else if (source == BT_VOL_EVENT) {
    settings->dabVolume = val * 21.0f / 127.0f; 
    setVolume(settings->dabVolume);
    if (dabVolSlider) {
      lv_slider_set_value(dabVolSlider, settings->dabVolume, LV_ANIM_OFF);
    }
    writeSettings();
  }
  else if (source == BT_META_EVENT) {
    gotMetadata = true;
    if (val == ESP_AVRC_MD_ATTR_TITLE) audio_showstreamtitle(txt);
    else if (val == ESP_AVRC_MD_ATTR_ARTIST) audio_showstation(txt);
    else if (val == ESP_AVRC_MD_ATTR_PLAYING_TIME) {
      trackLength = atoi(txt);
      updateTimeBar();
    }
  }
  else if (source == BT_STATE_EVENT) {
    currentPlayStatus = val;
    uint8_t infoLine = NAME;
    if (gotMetadata) infoLine = TEXT;
    if (showingTimeBar) {
      if (currentPlayStatus == ESP_AVRC_PLAYBACK_PLAYING) info(infoLine, 0, LV_SYMBOL_PLAY);
      else if (currentPlayStatus == ESP_AVRC_PLAYBACK_STOPPED) info(infoLine, 0, LV_SYMBOL_STOP);
      else if (currentPlayStatus == ESP_AVRC_PLAYBACK_PAUSED) info(infoLine, 0, LV_SYMBOL_PAUSE);
      else if (currentPlayStatus == ESP_AVRC_PLAYBACK_FWD_SEEK) info(infoLine, 0, LV_SYMBOL_RIGHT);
      else if (currentPlayStatus == ESP_AVRC_PLAYBACK_REV_SEEK) info(infoLine, 0, LV_SYMBOL_LEFT);
    } else {
      if (currentPlayStatus == ESP_AVRC_PLAYBACK_PLAYING) info(infoLine, 0, LV_SYMBOL_PLAY " Playing");
      else if (currentPlayStatus == ESP_AVRC_PLAYBACK_STOPPED) info(infoLine, 0, LV_SYMBOL_STOP " Stopped");
      else if (currentPlayStatus == ESP_AVRC_PLAYBACK_PAUSED) info(infoLine, 0, LV_SYMBOL_PAUSE " Paused");
      else if (currentPlayStatus == ESP_AVRC_PLAYBACK_FWD_SEEK) info(infoLine, 0, LV_SYMBOL_RIGHT " Fast Forward");
      else if (currentPlayStatus == ESP_AVRC_PLAYBACK_REV_SEEK) info(infoLine, 0, LV_SYMBOL_LEFT, " Rewind");
    }
    updatePlayButton();
  }
  else if (source == BT_SR_EVENT) {
    currentSampleRate = val;
    setSampleRate(currentSampleRate);
  }
  else if (source == BT_CH_EVENT) {
    updateSTMOLabel(val == 1 ? STMO_MONO : STMO_STEREO);    
  }
  else if (source == BT_POS_EVENT) {
    currentPlayPosition = val;
    updateTimeBar();
  }
  else if (source == BT_RSSI_EVENT) {
    if (val > 128) val = 128;
    val = val * 100 / 128.0f;
    char s[25] = {0};
    snprintf(s, 24, LV_SYMBOL_WIFI " %d%%", val);
    setSigStrengthLbl(s);    
  }
  else if (source == BT_RNAME_EVENT) {
    rName = txt;
    if (currentConnectionState)
      info(TEXT, 0, LV_SYMBOL_BLUETOOTH " Connected to %s", rName);
  }
  else if (source == BT_PASSKEY_EVENT) {
      info(NAME, 10, "Passkey: %d", val);
  }
  else if (source == BT_PASSREQ_EVENT) {
    getPasscode(passcodeEntered, true);
  }

}

//--------------------------------------------------------
//Address handling

bool readPeerAddress() {
  esp_bd_addr_t empty_connection = {0, 0, 0, 0, 0, 0};
  memcpy(peerAddress, settings->hostAddrBt, ESP_BD_ADDR_LEN);
  int result = memcmp(peerAddress, empty_connection, ESP_BD_ADDR_LEN);
  return result != 0;
}

void writePeerAddress(esp_bd_addr_t address) {
  memcpy(settings->hostAddrBt, address, ESP_BD_ADDR_LEN);
  writeSettings();
}

void clearPeerAddress() {
  esp_bd_addr_t cleanBda = {0};
  writePeerAddress(cleanBda);
}

//-------------------------------------------------------
// Threaded below - called from audio thread context

void bt_data_cb(const uint8_t *data, uint32_t len);
void bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
void bt_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
void bt_rc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);
void bt_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);

bool btConnected = false;
bool btVolNotify = false;
bool btPassNotify = false;
bool btAutoConnect = false;
uint8_t btAutoRetry = 0;
unsigned long btAutoTimeout = 0;
uint8_t btVolume = 0;
uint8_t btChannels = 0;
uint16_t btRemoteFeaturesFlag = 0;
int8_t lastRSSI = 0;
char remoteName[ESP_BT_GAP_MAX_BDNAME_LEN];
bool rssiActive = true;

#define BT_AUTOTIMER 2000

// AVRCP used transaction label
#define APP_RC_CT_TL_GET_CAPS            (0)
#define APP_RC_CT_TL_GET_META_DATA       (1)
#define APP_RC_CT_TL_RN_TRACK_CHANGE     (2)
#define APP_RC_CT_TL_RN_PLAYBACK_CHANGE  (3)
#define APP_RC_CT_TL_RN_PLAY_POS_CHANGE  (4)
#define ESP_AVRC_FEAT_FLAG_TG_COVER_ART  (0x0100)                     /*!< TG support Cover Art */


static esp_avrc_rn_evt_cap_mask_t s_avrc_peer_rn_cap;
//Prepare for Bluetooth - started by audio thread
bool initBT() {
  bool fail = false;
  // set up bluetooth classic via bluedroid
  esp_err_t err = esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
  if (err != ESP_OK) { fail = true; log_e("esp_bt_controller_mem_release:%d", err); }
  return (!fail);
}

//Start the Bluetooth stack - called from the audio thread
bool startBT() {
  bool fail = false;
  //Prepare audio output
  setSampleRate(44100);
  setBitsPerSample(16);
  setChannels(2);
  // start bluetooth classic via bluedroid
  if (!btStart()) { fail = true; log_e("btStart"); }
  esp_err_t err = esp_bluedroid_init();
  if (err != ESP_OK) { fail = true; log_e("esp_bluedroid_init:%d", err); }
  err = esp_bluedroid_enable();
  if (err != ESP_OK) { fail = true; log_e("esp_bluedroid_enable:%d", err); }

  // set up device name
  const char *dev_name = RADIONAME;
  err = esp_bt_dev_set_device_name(dev_name);
  if (err != ESP_OK) { fail = true; log_e("esp_bt_dev_set_device_name:%d", err); }

  // initialize AVRCP controller
  err = esp_avrc_ct_init();
  if (err != ESP_OK) { fail = true; log_e("esp_avrc_ct_init:%d", err); }
  err = esp_avrc_ct_register_callback(bt_rc_ct_cb);
  if (err != ESP_OK) { fail = true; log_e("esp_avrc_ct_register_callback:%d", err); }
  err = esp_avrc_tg_init();
  if (err != ESP_OK) { fail = true; log_e("esp_avrc_tg_init:%d", err); }
  err = esp_avrc_tg_register_callback(bt_rc_tg_cb);
  if (err != ESP_OK) { fail = true; log_e("esp_avrc_tg_register_callback:%d", err); }

  esp_avrc_rn_evt_cap_mask_t evt_set = {0};
  esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE);
  assert(esp_avrc_tg_set_rn_evt_cap(&evt_set) == ESP_OK);

  // initialize A2DP sink and set the data callback(A2DP is bluetooth audio)
  err = esp_a2d_register_callback(bt_a2d_cb);
  if (err != ESP_OK) { fail = true; log_e("esp_a2d_register_callback:%d", err); }
  err = esp_a2d_sink_register_data_callback(bt_data_cb);
  if (err != ESP_OK) { fail = true; log_e("esp_a2d_sink_register_data_callback:%d", err); }
  err = esp_a2d_sink_init();
  if (err != ESP_OK) { fail = true; log_e("esp_a2d_sink_init:%d", err); }
  
  // set discoverable and connectable mode, wait to be connected
  err = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE); // for newer ESP-IDF
  if (err != ESP_OK) { fail = true; log_e("esp_bt_gap_set_scan_mode:%d", err); }
  err = esp_bt_gap_register_callback(bt_gap_cb);
  if (err != ESP_OK) { fail = true; log_e("esp_bt_gap_register_callback:%d", err); }

#if (CONFIG_BT_SSP_ENABLED == true)
  /* Set default parameters for Secure Simple Pairing */
  esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
  esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
  err = esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
  if (err != ESP_OK) { fail = true; log_e("esp_bt_gap_set_security_param:%d", err); }
#endif

  esp_bt_pin_type_t pin_type;
  if (settings->pinreqBt) {
    // Set default parameters for Secure Simple Pairing
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IN;
    if(settings->pinreqBt == 1) iocap = ESP_BT_IO_CAP_OUT;
    err = esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
    if (err != ESP_OK) { fail = true; log_e("esp_bt_gap_set_security_param:%d", err); }
    // invokes callbacks
    pin_type = ESP_BT_PIN_TYPE_VARIABLE;
  } else {
    // Set default parameters for Legacy Pairing
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    err = esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
    if (err != ESP_OK) { fail = true; log_e("esp_bt_gap_set_security_param:%d", err); }
    // no callbacks
    pin_type = ESP_BT_PIN_TYPE_FIXED;
  }

  esp_bt_pin_code_t pin_code;
  memcpy(pin_code, settings->pincodeBt, 4);
  err = esp_bt_gap_set_pin(pin_type, 0, pin_code);
  if (err != ESP_OK) { fail = true; log_e("esp_bt_gap_set_pin:%d", err); }

  //Reconnect to last peer
  btAutoConnect = settings->reconnectBt;
  if (!fail && btAutoConnect && (btAutoConnect = readPeerAddress())) {
    btNotify(BT_TXT_EVENT, 0, "Reconnecting to last peer..");
    BTconnect(peerAddress);
    btAutoTimeout = millis() + BT_AUTOTIMER;
  } else {
    if (fail) btNotify(BT_TXT_EVENT, 0, "Bluetooth failed!");
    else btNotify(BT_TXT_EVENT, 0, "Bluetooth ready");
    btAutoTimeout = 0;
  }
  btAutoRetry = 0;
  currentStackState = (!fail);
  return (!fail);
}

void handleBT() {
  if (settings->mode == MODE_BT && btAutoConnect) {
    if (btAutoTimeout && btAutoTimeout < millis()) {
      if (btAutoRetry < 4) {
        char buf[32];
        sprintf(buf, "Searching for peer, attempt %d..", btAutoRetry);
        btNotify(BT_TXT_EVENT, 0, buf);
        BTconnect(peerAddress);
        btAutoRetry++;
        btAutoTimeout = millis() + BT_AUTOTIMER;
      } else {
        btNotify(BT_TXT_EVENT, 0, "Last peer not found.");
        btAutoTimeout = 0;
      }
    }
  }
}

//Shut down BT controller - called from audio thread
void stopBT() {
  currentStackState = false;
  esp_a2d_sink_deinit();
  esp_bluedroid_disable();
  esp_bluedroid_deinit();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();
}

//Send volume change - called from audio thread
void BTvolchange(uint32_t vol) {
  btVolume = vol * 127.0f / 21.0f;
  if (btConnected && btVolNotify) {
    esp_avrc_rn_param_t rn_param;
    rn_param.volume = btVolume;
    esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE, ESP_AVRC_RN_RSP_CHANGED, &rn_param);
    btVolNotify = false;
  }
}

//Send passthrough - called from audio thread
void BTpassthrough(uint8_t code, uint8_t state) {
  static uint8_t tl = 0;
  if (btConnected) {
    esp_avrc_ct_send_passthrough_cmd(tl, code, state);
    btPassNotify = false;
    tl = (tl + 1) % 16;
  }
}

//Send passcode
void BTauthorise(uint32_t code, bool accept) {
  if (esp_bt_gap_ssp_passkey_reply(peerAddress, accept, code) != ESP_OK) {
    log_e("esp_bt_gap_ssp_passkey_reply");
  }
}


/// Activates the rssi reporting
void BTsetRSSIActive(bool active) { rssiActive = active; }

/// Requests an update of the rssi delta value
bool BTupdateRssi() {
  if (rssiActive) esp_bt_gap_read_rssi_delta(peerAddress);
  return rssiActive;
}

//Connect to specific peer
bool BTconnect(esp_bd_addr_t peer) {
  esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
  esp_err_t err = esp_a2d_sink_connect(peer);
  if (err != ESP_OK) {
    log_e("esp_a2d_sink_connect:%d", err);
  }
  return err == ESP_OK;
}

//Disconnect from peer
bool BTdisconnect(esp_bd_addr_t peer) {
  esp_err_t status = esp_a2d_sink_disconnect(peer);
  if (status == ESP_FAIL) {
    log_e("esp_a2d_sink_disconnect:%d", status);
  }
  return status == ESP_OK;
}

//--------------------------------------------------------
// BT Callbacks

//BT Generic Access Profile
void bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
  switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT: {
      btNotify(BT_AUTH_EVENT, param->auth_cmpl.stat, (const char*)param->auth_cmpl.device_name);
      break;
    }
    case ESP_BT_GAP_PIN_REQ_EVT: 
#if (CONFIG_BT_SSP_ENABLED == true)
    case ESP_BT_GAP_KEY_NOTIF_EVT:
#endif
    {
      log_i("ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
      if (esp_bt_gap_ssp_passkey_reply(param->cfm_req.bda, true, param->key_notif.passkey) != ESP_OK)
        log_e("esp_bt_gap_ssp_passkey_reply");
      } break;
#if (CONFIG_BT_SSP_ENABLED == true)
    case ESP_BT_GAP_CFM_REQ_EVT:
      btAutoTimeout = 0;
      btNotify(BT_PASSKEY_EVENT, param->cfm_req.num_val, "");
      if (esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true) != ESP_OK) {
        log_e("esp_bt_gap_ssp_passkey_reply");
      }
      break;
    case ESP_BT_GAP_KEY_REQ_EVT:
      btAutoTimeout = 0;
      btNotify(BT_PASSREQ_EVENT, param->cfm_req.num_val, "");
      break;
#endif
    case ESP_BT_GAP_READ_RSSI_DELTA_EVT: {
      if (param->read_rssi_delta.stat == ESP_BT_STATUS_SUCCESS) {
        lastRSSI = param->read_rssi_delta.rssi_delta;
        btNotify(BT_RSSI_EVENT, lastRSSI + 128, "");
      }
      break;
    }
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
    case ESP_BT_GAP_READ_REMOTE_NAME_EVT: {
      if (param->read_rmt_name.stat == ESP_BT_STATUS_SUCCESS) {
        log_i("ESP_BT_GAP_READ_REMOTE_NAME_EVT remote name:%s", param->read_rmt_name.rmt_name);
        memcpy(remoteName, param->read_rmt_name.rmt_name, ESP_BT_GAP_MAX_BDNAME_LEN);
        btNotify(BT_RNAME_EVENT, 0, remoteName);
      }
    } break;
#endif
    case ESP_BT_GAP_CONFIG_EIR_DATA_EVT:
      break;
    case ESP_BT_GAP_MODE_CHG_EVT:
      break;
    default: {
      log_i("%s unhandled event %d\r\n", __func__, event);
      break;
    }
  }
}

//New track notification ack
static void bt_av_new_track(void)
{
    // request metadata
    uint8_t attr_mask = ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM | ESP_AVRC_MD_ATTR_PLAYING_TIME;
    esp_avrc_ct_send_metadata_cmd(APP_RC_CT_TL_GET_META_DATA, attr_mask);

    // register notification if peer support the event_id
    if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                           ESP_AVRC_RN_TRACK_CHANGE)) {
        esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_TRACK_CHANGE, ESP_AVRC_RN_TRACK_CHANGE, 0);
    }
}

//Playback changed notification ack
static void bt_av_playback_changed(void)
{
    if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                           ESP_AVRC_RN_PLAY_STATUS_CHANGE)) {
        esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_PLAYBACK_CHANGE, ESP_AVRC_RN_PLAY_STATUS_CHANGE, 0);
    }
}

//Playback position notify ack
static void bt_av_play_pos_changed(void)
{
    if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                           ESP_AVRC_RN_PLAY_POS_CHANGED)) {
        esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_PLAY_POS_CHANGE, ESP_AVRC_RN_PLAY_POS_CHANGED, 1);
    }
}

//BT Audio Video Remote Control Profile
void bt_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param) {
  uint8_t* attr_text;
  switch (event) {
    case ESP_AVRC_CT_METADATA_RSP_EVT: 
      attr_text = (uint8_t *)malloc (param->meta_rsp.attr_length + 1);
      memcpy(attr_text, param->meta_rsp.attr_text, param->meta_rsp.attr_length);
      attr_text[param->meta_rsp.attr_length] = 0;
      btNotify(BT_META_EVENT, param->meta_rsp.attr_id, (char*)attr_text);
      break;
    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
      if (param->conn_stat.connected) {
        esp_avrc_ct_send_get_rn_capabilities_cmd(APP_RC_CT_TL_GET_CAPS);
      } else {
        s_avrc_peer_rn_cap.bits = 0;
      }
      break;
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
      log_i("AVRC passthrough rsp: key_code 0x%x, key_state %d",
               param->psth_rsp.key_code, param->psth_rsp.key_state);
      break;
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT: {
      btRemoteFeaturesFlag = param->rmt_feats.tg_feat_flag;
      if ((param->rmt_feats.tg_feat_flag & ESP_AVRC_FEAT_FLAG_TG_COVER_ART)) {
        log_i("> BT: Peer support Cover Art feature");
        // start the cover art connection
      }      
      break;
    }
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
        switch (param->change_ntf.event_id) {
        case ESP_AVRC_RN_TRACK_CHANGE:
          bt_av_new_track();
          break;
        case ESP_AVRC_RN_PLAY_STATUS_CHANGE:
          btNotify(BT_STATE_EVENT, param->change_ntf.event_parameter.playback, "");
          bt_av_playback_changed();
          break;
        case ESP_AVRC_RN_PLAY_POS_CHANGED:
          btNotify(BT_POS_EVENT, param->change_ntf.event_parameter.play_pos, "");
          bt_av_play_pos_changed();
          break;
        default:
          log_i("AVRC unhandled evt %d", param->change_ntf.event_id);
          break;
      }
      break;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
    case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT: {
      s_avrc_peer_rn_cap.bits = param->get_rn_caps_rsp.evt_set.bits;
      bt_av_new_track();
      bt_av_playback_changed();
      bt_av_play_pos_changed();
      break;
    }
#endif

    default:
      log_i("%s unhandled event %d", __func__, event);
      break;
  }
}

//BT Audio Video Remote Control Target Profile
void bt_rc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param) {
  switch (event) {
  case ESP_AVRC_TG_CONNECTION_STATE_EVT: {
      break;
  }
  case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT: {
      log_i("AVRC passthrough cmd: key_code 0x%x, key_state %d", param->psth_cmd.key_code, param->psth_cmd.key_state);
      break;
  }
  case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT: {
      btVolume = param->set_abs_vol.volume;
      btNotify(BT_VOL_EVENT, btVolume, "");
      break;
  }
  case ESP_AVRC_TG_REGISTER_NOTIFICATION_EVT: {
      if (param->reg_ntf.event_id == ESP_AVRC_RN_VOLUME_CHANGE) {
          btVolNotify = true;
          esp_avrc_rn_param_t rn_param;
          rn_param.volume = btVolume;
          esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE, ESP_AVRC_RN_RSP_INTERIM, &rn_param);
      }
      break;
  }
  case ESP_AVRC_TG_REMOTE_FEATURES_EVT: {
      log_i("AVRC remote features %x, CT features %x\r\n", param->rmt_feats.feat_mask, param->rmt_feats.ct_feat_flag);
      break;
  }
  default:
      log_i("%s unhandled event %d\r\n", __func__, event);
      break;
  }

}

//BT Advanced Audio Distribution Profile Event
void bt_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
  switch (event) {
  case ESP_A2D_CONNECTION_STATE_EVT: {
    if (btAutoConnect && param->conn_stat.state != ESP_A2D_CONNECTION_STATE_CONNECTING){
      btAutoTimeout = 0;
      btAutoRetry = 0;
    }
    if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED){
      memcpy(peerAddress, param->conn_stat.remote_bda, ESP_BD_ADDR_LEN);
      writePeerAddress(peerAddress);
      esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
      // ask for the remote name
      esp_bt_gap_read_remote_name(param->conn_stat.remote_bda);
#endif
      // Get RSSI
      if (rssiActive) esp_bt_gap_read_rssi_delta(param->conn_stat.remote_bda);
    }
    else if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
      esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
      if (param->conn_stat.disc_rsn != ESP_A2D_DISC_RSN_NORMAL) {
        btAutoTimeout = millis() + BT_AUTOTIMER;
      }    
    }
    btNotify(BT_CONN_EVENT, param->conn_stat.state, "");
    break;
  }
  case ESP_A2D_AUDIO_STATE_EVT: {
    if (ESP_A2D_AUDIO_STATE_STARTED == param->audio_stat.state) {
      btConnected = true;
    } else if (ESP_A2D_AUDIO_STATE_STOPPED == param->audio_stat.state) {
      btChannels = 0;
      btConnected = false;
    }
    break;
  }
  case ESP_A2D_AUDIO_CFG_EVT: {
    if (param->audio_cfg.mcc.type == ESP_A2D_MCT_SBC) {
      int sample_rate = 16000;
      char oct0 = param->audio_cfg.mcc.cie.sbc[0];
      if (oct0 & (0x01 << 6)) sample_rate = 32000;
      else if (oct0 & (0x01 << 5)) sample_rate = 44100;
      else if (oct0 & (0x01 << 4)) sample_rate = 48000;
      btNotify(BT_SR_EVENT, sample_rate, "");
      uint8_t oct1 = param->audio_cfg.mcc.cie.sbc[1];
      if (oct1 & 0x08) btChannels = 1;  // mono
      else btChannels = 2;  // any other mode
      btNotify(BT_CH_EVENT, btChannels, "");
    }
    break;
  }
  case ESP_A2D_PROF_STATE_EVT:
    break;
  default:
    log_i("%s unhandled event %d\r\n", __func__, event);
    break;
  }
}

//BT Advanced Audio Distribution Profile Data
void bt_data_cb(const uint8_t *data, uint32_t len){
  if (!btChannels) return;
  // number of 16 bit sample pairs l/r or individual mono samples
  int n = len / (btChannels * 2);
  // point to a 16bit sample 
  int16_t* data16=(int16_t*)data;
   
  for(int i=0;i<n;i++){
    // put the current sample in fy
    
    int16_t sample[2];
    if (btChannels == 1) {
      sample[0] = sample[1] = (*data16++);
    } else {
      sample[0] = (*data16++);
      sample[1] = (*data16++);
    }
    while (1) { if(playSample(sample)) break; }
    
  }
}


#endif