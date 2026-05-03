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

void updateStat() {
  const char* symbol = "?";
  if (currentPlayStatus == ESP_AVRC_PLAYBACK_PLAYING) symbol = LV_SYMBOL_PLAY;
  else if (currentPlayStatus == ESP_AVRC_PLAYBACK_STOPPED) symbol = LV_SYMBOL_STOP;
  else if (currentPlayStatus == ESP_AVRC_PLAYBACK_PAUSED) symbol = LV_SYMBOL_PAUSE;
  else if (currentPlayStatus == ESP_AVRC_PLAYBACK_FWD_SEEK) symbol = LV_SYMBOL_RIGHT;
  else if (currentPlayStatus == ESP_AVRC_PLAYBACK_REV_SEEK) symbol = LV_SYMBOL_LEFT;
  printBufStat(symbol, "SBC", currentSampleRate * 16);
}

//Bluetooth events
#define BT_GAP_EVENT 0
#define BT_CONN_EVENT 1
#define BT_VOL_EVENT 2
#define BT_META_EVENT 3
#define BT_STATE_EVENT 4
#define BT_SR_EVENT 5
#define BT_POS_EVENT 6

//Called from audio message handler to deal with bluetooth related messages
void bluetoothMessage(uint32_t source, uint32_t val, const char* txt) {
  if (source == BT_GAP_EVENT) {
    serial.println(txt);
  }
  else if (source == BT_CONN_EVENT) {
    if (val) info(NAME, 0, LV_SYMBOL_BLUETOOTH " Connected");
    else {
      info(NAME, 0, LV_SYMBOL_BLUETOOTH " Disconnected");
      info(NOW, 0, "");
    }
  }
  else if (source == BT_VOL_EVENT) {
    //serial.printf("> BT Set Volume: %d\r\n", val);
    settings->dabVolume = val * 21.0f / 127.0f; 
    setVolume(settings->dabVolume);
    if (dabVolSlider) {
      lv_slider_set_value(dabVolSlider, settings->dabVolume, LV_ANIM_OFF);
    }
  }
  else if (source == BT_META_EVENT) {
    if (val == ESP_AVRC_MD_ATTR_TITLE) audio_showstreamtitle(txt);
    else if (val == ESP_AVRC_MD_ATTR_ARTIST) audio_showstation(txt);
    //serial.printf(">: %d: %s\r\n", val, txt);
  }
  else if (source == BT_STATE_EVENT) {
    currentPlayStatus = val;
    if (val == ESP_AVRC_PLAYBACK_PLAYING) info(TEXT, 0, LV_SYMBOL_PLAY " Playing");
    else if (val == ESP_AVRC_PLAYBACK_STOPPED) info(TEXT, 0, LV_SYMBOL_STOP " Stopped");
    else if (val == ESP_AVRC_PLAYBACK_PAUSED) info(TEXT, 0, LV_SYMBOL_PAUSE " Paused");
    else if (val == ESP_AVRC_PLAYBACK_FWD_SEEK) info(TEXT, 0, LV_SYMBOL_RIGHT " Forward");
    else if (val == ESP_AVRC_PLAYBACK_REV_SEEK) info(TEXT, 0, LV_SYMBOL_LEFT " Reverse");
    updateStat();
    updatePlayButton();
  }
  else if (source == BT_SR_EVENT) {
    currentSampleRate = val;
    setSampleRate(currentSampleRate);
    updateStat();
  }
  else if (source == BT_POS_EVENT) {
    //Not using this right now
  }
}

//-------------------------------------------------------
// Threaded below - called from audio thread

void bt_data_cb(const uint8_t *data, uint32_t len);
void bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
void bt_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
void bt_rc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);
void bt_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);

bool btConnected = false;
bool btVolNotify = false;
bool btPassNotify = false;
uint8_t btVolume = 0;
uint16_t btRemoteFeaturesFlag = 0;

// AVRCP used transaction label
#define APP_RC_CT_TL_GET_CAPS            (0)
#define APP_RC_CT_TL_GET_META_DATA       (1)
#define APP_RC_CT_TL_RN_TRACK_CHANGE     (2)
#define APP_RC_CT_TL_RN_PLAYBACK_CHANGE  (3)
#define APP_RC_CT_TL_RN_PLAY_POS_CHANGE  (4)
#define ESP_AVRC_FEAT_FLAG_TG_COVER_ART  (0x0100)                     /*!< TG support Cover Art */


static esp_avrc_rn_evt_cap_mask_t s_avrc_peer_rn_cap;


//Start the Bluetooth stack - called from the audio thread
void startBT() {
  // set up bluetooth classic via bluedroid
  esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
  setSampleRate(44100);
  setBitsPerSample(16);
  setChannels(2);
  btStart();
  esp_bluedroid_init();
  esp_bluedroid_enable();

  // set up device name
  const char *dev_name = "ESP_SPEAKER";
  esp_bt_dev_set_device_name(dev_name);

  // initialize AVRCP controller
  esp_avrc_ct_init();
  esp_avrc_ct_register_callback(bt_rc_ct_cb);
  esp_avrc_tg_init();
  esp_avrc_tg_register_callback(bt_rc_tg_cb);

  esp_avrc_rn_evt_cap_mask_t evt_set = {0};
  esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE);
  assert(esp_avrc_tg_set_rn_evt_cap(&evt_set) == ESP_OK);

  // initialize A2DP sink and set the data callback(A2DP is bluetooth audio)
  esp_a2d_register_callback(bt_a2d_cb);
  esp_a2d_sink_register_data_callback(bt_data_cb);
  esp_a2d_sink_init();
  
  // set discoverable and connectable mode, wait to be connected
  esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE); // for newer ESP-IDF
  esp_bt_gap_register_callback(bt_gap_cb);

#if (CONFIG_BT_SSP_ENABLED == true)
  /* Set default parameters for Secure Simple Pairing */
  esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
  esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
  esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
#endif

  /*
   * Set default parameters for Legacy Pairing
   * Use fixed pin code
  */
  esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
  esp_bt_pin_code_t pin_code;
	pin_code[0] = '1';
  pin_code[1] = '2';
  pin_code[2] = '3';
  pin_code[3] = '4';
  esp_bt_gap_set_pin(pin_type, 4, pin_code); 
}

void handleBT() {
}

//Shut down BT controller - called from audio thread
void stopBT() {
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

//--------------------------------------------------------
// Callbacks

//BT Generic Access Profile
void bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
  char buff[312];
  switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT: {
      if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
        sprintf(buff, "GAP Authentication success: %s\r\n", param->auth_cmpl.device_name);
        btNotify(BT_GAP_EVENT, event, buff);
      } else {
        sprintf(buff, "GAP Authentication failed, status:%d\r\n", param->auth_cmpl.stat);
        btNotify(BT_GAP_EVENT, event, buff);
      }
      break;
    }
#if (CONFIG_BT_SSP_ENABLED == true)
    case ESP_BT_GAP_CFM_REQ_EVT:
      Serial.printf("ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d\r\n", param->cfm_req.num_val);
      esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
      break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
      Serial.printf("ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d\r\n", param->key_notif.passkey);
      break;
    case ESP_BT_GAP_KEY_REQ_EVT:
      Serial.printf("ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!\r\n");
      break;
#endif
    case ESP_BT_GAP_MODE_CHG_EVT:
      break;
    default: {
      Serial.printf("%s unhandled event %d\r\n", __func__, event);
      break;
    }
  }
}

//New track notification ack
static void bt_av_new_track(void)
{
    // request metadata
    uint8_t attr_mask = ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM | ESP_AVRC_MD_ATTR_GENRE;
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
        esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_PLAY_POS_CHANGE, ESP_AVRC_RN_PLAY_POS_CHANGED, 10);
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
      Serial.printf("AVRC passthrough rsp: key_code 0x%x, key_state %d\r\n",
               param->psth_rsp.key_code, param->psth_rsp.key_state);
      break;
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT: {
      //Serial.printf("AVRC remote features %x\r\n", param->rmt_feats.feat_mask);
      btRemoteFeaturesFlag = param->rmt_feats.tg_feat_flag;
      if ((param->rmt_feats.tg_feat_flag & ESP_AVRC_FEAT_FLAG_TG_COVER_ART)) {
        Serial.println("> BT: Peer support Cover Art feature");
        // start the cover art connection
      }      
      break;
    }
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
        switch (param->change_ntf.event_id) {
        case ESP_AVRC_RN_TRACK_CHANGE:
          Serial.printf("ESP_AVRC_RN_TRACK_CHANGE\r\n");
          bt_av_new_track();
          break;
        case ESP_AVRC_RN_PLAY_STATUS_CHANGE:
          btNotify(BT_STATE_EVENT, param->change_ntf.event_parameter.playback, "");
          bt_av_playback_changed();
          break;
        case ESP_AVRC_RN_PLAY_POS_CHANGED:
          //Serial.printf("Play position changed: %d-ms\r\n",
          //        param->change_ntf.event_parameter.play_pos);
          btNotify(BT_POS_EVENT, param->change_ntf.event_parameter.play_pos, "");
          bt_av_play_pos_changed();
          break;

        default:
          Serial.printf("AVRC unhandled evt %d\r\n", param->change_ntf.event_id);
          break;
      }
      break;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)

    case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT: {
      //Serial.printf("AVRC remote rn_cap: count %d, bitmask 0x%x\r\n",
      //         param->get_rn_caps_rsp.cap_count, param->get_rn_caps_rsp.evt_set.bits);
      s_avrc_peer_rn_cap.bits = param->get_rn_caps_rsp.evt_set.bits;
      bt_av_new_track();
      bt_av_playback_changed();
      bt_av_play_pos_changed();
      break;
    }

#endif

    default:
      Serial.printf("%s unhandled event %d", __func__, event);
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
      Serial.printf("AVRC passthrough cmd: key_code 0x%x, key_state %d\r\n", param->psth_cmd.key_code, param->psth_cmd.key_state);
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
      Serial.printf("AVRC remote features %x, CT features %x\r\n", param->rmt_feats.feat_mask, param->rmt_feats.ct_feat_flag);
      break;
  }
  default:
      Serial.printf("%s unhandled event %d\r\n", __func__, event);
      break;
  }

}

//BT Advanced Audio Distribution Profile Event
void bt_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
  switch (event) {
  case ESP_A2D_CONNECTION_STATE_EVT: {
    if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
      esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    } else if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED){
      esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
    }
    btNotify(BT_CONN_EVENT, param->conn_stat.state, "");
    break;
  }
  case ESP_A2D_AUDIO_STATE_EVT: {
    if (ESP_A2D_AUDIO_STATE_STARTED == param->audio_stat.state) {
      btConnected = true;
    } else if (ESP_A2D_AUDIO_STATE_STOPPED == param->audio_stat.state) {
      btConnected = false;
    }
    break;
  }
  case ESP_A2D_AUDIO_CFG_EVT: {
    //Serial.printf("> A2DP audio stream config, codec type %d\r\n", param->audio_cfg.mcc.type);
    // for now only SBC stream is supported
    if (param->audio_cfg.mcc.type == ESP_A2D_MCT_SBC) {
      int sample_rate = 16000;
      char oct0 = param->audio_cfg.mcc.cie.sbc[0];
      if (oct0 & (0x01 << 6)) sample_rate = 32000;
      else if (oct0 & (0x01 << 5)) sample_rate = 44100;
      else if (oct0 & (0x01 << 4)) sample_rate = 48000;
      btNotify(BT_SR_EVENT, sample_rate, "");
    }
    break;
  }
  case ESP_A2D_PROF_STATE_EVT:
    break;
  default:
    Serial.printf("%s unhandled event %d\r\n", __func__, event);
    break;
  }
}

//BT Advanced Audio Distribution Profile Data
void bt_data_cb(const uint8_t *data, uint32_t len){

  // number of 16 bit sample pairs l/r
  int n = len/4;
  // point to a 16bit sample 
  int16_t* data16=(int16_t*)data;
   
  for(int i=0;i<n;i++){
    // put the current sample in fy
    
    int16_t sample[2];

    sample[0] = (*data16++); // half Vin so we can boost up to 6dB in filters
    sample[1] = (*data16++);
    while (1) { if(playSample(sample)) break; }
    
  }
}
/*
//-------------------------------------------------------------------------------------
// Cover art

// @brief AVRCP cover art service control block structure
typedef struct {
    bool connected;                          // Connection status flag 
    bool getting;                            // Flag indicating if image is being retrieved 
    // Related to the image 
    uint8_t image_hdl_old[7];                // Previous image handle, used to detect image changes 
    uint32_t image_size;                     // Size of the image data in bytes 
    uint8_t *image_data;                     // Pointer to the image data buffer 
    bool image_final;                        // Indicate whether the image reception has been completed 
} avrc_ca_service_cb_t;

static bool avrc_ca_handleCheck(uint8_t *image_handle, int len);
static void avrc_ca_freeImageData(void);

// avrcp cover art service control block
static avrc_ca_service_cb_t avrc_ca_serviceControlBlock;

static void avrc_ca_freeImageData(void)
{
    if (avrc_ca_serviceControlBlock.image_data) {
        free(avrc_ca_serviceControlBlock.image_data);
        avrc_ca_serviceControlBlock.image_data = NULL;
    }
    avrc_ca_serviceControlBlock.image_size = 0;
}

static void avrc_ca_imageReady(void)
{
    Serial.println("> BT JPEG image ready.");

}

void avrc_ca_open(void)
{
    memset(&avrc_ca_serviceControlBlock, 0, sizeof(avrc_ca_service_cb_t));
}

void avrc_ca_close(void)
{
    avrc_ca_freeImageData();
    memset(&avrc_ca_serviceControlBlock, 0, sizeof(avrc_ca_service_cb_t));
}

bool btc_avrc_ct_check_cover_art_support(void)
{
    return (btRemoteFeaturesFlag & ESP_AVRC_FEAT_FLAG_TG_COVER_ART);
}


esp_err_t esp_avrc_ct_cover_art_connect(uint16_t mtu)
{
    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED){
        return ESP_ERR_INVALID_STATE;
    }

    if (!btc_avrc_ct_check_cover_art_support()) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    btc_msg_t msg;
    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_AVRC_CT;
    msg.act = BTC_AVRC_CT_API_COVER_ART_CONNECT_EVT;

    btc_avrc_args_t arg;
    memset(&arg, 0, sizeof(btc_avrc_args_t));
    arg.ca_conn.mtu = mtu;

    // Switch to BTC context 
    bt_status_t stat = btc_transfer_context(&msg, &arg, sizeof(btc_avrc_args_t), NULL, NULL);
    return (stat == BT_STATUS_SUCCESS) ? ESP_OK : ESP_FAIL;
}

esp_err_t esp_avrc_ct_cover_art_disconnect(void)
{
    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!btc_avrc_ct_check_cover_art_support()) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    btc_msg_t msg;
    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_AVRC_CT;
    msg.act = BTC_AVRC_CT_API_COVER_ART_DISCONNECT_EVT;

    // Switch to BTC context
    bt_status_t stat = btc_transfer_context(&msg, NULL, 0, NULL, NULL);
    return (stat == BT_STATUS_SUCCESS) ? ESP_OK : ESP_FAIL;
}

esp_err_t esp_avrc_ct_cover_art_get_image_properties(uint8_t *image_handle)
{
    if ((esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) ||
        (!btc_avrc_ct_connected_p())) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!btc_avrc_ct_check_cover_art_support()) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    if (image_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    btc_msg_t msg;
    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_AVRC_CT;
    msg.act = BTC_AVRC_CT_API_COVER_ART_GET_IMAGE_PROPERTIES_EVT;

    btc_avrc_args_t arg;
    memset(&arg, 0, sizeof(btc_avrc_args_t));
    memcpy(arg.ca_get_img_prop.image_handle, image_handle, ESP_AVRC_CA_IMAGE_HANDLE_LEN);

    // Switch to BTC context
    bt_status_t stat = btc_transfer_context(&msg, &arg, sizeof(btc_avrc_args_t), NULL, NULL);
    return (stat == BT_STATUS_SUCCESS) ? ESP_OK : ESP_FAIL;
}

esp_err_t esp_avrc_ct_cover_art_get_image(uint8_t *image_handle, uint8_t *image_descriptor, uint16_t image_descriptor_len)
{
    if ((esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) ||
        (!btc_avrc_ct_connected_p())) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!btc_avrc_ct_check_cover_art_support()) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    if (image_handle == NULL || image_descriptor == NULL || image_descriptor_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    btc_msg_t msg;
    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_AVRC_CT;
    msg.act = BTC_AVRC_CT_API_COVER_ART_GET_IMAGE_EVT;

    btc_avrc_args_t arg;
    memset(&arg, 0, sizeof(btc_avrc_args_t));

    memcpy(arg.ca_get_img.image_handle, image_handle, ESP_AVRC_CA_IMAGE_HANDLE_LEN);
    arg.ca_get_img.image_descriptor_len = image_descriptor_len;
    arg.ca_get_img.image_descriptor = image_descriptor;

    // Switch to BTC context
    bt_status_t stat = btc_transfer_context(&msg, &arg, sizeof(btc_avrc_args_t), btc_avrc_arg_deep_copy, btc_avrc_arg_deep_free);
    return (stat == BT_STATUS_SUCCESS) ? ESP_OK : ESP_FAIL;
}



void avrc_cover_art_srv_connect(uint16_t mtu)
{
    if (!avrc_ca_serviceControlBlock.connected) {
        serial.println("> BT Start cover art connection...");
        // start the cover art connection
        esp_avrc_ct_cover_art_connect(mtu);
    }
}

void avrc_cover_art_srv_set_image_final(bool final)
{
    s_avrc_cover_art_srv_cb.image_final = final;
    if (s_avrc_cover_art_srv_cb.image_final) {
        ESP_LOGI(RC_CA_SRV_TAG, "Cover Art Client final data event, image size: %lu bytes", s_avrc_cover_art_srv_cb.image_size);

        // decode and display the image 
        avrc_cover_art_srv_decode_image();
        // display the image 
        avrc_cover_art_srv_display_image();
        // set the getting state to false, we can get next image now 
        s_avrc_cover_art_srv_cb.getting = false;
    }
}

void avrc_cover_art_srv_set_connected(bool connected)
{
    s_avrc_cover_art_srv_cb.connected = connected;
}

void avrc_cover_art_srv_ca_req(void)
{
    // request cover art 
    if (s_avrc_cover_art_srv_cb.connected) {
        uint8_t attr_mask = ESP_AVRC_MD_ATTR_COVER_ART;

        esp_avrc_ct_send_metadata_cmd(bt_avrc_common_alloc_tl(), attr_mask);
    }
}

void avrc_cover_art_srv_save_image_data(uint8_t *p_data, uint16_t data_len)
{
    s_avrc_cover_art_srv_cb.image_size += data_len;

    uint8_t *p_buf = (uint8_t *)realloc(s_avrc_cover_art_srv_cb.image_data, s_avrc_cover_art_srv_cb.image_size * sizeof(uint8_t));
    if (!p_buf) {
        ESP_LOGE(RC_CA_SRV_TAG, "%s: The memory allocation of Cover art image data failed", __func__);
        avrc_cover_art_srv_free_image_data();
        return;
    }
    s_avrc_cover_art_srv_cb.image_data = p_buf;
    memcpy(s_avrc_cover_art_srv_cb.image_data + s_avrc_cover_art_srv_cb.image_size - data_len, p_data, data_len);
}

void avrc_cover_art_srv_ct_metadata_update(uint8_t *image_handle, int len)
{
    if (s_avrc_cover_art_srv_cb.connected && !s_avrc_cover_art_srv_cb.getting) {
        // check image handle is valid and different with last one, we don't want to get an image repeatedly 
        if (avrc_cover_art_srv_image_handle_check(image_handle, len)) {
            // free the previous image data 
            avrc_cover_art_srv_free_image_data();
            // get the linked thumbnail 
            esp_avrc_ct_cover_art_get_linked_thumbnail(image_handle);
            s_avrc_cover_art_srv_cb.getting = true;
        }
    }
}
*/

#endif