#include "decls.h"

#ifdef BLUETOOTH

#include "BluetoothA2DPSink.h"
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
    case ESP_AVRC_PLAYBACK_STOPPED:                /*!< stopped */
      Serial.println(">>> BT Stopped.");
      break;
    case ESP_AVRC_PLAYBACK_PLAYING:                /*!< playing */
      Serial.println(">>> BT Playing.");
      break;    
    case ESP_AVRC_PLAYBACK_PAUSED:                 /*!< paused */
      Serial.println(">>> BT Paused.");
      break;
    case ESP_AVRC_PLAYBACK_FWD_SEEK:               /*!< forward seek */
      Serial.println(">>> BT Forward seek.");
      break;
    case ESP_AVRC_PLAYBACK_REV_SEEK:               /*!< reverse seek */
      Serial.println(">>> BT Backward seek.");
      break;
    case ESP_AVRC_PLAYBACK_ERROR:                  /*!< error */
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

#endif