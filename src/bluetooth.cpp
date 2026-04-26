#include "decls.h"

#ifdef BLUETOOTH

// bluetooth, config, discover and audio
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

#define BT_GAP_EVENT 0
#define BT_CONN_EVENT 1
#define BT_VOL_EVENT 2
#define BT_META_EVENT 3
#define BT_STATE_EVENT 4
#define BT_SR_EVENT 5

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
    serial.printf(">: %d: %s\r\n", val, txt);
  }
  else if (source == BT_STATE_EVENT) {
    if (val == ESP_AVRC_PLAYBACK_PLAYING) info(TEXT, 0, LV_SYMBOL_PLAY " Playing");
    else if (val == ESP_AVRC_PLAYBACK_STOPPED) info(TEXT, 0, LV_SYMBOL_STOP " Stopped");
    else if (val == ESP_AVRC_PLAYBACK_PAUSED) info(TEXT, 0, LV_SYMBOL_PAUSE " Paused");
    else if (val == ESP_AVRC_PLAYBACK_FWD_SEEK) info(TEXT, 0, LV_SYMBOL_LEFT " Forward");
    else if (val == ESP_AVRC_PLAYBACK_REV_SEEK) info(TEXT, 0, LV_SYMBOL_RIGHT " Reverse");
  }
  else if (source == BT_SR_EVENT) {
    setSampleRate(val);
    serial.printf("> BT Set Sample Rate: %d\r\n", val);
  }
}

//-------------------------------------------------------
// Threaded below - called from audio thread

void bt_data_cb(const uint8_t *data, uint32_t len);
void bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
void bt_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
void bt_rc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);
void bt_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);

bool btVolNotify = false;
uint8_t btVolume = 0;

// AVRCP used transaction label
#define APP_RC_CT_TL_GET_CAPS            (0)
#define APP_RC_CT_TL_GET_META_DATA       (1)
#define APP_RC_CT_TL_RN_TRACK_CHANGE     (2)
#define APP_RC_CT_TL_RN_PLAYBACK_CHANGE  (3)
#define APP_RC_CT_TL_RN_PLAY_POS_CHANGE  (4)

static esp_avrc_rn_evt_cap_mask_t s_avrc_peer_rn_cap;

//Start the Bluetooth stack - called from the audio thread
void startBT() {
  // set up bluetooth classic via bluedroid
  //esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
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

void BTvolchange(uint32_t vol) {
  btVolume = vol * 127.0f / 21.0f;
  if (btVolNotify) {
    esp_avrc_rn_param_t rn_param;
    rn_param.volume = btVolume;
    esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE, ESP_AVRC_RN_RSP_CHANGED, &rn_param);
    btVolNotify = false;
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
          bt_av_play_pos_changed();
          break;

        default:
          Serial.printf("AVRC unhandled evt %d\r\n", param->change_ntf.event_id);
          break;
      }
      break;
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT: {
      //Serial.printf("AVRC remote features %x\r\n", param->rmt_feats.feat_mask);
      break;
    }

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
/*    if (ESP_A2D_AUDIO_STATE_STARTED == param->audio_stat.state) {
      btNotify(BT_STATE_EVENT, BT_STATE_PLAY, "");
    } else if (ESP_A2D_AUDIO_STATE_STOPPED == param->audio_stat.state) {
      btNotify(BT_STATE_EVENT, BT_STATE_STOP, "");
    }*/
    break;
  }
  case ESP_A2D_AUDIO_CFG_EVT: {
    serial.printf("> A2DP audio stream config, codec type %d\r\n", param->audio_cfg.mcc.type);
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



#if defined(BTA2DP)

#include "BluetoothA2DPSink.h"
//#include "AudioTools.h"

BluetoothA2DPSink a2dp_sink;

// Write data to I2S
void read_data_stream(const uint8_t *data, uint32_t length) {
    // process all data
    int16_t *values = (int16_t*) data;
    for (int j=0; j<length/2; j+=2){
      // print the 2 channel values
      Serial.print(values[j]);
      Serial.print(",");
      Serial.println(values[j+1]);
    }
/*
    // number of 16 bit sample pairs l/r
  int n = length/4;
  // point to a 16bit sample 
  int16_t* data16=(int16_t*)data;
   
  for(int i=0;i<n;i++){
    // put the current sample in fy
    
    int16_t sample[2];

    sample[0] = (*data16++); // half Vin so we can boost up to 6dB in filters
    sample[1] = (*data16++);
    playSample(sample);    
  }*/
}

void btConnState(bool state) {
  if (state) serial.println(">>> BT Connected.");
  else serial.println(">>> BT Disconnected.");
}

void btPlayStatus(esp_avrc_playback_stat_t status) {
  switch(status) {
    case ESP_AVRC_PLAYBACK_STOPPED:                //!< stopped 
      serial.println(">>> BT Stopped.");
      break;
    case ESP_AVRC_PLAYBACK_PLAYING:                //!< playing 
      serial.println(">>> BT Playing.");
      break;    
    case ESP_AVRC_PLAYBACK_PAUSED:                 //!< paused 
      serial.println(">>> BT Paused.");
      break;
    case ESP_AVRC_PLAYBACK_FWD_SEEK:               //!< forward seek 
      serial.println(">>> BT Forward seek.");
      break;
    case ESP_AVRC_PLAYBACK_REV_SEEK:               //!< reverse seek 
      serial.println(">>> BT Backward seek.");
      break;
    case ESP_AVRC_PLAYBACK_ERROR:                  //!< error 
      serial.println(">>> BT Error!");
      break;
  }
}

void btVolChange(int vol) {
  serial.printf(">>> BT Volume change: %d\n", vol);
}


void btMetadata(uint8_t id, const uint8_t *text) {
  serial.printf(">>> BT metadata rsp: attribute id 0x%x, %s\n", id, text);
}

void btSampleRate(uint16_t rate) {
  setSampleRate(rate);
}

void initBT() {
  //AudioLogger::instance().begin(Serial, AudioLogger::Warning);

  // register callback
  a2dp_sink.set_output(serial);
  a2dp_sink.set_stream_reader(read_data_stream, false);
  a2dp_sink.set_avrc_connection_state_callback(btConnState);
  a2dp_sink.set_avrc_rn_playstatus_callback(btPlayStatus);
  a2dp_sink.set_avrc_rn_volumechange(btVolChange);
  a2dp_sink.set_avrc_metadata_callback(btMetadata);
  a2dp_sink.set_sample_rate_callback(btSampleRate);

  // Start Bluetooth Audio Receiver
  a2dp_sink.set_auto_reconnect(false);
  a2dp_sink.start("a2dp-i2s");

  setSampleRate(44100);
  setBitsPerSample(16);
  setChannels(2);
}

void startBT() {
  btStarting = true;
  serial.println("> Starting Bluetooth.");
  //if (isWebradioAllocated()) {
  //  webRadioDelete();
  //}
  //m_vol = settings->vsVolume;
}

void handleBT() {
  //Waiting for webradio to stop
  if(btStarting) {//} && !isWebradioAllocated()) {
    btStarting = false;
    initBT();
    serial.println("> Bluetooth Started.");
  }
}

void stopBT() {
  a2dp_sink.end();
}
#endif

#endif