#include "decls.h"

#ifdef MONKEYBOARD

//dabFrequency function flags
#define DAB_I2SIN    -1
#define DAB_LINEIN1  -2
#define DAB_LINEIN2  -3
#define DAB_SCAN     -4
#define DAB_STOP     -5
#define DAB_NONE     -6


//The Monkeyboard DAB module
MonkeyBoard dab = MonkeyBoard(&DAB_SERIAL, -1, DAB_RXD, DAB_TXD);

volatile bool DABFound = false;
volatile bool DABActive = false;
long dabFrequency = DAB_NONE;
uint8_t currentDabStatus = DAB_STATUS_NONE;
char fmStationName[35] = "";
bool render_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);

//DAB slideshow image thumbnail
uint16_t dab_img_data[80*60] = {0};
static lv_img_dsc_t dab_img_dsc = {
    .header = {
      .cf = LV_IMG_CF_TRUE_COLOR,          /*Set the color format*/
      .always_zero = 0,
      .reserved = 0,
      .w = 80,
      .h = 60,
    },
    .data_size = 80 * 60 * LV_COLOR_DEPTH / 8,
    .data = (uint8_t*)dab_img_data,
};

void dabStatusCallback(uint8_t status);
void dabNameCallback(char *name, int8_t typeIndex);
void dabTextCallback(char *text);
void dabFreqCallback(uint8_t frequency);
void dabStrengthCallback(uint8_t signal);
void dabStereoCallback(uint8_t mode);
void dabUserAppCallback(uint8_t userApp);
void dabTimeCallback();
void dabImageCallback(char *name, uint8_t type, uint16_t id, uint8_t *data, int length);
void dabProgressCallback(uint8_t percent);
void dabIdleCallback();
void setDabEQ(uint8_t eq);
long readDabStations();
void kickstartDab();
void dabScanCallback(uint8_t ver1, uint16_t ver2);
void dabPlay(long dabFreq);
void clear_output();

//Set up callbacks and allocate MOT
void prepareDab() {
  dab.SetStatusChangeCallback(dabStatusCallback);
  dab.SetNameChangeCallback(dabNameCallback);
  dab.SetTextChangeCallback(dabTextCallback);
  dab.SetFrequencyChangeCallback(dabFreqCallback);
  dab.SetStrengthChangeCallback(dabStrengthCallback);
  dab.SetStereoChangeCallback(dabStereoCallback);
  dab.SetUserAppChangeCallback(dabUserAppCallback);
  dab.SetTimeChangeCallback(dabTimeCallback);
  dab.SetIdleCallback(dabIdleCallback);
  //Prepare MOT slideshow
  dab.SetMotImageCallback(dabImageCallback);
  dab.SetMotProgressCallback(dabProgressCallback);
  dab.MotBegin();   //Allocate memory if not already
  //Get RTC going
  dab.EnableSyncClock();
  //Restore settings to controls  
  setVolume(settings->dabVolume);
  setDabEQ(settings->dabEQ);  
}

//Load station list, find last station and begin
void startDab() {
  info(NAME, 0, "Start DAB Radio..");
  dabFrequency = readDabStations();
  kickstartDab();
}

//Set frequency display and start
void startFM() {
  dabFrequency = settings->dabFM;
  serial.print("> Starting DAB FM freq:");
  serial.println(dabFrequency);
  updateFMDisplay(dabFrequency);
  kickstartDab();
}

//Start the I2S input
void startI2S() {
  dabFrequency = DAB_I2SIN;
  kickstartDab();
}

//Start the line in bypass modes
void startVSbypass(int line) {
  dabFrequency = line;
  kickstartDab();
}

//Start here - load settings to DAB and switch DAB mode 
void kickstartDab() {
  if (!DABFound) DABFound = dab.begin();      //try to resolve issues..
  if (DABFound) {  
    setVolume(settings->dabVolume);
    setDabEQ(settings->dabEQ);
    setStereoMode(dabFrequency < 10000 || !settings->mono);
    updateUrlEditText();
    if (currentDabStatus == DAB_STATUS_STOP)
      dabStatusCallback(currentDabStatus);
  } else errorContinue(3, "DAB+ Radio Module not responding!");   
}

void setVolume(uint8_t volume) {
  dab.SetVolume(volume);
  //digitalWrite(AMP_MUTE, volume == 0);
}

//DAB channel scan - init a software reset callback
void startDabScan() {
  serial.println("> Starting DAB+ Channel Scan");
  if (dabFrequency == DAB_SCAN) return;
  //if (scanSpinner) lv_obj_set_hidden(scanSpinner, false);
  dab.SoftReset(1, dabScanCallback); //Clear database 
  clearStations();
  dabFrequency = DAB_SCAN;
}

//Try to restore prior mode
void restartDab() {
  if (dabFrequency == DAB_LINEIN1 || dabFrequency == DAB_LINEIN2) 
    startVSbypass(dabFrequency);
  else if (dabFrequency == DAB_I2SIN) startI2S();
  else if (dabFrequency >= 10000) startFM();
  else startDab();  
}

//FM signal search up or down
bool dabFMSearching = false;
void dabFMSearch(uint8_t dir) {
  if (dabFMSearching) return;
  dab.FMSearch(dir);
  dabFMSearching = true;
}

//Keep animations smooth by updating screen in the DAB's delay loops 
void dabIdleCallback() {
  //if (!ScreenSaverActive) lv_refr_now(display);
}

//Pop, rock, classical, that kind of thing
// doesn't seem to work anyway..
void setDabEQ(uint8_t eq) {
  if (eq == 0) dab.SetEQ(false, 0);
  else dab.SetEQ(true, eq - 1);
}

//Channel scan reset init done, begin searching
// only scanning aussie channels..
void dabScanCallback(uint8_t ver1, uint16_t ver2) {
  serial.printf("> KeySight DAB+ v%d.%d\r\n", ver1, ver2);
  dab.SetNotification(127); //All of them
  if (currentDabStatus == DAB_STATUS_STOP) {
    dab.AutoSearchNoClear(0, 40); 
  }
}

//Station list item selected
void dabStationAction(lv_event_t * event) {
  lv_obj_t * obj = lv_event_get_target(event);
  listSetSelect(obj);
  const char* name = stationListName(obj);
  if (!name) return;
  //serial.printf("Clicked: %s\n", name);
  tabViewShowMain();
  int i = *(int*)lv_obj_get_user_data(obj);
  strncpy(settings->dabChannel, name, 34);
  settings->dabChannel[34] = '\0';
  writeSettings();
  dabPlay(i);
}

//Read dab stations into station list
long readDabStations() {
  char programName[35] = {0};
  long prevStation = DAB_STOP;
  info(NOW, 0, "Reading station list..");
  clearStations();
  dab.SetSorter(DAB_SORTER_NAME);     //Set the sorter before reading names
  if (dab.TotalProgram > 0) {
    serial.printf("> Reading %ld DAB stations.\r\n", dab.TotalProgram);
    for (int i = 0; i < dab.TotalProgram - 1; i++) {
      if (dab.GetProgramName(STREAM_MODE_DAB, i, DAB_NAMEMODE_LONG, programName, 35)) {
        //serial.printf("[%d]\t%s\r\n", i, programName);
        lv_obj_t * list_btn = addStationButton(programName, dabStationAction, &i, sizeof(int));
        if (strcasecmp(programName, settings->dabChannel) == 0) {
          serial.println("> Previous station found.");
          listSetSelect(list_btn);
          prevStation = i;        
        }  
      }
    }
  } else {
    serial.println("> No stations in DAB+ database!");
    info(TEXT, 0, LV_SYMBOL_WARNING " No Stations! Go to settings and do a DAB Scan.");
  }
  info(NOW, 0, "");  
  return prevStation;
}

//FM tuning dial movement
#define DAB_DIAL_DELAY  500
//Fired when tuning dial changes
unsigned long dabDialTimer = 0;
void dabTuningAction(int freq) {
  if (settings->mode == MODE_FM) {
    dabDialTimer = millis() + DAB_DIAL_DELAY;
    updateFMDisplay(freq);
  }
}

void dabRewind() {
  if (settings->mode == MODE_FM) dabFMSearch(0);
}

void dabFastforward() {
  if (settings->mode == MODE_FM) dabFMSearch(1);
}

//FM tuning dial movement ceased, start reception on new frequency
// Called from main loop
void updateTuning() {
  if (dabDialTimer && dabDialTimer < millis()) {
    dabDialTimer = 0;
    settings->dabFM = dabFrequency;
    writeSettings();
    dab.PlayStream(STREAM_MODE_FM, dabFrequency);    
    updateSTMOLabel(STMO_MONO);
  }
}

void setStereoMode(bool stereo) {
  dab.SetStereoMode(stereo);
  if (!stereo) updateSTMOLabel(STMO_MONO);
}
 
void initDab() {
  //Start DAB+ module
  if (!(DABFound = dab.begin())) errorContinue(3, "DAB+ Radio Module Fail!");
  else serial.println("> DAB+ Radio Setup OK.");
  prepareDab();   //set callbacks and allocate memory
}

void dabHandle() {
  dab.handle();
  updateTuning();
}

//DAB status change callback
bool wasTuning = false;
void dabStatusCallback(uint8_t status) {
  currentDabStatus = status;
  fmStationName[0] = '\0';
  if (status == DAB_STATUS_PLAYING) {
    if (settings->mode == MODE_FM) {
      if (dabFMSearching) {
        dabFrequency = dab.GetPlayIndex();
        updateFMDisplay(dabFrequency);
        settings->dabFM = dabFrequency;
        writeSettings();
      }
    }    
    if (dabFrequency != DAB_I2SIN && dabFrequency != DAB_LINEIN1 && dabFrequency != DAB_LINEIN2) {
      info(NAME, 0, LV_SYMBOL_PLAY " Playing");
      updateUrlEditText();
    }
    info(TEXT, 0, ""); //remove "No signal"
    dabFMSearching = false;
    wasTuning = false;
  }
  
  if (status == DAB_STATUS_SEARCHING) {
    if (settings->mode == MODE_DAB) setDabScanLabel(LV_SYMBOL_WIFI " Searching..");
    info(NAME, 0, LV_SYMBOL_WIFI " Searching..");
    wasTuning = false;
  } else {
    if (dabFrequency == DAB_SCAN) {
      char buf[30];
      sprintf(buf, "%d Stations Found.", (int)dab.GetTotalProgram());
      setDabScanLabel(buf);
      hideScanSpinner(true);
      dabStop();
      restartDab();
    }    
  }
  
  if (status == DAB_STATUS_TUNING) {
    if (!wasTuning) {
      info(NAME, 0, LV_SYMBOL_WIFI " Tuning %s", settings->dabChannel);
      setSigStrengthLbl(LV_SYMBOL_WIFI " 0%");
    }
    wasTuning = true;
  }
  
  if (status == DAB_STATUS_STOP) {
    if (dabFrequency > 10000) {
      setStereoMode(!settings->mono);
      dab.PlayStream(STREAM_MODE_FM, dabFrequency);
      updateSTMOLabel(STMO_MONO);
    }
    else if (dabFrequency >= 0) {
      dab.PlayStream(STREAM_MODE_DAB, dabFrequency);
      if (wasTuning) {
        info(TEXT, 0, LV_SYMBOL_WARNING " No Signal!");
      }
      updateSTMOLabel(STMO_STEREO);
    }
    else if (dabFrequency == DAB_I2SIN) {
      serial.print("> DAB I2S Input now: ");
      if (dab.PlayStream(STREAM_MODE_I2SIN, 0)) serial.println("OK");
      else serial.println("Error!");
    }
    else if (dabFrequency == DAB_LINEIN1) {
      serial.print("> DAB LINE1 Passthrough now: ");
      if (dab.PlayStream(STREAM_MODE_LINE1, 0)) serial.println("OK");
      else serial.println("Error!");
    }
    else if (dabFrequency == DAB_LINEIN2) {
      serial.print("> DAB LINE2 Passthrough now: ");
      if (dab.PlayStream(STREAM_MODE_LINE2, 0)) serial.println("OK");
      else serial.println("Error!");
    }
    /*else if (dabFrequency == DAB_SCAN) {
      serial.print("> DAB Channel Scan now: ");
      if (dab.AutoSearch(0, 40)) {
        if (dabScanLabel) lv_label_set_text(dabScanLabel, "Scan commences..");
        serial.println("OK");
      }
      else {
        if (dabScanLabel) lv_label_set_text(dabScanLabel, "Can't start scan!");
        serial.println("Error!");
      }
    }*/
    else {
      if (dabFrequency != DAB_LINEIN1 && dabFrequency != DAB_LINEIN2) 
        info(NAME, 0, LV_SYMBOL_STOP " Stopped");
    }
  }
  
}

//Thumbnail image
LV_IMG_DECLARE(dabLogo);

//Stop the DAB now
void dabStop() {
  dabFrequency = DAB_STOP;
  DABActive = false;
  clear_output();
  setDabImg(&dabLogo);
  setDabBar(0, LV_ANIM_OFF);
  dab.StopStream();
}

//Change the DAB to a given frequency
void dabPlay(long dabFreq) {
  dabStop();
  clearProgLbl();
  updateSTMOLabel(STMO_MONO);
  serial.println("> DAB Starting now");
  if (dabFreq > 10000) dab.PlayStream(STREAM_MODE_FM, dabFreq);
  else if (dabFreq >= 0) dab.PlayStream(STREAM_MODE_DAB, dabFreq);
  else if (dabFreq == DAB_LINEIN1) dab.PlayStream(STREAM_MODE_LINE1, 0);
  else if (dabFreq == DAB_LINEIN2) dab.PlayStream(STREAM_MODE_LINE2, 0);
  dabFrequency = dabFreq;
}

//Station name callback
void dabNameCallback(char *name, int8_t typeIndex) {
  char buf[64];
  if (settings->mode != MODE_DAB && settings->mode != MODE_FM) return;
  if (settings->mode == MODE_FM) {
    strncpy(fmStationName, name, 34);
    fmStationName[34] = '\0';
  }
  strncpy(buf, name, 34);
  buf[34] = '\0';
  if (typeIndex > 0) {
    const char* type = dab.GetTypeString(typeIndex);
    int len = strlen(name);
    if (len < 35) {
      strcpy(&buf[len], "  (");
      strcpy(&buf[len+3], type);
      len += strlen(type) + 3;
      if (len < 62) strcpy(&buf[len++], ")");
      buf[len] = '\0';
    }
  }
  info(NOW, 0, buf);
}

//Program text callback
void dabTextCallback(char *text) {
  int offset = 0;
  if (settings->mode != MODE_DAB && settings->mode != MODE_FM) return;
  if (strncasecmp(text, "now", 3) == 0) {
    offset = 3;
    if (text[offset] == ' ') offset++;
    if (strncasecmp(&text[offset], "playing", 7) == 0) offset += 7;
    if (text[offset] == ' ') offset++;
    if (text[offset] == ':') offset++;
    if (strncmp(&text[offset], "...", 3) == 0) offset += 3;
    if (text[offset] == ' ') offset++;
    String txt = ((String)LV_SYMBOL_AUDIO) + " -" + (&text[offset]) + " -";            
    info(NAME, 0, txt.c_str());
  }
  else if (strncasecmp(text, "on now", 6) == 0) {
    offset = 6;
    if (text[offset] == ' ') offset++;
    if (text[offset] == ':') offset++;
    if (strncmp(&text[offset], "...", 3) == 0) offset += 3;
    if (text[offset] == ' ') offset++;
    String txt = ((String)LV_SYMBOL_AUDIO) + " -" + (&text[offset]) + " -";            
    info(NAME, 0, txt.c_str());
  }
  else info(TEXT, 0, text);
}

//Frequency changed during channel scan
void dabFreqCallback(uint8_t frequency) {
  char str[30];
  if (settings->mode != MODE_DAB) return;    //Silence progress if DAB not visible
  if (frequency < 100) {
    sprintf(str, "[%d] %s", (int)dab.GetTotalProgram(), dab.GetFrequencyString(frequency));
    setDabScanLabel(str);
  } 
}

//DAB signal strength callback
void dabStrengthCallback(uint8_t signal) {
  char s[25] = {0};
  if (settings->mode != MODE_DAB) return;
  sprintf(s, LV_SYMBOL_WIFI " %d%%", signal);
  setSigStrengthLbl(s);
}

//DAB stereo mode callback
void dabStereoCallback(uint8_t mode) {
  if (settings->mode != MODE_DAB && settings->mode != MODE_FM) return;
  if (mode == DAB_STEREO_MONO) 
    updateSTMOLabel(STMO_MONO);
  else if (mode == DAB_STEREO_STEREO || mode == DAB_STEREO_JOINT || mode == DAB_STEREO_DUAL)
    updateSTMOLabel(STMO_STEREO);
}

//DAB slideshow available callback
void dabUserAppCallback(uint8_t userApp) {
  if (settings->mode != MODE_DAB) return;
  if (userApp == KSApplicationType_SLS) updateSTMOLabel(STMO_SLIDE);
}

//Time of day from DAB+
void dabTimeCallback() {
  unsigned char sec, min, hour, day, month, year;
  //Set RTC if not set
  if (!RTCSet && dab.GetRTC(&sec, &min, &hour, &day, &month, &year)) {
    serial.print("> Got time from DAB: ");
    rtc.setTime(sec, min, hour, day, month, year + 2000);
    serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));
    RTCSet = true;
  }
}

//Slideshow image arrived
uint16_t transport_id = 0;
void dabImageCallback(char *name, uint8_t type, uint16_t id, uint8_t *data, int length) {
  uint16_t w = 0, h = 0;
  if (settings->mode != MODE_DAB) return;
  //if (dab_img_bar) lv_bar_set_value(dab_img_bar, 0, LV_ANIM_OFF);
  TJpgDec.getJpgSize(&w, &h, data, length);
  if (w == 0 || h == 0) return;     //Problem with image
  if (ScreenSaverPending) {
    if (!ScreenSaverActive || id != transport_id) {
      if (w <= 320 && h <= 240) TJpgDec.setJpgScale(1);
      else if (w <= 640 && h <= 480) TJpgDec.setJpgScale(2);
      else TJpgDec.setJpgScale(4);
      TJpgDec.setCallback(tft_output);
      // Draw the image, top left at 0,0
      TJpgDec.drawJpg(0, 0, data, length);
      ScreenSaverActive = true;
    }
  }
  if (id != transport_id) {
    if (w <= 320 && h <= 240) TJpgDec.setJpgScale(4);
    else if (w <= 640 && h <= 480) TJpgDec.setJpgScale(8);
    else return;          //Too big
    TJpgDec.setCallback(render_output);
    TJpgDec.drawJpg(0, 0, data, length);      
    setDabImg(&dab_img_dsc);
    transport_id = id;
  }
}

//Slideshow image is building
void dabProgressCallback(uint8_t percent) {
  if (settings->mode != MODE_DAB) return;
  //if (ScreenSaverActive) tft.fillRect(0, 236, percent * 3.19, 4, ILI9341_RED);
  setDabBar(percent, LV_ANIM_OFF);
}


// This next function will be called during decoding of the jpeg file to
// render the thumbnail.
bool render_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  uint16_t *d = bitmap;
  for (int r = y; r < y+h; r++) {
    int p = r * 80;
    for (int c = x; c < x+w; c++) { 
#if LV_COLOR_16_SWAP == 0
      /*Pixel format: Red: 5 bit, Green: 6 bit, Blue: 5 bit*/
      dab_img_data[p + c] = *d++;
#else
      /*Pixel format: Red: 5 bit, Green: 6 bit, Blue: 5 bit BUT the 2 bytes are swapped*/
      uint16_t o = *d++;
      dab_img_data[p + c] = (o>>8) | (o<<8);
#endif
    }
  }
  // Return 1 to decode next block
  return 1;
}

//Clear the thumbnail
void clear_output() {
  for (int r = 0; r < 60; r++) {
    int p = r * 80;
    for (int c = 0; c < 80; c++) 
      dab_img_data[p + c] = 0;
  }
}

#endif