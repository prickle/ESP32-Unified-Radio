#include "decls.h"
bool powerOff = false;

//Power button pushed
// Toggles from standby to on
void powerButton() {
  if (!powerOff) {
    webradioStop();
    wifiDisconnect();
#ifdef BLUETOOTH
    stopBluetooth();
#endif
    clearProgLbl();
    setSigStrengthLbl("");
    setObjectVisibility();
    clearStations();
    setBrightness(0);
    setVolume(0);
    delay(500);
  } else {
    setBrightness(settings->brightness);
    setupWifi();
    setVolume(settings->dabVolume);
    setRadioMode(settings->mode);
  }
  powerOff = !powerOff;
}

#if defined(TWINSCROLL) || defined(PANASCROLL)
#include <ESP32Encoder.h>

//Button control using two rotary encoders with click push

//Wheel encoders
ESP32Encoder volEncoder;
ESP32Encoder funcEncoder;

int64_t volEncOld = 0;
int64_t funcEncOld = 0;
bool setBtnOld = false;
bool modeBtnOld = false;

int setBtnTimer = 0;

void prepareEncoders() {
  //ESP32Encoder::useInternalWeakPullResistors=DOWN;
  volEncoder.attachHalfQuad(INP_Q2A, INP_Q2B);  
  //volEncOld = settings.dabVolume;
  //volEncoder.setCount((volEncOld << 1) + 1);
  funcEncoder.attachHalfQuad(INP_Q1A, INP_Q1B); 
  //if (dabFrequency >= 0) funcEncOld = dabFrequency;
  //funcEncoder.setCount((dabFrequency << 1) + 1);
  pinMode(INP_B1C, INPUT);
  setBtnOld = !digitalRead(INP_B1C);
#ifdef TWINSCROLL
  pinMode(INP_B2C, INPUT);
  modeBtnOld = !digitalRead(INP_B2C);
#endif
}

unsigned long encoderTimer;
#define ENCODER_TIMEOUT 1000;

void encodersHandle() {
  if (factoryMode) return;
  //Volume control
  int64_t volEnc = volEncoder.getCount();
  int volDiff = (volEnc - volEncOld) / 2;
  if (volDiff) {
    volEncOld = volEnc;
    screenSaverInteraction();
    //Volume control scroll
    int volume = settings->dabVolume + volDiff;
    if (volume > 21) volume = 21;
    if (volume < 0) volume = 0;
    encoderTimer = millis() + ENCODER_TIMEOUT; 
    //if (tabView && lv_tabview_get_tab_act(tabView) != 0) 
    //  lv_tabview_set_act(tabView, 0, LV_ANIM_ON);
    settings->dabVolume = volume; 
    setVolume(settings->dabVolume);
#ifdef BLUETOOTH
    notifyVolumeChange(settings->dabVolume);
#endif  
    if (dabVolSlider) {
      //lv_obj_scroll_to_view(mainContainer, LV_ANIM_ON);
      lv_slider_set_value(dabVolSlider, settings->dabVolume, LV_ANIM_OFF);
    }
  }

  //Save changes
  if (encoderTimer && encoderTimer < millis()) {
    encoderTimer = 0;
    writeSettings();
  }

  int64_t funcEnc = funcEncoder.getCount();
  int funcDiff = (funcEnc - funcEncOld) / 2;
  if (funcDiff) {
    funcEncOld = funcEnc;
    screenSaverInteraction();
    //Station list scroll
    if (settings->mode == MODE_DAB || settings->mode == MODE_WEB) {
      if (tabView && lv_tabview_get_tab_act(tabView) != 1) 
        lv_tabview_set_act(tabView, 1, LV_ANIM_ON);
      int numItems = lv_obj_get_child_cnt(dabStationList) - 1;
      int index = listGetSelect(dabStationList) + funcDiff;
      if (index > numItems) index = numItems;
      if (index < 0) index = 0;
      listSetSelect(dabStationList, index);
    }
    //Podcast scroll
    if (settings->mode == MODE_POD) {
      if (tabView && lv_tabview_get_tab_act(tabView) != 1) 
        lv_tabview_set_act(tabView, 1, LV_ANIM_ON);
      podListMove(funcDiff);      
    }

    //FM frequency scroll
#ifdef MONKEYBOARD
    else if (settings->mode == MODE_FM) {
      dabFrequency += (funcDiff * 100);
      if (dabFrequency > 108000) dabFrequency = 108000;
      if (dabFrequency < 88000) dabFrequency = 88000;
      dabTuningAction(dabFrequency);
    }
#endif
#ifdef NXP6686
    else if (settings->mode == MODE_NFM || settings->mode == MODE_NSW || settings->mode == MODE_NMW || settings->mode == MODE_NLW)
      NXPChangeFrequency(funcDiff < 0);
#endif
  }
  bool setBtn = !digitalRead(INP_B1C);
  if (setBtn != setBtnOld) {
    setBtnOld = setBtn;
#ifdef PANASONIC
    if (powerOff) { if(setBtn) powerButton(); }
    else if (!setBtn) {
#else
    if (setBtn) {
#endif
      screenSaverInteraction();
      lv_obj_t * ddlist = lv_dropdown_get_list(modeList);
      lv_state_t state = lv_obj_get_state(modeList);
      //Mode selected
      if (ddlist && (state & LV_STATE_CHECKED)) {
        lv_dropdown_set_selected(modeList, settings->mode);
        lv_dropdown_close(modeList);
      }
      //Station selected
      else {
        if (settings->mode == MODE_DAB || settings->mode == MODE_WEB) {
          listDoSelect();
        }
#ifdef NXP6686        
        if (settings->mode == MODE_NSW || settings->mode == MODE_NMW || settings->mode == MODE_NLW) {
          NXPFineTune();
        }
#endif        
        if (settings->mode == MODE_POD) {
          podActivate();
        }
      }
#ifdef PANASONIC
      setBtnTimer = 0;
    } else setBtnTimer = millis();
  }   
  if (setBtn && setBtnTimer && setBtnTimer + 1000 < millis()) {
    setBtnTimer = 0;
    powerButton();
  }    
#else
    }
  }
#endif
#ifdef TWINSCROLL
  bool modeBtn = !digitalRead(INP_B1C);
  if (modeBtn != modeBtnOld) {
    modeBtnOld = modeBtn;
    if (modeBtn) {
      screenSaverInteraction();
      lv_obj_t * ddlist = lv_dropdown_get_list(modeList);
      lv_state_t state = lv_obj_get_state(modeList);
      //Mode selected
      if (ddlist && (state & LV_STATE_CHECKED)) {
        uint16_t opt = lv_dropdown_get_selected(modeList) + 1;      //Get the id of selected option
        lv_dropdown_close(modeList);
        if (opt != settings->mode) {
          settings->mode = opt;
          setRadioMode(settings->mode);
          writeSettings();
        }
      }
      //Open/Close Mode list 
      else {
        lv_dropdown_open(modeList);
        lv_obj_t * ddlist = lv_dropdown_get_list(modeList);
        lv_obj_add_style(ddlist, &style_biggestfont, LV_PART_MAIN);
        lv_obj_set_style_max_height(ddlist, 300, LV_PART_MAIN);
        lv_obj_set_height(ddlist, LV_SIZE_CONTENT);
      }
    }
  } 
#endif
}
#endif

#ifdef MINIBUTTONS

const char * buttonString[] = { "Prog", "Rew", "FF", "Stop", "Play", "Ipod", "OK", "Mode", 
  "Alarm1", "Down", "Alarm2", "Up", "Left", "Right", "Repeat", "Sleep", "Source", "Power" };

#define BTN_PROG 0
#define BTN_REW  1
#define BTN_FF   2
#define BTN_STOP 3
#define BTN_PLAY 4
#define BTN_IPOD 5
#define BTN_OK   6
#define BTN_MODE 7
#define BTN_ALM1 8
#define BTN_DOWN 9
#define BTN_ALM2 10
#define BTN_UP   11
#define BTN_LEFT 12
#define BTN_RIGT 13
#define BTN_RPT  14
#define BTN_SLP  15
#define BTN_SRC  16
#define BTN_PWR  17

#define REPEAT_TIMER  200


void prepareButtons() {
  pinMode(INP_PWR, OUTPUT);
  digitalWrite(INP_PWR, HIGH);
  Serial2.begin(57600, SERIAL_8N1, INP_TXD, INP_RXD);
}


//Act on button presses
void buttonPressed(int index) {
  static bool volChanged = false;
  
  //Individual button presses
  //Power button
  if (index == BTN_PWR) powerButton();
  if (powerOff) return;
  //Other buttons
  screenSaverInteraction();
  if (index >= 0) {
    if (index == BTN_LEFT) listMoveSelect(-1);
    else if (index == BTN_RIGT) listMoveSelect(1);
    else if (index == BTN_OK) listDoSelect();
    else if (index == BTN_UP) {
      if (tabView && lv_tabview_get_tab_act(tabView) != 0) 
        lv_tabview_set_act(tabView, 0, LV_ANIM_ON);
      int vol = settings->dabVolume;
      if (++vol > 21) vol = 21;
      setVolume(settings->dabVolume = vol);
      if (dabVolSlider) lv_slider_set_value(dabVolSlider, settings->dabVolume, LV_ANIM_OFF);
      volChanged = true;
    } else if (index == BTN_DOWN) {
      if (tabView && lv_tabview_get_tab_act(tabView) != 0) 
        lv_tabview_set_act(tabView, 0, LV_ANIM_ON);
      int vol = settings->dabVolume;
      if (--vol < 0) vol = 0;
      setVolume(settings->dabVolume = vol);
      if (dabVolSlider) lv_slider_set_value(dabVolSlider, settings->dabVolume, LV_ANIM_OFF);
      volChanged = true;
    } 
  } else {      //Save any changes on release
    if (volChanged) {
      writeSettings();
      volChanged = false;
    }
  }
}

//Detect and debounce button presses
void buttonsHandle() {
  static int8_t lastBtn = -1;
  static uint64_t btnTimer = 0;

  //Get button presses
  if(Serial2.available()) {
    int8_t btn = Serial2.read();
    //Initialization flag
    if (btn == '!') serial.println("> Button Controller Ready.");
    //Normal button pushes
    else if (btn >= 65 && btn <= 83) {  
      btn -= 66;
      lastBtn = btn;
      if (btn == BTN_UP || btn == BTN_DOWN) 
        btnTimer = millis() + REPEAT_TIMER;
      else btnTimer = 0;
      buttonPressed(btn);
    }
  }
  
  if (btnTimer && millis() > btnTimer) {
    btnTimer = millis() + REPEAT_TIMER;
    buttonPressed(lastBtn);      
  }
}

#endif

//--------------------------------------------------------
//Touchscreen and volume control coprocessor
#ifdef TOUCH_VOLUME

//Touchscreen results
int tx, ty; 
bool tz;

//Volume control results
int volPot = -1;


bool firstComm = true;
void touchSetup() {
  serial.println("> Start coprocessor comms.");
  Serial1.begin(115200, SERIAL_8N1, TOUCH_RX, TOUCH_TX);
}

void waitForTouch() {
  serial.println("> Wait for coprocessor.");
  while(!Serial1.available()) {terminalHandle();}
}

//Read touch screen and volume control from Arduino coprocessor.
void touchHandle() {
  static int byteCount = 0;
  static uint8_t checksum = 0;
  static uint8_t vals[5];
  while (Serial1.available()) {
    int val = Serial1.read();
    //Serial.print((char)val);
    if (val >= 128) {
      if (byteCount != 0) log_w("Warning: Coprocessor comms lost sync!");
      byteCount = 0;
      checksum = 0;
    }
    vals[byteCount++] = val;
    if (byteCount == 5) {
      if (checksum == val) {
        if (firstComm) {
          firstComm = false;
          serial.println("> Coprocessor communication established.");
        }
        //Read touch values
        tz = (vals[0] & 0x40) != 0;
        ty = ((vals[0] & 0x3f) << 4) | ((vals[1] >> 3) & 0x0f);
        tx = 1024 - (((vals[1] & 0x07) << 7) | vals[2]);
        //Read volume pot
        int v = (float)vals[3] / 127 * (VOLUME_STEPS + 1);
        if (v != volPot) {
          //Serial.print("Volume set to: ");
          //Serial.println(v);
          volPot = v;
          setVolume(volPot);
          if (dabVolSlider) lv_slider_set_value(dabVolSlider, volPot, LV_ANIM_OFF);
        }
      } 
      else log_w("Warning: Coprocessor comms checksum error!");
      byteCount = 0;
    }
    checksum += val;
    checksum &= 0x7F;
  }
}

#endif