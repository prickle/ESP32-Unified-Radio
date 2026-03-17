#include "decls.h"
//Audioplayer task on core 0

//Audio library
Audio *audio;
TaskHandle_t radioTaskHandle;

uint8_t wrVolume   = 21;
uint8_t wrChannels = 0;
char* wrurl; //[FTP_NAME_LENGTH];
bool wrMeta;
uint32_t wrResume = 0;
//String buffers
#define METABUF_LEN 128
char* wrTitle;
char* wrStation;
char* wrIcyurl;
//Flags
bool wrIsRunning = false;
//Action triggers
bool newURL = false;
bool newFile = false;
bool newFtp = false;
//Retry timer
unsigned long retryms = 0;

//Webradio commands
enum : uint8_t { WR_START, WR_SETVOLUME, WR_GETVOLUME, WR_CONNECTTOHOST, WR_CONNECTTOFS, WR_STOPSONG, 
                 WR_META, WR_CONNECTING, WR_SETTONE, WR_STATS, WR_TITLE, WR_STATION, WR_ICYURL, WR_DELETE, 
                 WR_VU, WR_EOF, WR_EMBED, WR_RESPONSE, WR_MONO, WR_SRATE, WR_SETWIDE };

//Webradio message
struct audioMessage{
    uint8_t     cmd;
    uint8_t     channels;
    const char* txt;
    uint32_t    value1;
    uint32_t    value2;
    uint32_t    value3;
    uint32_t    pos;
    uint32_t    size;
    uint32_t    ret;
} audioTxMessage, audioRxMessage;

QueueHandle_t audioSetQueue = NULL;
QueueHandle_t audioGetQueue = NULL;

#ifdef FFTMETER
QueueHandle_t fftQueue = NULL;
uint8_t fftBars[64] = { 0 };
unsigned long fft_ms = 0;
#endif

void radioTask( void * pvParameters );

void CreateQueues(){
  if(!audioSetQueue) audioSetQueue = xQueueCreate(10, sizeof(struct audioMessage));
  if(!audioGetQueue) audioGetQueue = xQueueCreate(10, sizeof(struct audioMessage));
#ifdef FFTMETER
  if(!fftQueue) fftQueue = xQueueCreate(2, (FFT_SIZE) * sizeof(uint16_t));
#endif
  if(!audioSetQueue || !audioGetQueue
#ifdef FFTMETER
   || !fftQueue
#endif  
  ) {
    errorContinue(0, "Queues are not initialized");
  }
}

//Called once from setup to get things going

void webradioSetup() {
  //current station info storage
  wrurl = (char*)ps_malloc(FTP_NAME_LENGTH);
  wrTitle = (char*)ps_malloc(METABUF_LEN);
  wrStation = (char*)ps_malloc(METABUF_LEN);
  wrIcyurl = (char*)ps_malloc(METABUF_LEN);
  //be good boys and check  
  if(!wrurl || !wrTitle || !wrStation || !wrIcyurl){
    errorContinue(0, "Out of memory in webradioSetup()");
  }
  wrurl[0] = '\0';
  //Allocate command queues
  CreateQueues();
  //Send some setup messages
  //Set volume
#if defined(MONKEYBOARD) || defined(NXP6686)  
  //Using volume attenuator on DAB+ module, so set DSP vol to max
  setDspVolume(21);   
#else
  //Use DSP volume attenuator
  setDspVolume(settings->dabVolume);
#endif
  //Set tone if available  
#ifdef EQUALIZER
  setVSTone(settings->vsTone);
#endif
  setStereoWide(settings->wide);
  //Create webradio thread
  xTaskCreatePinnedToCore(
             radioTask,           // Task function. 
             "WebRadio",          // name of task. 
             WR_STACK_SIZE,               // Stack size of task 
             NULL,                // parameter of the task 
             16,                   // priority of the task 
             &radioTaskHandle,    // Task handle to keep track of created task 
               0);                  // pin task to core 0 
}

//Command the webradio to delete itself, try to recover memory.
void webRadioDelete() {
  if (!audioSetQueue) return;
  audioTxMessage.cmd = WR_DELETE;
  xQueueSend(audioSetQueue, &audioTxMessage, 0);  
}

//Set volume
void setDspVolume(uint8_t volume) {
  if (!audioSetQueue) return;
  audioTxMessage.cmd = WR_SETVOLUME;
  audioTxMessage.value1 = volume;
  xQueueSend(audioSetQueue, &audioTxMessage, 0);
}

#if !defined(MONKEYBOARD) && !defined(NXP6686)
void setVolume(uint8_t volume) { setDspVolume(volume); }
#endif

//Stereo Wide
void setStereoWide(bool wide) {
  if (!audioSetQueue) return;
  audioTxMessage.cmd = WR_SETWIDE;
  audioTxMessage.value1 = wide;
  xQueueSend(audioSetQueue, &audioTxMessage, 0);
}

//Webradio - start a connection to a webradio URL
// if metadata true, try to stream IcyRadio
// if metadata false, stream plain webfile
void connectToHost(const char * server, bool metadata) {
  if (server[0]) {
    info(NAME, 0, LV_SYMBOL_BROADCAST " Open Website..");
    strncpy(wrurl, server, FTP_NAME_LENGTH-1);
    log_d("URL: %s", wrurl);
    wrurl[FTP_NAME_LENGTH-1] = 0;
    wrMeta = metadata;
    clearBufStat();
    showLoadSpinner();
    retryms = 0;
    newURL = true;
  }
}

//Webradio - play a file from sd or ftp
// if path starts with d:/... reads from SD 
// if path starts with /... reads from FTP 
void connectToFS(char * path, uint32_t resumePos) {
  if (path[0]) {
    strncpy(wrurl, path, FTP_NAME_LENGTH-1);
    wrurl[FTP_NAME_LENGTH-1] = 0;
    int index = getPlaylistIndex();
    if (index) info(NAME, 0, LV_SYMBOL_PLAY " Playlist (%d of %d)..", index, getPlaylistCount());
    else info(NAME, 0, LV_SYMBOL_PLAY " Play File..");
    info(NOW, 0, wrurl);
    info(TEXT, 0, "");
    clearBufStat();
    showLoadSpinner();
    retryms = 0;
    newFile = true;
    wrResume = resumePos;
  }
}

//Stop playback
void webradioStop() {
  if (!audioSetQueue) return;
  clearBufStat();
  retryms = 0;
  wrurl[0] = '\0';
  newURL = false;
  newFile = false;
  audioTxMessage.cmd = WR_STOPSONG;
  xQueueSend(audioSetQueue, &audioTxMessage, 0);
}

//values 0 to 12 (-6..0..+6)
uint16_t formatVSTone(int8_t bass, int8_t mid, int8_t treb) {
  uint16_t value = 0 ;                                  // Value to send to SCI_BASS
  value = bass & 0x0F;                 // Shift next nibble in
  value = ( value << 4 ) | (mid & 0x0F);                 // Shift next nibble in
  value = ( value << 4 ) | (treb & 0x0F);                 // Shift next nibble in
  return value;   
}

//Set tone as formatted above
void setVSTone ( uint16_t rtone ) {                // Set bass/treble (4 nibbles)
  if (!audioSetQueue) return;
  audioTxMessage.cmd = WR_SETTONE;
  audioTxMessage.value1 = rtone;
  xQueueSend(audioSetQueue, &audioTxMessage, 0);
}

void setMonoOutput(bool mono) {
  if (!audioSetQueue) return;
  audioTxMessage.cmd = WR_MONO;
  audioTxMessage.value1 = mono;
  xQueueSend(audioSetQueue, &audioTxMessage, 0);
}

//-------------------------------------------------------------
// Webradio UI

//Add currently playing station to station list
void addNewStation() {
  lv_obj_t * btn = addStationButton(webStationName, webRadioListAction, settings->server, strlen(settings->server)+1);
  listSetSelect(btn);
  stationsChanged();
}


//Handle UI updates from the main loop
void webradioHandle() {
  //static unsigned long fft_ms = 0;
  static bool wrWasRunning = false;
  static const char* response = NULL;
  
  if (!audioGetQueue) return;

  //Webradio URL, delay until connected
  if ((settings->mode == MODE_WEB || settings->mode == MODE_POD || settings->mode == MODE_DLNA) && WiFi.status() == WL_CONNECTED) {
    //Retry connecting if retryms is set
    if (retryms && millis() > retryms) {
      info(NAME, 0, LV_SYMBOL_BROADCAST " Retry Website..");
      showLoadSpinner();
      retryms = 0;
      newURL = true;
    }
    if (newURL) {
      newURL = false;
      info(TEXT, 0, "");
      audioTxMessage.cmd = WR_CONNECTTOHOST;
      audioTxMessage.txt = wrurl;
      audioTxMessage.value1 = wrMeta;
      xQueueSend(audioSetQueue, &audioTxMessage, 0);
      response = NULL;
    }
  }

#ifdef SDPLAYER
  //File player, do immediately if SD, wait until connected if FTP
  if (newFile && (settings->mode == MODE_SD || (settings->mode == MODE_FTP && WiFi.status() == WL_CONNECTED))) {
#else
  if (newFile && (settings->mode == MODE_FTP && WiFi.status() == WL_CONNECTED)) {
#endif
    newFile = false;
    audioTxMessage.cmd = WR_CONNECTTOFS;
    audioTxMessage.txt = wrurl;
    audioTxMessage.value1 = wrResume;
    xQueueSend(audioSetQueue, &audioTxMessage, 0);
  }
  
  //Webradio messages
  if(xQueueReceive(audioGetQueue, &audioRxMessage, 0) == pdPASS){
    //Radio thread started
    if (audioRxMessage.cmd == WR_START) {
      if (!audioRxMessage.ret) serial.println("> Audio - Using Internal DAC.");
    }

    //Statistics
    if (audioRxMessage.cmd == WR_STATS) {
      wrIsRunning = (audioRxMessage.channels >> 7);
      if (wrIsRunning) {
        //Mono detection
        int channels = (audioRxMessage.channels & 0x7f);
        if (wrChannels != channels) {
          wrChannels = channels;
          updateSTMOLabel(channels == 1 ? STMO_MONO : STMO_STEREO);
        }
        //Buffer stats
        int wrBufUsed = audioRxMessage.value1;
        int wrBufFree = audioRxMessage.value2;
        printBufStat(wrIsRunning, audioRxMessage.ret, audioRxMessage.value3);
        setBufMeter(wrBufUsed * 100.0 / (wrBufUsed + wrBufFree));
      }
      //File playback progress bar
      uint8_t pct = 0;
      if (wrIsRunning && audioRxMessage.size != 0) {
        pct = (audioRxMessage.pos * 100.0) / audioRxMessage.size;
        if (pct > 100) pct = 100;
        setTimeBar(pct);
      }
      //EOF detection and "Playing/Stopped" messages
      if (wrIsRunning != wrWasRunning) {
        wrWasRunning = wrIsRunning;
        if (wrIsRunning) {
          info(NAME, 0, LV_SYMBOL_PLAY " Playing");
          if (settings->mode == MODE_DLNA) showDlnaMetadata();
        }
        else {
          //"Stopped" message for file players
#ifdef SDPLAYER
          if ((settings->mode != MODE_SD && settings->mode != MODE_FTP && settings->mode != MODE_DLNA) || !getPlaylistIndex()) {
            if (settings->mode != MODE_SD && !SDFound) info(NAME, 0, LV_SYMBOL_SD_CARD" SD Card not found!");
            else info(NAME, 0, LV_SYMBOL_STOP " Stopped");
#else
          if ((settings->mode != MODE_FTP && settings->mode != MODE_DLNA) || !getPlaylistIndex()) {
            info(NAME, 0, LV_SYMBOL_STOP " Stopped");
#endif
          }
          //If webstreaming ever stops, something is wrong?
          if (settings->mode == MODE_WEB) {
            if (strlen(wrurl) > 0 && !newURL) retryms = millis() + 2000;  //We will try to reconnect in 2 seconds
            else info(NOW, 0, LV_SYMBOL_LEFT " Choose stations from the list to the left.. ");
            //Did the server report anything?
            if (response && strlen(response)) {
              if (atoi(response) != 200)          //Ignore "200 OK" because the problem is elsewhere (DNS?) and this wouldn't make sense
                info(TEXT, 0, LV_SYMBOL_WARNING "Server Responded: %s", response);              
            }
          }
          //Call the end of file functions here
#ifdef SDPLAYER
          if (settings->mode == MODE_SD || settings->mode == MODE_FTP || settings->mode == MODE_DLNA) sdSongFinished();
#else
          if (settings->mode == MODE_FTP || settings->mode == MODE_DLNA) sdSongFinished();
#endif
          if (settings->mode == MODE_POD) podEOF();

        }
      }
    }
    
    //Song title
    else if (audioRxMessage.cmd == WR_TITLE) {
      if (audioRxMessage.txt[0] != 0) {
        char title[128] = LV_SYMBOL_AUDIO " - " ;
        strncat(title, audioRxMessage.txt, 120);
        title[120] = '\0';
        strcat(title, " -");
        info(NAME, 0, title);  //Set the text
      } else info(NAME, 0, LV_SYMBOL_PLAY " Playing");   
    }

    //Station name
    else if (audioRxMessage.cmd == WR_STATION) {
      if (settings->mode == MODE_POD) {
        podShowEpisodeInfo();
      } else {
        //Show the station's reported name if it isn't "Streaming by Securenet Systems Cirrus(R)"
        if (audioRxMessage.txt && strlen(audioRxMessage.txt) > 0 && strcmp(audioRxMessage.txt, "Streaming by Securenet Systems Cirrus(R)") != 0) { 
          // if current name is empty, or the new name is not too short or long..
          if (strlen(webStationName) == 0 ||
              (strlen(audioRxMessage.txt) > 3 && strlen(audioRxMessage.txt) < 35)) {
            //then keep this name
            strncpy(webStationName, audioRxMessage.txt, 34);
            webStationName[34] = '\0';
          }
          //Print the full name
          info(NOW, 0, audioRxMessage.txt);
        } else 
          //Print the saved short name
          info(NOW, 0, webStationName);
        //See if we have this station  
        if (strlen(webStationName) > 0 && !stationInList(settings->server)) {
          char buf[64];
          snprintf(buf, 63, " " LV_SYMBOL_DOWNLOAD "  Keep '%s'?", webStationName);
          popup(buf, addNewStation, true);
        } else closePopup(true);
      }
    }

    //Connecting to new URL messages
    else if (audioRxMessage.cmd == WR_CONNECTING) {
      info(NOW, 0, "Connecting to: %s", audioRxMessage.txt);  //Set the text
    }

    //Station URL
    else if (audioRxMessage.cmd == WR_ICYURL) {
      info(TEXT, 0, audioRxMessage.txt);  //Set the text
    }

    //Command replies
    else if (audioRxMessage.cmd == WR_CONNECTTOHOST) {
      if (audioRxMessage.ret != 1) {
        log_e("Error '%s' connecting to host!\n", audioRxMessage.txt);
        info(NOW, 0, LV_SYMBOL_WARNING " Failed to connect to server!");
        retryms = millis() + 2000;
      }
      hideWebControls();
    }
    else if (audioRxMessage.cmd == WR_CONNECTTOFS) {
      if (audioRxMessage.ret != 1) {
        log_e("Error opening Filesystem!");
        info(NOW, 0, LV_SYMBOL_WARNING " Error opening Filesystem!");
      }
      hideWebControls();
      //Start looking for album art
      findAlbumArt(wrurl);
    }
    else if (audioRxMessage.cmd == WR_RESPONSE) {
      response = audioRxMessage.txt;    //Hold onto the most recent server response code
    }
    else if (audioRxMessage.cmd == WR_GETVOLUME) {
      //not used
    }
    else if (audioRxMessage.cmd == WR_STOPSONG) {
      log_d("Song stopped.");  //Notification from stop command
    }
    else if (audioRxMessage.cmd == WR_EOF) {
      //this only triggers on successful completion of playback
      sdEOF(); 
    }
    else if (audioRxMessage.cmd == WR_META) {
      //File-type metadata found
      metadataSD();
    }
    else if (audioRxMessage.cmd == WR_SRATE) {
      //Sample rate change
#ifdef NXP6686
      NXPSetI2SRate(audioRxMessage.value1);
#endif
    }
    else if (audioRxMessage.cmd == WR_EMBED) {
      //Hardly worth the bother..
      //artFoundEmbedded(audioRxMessage.value1, audioRxMessage.value2);
    }
    else if (audioRxMessage.cmd == WR_DELETE) {
      //Webradio thread has self-destructed, tidy up on this side by deleting the queues
      vQueueDelete(audioGetQueue);
      vQueueDelete(audioSetQueue);
      //vQueueDelete(fftQueue);
      audioGetQueue = audioSetQueue = 0; //fftQueue = 0; 
      return;     
    }
#ifdef VUMETER    
    else if (audioRxMessage.cmd == WR_VU) {
      //VU data coming through
      drawVU(audioRxMessage.value1, audioRxMessage.value2);
    }
#endif    
  }
#ifdef FFTMETER  
  //FFT Meter
  if(lv_tabview_get_tab_act(tabView) == mainWindowIndex) {
    int16_t fftBuf[FFT_SIZE];  
    if(xQueueReceive(fftQueue, fftBuf, 0) == pdPASS){
      execFFT(fftBuf);
      for (int i = 0; i < 64; i++) {
        int32_t f = getFFT(i) >> 10;
        fftBars[i] = f < 256 ? f : 255;
      }
    }
    //FFT meter refresh every 50 ms or so..
    if (millis() > fft_ms) {
      fft_ms = millis() + 50;
      drawFFT(fftMeter, fftBars);
      //reset saved levels
      memset(fftBars, 0, sizeof(fftBars));
    }
  }
#endif  
}


//----------------------------------------------------------------------------
// Webradio task

String serverResponse = "";
String connectingURL = "";

//(re)start a connection from the webradio task
uint32_t StartNewURL(const char* host, bool meta) {
  if (WiFi.status() != WL_CONNECTED) {
    log_e("Webradio: ESP32 WiFi not connected.");
    return 2;
  }
  log_d("Webradio: Changing URL to: %s, vol=%d, metadata = %s\n", host, wrVolume, meta?"on":"off");
  serverResponse = "";
  return audio->connecttohost(host, "", "", meta);
}

//Decode 3-band EQ from uint16_t and apply
void updateVSTone(uint16_t val) {
  int8_t bass = ((val >> 8) & 0x0F) - 7;
  int8_t mid = ((val >> 4) & 0x0F) - 7;
  int8_t treb = (val & 0x0F) - 7;
  audio->setTone(bass, mid, treb);
}

//Info function overrides
void audio_info(const char *info){
  struct audioMessage audioTxTaskMessage;
  if (settings->logLevel == ARDUHAL_LOG_LEVEL_DEBUG) {
    serial.print("* "); 
    serial.println(info);
  }
  //Look for new server connections including redirects
  // also clear serverResponse on new connection
  if (strlen(info) > 12 && strncmp(info, "Connect to \"", 12) == 0) {
    serverResponse = "";
    //Extract the server name
    char* comma = strchr(info, ',');
    if (comma) *--comma = '\0';
    audioTxTaskMessage.cmd = WR_CONNECTING;
    connectingURL = &info[12];
    //Send server connection notification message
    audioTxTaskMessage.txt = connectingURL.c_str();
    xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
  }

  //Look for server response codes
  //Skip 'http/1.x '
  if (strlen(info) > 10 && strncasecmp(info, "HTTP/1.", 7) == 0) info = &info[9];
  //Server response 'nnn blah'
  if (isDigit(info[0]) && isDigit(info[1]) && isDigit(info[2]) && info[3] == ' ') {
    audioTxTaskMessage.cmd = WR_RESPONSE;
    serverResponse = info;
    //Send response code message
    audioTxTaskMessage.txt = serverResponse.c_str();
    xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
  }

}

//Metadata from Webradio - station name
void audio_showstation(const char *info){
  struct audioMessage audioTxTaskMessage;
  strncpy(wrStation, info, METABUF_LEN-1);
  wrStation[METABUF_LEN-1] = '\0';
  audioTxTaskMessage.cmd = WR_STATION;
  audioTxTaskMessage.txt = wrStation;
  xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
}

//Metadata from Webradio - stream title
void audio_showstreamtitle(const char *info){
  struct audioMessage audioTxTaskMessage;
  strncpy(wrTitle, info, METABUF_LEN-1);
  wrTitle[METABUF_LEN-1] = '\0';
  audioTxTaskMessage.cmd = WR_TITLE;
  audioTxTaskMessage.txt = wrTitle;
  xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
}


//icydescription?

//Metadata from Webradio - station homepage
void audio_icyurl(const char *info){  //homepage
  struct audioMessage audioTxTaskMessage;
  strncpy(wrIcyurl, info, METABUF_LEN-1);
  wrIcyurl[METABUF_LEN-1] = '\0';
  audioTxTaskMessage.cmd = WR_ICYURL;
  audioTxTaskMessage.txt = wrIcyurl;
  xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
}

//End of file notification
void audio_eof_mp3(const char *info){ 
  struct audioMessage audioTxTaskMessage;
  audioTxTaskMessage.cmd = WR_EOF;
  xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
}

//End of file notification for webstreams
void audio_eof_stream(const char *info){ 
  struct audioMessage audioTxTaskMessage;
  audioTxTaskMessage.cmd = WR_EOF;
  xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
}

void audio_set_sample_rate(uint32_t rate){ 
  struct audioMessage audioTxTaskMessage;
  audioTxTaskMessage.cmd = WR_SRATE;
  audioTxTaskMessage.value1 = rate;
  xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
}

//Metadata from file
void audio_id3data(const char *info){  //id3 metadata
  struct audioMessage audioTxTaskMessage;
  if (strncasecmp(info, "artist", 6) == 0) 
    metaArtist = (String)&info[8];
  else if (strncasecmp(info, "title", 5) == 0)
    metaTitle = (String)&info[7];
  else if (strncasecmp(info, "album", 5) == 0)
    metaAlbum = (String)&info[7];
  else if (strncasecmp(info, "year", 4) == 0) 
    metaYear = (String)&info[6];
  else return;
  audioTxTaskMessage.cmd = WR_META;
  xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
}

//Notify main thread we have an embedded image in our file
void audio_id3image(File& file, const size_t pos, const size_t size) {
  struct audioMessage audioTxTaskMessage;
  audioTxTaskMessage.cmd = WR_EMBED;
  audioTxTaskMessage.value1 = pos;
  audioTxTaskMessage.value2 = size;
  xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
}

//Intercept audio data for VU meter
void audio_process_extern(int16_t* buf, uint16_t len, bool* cont) {
  *cont = true;           //Continue, please..
#ifdef VUMETER
  struct audioMessage audioTxTaskMessage;
  //Find left and right channel peak in data block
  int16_t max_l = 0, max_r = 0;
  for (int i = 0; i < len; i+=2) {
    if (buf[i] > max_l) max_l = buf[i];
    if (buf[i+1] > max_r) max_r = buf[i+1];
  }
  //send message  
  audioTxTaskMessage.cmd = WR_VU;
  audioTxTaskMessage.value1 = max_l;
  audioTxTaskMessage.value2 = max_r;
  xQueueSend(audioGetQueue, &audioTxTaskMessage, 0);    //Not important, skip if queue full
#endif
#ifdef FFTMETER
  if (len < FFT_SIZE) return;
  xQueueSend(fftQueue, buf, 0);
#endif
}

#ifdef SUPERWOOFER
//Set I2C pins to internal mono DAC or external stereo DAC
bool setOutput(bool external) {
  if (external) {
    audio->setPinout(I2S_EXT_BCLK, I2S_EXT_LRC, I2S_EXT_DOUT);
    audio->forceMono(false);
  } else {
    audio->setPinout(I2S_INT_BCLK, I2S_INT_LRC, I2S_INT_DOUT);  
    audio->forceMono(true);
  }
  return external;
}
#endif

//Handle Webradio in a task on the WiFi CPU
void radioTask( void * pvParameters ) {
  static unsigned long lastms = 0;
  struct audioMessage audioRxTaskMessage;
  struct audioMessage audioTxTaskMessage;
  if (!audio) audio = new Audio();
  audio->setConnectionTimeout(1000, 4000);
  //Start message
  audioTxTaskMessage.cmd = WR_START;
#ifdef SUPERWOOFER
  audioTxTaskMessage.ret = setOutput(ADCFound);            //If we found the ADC, we assume the external DAC is also present
#else
  audioTxTaskMessage.ret = true;  //External output only
  audio->setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
#endif
#ifdef FORCE_MONO
  audio->forceMono(true);
#endif  
  xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
  for(;;) {                     //Infinite loop
    if (!audio->isRunning()) sleep(1);
    else taskYIELD(); //vTaskDelay(1); //taskYIELD();                  //Allow other tasks to run
    //Got a message from the main thread?
    if(xQueueReceive(audioSetQueue, &audioRxTaskMessage, 1) == pdPASS) {
      //Volume
      if(audioRxTaskMessage.cmd == WR_SETVOLUME){
        audio->setVolume(audioRxTaskMessage.value1);
      }
      //Stereo Wide
      else if(audioRxTaskMessage.cmd == WR_SETWIDE){
        audio->stereoWide(audioRxTaskMessage.value1);
      }
      //Stereo/mono
      else if(audioRxTaskMessage.cmd == WR_MONO){
        audio->forceMono(audioRxTaskMessage.value1);
      }
      //Play Webradio
      else if(audioRxTaskMessage.cmd == WR_CONNECTTOHOST){
        audioTxTaskMessage.cmd = WR_CONNECTTOHOST;
        const char* host = audioRxTaskMessage.txt;
        audioTxTaskMessage.ret = StartNewURL(host, audioRxTaskMessage.value1);
        audioTxTaskMessage.txt = serverResponse.c_str();
        xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
      }
      //Play File or FTP
      else if(audioRxTaskMessage.cmd == WR_CONNECTTOFS){
        audioTxTaskMessage.cmd = WR_CONNECTTOFS;
        metaArtist = metaAlbum = metaTitle = metaYear = "";
        if (audioRxTaskMessage.txt[0] == 'D') //file on the SD card
          audioTxTaskMessage.ret = audio->connecttoSD(&audioRxTaskMessage.txt[2], audioRxTaskMessage.value1);
        else if (audioRxTaskMessage.txt[0] == 'E') //file on the FTPFS
          audioTxTaskMessage.ret = audio->connecttoFS(ftpFS, &audioRxTaskMessage.txt[2], audioRxTaskMessage.value1);
        else
          audioTxTaskMessage.ret = audio->connecttoFS(ftpFS, audioRxTaskMessage.txt, audioRxTaskMessage.value1);
        xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
      }
      //Fetch the current volume
      else if(audioRxTaskMessage.cmd == WR_GETVOLUME){
        audioTxTaskMessage.cmd = WR_GETVOLUME;
        audioTxTaskMessage.ret = audio->getVolume();
        xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
      }
      //Stop song
      else if(audioRxTaskMessage.cmd == WR_STOPSONG){
        audioTxTaskMessage.cmd = WR_STOPSONG;
        audioTxTaskMessage.ret = audio->stopSong();
        audio->setDefaults();     //Free memory
        xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
      }
      //Set 3-band EQ if enabled
      else if(audioRxTaskMessage.cmd == WR_SETTONE){
        updateVSTone(audioRxTaskMessage.value1);
      }
      //Delete the task and free memory
      else if(audioRxTaskMessage.cmd == WR_DELETE){
        audioTxTaskMessage.cmd = WR_DELETE;
        xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
        audio->stopSong();
        delete(audio);
        audio = NULL;
        vTaskDelete(NULL);
      }
      //Don't know?
      else{
        log_e("Unknown audioTaskMessage");
      }

    }
    //100ms timer for buffer stats update
    if (millis() > lastms) {
      lastms = millis() + 100;
      //Send stats
      //Serial.printf("Webradio: Free mem=%d, stack=%d\n", ESP.getFreeHeap(), uxTaskGetStackHighWaterMark(NULL));
      audioTxTaskMessage.cmd = WR_STATS;
      audioTxTaskMessage.value1 = audio->inBufferFilled();
      audioTxTaskMessage.value2 = audio->inBufferFree();
      audioTxTaskMessage.value3 = audio->getBitRate(true);
      int startPos = audio->getAudioDataStartPos();
      int pos = audio->getFilePos() - startPos;
      //Serial.printf("File: %d, Pos: %d\n", audio->getFilePos(), pos);
      audioTxTaskMessage.pos = pos<0?0:pos;
      audioTxTaskMessage.size = audio->getFileSize() - startPos;
      audioTxTaskMessage.ret = audio->getCodec();
      audioTxTaskMessage.channels = (audio->isRunning() << 7) | audio->getChannels();
      xQueueSend(audioGetQueue, &audioTxTaskMessage, 0);    //Don't care, skip if queue full
    }     
    //Audio handling loop
    audio->loop();
  }
}  
