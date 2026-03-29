#include "decls.h"
//EEPROM Settings object
// Settings window
// WiFi SSID and scanning
// Touchscreen calibration window

unsigned int eeAddress = 0;   //Location we want the data to be put.
settingsObject* settings;

#include "nvs_flash.h"

//Doing it this brain-dead looking way because EEPROM.put() blocks interrupts too long
// and EEPROM functions dont play nice with psram?
void writeSettings() {
  for (int i = 0; i < sizeof(settingsObject); i++)
    EEPROM.write(eeAddress + i, ((uint8_t*)settings)[i]);
  EEPROM.commit();
}

bool readSettings() {
  serial.printf("> Size of Settings: %d (Max %d)\r\n", sizeof(settingsObject), EE_SIZE);  
  if (EEPROM.read(eeAddress) != EE_MAGIC) {
    serial.println("> EEPROM Uninitialised! Writing defaults.");
    writeSettings();
    return false;
  }
  for (int i = 0; i < sizeof(settingsObject); i++)
    ((uint8_t*)settings)[i] = EEPROM.read(eeAddress + i);
  return true;
}

void setDefaults() {
  //Default settings get copied in
  settingsObject defaults = {
    EE_MAGIC,
    1,          //mode
    90,        //vsVolume
    0x0806,     //vsTone
    0,          //Balance
    16,         //dabVolume
    0,         //dabEQ
    "MMM HARD N HEAVY",         //dabChannel
    105100,      //dabFM
    0,          //playlistIndex (none)
    "http://radio1.internode.on.net:8000/296",         //Server
    0,          //Current Network
    { { "<Empty>", "" }, //password
      { "<Empty>", "" },
      { "<Empty>", "" },
      { "<Empty>", "" }
    },
    {},
    false,
    //FTP Data
    IPAddress(192,168,0,53),
    0,
    "anonymous",
    "pass@word",
    //Misc
    true,       //Autosave
    0,          //DLNA playlist index
    2,          //Log Level
    false,      //Mono
    255,        //Brightness
    621,        //freqMW
    144,        //freqLW
    2300,       //freqSW
    0,          //bwFM
    3,          //bwMW
    3,          //bwSW
    3,          //bwLW
    0,          //agcFM
    0,          //agcMW
    0,          //agcLW
    0,          //agcSW
    TZ,         //timezone
    false       //Stereo Wide
  }; 
  memcpy(settings, &defaults, sizeof(settingsObject));
}

void initSettings() {
  Serial.print("> Starting EEPROM..");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

  if (!EEPROM.begin(EE_SIZE)) {
    Serial.println("begin failed!");
  } else { 
    Serial.print("begin ok..");
  }
  //Allocate settings in psram
  settings = (settingsObject*)ps_malloc(sizeof(settingsObject));
  setDefaults();
  //Now read actual settings if available
  Serial.print("read..");
  readSettings();
  //Force new Timezone setting
  //strncpy(settings->tz, TZ, 63);
  //writeSettings();
}

//------------------------------------------------------------------
//lvgl widgets

void dabVolumeAction(lv_event_t * event);
void wideAction(lv_event_t * event);
void settingReleasedAction(lv_event_t * event);
void wifiDisconnectAction(lv_event_t * event);
void wifiLongPressAction(lv_event_t * event);
void wifiSelectedAction(lv_event_t * event);
void wifiScanAction(lv_event_t * event);
void passwordEditAction(lv_event_t * event);
void keyboardPasswordKeyAction(lv_event_t * event);
void wifiDisconnectAction(lv_event_t * event);
void ftpSetEditText();
void ftpServerAction(lv_event_t * event);
void ftpUserAction(lv_event_t * event);
void ftpPassAction(lv_event_t * event);
void ftpShowPassAction(lv_event_t * event);
void keyboardFtpKeyAction(lv_event_t * event);
void dabScanAction(lv_event_t * event);
void dabEqAction(lv_event_t * event);
void dabOpenEqAction(lv_event_t * event);
void dstAction(lv_event_t * event);
void autosaveAction(lv_event_t * event);
void brightnessAction(lv_event_t * event);
void monoAction(lv_event_t * event);
void webToneAction(lv_event_t * event);
void tzEditAction(lv_event_t * event);
void keyboardTzKeyAction(lv_event_t * event);

// *******************************************************************
//Settings window
lv_obj_t * dabVolSlider;
lv_obj_t * mainContainer;
static lv_obj_t * wifiContainer;
static lv_obj_t * ftpContainer;
static lv_obj_t * dabContainer;
static lv_obj_t * timeContainer;
static lv_obj_t * wifiNetworkList;
static lv_obj_t * wifiStatusLbl = NULL;
static lv_obj_t * wifiDisconnectBtn = NULL;
static lv_obj_t * wifiDisconnectBtnLbl = NULL;
static lv_obj_t * passwordEditText = NULL;
static lv_obj_t * dabEqList;
static lv_obj_t * dabScanLabel;
static lv_obj_t * scanSpinner;
static lv_obj_t * dstLabel;
static lv_obj_t * dstSwitch;
static lv_obj_t * tzLabel;
static lv_obj_t * tzEditText;
static lv_obj_t * ftpServerText;
static lv_obj_t * ftpUserText;
static lv_obj_t * ftpPassText;
static lv_obj_t * saveSwitch;
static lv_obj_t * monoSwitch;
static lv_obj_t * brightSlider;
static lv_obj_t * webToneSlider1;
static lv_obj_t * webToneSlider2;
static lv_obj_t * webToneSlider3;
static lv_obj_t * wideSwitch;

bool editWifiPassword = false;
const char * wlanStateString[] = { "WiFi Idle", "No SSID", "Scan Completed", "Connected", 
                                   "Connect Failed", "Connection Lost", "Disconnected"};


void createSettingsWindow(lv_obj_t * page) {
  static lv_style_t style_bg;
  lv_style_init(&style_bg);
  lv_style_set_bg_color(&style_bg, lv_color_black());
  lv_style_set_bg_grad_color(&style_bg, lv_color_black());
  lv_style_set_bg_grad_dir(&style_bg, LV_GRAD_DIR_VER);
  lv_style_set_bg_opa(&style_bg, LV_OPA_COVER);
  
  static lv_style_t style_caption;
  lv_style_init(&style_caption);
  lv_style_set_text_font(&style_caption, &lv_font_montserrat_12);

  static lv_style_t style_slider;
  lv_style_init(&style_slider);
#ifdef THEME_BLUE
  lv_style_set_border_color(&style_slider, lv_color_hex(0x487fb7));
#else
  lv_style_set_border_color(&style_slider, lv_color_hex(0xFF6025));
#endif
  lv_style_set_border_width(&style_slider, 2);
  lv_style_set_border_opa(&style_slider, LV_OPA_COVER);
  lv_style_set_radius(&style_slider, 5);
  lv_style_set_pad_all(&style_slider, 7);
  static lv_style_t style_sliderbg;
  lv_style_init(&style_sliderbg);
  lv_style_set_radius(&style_sliderbg, 5);
  static lv_style_t style_knob;
  lv_style_init(&style_knob);
  lv_style_set_radius(&style_knob, 5);
  lv_style_set_pad_all(&style_knob, 0);

  static lv_style_t style_ta;
  lv_style_init(&style_ta);
  lv_style_set_bg_color(&style_ta, lv_color_black());
  lv_style_set_bg_grad_color(&style_ta, lv_color_black());
  lv_style_set_bg_opa(&style_ta, LV_OPA_100);
  lv_style_set_radius(&style_ta, 3);
  lv_style_set_border_width(&style_ta, 1);
  lv_style_set_border_color(&style_ta, lv_color_hex(0xFF7F50));
  lv_style_set_text_color(&style_ta, lv_color_white());
  lv_style_set_text_font(&style_ta, &lv_font_montserrat_14);
  lv_style_set_pad_top(&style_ta, 2);
  lv_style_set_pad_bottom(&style_ta, 4);
  lv_style_set_pad_left(&style_ta, 6);
  lv_style_set_pad_right(&style_ta, 6);

  lv_obj_add_style(page, &style_bigfont, LV_PART_MAIN);

  //The main info container window
  mainContainer = lv_obj_create(page);
  int height = 116;
#ifdef EQUALIZER
  height += 94;
#endif
#ifndef FORCE_MONO
  height += 36;
#endif
  int width = lv_obj_get_content_width(page);
#if (TFT_WIDTH == 480)
  lv_obj_set_pos(mainContainer, 4, 8);
  width -= 8;
#else  
  lv_obj_set_pos(mainContainer, 0, 0);
#endif

    lv_obj_set_size(mainContainer, width, height);
    lv_obj_add_style(mainContainer, &style_groupbox, LV_PART_MAIN);
    lv_obj_clear_flag(mainContainer, LV_OBJ_FLAG_SCROLLABLE);
 
    // Add the master (DAB) volume slider
    lv_obj_t * volDABLbl = lv_label_create(mainContainer); //First parameters (scr) is the parent
    lv_obj_set_pos(volDABLbl, 10, 10);
    lv_label_set_text(volDABLbl, "Master Vol");  //Set the text
    dabVolSlider = lv_slider_create(mainContainer);                            //Create a slider
    lv_obj_set_size(dabVolSlider, width - 116, 18);   //Set the size
    lv_obj_align_to(dabVolSlider, volDABLbl, LV_ALIGN_OUT_RIGHT_MID, 10, 0);         //Align next to the slider
    lv_slider_set_range(dabVolSlider, 0, VOLUME_STEPS);                                            //Set the current value
    lv_slider_set_value(dabVolSlider, settings->dabVolume, LV_ANIM_OFF);
    lv_obj_add_style(dabVolSlider, &style_slider, LV_PART_MAIN);
    lv_obj_add_style(dabVolSlider, &style_sliderbg, LV_PART_INDICATOR);
    lv_obj_add_style(dabVolSlider, &style_knob, LV_PART_KNOB);
    lv_obj_add_event_cb(dabVolSlider, dabVolumeAction, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(dabVolSlider, settingReleasedAction, LV_EVENT_RELEASED, NULL);

#ifdef EQUALIZER
    lv_obj_t * toneLbl = lv_label_create(mainContainer); //First parameters (scr) is the parent
    lv_obj_align_to(toneLbl, volDABLbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 14);         //Align next to the slider
    lv_label_set_text(toneLbl, "Codec\nTone");  //Set the text

    webToneSlider1 = lv_slider_create(mainContainer);                            //Create a slider
    lv_obj_set_size(webToneSlider1, 18, 60);   //Set the size
    lv_obj_align_to(webToneSlider1, toneLbl, LV_ALIGN_OUT_RIGHT_TOP, 65, 4);                //Align below the first button
    //lv_slider_set_knob_in(webToneSlider1, true);
    lv_slider_set_range(webToneSlider1, 0, 13);                                            //Set the current value
    lv_slider_set_value(webToneSlider1, (settings->vsTone >> 8) & 0x0F, LV_ANIM_OFF);                                            //Set the current value
    lv_obj_add_style(webToneSlider1, &style_slider, LV_PART_MAIN); 
    lv_obj_add_style(webToneSlider1, &style_sliderbg, LV_PART_INDICATOR);
    lv_obj_add_style(webToneSlider1, &style_knob, LV_PART_KNOB);
    lv_obj_t * label = lv_label_create(mainContainer); //First parameters (scr) is the parent
    lv_label_set_text(label, "Bass");  //Set the text
    lv_obj_add_style(label, &style_caption, LV_PART_MAIN);
    lv_obj_align_to(label, webToneSlider1, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);                //Align below the first button
    label = lv_label_create(mainContainer); //First parameters (scr) is the parent
    lv_label_set_text(label, "+6");  //Set the text
    lv_obj_add_style(label, &style_caption, LV_PART_MAIN);
    lv_obj_align_to(label, webToneSlider1, LV_ALIGN_OUT_LEFT_TOP, -4, 0);                //Align below the first button
    label = lv_label_create(mainContainer); //First parameters (scr) is the parent
    lv_label_set_text(label, "-6");  //Set the text
    lv_obj_add_style(label, &style_caption, LV_PART_MAIN);
    lv_obj_align_to(label, webToneSlider1, LV_ALIGN_OUT_LEFT_BOTTOM, -4, 0);                //Align below the first button
    lv_obj_add_event_cb(webToneSlider1, webToneAction, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(webToneSlider1, settingReleasedAction, LV_EVENT_RELEASED, NULL);

    webToneSlider2 = lv_slider_create(mainContainer);                            //Create a slider
    lv_obj_set_size(webToneSlider2, 18, 60);   //Set the size
    lv_obj_align_to(webToneSlider2, webToneSlider1, LV_ALIGN_OUT_RIGHT_TOP, 40, 0);                //Align below the first button
    //lv_slider_set_knob_in(webToneSlider2, true);
    lv_slider_set_range(webToneSlider2, 0, 13);                                            //Set the current value
    lv_slider_set_value(webToneSlider2, (settings->vsTone >> 4) & 0x0F, LV_ANIM_OFF);                                            //Set the current value
    lv_obj_add_style(webToneSlider2, &style_slider, LV_PART_MAIN);
    lv_obj_add_style(webToneSlider2, &style_sliderbg, LV_PART_INDICATOR);
    lv_obj_add_style(webToneSlider2, &style_knob, LV_PART_KNOB);
    label = lv_label_create(mainContainer); //First parameters (scr) is the parent
    lv_label_set_text(label, "Mid");  //Set the text
    lv_obj_add_style(label, &style_caption, LV_PART_MAIN);
    lv_obj_align_to(label, webToneSlider2, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);                //Align below the first button
    label = lv_label_create(mainContainer); //First parameters (scr) is the parent
    lv_label_set_text(label, "+6");  //Set the text
    lv_obj_add_style(label, &style_caption, LV_PART_MAIN);
    lv_obj_align_to(label, webToneSlider2, LV_ALIGN_OUT_LEFT_TOP, -4, 0);                //Align below the first button
    label = lv_label_create(mainContainer); //First parameters (scr) is the parent
    lv_label_set_text(label, "-6");  //Set the text
    lv_obj_add_style(label, &style_caption, LV_PART_MAIN);
    lv_obj_align_to(label, webToneSlider2, LV_ALIGN_OUT_LEFT_BOTTOM, -4, 0);                //Align below the first button
    lv_obj_add_event_cb(webToneSlider2, webToneAction, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(webToneSlider2, settingReleasedAction, LV_EVENT_RELEASED, NULL);

    webToneSlider3 = lv_slider_create(mainContainer);                            //Create a slider
    lv_obj_set_size(webToneSlider3, 18, 60);   //Set the size
    lv_obj_align_to(webToneSlider3, webToneSlider2, LV_ALIGN_OUT_RIGHT_TOP, 40, 0);                //Align below the first button
    //lv_slider_set_knob_in(webToneSlider3, true);
    lv_slider_set_range(webToneSlider3, 0, 13);                                            //Set the current value
    lv_slider_set_value(webToneSlider3, settings->vsTone & 0x0F, LV_ANIM_OFF);                                            //Set the current value
    lv_obj_add_style(webToneSlider3, &style_slider, LV_PART_MAIN);
    lv_obj_add_style(webToneSlider3, &style_sliderbg, LV_PART_INDICATOR);
    lv_obj_add_style(webToneSlider3, &style_knob, LV_PART_KNOB);
    label = lv_label_create(mainContainer); //First parameters (scr) is the parent
    lv_label_set_text(label, "Treble");  //Set the text
    lv_obj_add_style(label, &style_caption, LV_PART_MAIN);
    lv_obj_align_to(label, webToneSlider3, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);                //Align below the first button
    label = lv_label_create(mainContainer); //First parameters (scr) is the parent
    lv_label_set_text(label, "+6");  //Set the text
    lv_obj_add_style(label, &style_caption, LV_PART_MAIN);
    lv_obj_align_to(label, webToneSlider3, LV_ALIGN_OUT_LEFT_TOP, -4, 0);                //Align below the first button
    label = lv_label_create(mainContainer); //First parameters (scr) is the parent
    lv_label_set_text(label, "-6");  //Set the text
    lv_obj_add_style(label, &style_caption, LV_PART_MAIN);
    lv_obj_align_to(label, webToneSlider3, LV_ALIGN_OUT_LEFT_BOTTOM, -4, 0);                //Align below the first button
    lv_obj_add_event_cb(webToneSlider3, webToneAction, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(webToneSlider3, settingReleasedAction, LV_EVENT_RELEASED, NULL);
#endif

#ifndef FORCE_MONO
    lv_obj_t * wideLabel = lv_label_create(mainContainer);
    lv_label_set_text(wideLabel, "Stereo Wide");  //Set the text
#ifdef EQUALIZER
    lv_obj_align_to(wideLabel, toneLbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 66);
#else
    lv_obj_align_to(wideLabel, volDABLbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 16);
#endif
    wideSwitch = lv_switch_create(mainContainer);
    lv_obj_set_size(wideSwitch, 44, 20);
    if (settings->wide) lv_obj_add_state(wideSwitch, LV_STATE_CHECKED);
    else lv_obj_clear_state(wideSwitch, LV_STATE_CHECKED);
    lv_obj_align_to(wideSwitch, wideLabel, LV_ALIGN_OUT_RIGHT_MID, 20, 0);         //Align next to the slider
    lv_obj_add_event_cb(wideSwitch, wideAction, LV_EVENT_VALUE_CHANGED, NULL); 
#endif

    // Add the brightness slider
    lv_obj_t * brightLbl = lv_label_create(mainContainer); //First parameters (scr) is the parent
#ifndef FORCE_MONO
    lv_obj_align_to(brightLbl, wideLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 16);
#else
#ifdef EQUALIZER
    lv_obj_align_to(brightLbl, toneLbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 66);
#else
    lv_obj_align_to(wideLabel, volDABLbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 16);
#endif
#endif
    lv_label_set_text(brightLbl, "Brightness");  //Set the text
    brightSlider = lv_slider_create(mainContainer);                            //Create a slider
    lv_obj_set_size(brightSlider, width - 122, 18);   //Set the size
    lv_obj_align_to(brightSlider, brightLbl, LV_ALIGN_OUT_RIGHT_MID, 10, 0);         //Align next to the slider
    lv_slider_set_range(brightSlider, 8, 255);                                            //Set the current value
    lv_slider_set_value(brightSlider, settings->brightness, LV_ANIM_OFF);
    lv_obj_add_style(brightSlider, &style_slider, LV_PART_MAIN);
    lv_obj_add_style(brightSlider, &style_sliderbg, LV_PART_INDICATOR);
    lv_obj_add_style(brightSlider, &style_knob, LV_PART_KNOB);
    lv_obj_add_event_cb(brightSlider, brightnessAction, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(brightSlider, settingReleasedAction, LV_EVENT_RELEASED, NULL);

    //Autosave switch
    lv_obj_t * saveLabel = lv_label_create(mainContainer);
    lv_label_set_text(saveLabel, "Autosave Station Changes");  //Set the text
    lv_obj_align_to(saveLabel, brightLbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 16);
    saveSwitch = lv_switch_create(mainContainer);
    lv_obj_set_size(saveSwitch, 44, 20);
    if (settings->autosave) lv_obj_add_state(saveSwitch, LV_STATE_CHECKED);
    else lv_obj_clear_state(saveSwitch, LV_STATE_CHECKED);
    lv_obj_align_to(saveSwitch, saveLabel, LV_ALIGN_OUT_RIGHT_MID, 20, 0);         //Align next to the slider
    lv_obj_add_event_cb(saveSwitch, autosaveAction, LV_EVENT_VALUE_CHANGED, NULL); 

    // -- WIFI controls
      //The WIFI info container window
    wifiContainer = lv_obj_create(page);
    lv_obj_set_size(wifiContainer, width, 80);
    lv_obj_add_style(wifiContainer, &style_groupbox, LV_PART_MAIN);
    lv_obj_clear_flag(wifiContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align_to(wifiContainer, mainContainer, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);         //Align next to the slider
    //lv_obj_set_hidden(wifiContainer, true);

    //Create a network dropdown
    lv_obj_t * btn_label = lv_label_create(wifiContainer);
    lv_label_set_text(btn_label, "WIFI");
    lv_obj_set_pos(btn_label, 10, 10);                //Align below the first button
    wifiNetworkList = lv_dropdown_create(wifiContainer);
    lv_obj_align_to(wifiNetworkList, btn_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);         //Align next to the slider
    lv_obj_set_width(wifiNetworkList, width - 70);                //Align below the first button
    lv_obj_add_event_cb(wifiNetworkList, wifiSelectedAction, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(wifiNetworkList, wifiScanAction, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(wifiNetworkList, wifiLongPressAction, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_style(wifiNetworkList, &style_bg, LV_PART_MAIN);
    lv_dropdown_set_options(wifiNetworkList, settings->networks[settings->currentNetwork].ssid);
    
    wifiStatusLbl = lv_label_create(wifiContainer);
    lv_obj_align_to(wifiStatusLbl, btn_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 16);         //Align next to the slider
    //setWifiStatus(getWlanState());

    passwordEditText = lv_textarea_create(wifiContainer);
    lv_obj_set_size(passwordEditText, width - 70, 20);
    lv_obj_align_to(passwordEditText, wifiNetworkList, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 9);         //Align next to the slider
    lv_obj_add_style(passwordEditText, &style_ta, LV_PART_MAIN);
    lv_textarea_set_text_selection(passwordEditText, false);
    lv_textarea_set_one_line(passwordEditText, true);
    lv_textarea_set_text(passwordEditText, settings->networks[settings->currentNetwork].password);
    lv_obj_add_event_cb(passwordEditText, passwordEditAction, LV_EVENT_PRESSED, NULL);
    
    wifiDisconnectBtn = lv_btn_create(wifiContainer);
    lv_obj_add_event_cb(wifiDisconnectBtn, wifiDisconnectAction, LV_EVENT_CLICKED, NULL);
    lv_obj_add_style(wifiDisconnectBtn, &style_bg, LV_PART_MAIN);
    wifiDisconnectBtnLbl = lv_label_create(wifiDisconnectBtn);
    lv_label_set_text(wifiDisconnectBtnLbl, LV_SYMBOL_CLOSE);
    lv_obj_align_to(wifiDisconnectBtn, wifiNetworkList, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 10);         //Align next to the slider
    setPasswordVisibility(false, false);

    // -- FTP controls
    ftpContainer = lv_obj_create(page);
    lv_obj_set_size(ftpContainer, width, 116);
    lv_obj_add_style(ftpContainer, &style_groupbox, LV_PART_MAIN);
    lv_obj_clear_flag(ftpContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align_to(ftpContainer, wifiContainer, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);         //Align next to the slider
    lv_obj_set_hidden(ftpContainer, true);

    btn_label = lv_label_create(ftpContainer);
    lv_label_set_text(btn_label, "FTP");
    lv_obj_set_pos(btn_label, 10, 12);                //Align below the first button

    ftpServerText = lv_textarea_create(ftpContainer);
    lv_obj_set_size(ftpServerText, width - 70, 20);
    lv_obj_align_to(ftpServerText, btn_label, LV_ALIGN_OUT_RIGHT_MID, 16, 0);         //Align next to the slider
    lv_obj_add_style(ftpServerText, &style_ta, LV_PART_MAIN);
    lv_textarea_set_text_selection(ftpServerText, false);
    lv_textarea_set_one_line(ftpServerText, true);
    lv_obj_add_event_cb(ftpServerText, ftpServerAction, LV_EVENT_PRESSED, NULL);

    btn_label = lv_label_create(ftpContainer);
    lv_label_set_text(btn_label, "User");
    lv_obj_set_pos(btn_label, 10, 43);                //Align below the first button

    ftpUserText = lv_textarea_create(ftpContainer);
    lv_obj_set_size(ftpUserText, width - 70, 20);
    lv_obj_align_to(ftpUserText, ftpServerText, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);         //Align next to the slider
    lv_obj_add_style(ftpUserText, &style_ta, LV_PART_MAIN);
    lv_textarea_set_text_selection(ftpUserText, false);
    lv_textarea_set_one_line(ftpUserText, true);
    lv_obj_add_event_cb(ftpUserText, ftpUserAction, LV_EVENT_PRESSED, NULL);
    lv_textarea_set_max_length(ftpUserText, 31);

    btn_label = lv_label_create(ftpContainer);
    lv_label_set_text(btn_label, "Pass");
    lv_obj_set_pos(btn_label, 10, 74);                //Align below the first button

    ftpPassText = lv_textarea_create(ftpContainer);
    lv_obj_set_size(ftpPassText, width - 106, 20);
    lv_obj_align_to(ftpPassText, ftpUserText, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);         //Align next to the slider
    lv_obj_add_style(ftpPassText, &style_ta, LV_PART_MAIN);
    lv_textarea_set_text_selection(ftpPassText, false);
    lv_textarea_set_one_line(ftpPassText, true);
    lv_obj_add_event_cb(ftpPassText, ftpPassAction, LV_EVENT_PRESSED, NULL);
    lv_textarea_set_password_mode(ftpPassText, true);
    lv_textarea_set_max_length(ftpPassText, 31);

    lv_obj_t * passBtn = lv_btn_create(ftpContainer);
    lv_obj_add_event_cb(passBtn, ftpShowPassAction, LV_EVENT_CLICKED, NULL);
    lv_obj_add_style(passBtn, &style_bg, LV_PART_MAIN);
    lv_obj_add_style(passBtn, &style_biggerfont, LV_PART_MAIN);
    //lv_obj_set_size(passBtn, 30, 20);
    lv_obj_t * passBtnLbl = lv_label_create(passBtn);
    lv_label_set_text(passBtnLbl, LV_SYMBOL_EYE_OPEN);
    lv_obj_align_to(passBtn, ftpPassText, LV_ALIGN_OUT_RIGHT_MID, 6, 0);         //Align next to the slider

    ftpSetEditText();

    // -- DAB controls
      //The DAB Radio info container window
    dabContainer = lv_obj_create(page);
    lv_obj_set_size(dabContainer, width, 120);
    lv_obj_add_style(dabContainer, &style_groupbox, LV_PART_MAIN);
    lv_obj_clear_flag(dabContainer, LV_OBJ_FLAG_SCROLLABLE);
#ifdef USE_OTA  
    lv_obj_align_to(dabContainer, wifiContainer, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);         //Align next to the slider
#else
    lv_obj_align_to(dabContainer, ftpContainer, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);         //Align next to the slider
#endif
    lv_obj_set_hidden(dabContainer, true);

        //Create a scan button
    lv_obj_t * scanBtn = lv_btn_create(dabContainer);
    lv_obj_set_pos(scanBtn, 10, 10);                //Align below the first button
    lv_obj_add_event_cb(scanBtn, dabScanAction, LV_EVENT_CLICKED, NULL);
    lv_obj_add_style(scanBtn, &style_bg, LV_PART_MAIN);
    btn_label = lv_label_create(scanBtn);
    lv_label_set_text(btn_label, "DAB Scan");

    dabScanLabel = lv_label_create(dabContainer);
    lv_obj_align_to(dabScanLabel, scanBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);         //Align next to the slider
    lv_obj_set_width(dabScanLabel, 155);
    lv_label_set_text(dabScanLabel, "");

    //Loading spinner
    scanSpinner = lv_spinner_create(dabContainer, 1000, 60);
    lv_obj_set_size(scanSpinner, 25, 25);
    lv_obj_align_to(scanSpinner, dabScanLabel, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_set_hidden(scanSpinner, true);

    // ADD A DROP DOWN LIST
    lv_obj_t * eqLbl = lv_label_create(dabContainer); //First parameters (scr) is the parent
    lv_obj_align_to(eqLbl, scanBtn, LV_ALIGN_OUT_BOTTOM_LEFT, 5, 14);         //Align next to the slider
    lv_label_set_text(eqLbl, "DAB DSP");  //Set the text

    dabEqList = lv_dropdown_create(dabContainer);            //Create a drop down list
    lv_obj_align_to(dabEqList, eqLbl, LV_ALIGN_OUT_RIGHT_MID, 20, 0);         //Align next to the slider
    lv_obj_set_size(dabEqList, 114, 22);
    lv_obj_add_style(dabEqList, &style_bg, LV_PART_MAIN);
    lv_dropdown_set_options(dabEqList, "EQ Off\nBass Boost\nJazz\nLive\nVocal\nAcoustic"); //Set the options
    lv_dropdown_set_selected(dabEqList, settings->dabEQ);
    lv_obj_add_event_cb(dabEqList, dabEqAction, LV_EVENT_VALUE_CHANGED, NULL);                         //Set function to call on new option is chosen
    lv_obj_add_event_cb(dabEqList, dabOpenEqAction, LV_EVENT_CLICKED, NULL);

    //Mono switch
    lv_obj_t * monoLabel = lv_label_create(dabContainer);
    lv_label_set_text(monoLabel, "Force FM Mono Reception");  //Set the text
    lv_obj_align_to(monoLabel, eqLbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 16);
    monoSwitch = lv_switch_create(dabContainer);
    lv_obj_set_size(monoSwitch, 44, 20);
    if (settings->mono) lv_obj_add_state(monoSwitch, LV_STATE_CHECKED);
    else lv_obj_clear_state(monoSwitch, LV_STATE_CHECKED);
    lv_obj_align_to(monoSwitch, monoLabel, LV_ALIGN_OUT_RIGHT_MID, 20, 0);         //Align next to the slider
    lv_obj_add_event_cb(monoSwitch, monoAction, LV_EVENT_VALUE_CHANGED, NULL); 


    //Time settings
    timeContainer = lv_obj_create(page);
    lv_obj_set_size(timeContainer, width, 80);
    lv_obj_align_to(timeContainer, dabContainer, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);         //Align next to the slider
    lv_obj_add_style(timeContainer, &style_groupbox, LV_PART_MAIN);
    lv_obj_clear_flag(timeContainer, LV_OBJ_FLAG_SCROLLABLE);

    dstLabel = lv_label_create(timeContainer);
    lv_obj_set_pos(dstLabel, 10, 10);                //Align below the first button
    //lv_obj_set_width(dstLabel, 155);
    lv_label_set_text(dstLabel, "Daylight Savings Time");

    dstSwitch = lv_switch_create(timeContainer);
    lv_obj_set_size(dstSwitch, 44, 20);
    if (settings->dst) lv_obj_add_state(dstSwitch, LV_STATE_CHECKED);
    else lv_obj_clear_state(dstSwitch, LV_STATE_CHECKED);
    lv_obj_align_to(dstSwitch, dstLabel, LV_ALIGN_OUT_RIGHT_MID, 20, 0);         //Align next to the slider
    lv_obj_add_event_cb(dstSwitch, dstAction, LV_EVENT_VALUE_CHANGED, NULL); 
 
    tzLabel = lv_label_create(timeContainer);
    lv_obj_align_to(tzLabel, dstLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 12);         //Align next to the slider
    lv_label_set_text(tzLabel, "Time Zone");

    
    tzEditText = lv_textarea_create(timeContainer);
    lv_obj_set_size(tzEditText, width - 130, 20);
    lv_obj_align_to(tzEditText, tzLabel, LV_ALIGN_OUT_RIGHT_MID, 20, 0);         //Align next to the slider
    lv_obj_add_style(tzEditText, &style_ta, LV_PART_MAIN);
    lv_obj_add_event_cb(tzEditText, tzEditAction, LV_EVENT_PRESSED, NULL);
    lv_textarea_set_one_line(tzEditText, true);
    lv_textarea_set_text(tzEditText, settings->tz);
    
}

void setSettingsVisibility() {
  lv_obj_t* above = mainContainer;
#ifndef USE_OTA  
  bool showWifi = (settings->mode == MODE_WEB || settings->mode == MODE_FTP || settings->mode == MODE_POD || settings->mode == MODE_DLNA);
#else
  bool showWifi = true;
#endif
  lv_obj_set_hidden(wifiContainer, !showWifi);
  above = showWifi?wifiContainer:above;
#ifdef MONKEYBOARD
  bool showDab = (settings->mode == MODE_DAB || settings->mode == MODE_FM);
  lv_obj_set_hidden(dabContainer, !showDab);
  lv_obj_align_to(dabContainer, above, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);         //Align next to the slider
  above = showDab?dabContainer:above;
#endif
  bool showFtp = (settings->mode == MODE_FTP);
  lv_obj_set_hidden(ftpContainer, !showFtp);
  lv_obj_align_to(ftpContainer, above, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);         //Align next to the slider
  above = showFtp?ftpContainer:above;
  lv_obj_align_to(timeContainer, above, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);
}

//-----------------------------------------------------------------------
// main container actions

void dabVolumeAction(lv_event_t * event) {
  settings->dabVolume = lv_slider_get_value(dabVolSlider); 
#if defined(MONKEYBOARD) || defined(NXP6686)
  setVolume(settings->dabVolume);
#else
  setDspVolume(settings->dabVolume);
#endif  
}

void wideAction(lv_event_t * event) {
  settings->wide = lv_obj_has_state(wideSwitch, LV_STATE_CHECKED);
  setStereoWide(settings->wide);
  writeSettings();
}

void settingReleasedAction(lv_event_t * event) {
  writeSettings();
}

void webToneAction(lv_event_t * event) {
  settings->vsTone = formatVSTone(lv_slider_get_value(webToneSlider1), lv_slider_get_value(webToneSlider2), lv_slider_get_value(webToneSlider3)); 
  setVSTone(settings->vsTone);
}

void autosaveAction(lv_event_t * event) {
  settings->autosave = lv_obj_has_state(saveSwitch, LV_STATE_CHECKED);
  writeSettings();
}

void brightnessAction(lv_event_t * event) {
  settings->brightness = lv_slider_get_value(brightSlider); 
  setBrightness(settings->brightness);
}

void monoAction(lv_event_t * event) {
  settings->mono = lv_obj_has_state(monoSwitch, LV_STATE_CHECKED);
#ifdef MONKEYBOARD
  setStereoMode(!settings->mono);
#endif  
  writeSettings();
}

//-----------------------------------------------------------------------
// WIFI container actions

void wifiDisconnectAction(lv_event_t * event) {
  int state = WiFi.status();
  if (state != WL_CONNECTED) {
    wlanConnect();
    if (settings->mode == MODE_WEB) connectToHost(settings->server, true); 
  } else {
    WiFi.disconnect(true, true);
  }
}

void setWifiStatus(int state) {
#ifndef USE_OTA  
  if ((settings->mode != MODE_WEB && settings->mode != MODE_FTP && settings->mode != MODE_POD && settings->mode != MODE_DLNA) || editWifiPassword) return;
#endif
  if (state < 0 || state > 6) state = 0;  //Eh? Whats that? WiFi must be idle..
  if (wifiStatusLbl) {
    lv_label_set_text_fmt(wifiStatusLbl, "Status: %s", wlanStateString[state]);  //Set the text
    if (progNowLbl && (settings->mode == MODE_WEB || settings->mode == MODE_FTP || settings->mode == MODE_POD || settings->mode == MODE_DLNA))
      info(NOW, 2, LV_SYMBOL_WIFI " WiFi Status: %s", wlanStateString[state]);  //Set the text
  }
  if (state != WL_CONNECTED) {
    if (wifiDisconnectBtnLbl && wifiNetworkList) {
      lv_label_set_text(wifiDisconnectBtnLbl, LV_SYMBOL_REFRESH);
      lv_obj_align_to(wifiDisconnectBtn, wifiNetworkList, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 10);         //Align next to the slider
    }
    setSigStrengthLbl(LV_SYMBOL_WIFI " ?");   
  } else {
    if (wifiDisconnectBtnLbl && wifiNetworkList) {
      lv_label_set_text(wifiDisconnectBtnLbl, LV_SYMBOL_CLOSE);
      lv_obj_align_to(wifiDisconnectBtn, wifiNetworkList, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 10);         //Align next to the slider
    }
  }
}

bool wifiLongPressed = false;
void wifiLongPressAction(lv_event_t * event) {
  lv_obj_t * dropdown = lv_event_get_target(event);
  lv_obj_t * ddlist = lv_dropdown_get_list(dropdown);
  if (!ddlist) {  
    if (passwordEditText) lv_textarea_set_text(passwordEditText, settings->networks[settings->currentNetwork].password);
    setPasswordVisibility(!editWifiPassword, true);
    wifiLongPressed = true;
  }
}

void closeNetworkDropdown() {
  lv_dropdown_close(wifiNetworkList);
}

void wifiScanAction(lv_event_t * event) {
  if (wifiLongPressed) {
    wifiLongPressed = false;
    lv_dropdown_close(wifiNetworkList);
    return;
  }
  lv_obj_t * dropdown = lv_event_get_target(event);
  lv_obj_t * ddlist = lv_dropdown_get_list(dropdown);
  lv_state_t state = lv_obj_get_state(dropdown);
  if (ddlist && (state & LV_STATE_CHECKED)) {  
    serial.println("Starting WIFI AP Scan..");
    lv_dropdown_close(wifiNetworkList);
    wifiBeginScan();
  }
}

void wifiScanMessage(const char* message) {
  if (!wifiNetworkList) return;
  lv_dropdown_set_options(wifiNetworkList, message);
  lv_dropdown_close(wifiNetworkList);
}

void wifiScanEntry(int index, const char* entry) {
  if (!wifiNetworkList) return;
  if (index <= 1) {
    setPasswordVisibility(false, false);
    lv_dropdown_clear_options(wifiNetworkList);
  }
  if (!entry || !strlen(entry)) return;
  char str[32];
  bool us = (strcasecmp(entry, settings->networks[settings->currentNetwork].ssid) == 0);
  snprintf(str, 31, "%s%s", us? LV_SYMBOL_WIFI " ":"", entry); 
  lv_dropdown_add_option(wifiNetworkList, str, LV_DROPDOWN_POS_LAST);
    if (us) lv_dropdown_set_selected(wifiNetworkList, index - 1);
  lv_dropdown_open(wifiNetworkList);
}

void wifiSelectedAction(lv_event_t * event) {
  char name[28];
  lv_dropdown_get_selected_str(wifiNetworkList, name, 27);
  bool known = false;
  bool added = false;
  for(int net = 0; net < 4; net++) {
    if (strcmp(settings->networks[net].ssid, name) == 0) {
      if (net != settings->currentNetwork) {
        settings->currentNetwork = net;
        writeSettings();
        serial.print("Known network:");
        serial.println(net);
        
      } else { 
        serial.print("Same network:");
        serial.println(net);
      }
      wlanConnect();
      if (settings->mode == MODE_WEB) connectToHost(settings->server, true); 
      known = true;
      added = true;
      break;
    }
  }
  if (!known) {
    for(int net = 0; net < 4; net++) {
      if (strcasecmp(settings->networks[net].ssid, "<Empty>") == 0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
        strncpy(settings->networks[net].ssid, name, 27);
#pragma GCC diagnostic pop
        settings->networks[net].ssid[27] = '\0';
        strcpy(settings->networks[settings->currentNetwork].password, "");
        settings->currentNetwork = net;
        writeSettings();
        wlanConnect();
        if (settings->mode == MODE_WEB) connectToHost(settings->server, true); 
        serial.print("New network:");
        serial.println(net);
        added = true;
        break;
        //c
      }  
    }
  }
  if (!added) {
    if (++settings->currentNetwork >= 4) settings->currentNetwork = 0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
    strncpy(settings->networks[settings->currentNetwork].ssid, name, 27);
#pragma GCC diagnostic pop
    settings->networks[settings->currentNetwork].ssid[27] = '\0';
    strcpy(settings->networks[settings->currentNetwork].password, "");
    writeSettings();
    wlanConnect();
    if (settings->mode == MODE_WEB) connectToHost(settings->server, true); 
        serial.print("Networks Full:");
        serial.println(settings->currentNetwork);
  }
  if (passwordEditText) 
    lv_textarea_set_text(passwordEditText, settings->networks[settings->currentNetwork].password);
}

#define PASSWORD_SHOW_TIME  5000  //five secs

uint64_t passwordCountdown = 0;
void beginPasswordCountdown() {
  passwordCountdown = millis() + PASSWORD_SHOW_TIME;
}

void passwordHandle() {
  if (!keyboardShowing() && passwordCountdown && passwordCountdown < millis()) {
    setPasswordVisibility(false, false);
  }
}

void setPasswordVisibility(bool visible, bool timed) {
  if ((editWifiPassword = visible)) {
    if (wifiStatusLbl) lv_label_set_text(wifiStatusLbl, "PASS");
    if (wifiDisconnectBtn) lv_obj_add_flag(wifiDisconnectBtn, LV_OBJ_FLAG_HIDDEN);
    if (passwordEditText) lv_obj_clear_flag(passwordEditText, LV_OBJ_FLAG_HIDDEN);
    if (timed) beginPasswordCountdown();
  } else {
    setWifiStatus(WiFi.status());
    if (passwordEditText) lv_obj_add_flag(passwordEditText, LV_OBJ_FLAG_HIDDEN);
    if (wifiDisconnectBtn) lv_obj_clear_flag(wifiDisconnectBtn, LV_OBJ_FLAG_HIDDEN);
    passwordCountdown = 0;
  }
}

void passwordEditAction(lv_event_t * event) {
  if (!keyboardShowing()) {
    keyboardShow(lv_scr_act(), passwordEditText, keyboardPasswordKeyAction);
    beginPasswordCountdown();
  }
}

void keyboardPasswordKeyAction(lv_event_t * event) {
  uint32_t res = lv_event_get_code(event);
  if(res == LV_EVENT_READY || res == LV_EVENT_CANCEL){
    if (keyboardShowing()) keyboardHide(true, NULL);
    if(res == LV_EVENT_READY) {
      setPasswordVisibility(false, false);
      strncpy(settings->networks[settings->currentNetwork].password, lv_textarea_get_text(passwordEditText), 31); 
      settings->networks[settings->currentNetwork].password[31] = '\0';
      writeSettings();
      wlanConnect();
      if (settings->mode == MODE_WEB) connectToHost(settings->server, true); 
    }
    else if (res == LV_EVENT_CANCEL){
      lv_textarea_set_text(passwordEditText, settings->networks[settings->currentNetwork].password);  
    }
  }
  beginPasswordCountdown();
}


void wifiScanComplete() {
  serial.println("WIFI AP Scan Finished.");
}

//-----------------------------------------------------------------------
// FTP Container actions
bool ftpPassHidden = true;

void ftpSetEditText() {
  IPAddress ip = settings->ftpAddress;
  char buf[20];
  snprintf(buf, 19, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  lv_textarea_set_text(ftpServerText, buf);  
  lv_textarea_set_text(ftpUserText, settings->ftpUser);  
  lv_textarea_set_text(ftpPassText, settings->ftpPass);  
}

void ftpServerAction(lv_event_t * event) {
  if (!keyboardShowing()) {
    keyboardShow(lv_scr_act(), ftpServerText, keyboardFtpKeyAction);
  }
}

void ftpUserAction(lv_event_t * event) {
  if (!keyboardShowing()) {
    keyboardShow(lv_scr_act(), ftpUserText, keyboardFtpKeyAction);
  }
}

void ftpPassAction(lv_event_t * event) {
  if (!keyboardShowing()) {
    keyboardShow(lv_scr_act(), ftpPassText, keyboardFtpKeyAction);
  }
}


void ftpShowPassAction(lv_event_t * event) {
  lv_textarea_set_password_mode(ftpPassText, (ftpPassHidden = !ftpPassHidden));
}

void keyboardFtpKeyAction(lv_event_t * event) {
  uint32_t res = lv_event_get_code(event);
  if(res == LV_EVENT_READY || res == LV_EVENT_CANCEL){
    if (keyBoard) keyboardHide(true, NULL);
    if(res == LV_EVENT_READY) {
      IPAddress ip;
      if (ip.fromString(lv_textarea_get_text(ftpServerText))) {
        settings->ftpAddress = ip;
        strncpy(settings->ftpUser, lv_textarea_get_text(ftpUserText), 31);
        settings->ftpUser[31] = '\0';
        strncpy(settings->ftpPass, lv_textarea_get_text(ftpPassText), 31);
        settings->ftpPass[31] = '\0';
        writeSettings();
        lv_textarea_set_password_mode(ftpPassText, (ftpPassHidden = true));
        ftpPopulateBrowser(NULL);
      } else ftpSetEditText();
    }
    else if (res == LV_EVENT_CANCEL){
      ftpSetEditText();
    }
  }
}



//-----------------------------------------------------------------------
// DAB container actions

void dabScanAction(lv_event_t * event) {
  if (dabScanLabel) lv_label_set_text(dabScanLabel, "Starting..");
  //startDabScan();
}

void dabEqAction(lv_event_t * event) {
  lv_obj_t * obj = lv_event_get_target(event);
  uint16_t opt = lv_dropdown_get_selected(obj);      //Get the id of selected option
  settings->dabEQ = opt; 
  //setDabEQ(opt);
  writeSettings();
}

void dabOpenEqAction(lv_event_t * event) {
  lv_obj_t * dropdown = lv_event_get_target(event);
  lv_obj_t * ddlist = lv_dropdown_get_list(dropdown);
  lv_state_t state = lv_obj_get_state(dropdown);
  if (ddlist && (state & LV_STATE_CHECKED)) {  
    lv_obj_add_style(ddlist, &style_bigfont, LV_PART_MAIN);
  }
}


void setDabScanLabel(const char* buf) { 
  if (dabScanLabel) lv_label_set_text(dabScanLabel, buf);
}

void hideScanSpinner(bool hide) {
  if (scanSpinner) lv_obj_set_hidden(scanSpinner, hide);
} 

//-----------------------------------------------------------------------
// Time container actions

void dstAction(lv_event_t * event) {
  settings->dst = lv_obj_has_state(dstSwitch, LV_STATE_CHECKED);
  writeSettings();
  NTPUpdate();
}

void tzEditAction(lv_event_t * event) {
  if (!keyboardShowing()) {
    keyboardShow(lv_scr_act(), tzEditText, keyboardTzKeyAction);
  }
}

void keyboardTzKeyAction(lv_event_t * event) {
  uint32_t res = lv_event_get_code(event);
  if(res == LV_EVENT_READY || res == LV_EVENT_CANCEL){
    if (keyboardShowing()) keyboardHide(true, NULL);
    if(res == LV_EVENT_READY) {
      strncpy(settings->tz, lv_textarea_get_text(tzEditText), 63); 
      settings->tz[63] = '\0';
      writeSettings();
    }
    else if (res == LV_EVENT_CANCEL){
      lv_textarea_set_text(tzEditText, settings->tz);  
    }
  }
}

