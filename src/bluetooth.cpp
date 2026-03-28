#include "decls.h"

#ifdef BLUETOOTH

// bluetooth, config, discover and audio
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"

bool btStarting = false;
bool btStarted = false;

void initBT() {
  // i2s configuration
  static const i2s_config_t i2s_config = {
    .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = static_cast<i2s_comm_format_t>(I2S_COMM_FORMAT_I2S|I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // default interrupt priority
    .dma_buf_count = 8,
    .dma_buf_len = 1000,
    .use_apll = false,
    .tx_desc_auto_clear = true
  };

  // i2s pinout
  static const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,//26
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT, //
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  // now configure i2s with constructed pinout and config
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_set_clk(I2S_NUM_0, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
  i2s_set_sample_rates(I2S_NUM_0, 44100);

  // set up bluetooth classic via bluedroid
  //esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
  btStart();
  esp_bluedroid_init();
  esp_bluedroid_enable();

 
  // set up device name
  const char *dev_name = "ESP_SPEAKER";
  esp_bt_dev_set_device_name(dev_name);

  // initialize A2DP sink and set the data callback(A2DP is bluetooth audio)
  esp_a2d_sink_register_data_callback(bt_data_cb);
  esp_a2d_sink_init();
  
  // set discoverable and connectable mode, wait to be connected
  esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE); // for newer ESP-IDF
  btStarted = true;
}


void startBT() {
  bool btStarting = true;
  if (isWebradioAllocated()) {
    webradioStop();
  }
}

void handleBT() {
  //Waiting for webradio to stop
  if(btStarting && !webRadioAllocated()) {
    btStarting = false;
    initBT();
  }
}

void stopBT() {
  if (btStarted) {
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    i2s_driver_uninstall(I2S_NUM_0);
    btStarted = false;
  }
}

// the callback(processes bluetooth data).
// this is the most important function. 
void bt_data_cb(const uint8_t *data, uint32_t len){
   // number of 16 bit samples
   int n = len/2;
   
   // point to a 16bit sample 
   int16_t* data16=(int16_t*)data;
   
   // create a variable (potentially processed) that we'll pass via I2S 
   int16_t fy; 
   
   // Records number of bytes written via I2S
   size_t i2s_bytes_write = 0;

   for(int i=0;i<n;i++){
    // put the current sample in fy
    fy=*data16;
    
    //making this value larger will decrease the volume(Very simple DSP!). 
    fy/=1;
    
    // write data to I2S buffer 
    i2s_write(I2S_NUM_0, &fy, 2, &i2s_bytes_write,  10 );
    
    //move to next memory address housing 16 bit data 
    data16++;
   }
   bool cont;
   audio_process_extern((int16_t*) data, len, &cont);
}



/*#include "BluetoothA2DPSink.h"
#include "AudioTools.h"

BluetoothA2DPSink a2dp_sink;
I2SStream i2s;

// Write data to I2S
void read_data_stream(const uint8_t *data, uint32_t length) {
  i2s.write(data, length);
}

void btConnState(bool state) {
  if (state) Serial.println(">>> BT Connected.");
  else Serial.println(">>> BT Disconnected.");
}

void btPlayStatus(esp_avrc_playback_stat_t status) {
  switch(status) {
    case ESP_AVRC_PLAYBACK_STOPPED:                //!< stopped 
      Serial.println(">>> BT Stopped.");
      break;
    case ESP_AVRC_PLAYBACK_PLAYING:                //!< playing 
      Serial.println(">>> BT Playing.");
      break;    
    case ESP_AVRC_PLAYBACK_PAUSED:                 //!< paused 
      Serial.println(">>> BT Paused.");
      break;
    case ESP_AVRC_PLAYBACK_FWD_SEEK:               //!< forward seek 
      Serial.println(">>> BT Forward seek.");
      break;
    case ESP_AVRC_PLAYBACK_REV_SEEK:               //!< reverse seek 
      Serial.println(">>> BT Backward seek.");
      break;
    case ESP_AVRC_PLAYBACK_ERROR:                  //!< error 
      Serial.println(">>> BT Error!");
      break;
  }
}

void btVolChange(int vol) {
  Serial.printf(">>> BT Volume change: %d\n", vol);
}


void btMetadata(uint8_t id, const uint8_t *text) {
  Serial.printf(">>> BT metadata rsp: attribute id 0x%x, %s\n", id, text);
}


void setupBluetooth() {
  AudioLogger::instance().begin(Serial, AudioLogger::Warning);

  // register callback
  a2dp_sink.set_stream_reader(read_data_stream, false);
  a2dp_sink.set_avrc_connection_state_callback(btConnState);
  a2dp_sink.set_avrc_rn_playstatus_callback(btPlayStatus);
  a2dp_sink.set_avrc_rn_volumechange(btVolChange);
  a2dp_sink.set_avrc_metadata_callback(btMetadata);


  // Start Bluetooth Audio Receiver
  a2dp_sink.set_auto_reconnect(false);
  a2dp_sink.start("a2dp-i2s");

  // setup output
  auto cfg = i2s.defaultConfig();
  cfg.pin_data = I2S_DOUT;
  cfg.pin_bck = I2S_BCLK;
  cfg.pin_ws = I2S_LRC;
  cfg.sample_rate = a2dp_sink.sample_rate();
  cfg.channels = 2;
  cfg.bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.buffer_count         = 8;      // max buffers
  cfg.buffer_size          = 1024;   // max value
  cfg.use_apll             = false; // must be disabled in V2.0.1-RC1
  cfg.auto_clear           = true;   // new in V1.0.1
  cfg.fixed_mclk           = I2S_PIN_NO_CHANGE;
  i2s.begin(cfg);
}
*/

#endif