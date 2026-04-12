#include "decls.h"

bool tabViewScrolling = false;

//Mode lists
uint8_t MODE_UNKNOWN, MODE_DAB, MODE_FM, MODE_FTP, MODE_WEB, MODE_SD, MODE_POD, MODE_DLNA, MODE_LINE, MODE_BT,
        MODE_NLW, MODE_NMW, MODE_NSW, MODE_NFM;
const char * modeString[14];
int totalModes;
char modeListLine[256];
uint8_t mainWindowIndex = 2;

//Construct the modelists based on available functionality
void setupModes() {
  totalModes = 0;
  //Mode 0 is reserved to catch null mode errors
  char* list = modeListLine;
  modeString[totalModes] = "Unknown";
  MODE_UNKNOWN = totalModes++;
#ifdef MONKEYBOARD
  //DAB+ Radio on monkeyboard module
  list = strcat(list, LV_SYMBOL_PLUS " DAB Radio\n");
  modeString[totalModes] = "DAB+";
  MODE_DAB = totalModes++;
  //FM radio from monkeyboard
  list = strcat(list, LV_SYMBOL_AUDIO " Digital FM\n");
  modeString[totalModes] = "Digital FM";
  MODE_FM = totalModes++;
#endif
#ifdef NXP6686
  //AM radio from NXP6686
  list = strcat(list, LV_SYMBOL_AUDIO " AM Radio\n");
  modeString[totalModes] = "AM Radio";
  MODE_NMW = totalModes++;
  //FM radio from NXP6686
  list = strcat(list, LV_SYMBOL_AUDIO " FM Radio\n");
  modeString[totalModes] = "FM Radio";
  MODE_NFM = totalModes++;
  //SW radio from NXP6686
  list = strcat(list, LV_SYMBOL_AUDIO " Shortwave\n");
  modeString[totalModes] = "Shortwave Radio";
  MODE_NSW = totalModes++;
  //LW radio from NXP6686
  list = strcat(list, LV_SYMBOL_AUDIO " Longwave\n");
  modeString[totalModes] = "Longwave Radio";
  MODE_NLW = totalModes++;
#endif
  //Web radio
  list = strcat(list, LV_SYMBOL_BROADCAST " Web Radio\n");
  modeString[totalModes] = "Web Radio";
  MODE_WEB = totalModes++;
  //Local FTP streaming
  list = strcat(list, LV_SYMBOL_DOWNLOAD " Local FTP\n");
  modeString[totalModes] = "Local FTP";
  MODE_FTP = totalModes++;
#ifdef SDPLAYER
  //SD Card file player
  list = strcat(list, LV_SYMBOL_SD_CARD " SD Card\n");
  modeString[totalModes] = "SD Card";
  MODE_SD = totalModes++;
#endif
  //Podcast client
  list = strcat(list, LV_SYMBOL_BULLHORN "  Podcasts\n");
  modeString[totalModes] = "Podcast";
  MODE_POD = totalModes++;
  //DLNA Client
  list = strcat(list, LV_SYMBOL_DRIVE " Media Svr");
  modeString[totalModes] = "Media Server";
  MODE_DLNA = totalModes++;
#ifdef LINEIN
  //Line in function
  list = strcat(list, "\n" LV_SYMBOL_AUDIO " Line In");
  modeString[totalModes] = "Line In";
  MODE_LINE = totalModes++;
#endif
#ifdef BLUETOOTH
  //Bluetooth sink
  list = strcat(list, "\n" LV_SYMBOL_BLUETOOTH " Bluetooth");
  modeString[totalModes] = "Bluetooth";
  MODE_BT = totalModes++;
#endif
  serial.printf("> Configured %d modes.\r\n", totalModes - 1);
}


//Top level objects
lv_obj_t * modeList;
lv_obj_t * functionLbl;
lv_obj_t * battChgLbl;
lv_obj_t * sigStrengthLbl;
lv_obj_t * stmoLbl;
lv_obj_t * timeLbl;
lv_obj_t * sdcardLbl;
lv_obj_t * factoryBtn = NULL;

//Shared styles
lv_style_t style_wp;
lv_style_t style_win;
lv_style_t style_transp;
lv_style_t style_halfopa;
lv_style_t style_ta;
lv_style_t style_groupbox;
lv_style_t style_bigfont;
lv_style_t style_list;
lv_style_t style_listsel;
lv_style_t style_digitalfont;
lv_style_t style_biggerfont;
lv_style_t style_biggestfont;
lv_style_t style_bigfont_orange;

//The keyboard
lv_obj_t * keyBoard = NULL;

//Top-level shared objects
lv_obj_t * tabView;
lv_obj_t * vuMeter;
lv_obj_t * fftMeter;
lv_obj_t * stationsTab;

//Keyboard styles
lv_style_t style_kb;
lv_style_t style_kb_rel;
lv_style_t style_kb_pr;

void dabModeAction(lv_event_t * event);
void modeListActivated(lv_event_t * event);
void tabViewAction(lv_event_t * event);
void keyboardShownAction(lv_anim_t * a);
void keyboardHiddenAction(lv_anim_t * a);
void factoryModeActivated(lv_event_t * event);
void createFactoryWindow();
void loglvlCloseDropdown();

bool factoryMode = false;

//Init the screen and build the GUI windows
void screenInit(void) {
   //Initialize the theme (this will need the extra fonts in future LVGL versions)
#ifndef THEME_HIVIS
   lv_theme_t * th = lv_theme_default_init(NULL,  //Use the DPI, size, etc from default display 
#ifdef THEME_BLUE
                                          lv_palette_main(LV_PALETTE_LIGHT_BLUE), lv_palette_main(LV_PALETTE_CYAN),   //Primary and secondary palette
#else
                                          lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_GREEN),   //Primary and secondary palette
#endif
                                          true,    //Light or dark mode 
                                          &lv_font_terminal12); //Small, normal, large fonts
#else
  lv_theme_t * th = lv_theme_mono_init(NULL,  //Use the DPI, size, etc from default display 
                                          true,    //Light or dark mode 
                                          &lv_font_terminal12); //Small, normal, large fonts
#endif                                          

  lv_disp_set_theme(NULL, th); //Assign the theme to the default display

  //Style of the wallpaper
  lv_style_init(&style_wp);
#ifdef THEME_BLUE
  lv_style_set_bg_color(&style_wp, lv_color_hex(0x707070));
#elif defined(THEME_HIVIS)
  lv_style_set_bg_color(&style_wp, lv_color_black());
#else
  lv_style_set_bg_color(&style_wp, lv_color_hex(0x404040));
#endif
  lv_style_set_bg_grad_color(&style_wp, lv_color_black());
  lv_style_set_text_color(&style_wp, lv_color_white());
  lv_style_set_bg_grad_dir(&style_wp, LV_GRAD_DIR_VER);
  lv_obj_add_style(lv_scr_act(), &style_wp, 0);

  //Create a wallpaper on the screen
#ifdef WALLPAPER
  LV_IMG_DECLARE(wallpaper);
  lv_obj_t * wp = lv_img_create(lv_scr_act());
  //lv_obj_set_protect(wp, LV_PROTECT_PARENT);          //Don't let to move the wallpaper by the layout 
  lv_obj_set_parent(wp, lv_scr_act());
  lv_img_set_src(wp, &wallpaper);//bgimg);
  lv_obj_set_pos(wp, 0, 33);
  //lv_img_set_auto_size(wp, false);
#endif

  //Setup some styles
  lv_style_init(&style_bigfont);
#ifdef THEME_HIVIS  
  lv_style_set_text_font(&style_bigfont, &lv_font_terminal12);
#else
  lv_style_set_text_font(&style_bigfont, &lv_font_montserrat_14);
#endif
  lv_style_init(&style_biggerfont);
  lv_style_set_text_font(&style_biggerfont, &lv_font_montserrat_16);
  lv_style_init(&style_biggestfont);
  lv_style_set_text_font(&style_biggestfont, &lv_font_mymontserrat_20);
  lv_style_init(&style_bigfont_orange);
  lv_style_set_text_font(&style_bigfont_orange, &lv_font_mymontserrat_20);
#ifdef THEME_BLUE
  lv_style_set_text_color(&style_bigfont_orange, lv_palette_main(LV_PALETTE_LIGHT_BLUE));
#elif defined(THEME_HIVIS)
  lv_style_set_text_color(&style_bigfont_orange, lv_color_hex(0x00ffff));
#else
  lv_style_set_text_color(&style_bigfont_orange, lv_color_hex(0xff1010));
#endif
  lv_style_init(&style_halfopa);
  lv_style_set_bg_opa(&style_halfopa, LV_OPA_50);
  lv_style_init(&style_groupbox);
  lv_style_set_bg_color(&style_groupbox, lv_color_hex(0x0));//487fb7);
  lv_style_set_bg_grad_color(&style_groupbox, lv_color_hex(0x0));//487fb7);
  lv_style_set_bg_opa(&style_groupbox, LV_OPA_50);
  lv_style_set_text_color(&style_groupbox, lv_color_white());
  lv_style_set_border_width(&style_groupbox, 3);
#ifdef THEME_BLUE
  lv_style_set_border_color(&style_groupbox, lv_color_hex(0x202020));
#else
  lv_style_set_border_color(&style_groupbox, lv_color_hex(0x303030));
#endif
  lv_style_init(&style_win);
  lv_style_set_text_font(&style_win, &lv_font_mymontserrat_20);
  lv_style_set_bg_opa(&style_win, LV_OPA_TRANSP);
#ifdef THEME_HIVIS
  lv_style_set_bg_color(&style_win, lv_color_black());
#else
  lv_style_set_bg_color(&style_win, lv_color_hex(0x101010));
#endif
  lv_style_set_border_width(&style_win, 0);
  lv_style_set_pad_top(&style_win, 0);
  lv_style_set_pad_bottom(&style_win, 0);
  lv_style_set_pad_left(&style_win, 0);
  lv_style_set_pad_right(&style_win, 0);
  lv_style_init(&style_list);
  lv_style_set_bg_opa(&style_list, LV_OPA_50);
  //lv_style_set_text_font(&style_list, &lv_font_montserrat_16);
  lv_style_init(&style_listsel);
  lv_style_set_radius(&style_listsel, 6);
  lv_style_set_bg_color(&style_listsel, lv_color_hex(0x0));//487fb7);
#ifdef THEME_HIVIS
  lv_style_set_bg_opa(&style_listsel, LV_OPA_100);
#else  
  lv_style_set_bg_opa(&style_listsel, LV_OPA_70);
#endif
  lv_style_set_border_width(&style_listsel, 2);
#ifdef THEME_BLUE  
  lv_style_set_border_color(&style_listsel, lv_color_hex(0x487fb7));
#else
  lv_style_set_border_color(&style_listsel, lv_color_hex(0xFF7F50));
#endif
  lv_style_set_border_side(&style_listsel, LV_BORDER_SIDE_FULL);
  lv_style_init(&style_ta);
  lv_style_set_bg_color(&style_ta, lv_color_black());
  lv_style_set_bg_grad_color(&style_ta, lv_color_black());
  lv_style_set_bg_opa(&style_ta, LV_OPA_100);
  lv_style_set_radius(&style_ta, 3);
  lv_style_set_border_width(&style_ta, 3);
#ifdef THEME_BLUE
  lv_style_set_border_color(&style_ta, lv_color_hex(0x487fb7));
#else
  lv_style_set_border_color(&style_ta, lv_color_hex(0xFF7F50));
#endif
  lv_style_set_text_color(&style_ta, lv_color_white());
  lv_style_set_text_font(&style_ta, &lv_font_montserrat_16);
  lv_style_set_pad_top(&style_ta, 6);
  lv_style_set_pad_bottom(&style_ta, 8);
  lv_style_set_pad_left(&style_ta, 8);
  lv_style_set_pad_right(&style_ta, 8);
  lv_style_init(&style_digitalfont);
  lv_style_set_text_color(&style_digitalfont, lv_color_hex(0xFF0000));
  lv_style_set_text_font(&style_digitalfont, &digital);
  
  //Keyboard style
  lv_style_init(&style_kb);
  lv_style_set_bg_color(&style_kb, lv_color_black());
  lv_style_set_bg_grad_color(&style_kb, lv_color_black());
  lv_style_set_bg_grad_dir(&style_kb, LV_GRAD_DIR_VER);
  lv_style_set_text_font(&style_kb, &lv_font_mymontserrat_20);
  lv_style_set_bg_opa(&style_kb, LV_OPA_70);
  lv_style_set_pad_top(&style_kb, 0);
  lv_style_set_pad_bottom(&style_kb, 0);
  lv_style_set_pad_left(&style_kb, 0);
  lv_style_set_pad_right(&style_kb, 0);
  lv_style_init(&style_kb_rel);
  lv_style_set_bg_color(&style_kb_rel, lv_color_hex3(0x333));
  lv_style_set_bg_grad_color(&style_kb_rel, lv_color_hex3(0x333));
  lv_style_set_radius(&style_kb_rel, 0);
  lv_style_set_border_width(&style_kb_rel, 1);
  lv_style_set_border_color(&style_kb_rel, lv_color_hex(0xC0C0C0));
  lv_style_set_border_opa(&style_kb_rel, LV_OPA_50);
  lv_style_set_text_color(&style_kb_rel, lv_color_white());
  lv_style_init(&style_kb_pr);
  lv_style_set_radius(&style_kb_pr, 0);
  lv_style_set_bg_opa(&style_kb_pr, LV_OPA_50);
  lv_style_set_bg_color(&style_kb_pr, lv_color_white());
  lv_style_set_bg_grad_color(&style_kb_pr, lv_color_white());
  lv_style_set_border_width(&style_kb_pr, 1);
  lv_style_set_border_color(&style_kb_pr, lv_color_hex(0xC0C0C0));

  // Top line mode (dab, web, sd..) dropdown list
  modeList = lv_dropdown_create(lv_scr_act());            //Create a drop down list
  lv_obj_add_style(modeList, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(modeList, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_set_pos(modeList, 2, 2);  
  lv_dropdown_set_options(modeList, modeListLine); //Set the options
  lv_dropdown_set_selected(modeList, settings->mode - 1);
  lv_obj_set_size(modeList, 170, 36);
  lv_obj_add_event_cb(modeList, dabModeAction, LV_EVENT_VALUE_CHANGED, NULL);                         //Set function to call on new option is chosen
  lv_obj_add_event_cb(modeList, modeListActivated, LV_EVENT_CLICKED, NULL);
  lv_obj_add_flag(modeList, LV_OBJ_FLAG_HIDDEN);

  //Top line labels
  functionLbl = lv_label_create(lv_scr_act()); //First parameters (scr) is the parent
  lv_obj_set_pos(functionLbl, 8, 8);  
  lv_label_set_text(functionLbl, LV_SYMBOL_AUDIO " " RADIONAME);
  lv_obj_add_style(functionLbl, &style_biggerfont, LV_PART_MAIN);
  sdcardLbl = lv_label_create(lv_scr_act()); //First parameters (scr) is the parent
  lv_label_set_text(sdcardLbl, "");
  lv_obj_add_style(sdcardLbl, &style_biggerfont, LV_PART_MAIN);
#ifdef SDPLAYER
  lv_obj_align(sdcardLbl, LV_ALIGN_TOP_RIGHT, -8, 8);  //Align in top right corner
#else
  lv_obj_align(sdcardLbl, LV_ALIGN_TOP_RIGHT, 0, 8);  //No size
#endif
  battChgLbl = lv_label_create(lv_scr_act()); //First parameters (scr) is the parent
  lv_label_set_text(battChgLbl, "");
  lv_obj_add_style(battChgLbl, &style_biggerfont, LV_PART_MAIN);
#ifdef BATTERYMON
  lv_obj_align_to(battChgLbl, sdcardLbl, LV_ALIGN_OUT_LEFT_MID, -10, 0);  //next to SDCARD
#else
  lv_obj_align_to(battChgLbl, sdcardLbl, LV_ALIGN_OUT_LEFT_MID, 0, 0);  //No size
#endif
  sigStrengthLbl = lv_label_create(lv_scr_act()); //First parameters (scr) is the parent
  lv_label_set_text(sigStrengthLbl, "");
#if (TFT_WIDTH == 480)
  lv_obj_add_style(sigStrengthLbl, &style_biggerfont, LV_PART_MAIN);
  lv_obj_align_to(sigStrengthLbl, battChgLbl, LV_ALIGN_OUT_LEFT_MID, -6, 0);  //Align below the label
#else
  lv_obj_add_style(sigStrengthLbl, &style_bigfont, LV_PART_MAIN);
  lv_obj_align_to(sigStrengthLbl, battChgLbl, LV_ALIGN_OUT_LEFT_MID, -4, 0);  //Align below the label
#endif
  stmoLbl = lv_label_create(lv_scr_act()); //First parameters (scr) is the parent
  lv_label_set_text(stmoLbl, "");
#if (TFT_WIDTH == 480)
  lv_obj_add_style(stmoLbl, &style_biggerfont, LV_PART_MAIN);
  lv_obj_align_to(stmoLbl, sigStrengthLbl, LV_ALIGN_OUT_LEFT_MID, -10, 0);  //Align below the label
#else
  lv_obj_add_style(stmoLbl, &style_bigfont, LV_PART_MAIN);
  lv_obj_align_to(stmoLbl, sigStrengthLbl, LV_ALIGN_OUT_LEFT_MID, -6, 0);  //Align below the label
#endif
  static lv_style_t style_yellow;
  lv_style_init(&style_yellow);
#ifdef THEME_BLUE  
  lv_style_set_text_color(&style_yellow, lv_palette_main(LV_PALETTE_CYAN));
#else
  lv_style_set_text_color(&style_yellow, lv_color_hex(0xFF7F00));
#endif
  timeLbl = lv_label_create(lv_scr_act()); //First parameters (scr) is the parent
  lv_label_set_text(timeLbl, "");
  lv_obj_add_style(timeLbl, &style_biggestfont, LV_PART_MAIN);
  lv_obj_add_style(timeLbl, &style_yellow, LV_PART_MAIN);
#if (TFT_WIDTH == 480)
  lv_obj_align_to(timeLbl, modeList, LV_ALIGN_OUT_RIGHT_MID, 10, -2);  //Align below the label
#else
  lv_obj_align_to(timeLbl, modeList, LV_ALIGN_OUT_RIGHT_MID, 6, -4);  //Align below the label
#endif

  //Create a tabview for the body container
  lv_style_init(&style_transp);
  lv_style_set_bg_opa(&style_transp, LV_OPA_0);
  lv_style_set_border_opa(&style_transp, LV_OPA_0);
  tabView = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 38);
  lv_obj_t * tabButtons = lv_tabview_get_tab_btns(tabView);
  lv_obj_add_flag(tabButtons, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_style(tabView, &style_transp, LV_PART_MAIN);
  lv_obj_set_size(tabView, LV_HOR_RES, LV_VER_RES - 38);
  lv_obj_set_pos(tabView, 0, 38);
  //get the drag event - used to cancel some actions when screen is slid away
  lv_obj_t * cont = lv_tabview_get_content(tabView);
  lv_obj_add_event_cb(cont, tabViewAction, LV_EVENT_ALL, NULL);
  
  //Create the tabs
  lv_obj_t * settingsTab = lv_tabview_add_tab(tabView, "Settings");
  stationsTab = lv_tabview_add_tab(tabView, "Stations");
  lv_obj_t * mainTab = lv_tabview_add_tab(tabView, "Main");
  lv_obj_t * presetsTab = lv_tabview_add_tab(tabView, "Presets");
  lv_obj_t * terminalTab = lv_tabview_add_tab(tabView, "Terminal");
  lv_obj_t * sysmonTab = lv_tabview_add_tab(tabView, "Debug");
  lv_tabview_set_act(tabView, 4, LV_ANIM_OFF);              //Start on the terminal

  //Put the windows in the tabs
  createSettingsWindow(settingsTab);
  createStationsWindow(stationsTab);
  createFileBrowserWindow(stationsTab);
  createPodcastWindow(stationsTab);
  createDlnaWindow(stationsTab);
  createSearchWindow(stationsTab);
  createPodSearchWindow(stationsTab);
  createMainWindow(mainTab);
  createPresetsWindow(presetsTab);
  createPlaylistWindow(presetsTab);
  createTerminalWindow(terminalTab);
  createSysmonWindow(sysmonTab);

  //Create the invisible 'Factory mode' button
  factoryBtn = lv_obj_create(lv_scr_act());
  lv_obj_set_style_bg_opa(factoryBtn, LV_OPA_0, LV_PART_MAIN);
  lv_obj_set_style_border_opa(factoryBtn, LV_OPA_0, LV_PART_MAIN);
  lv_obj_set_size(factoryBtn, 100, 100);
  lv_obj_set_pos(factoryBtn, TFT_WIDTH - 105, TFT_HEIGHT - 105);
  lv_obj_add_event_cb(factoryBtn, factoryModeActivated, LV_EVENT_PRESSING, NULL);

}

//Factory button disappears after bootup
void removeFactoryBtn() {
  if (factoryBtn) {
    lv_obj_del(factoryBtn);
    factoryBtn = NULL;
  }
}

//Tabview slid left or right - cancel any editing actions
void tabViewAction(lv_event_t * event) {
  lv_event_code_t code = lv_event_get_code(event);
  if(code == LV_EVENT_SCROLL_BEGIN) {
    if (keyboardShowing()) {
      keyboardHide(false, NULL);
      //lv_textarea_set_text(urlEditText, settings->server);  
      //lv_obj_scroll_to(urlEditText, 0, 0, LV_ANIM_ON);
    }
    loglvlCloseDropdown();
    searchCloseDropdown();
    closeNetworkDropdown();
    //Restore current mode in modelist
    lv_obj_t * ddlist = lv_dropdown_get_list(modeList);
    lv_state_t state = lv_obj_get_state(modeList);
    if (ddlist && (state & LV_STATE_CHECKED)) 
      lv_dropdown_set_selected(modeList, settings->mode - 1);
    closeModeList();
    tabViewScrolling = true;
  }
  else if(code == LV_EVENT_SCROLL_END) tabViewScrolling = false;
}

bool tabViewIsScrolling() { return tabViewScrolling; }

//Slide tabview to main screen
void tabViewShowMain() {
  if (tabView) lv_tabview_set_act(tabView, mainWindowIndex, LV_ANIM_ON);
}

void closeModeList() {
  if (modeList) lv_dropdown_close(modeList);
}

int activeTab() {
  return lv_tabview_get_tab_act(tabView);
}

void setSelectedMode(int mode) {
  lv_dropdown_set_selected(modeList, mode - 1);      //Set the id of selected option
}

//Mode dropdown opened, set the style of the dropdown
void modeListActivated(lv_event_t * event) {
  lv_obj_t * dropdown = lv_event_get_target(event);
  lv_obj_t * ddlist = lv_dropdown_get_list(dropdown);
  lv_state_t state = lv_obj_get_state(dropdown);
  if (ddlist && (state & LV_STATE_CHECKED)) {  
    lv_obj_add_style(ddlist, &style_biggestfont, LV_PART_MAIN);
    lv_obj_set_style_max_height(ddlist, 300, LV_PART_MAIN);
    lv_obj_set_height(ddlist, LV_SIZE_CONTENT);
  }
}

//Mode dropdown selection made
void dabModeAction(lv_event_t * event) {
  uint16_t opt = lv_dropdown_get_selected(lv_event_get_target(event)) + 1;      //Get the id of selected option
  if (opt != settings->mode) setRadioMode(opt);
}

//Required when changing the top line labels to preserve their layout
void realignTopLabels() {
#ifdef SDPLAYER
  if (sdcardLbl) lv_obj_align(sdcardLbl, LV_ALIGN_TOP_RIGHT, -8, 8);  //Align below the label
#else
  if (sdcardLbl) lv_obj_align(sdcardLbl, LV_ALIGN_TOP_RIGHT, 0, 8);  //Align below the label
#endif
#ifdef BATTERYMON
  if (battChgLbl) lv_obj_align_to(battChgLbl, sdcardLbl, LV_ALIGN_OUT_LEFT_MID, -12, 0);  //Align below the label  
#else
  if (battChgLbl) lv_obj_align_to(battChgLbl, sdcardLbl, LV_ALIGN_OUT_LEFT_MID, 0, 0);  //Align below the label  
#endif
  if (sigStrengthLbl) lv_obj_align_to(sigStrengthLbl, battChgLbl, LV_ALIGN_OUT_LEFT_MID, -6, 0);  //Align below the label
  if (stmoLbl) lv_obj_align_to(stmoLbl, sigStrengthLbl, LV_ALIGN_OUT_LEFT_MID, -6, 0);  //Align below the label  
}

//Signal strength
void setSigStrengthLbl(const char * txt) {
  if (sigStrengthLbl) {
    lv_label_set_text(sigStrengthLbl, txt);
    realignTopLabels();  
  }
}

//Battery charge
void setBattChgLbl(const char * txt) {
  if (battChgLbl) {
    lv_label_set_text(battChgLbl, txt);
    realignTopLabels();  
  }
}

//Stero/mono/slideshow
void setStmoLbl(const char * txt) {
  if (stmoLbl) {
    lv_label_set_text(stmoLbl, txt);
    realignTopLabels();  
  }
}

//SD Card
void setSdcardLbl(const char * txt) {
  if (sdcardLbl) {
    lv_label_set_text(sdcardLbl, txt);
    realignTopLabels();  
  }
}

bool stmoSlideshow = false;
void updateSTMOLabel(uint8_t mode) {
  if (mode == STMO_MONO) {
    setStmoLbl("MO");        
    stmoSlideshow = false;
  } else if (mode == STMO_STEREO) {
    if (!stmoSlideshow)
      setStmoLbl("");        
  } else if (mode == STMO_SLIDE) {
    setStmoLbl(LV_SYMBOL_IMAGE);
    stmoSlideshow = true;
  } else {
    setStmoLbl("");        
  }
}

void setTimeLbl(int hour, int min, bool colon) {
  char txt[20];
  snprintf(txt, 19, "%d%s%02d", hour, colon?":":" ", min); 
  if (timeLbl) lv_label_set_text(timeLbl, txt);
}

//Show RTC on header
void timeHandle() {
  static int oldMinute;
  static long oldInterval = 0;
  bool noflash = false;
#ifdef MONKEYBOARD
  noflash = (settings->mode == MODE_FM || settings->mode == MODE_DAB);
#endif
  if (noflash) {
    //No flashing colon
    if (RTCSet) {
      int minute = getMinute();
      if (minute != oldMinute) {
        oldMinute = minute;
        setTimeLbl(getHour(false), getMinute(), true);
      }
    }   
  }
  else {
    long interval = millis() / 500;   //Deliberately round off
    if (interval != oldInterval) {          //tick every half second, on the second
      oldInterval = interval;
      if (RTCSet) setTimeLbl(getHour(false), getMinute(), interval % 2);
    }
  }  
}
 
//Hide the mode list and show a control function name
void setFunctionLbl(const char* txt) {
  lv_obj_set_hidden(modeList, true);
  lv_obj_set_hidden(timeLbl, true);
  lv_obj_set_hidden(functionLbl, false);
  lv_label_set_text(functionLbl, txt);
}

//Object visibility matrix - shows/hides the widgets to suit the mode and function
void setObjectVisibility() {
  lv_tabview_set_act(tabView, mainWindowIndex, LV_ANIM_OFF);              //Switch to main  
  //main window
  lv_obj_set_hidden(modeList, false);
  lv_obj_set_hidden(functionLbl, true);
  setMainVisibility();
  //stations window
  setDlnaVisibility();
  setStationsVisibility();
  setBrowserVisibility();
  setPlaylistVisibility();
  setPodVisibility();
  //settings window
  setSettingsVisibility();
  //Presets window
#ifdef SDPLAYER
  showPresets(settings->mode != MODE_SD && settings->mode != MODE_FTP && settings->mode != MODE_DLNA);                            // and bring up the presets
#else
  showPresets(settings->mode != MODE_FTP && settings->mode != MODE_DLNA);                            // and bring up the presets
#endif
  setPlaylistBtnVisibility();
  hideSearchWindow(true);
  hidePodSearchWindow(true);
  setStaticLabels(settings->mode == MODE_FM);
}

void prepareI2S() {
#ifdef MONKEYBOARD
  startI2S();
#endif
#ifdef NXP6686
  NXPStartI2S();
#endif
}

//Main mode change entry - clears all state and resets peripherals as required
// Sets up and starts function and mode based on current settings
void setRadioMode(int mode) {
  //Clean up all things
  removeFactoryBtn();
  if (factoryMode) return;
  //Clear state and stop any current activities
  clearProgLbl();
  setSigStrengthLbl("");
  updateSTMOLabel(STMO_NONE);
  clearStations();
  clearPodcasts();
  sdStop();
  sdSongFinished();
  closePopup(true);
  hideWebControls();
#ifdef MONKEYBOARD
  dabStop();
#endif
#ifdef NXP6686
  NXPStop();
#endif
  clearPlaylist();
  webStationName[0] = '\0';
  //Switch mode now, if required
  if (mode != settings->mode) {
    settings->mode = mode;
    serial.printf("> Set mode %d: %s\r\n", settings->mode, modeString[settings->mode]);
    writeSettings();
  }
  setObjectVisibility();
#ifndef USE_OTA
  if (settings->mode != MODE_WEB && settings->mode != MODE_FTP && settings->mode != MODE_POD && settings->mode != MODE_DLNA) 
    wifiDisconnect();
#endif
#ifdef BLUETOOTH
  if (isWebradioAllocated()) webradioStop();
#endif    
  //Set up new mode
  if (settings->mode == MODE_FTP) {
    prepareI2S(); 
#ifndef USE_OTA
    wlanConnect();
#endif
    ftpPopulateBrowser(NULL);
    info(NAME, 0, LV_SYMBOL_STOP " Stopped");    
    info(NOW, 0, LV_SYMBOL_LEFT " Choose files from the list to the left.. ");
    resumePlaylist();
  }
#ifdef MONKEYBOARD
  else if (settings->mode == MODE_DAB) startDab();
  else if (settings->mode == MODE_FM) startFM();
#endif
#ifdef NXP6686
  else if (settings->mode == MODE_NMW) NXPStartMW();
  else if (settings->mode == MODE_NLW) NXPStartLW();
  else if (settings->mode == MODE_NSW) NXPStartSW();
  else if (settings->mode == MODE_NFM) NXPStartFM();
#endif
  else if (settings->mode == MODE_WEB) {
    prepareI2S(); 
#ifndef USE_OTA
    wlanConnect();
#endif  
    readWebStations();
    //web stuff
    if (!strlen(settings->server))
      strcpy(settings->server, "http://radio1.internode.on.net:8000/296");
    connectToHost(settings->server, true); 
  }
#ifdef SDPLAYER  
  else if (settings->mode == MODE_SD) { 
    prepareI2S(); 
    sdPopulateBrowser(NULL);          //kick off the file browser
    if (!SDFound) {
      info(NAME, 0, LV_SYMBOL_SD_CARD" SD Card not found!");
      showReloadBtn();
    } else {
      info(NAME, 0, LV_SYMBOL_STOP " Stopped");
      info(NOW, 0, LV_SYMBOL_LEFT " Choose files from the list to the left.. ");
      resumePlaylist();
    }
  }
#endif  
  else if (settings->mode == MODE_POD) {
    prepareI2S(); 
#ifndef USE_OTA
    wlanConnect();
#endif
    readPodcasts();
    info(NAME, 0, LV_SYMBOL_STOP " Stopped");
    info(NOW, 0, LV_SYMBOL_LEFT " Choose podcasts from the list to the left.. ");
  }
  else if (settings->mode == MODE_DLNA) {
    prepareI2S(); 
#ifndef USE_OTA
    wlanConnect();
#endif
    info(NAME, 0, LV_SYMBOL_STOP " Stopped");
    info(NOW, 0, LV_SYMBOL_LEFT " Browse media from the list to the left.. ");
    DLNAGetServers(false);
    resumePlaylist();
  }
#ifdef BLUETOOTH  
  else if (settings->mode == MODE_BT) {
    startBT();
    //prepareI2S(); 
    //setupBluetooth();
  }
#endif  
  updateUrlEditText();
  
}

//-------------------------------------------------------------------------
// List context menu

//Object containing pointers to listMenu widgets and index of previously selected list item
typedef struct listMenuObj {
  lv_obj_t* list;
  lv_obj_t* back;
  listMenuObj(lv_obj_t* menuObj, lv_obj_t* menuBack) : list(menuObj), back(menuBack) {}
} listMenuObj;

//Destroy listMenu
void listMenuDelete(lv_anim_t * a) {
  lv_obj_t* list = (lv_obj_t*)a->user_data;
  listMenuObj* menu = (listMenuObj*)lv_obj_get_user_data(list);
  lv_obj_del(menu->back);
  lv_obj_del(menu->list);
  lv_obj_set_user_data(list, NULL);
}

//Finished with listMenu, make it go away
void listMenuDone(lv_obj_t* list, bool animated) {
  listMenuObj* menu = (listMenuObj*)lv_obj_get_user_data(list);
  if (menu) {     //Appears to come from listMenu?
    if (animated) {
      lv_anim_t a;
      lv_anim_init(&a);
      lv_anim_set_var(&a, menu->list);
      lv_anim_set_values(&a, lv_obj_get_height(menu->list), 0);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
      lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_height);
#pragma GCC diagnostic pop
      lv_anim_set_ready_cb(&a, listMenuDelete);
      lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
      lv_anim_set_time(&a, 300);
      a.user_data = list;
      lv_anim_start(&a);
    }
    else {
      lv_obj_del(menu->back);
      lv_obj_del(menu->list);
      lv_obj_set_user_data(list, NULL);
    }
  }
}

//Close listMenu - call from menu actions
void listMenuClose(lv_event_t * event) {
  lv_obj_t* obj = lv_event_get_target(event);
  lv_obj_t* list = lv_obj_get_parent(lv_obj_get_parent(obj));
  listMenuDone(list, true);
}

//Close listMenu - call from menu actions
void listMenuClose(lv_obj_t * list, bool animated) {
  listMenuDone(list, animated);
}

//Dismiss listMenu when click outside menu area on the background object
void listMenuDismiss(lv_event_t * event) {
  lv_obj_t* obj = lv_event_get_target(event);
  lv_obj_t* list = lv_obj_get_parent(obj);
  listMenuDone(list, true);
}

//listMenu has been shown, ensure it's fully visible
void listMenuShown(lv_anim_t * a) {
  lv_obj_t* target = (lv_obj_t*)a->user_data;
  lv_obj_scroll_to_view(target, LV_ANIM_ON);
}

//Start listMenu to appear - call at end of construction function
void listMenuActivate(lv_obj_t* menu) {
  lv_obj_update_layout(menu);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, menu);
  lv_anim_set_values(&a, 0, lv_obj_get_height(menu));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
  lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_height);
#pragma GCC diagnostic pop
  lv_anim_set_ready_cb(&a, listMenuShown);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  lv_anim_set_time(&a, 300);
  a.user_data = menu;
  lv_anim_start(&a);
}

//Override click action - add to list buttons with menu as LV_EVENT_CLICK
// use LV_EVENT_SHORT_CLICK for list actions, and LV_EVENT_LONG_PRESS calls construction function
void listMenuClicked(lv_event_t * event) {
  lv_obj_t * item = lv_event_get_target(event);
  listSetSelect(item);
}

//call from construction function on list button to get new menu list
lv_obj_t * createListMenu(lv_obj_t* button) {
  lv_obj_t* list = lv_obj_get_parent(button);
  listSetSelect(button);
  //Create backing object to cover list
  lv_obj_t* menuBack = lv_obj_create(list);
  lv_obj_add_flag(menuBack, LV_OBJ_FLAG_FLOATING);
  lv_obj_set_x(menuBack, 0);
  lv_obj_set_y(menuBack, 0);
  lv_obj_set_width(menuBack, lv_obj_get_width(list));
  lv_obj_set_height(menuBack, lv_obj_get_height(list));
  lv_obj_add_event_cb(menuBack, listMenuDismiss, LV_EVENT_PRESSED, NULL);
  lv_obj_set_style_bg_opa(menuBack, LV_OPA_0, LV_PART_MAIN);  
  //Create the menu list
  lv_obj_t* menuObj = lv_list_create(list);
  lv_obj_add_flag(menuObj, LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_set_x(menuObj, lv_obj_get_x(button) + lv_obj_get_width(button) / 4);
  lv_obj_set_y(menuObj, lv_obj_get_y(button) + lv_obj_get_height(button) - 2);
  lv_obj_set_width(menuObj, lv_obj_get_width(button) / 2);
  lv_obj_set_height(menuObj, LV_SIZE_CONTENT);
  lv_obj_set_style_border_width(menuObj, 2, LV_PART_MAIN);
  lv_obj_set_style_border_color(menuObj, lv_color_hex(0xFF7F50), LV_PART_MAIN);
  lv_obj_set_style_border_side(menuObj, LV_BORDER_SIDE_FULL, LV_PART_MAIN);
  lv_obj_move_foreground(menuObj);
  listMenuObj* menu = new listMenuObj(menuObj, menuBack);
  lv_obj_set_user_data(list, menu);
  return menuObj;
}

//==========================================================================
// Keyboard

bool keyboardShowing() {
  return keyBoard != NULL;
}

void keyboardShow(lv_obj_t * parent, lv_obj_t * editText, lv_event_cb_t cb) {
  keyboardShow(parent, editText);
  lv_obj_add_event_cb(keyBoard, cb, LV_EVENT_ALL, NULL);
}

//Bring up the keyboard to a given textarea
void keyboardShow(lv_obj_t * parent, lv_obj_t * textarea) {
  if(keyBoard == NULL) {
    keyBoard = lv_keyboard_create(parent);
    lv_obj_add_flag(keyBoard, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(keyBoard, lv_obj_get_content_width(parent), lv_obj_get_content_height(lv_scr_act()) / 2);
    lv_obj_align_to(keyBoard, textarea, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_set_x(keyBoard, 0);
    lv_obj_update_layout(keyBoard);
    lv_keyboard_set_textarea(keyBoard, textarea);
    lv_obj_add_style(keyBoard, &style_kb, LV_PART_MAIN | LV_PART_ITEMS);
    lv_obj_add_style(keyBoard, &style_kb_rel, LV_STATE_FOCUSED | LV_PART_ITEMS);
    lv_obj_add_style(keyBoard, &style_kb_pr, LV_STATE_PRESSED | LV_PART_ITEMS);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, keyBoard);
    lv_anim_set_values(&a, lv_obj_get_y(keyBoard) + LV_VER_RES, lv_obj_get_y(keyBoard));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
#pragma GCC diagnostic pop
    lv_anim_set_ready_cb(&a, keyboardShownAction);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_time(&a, 700);
    lv_anim_start(&a);
  }
}

//Ensure keyboard is fully on screen by scrolling the parent
void keyboardShownAction(lv_anim_t * a) {
  lv_obj_scroll_to_view(keyBoard, LV_ANIM_ON);
}

//Slide the keyboard back off the screen
void keyboardHide(bool animated, void (*doneAction)()) {
  if (keyBoard == NULL) return;    //already hidden
  if (animated) {
    lv_obj_set_user_data(keyBoard, (void*)doneAction);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, keyBoard);
    lv_anim_set_values(&a, lv_obj_get_y(keyBoard), lv_obj_get_y(keyBoard) + LV_VER_RES);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
#pragma GCC diagnostic pop
    lv_anim_set_ready_cb(&a, keyboardHiddenAction);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_set_time(&a, 700);
    lv_anim_start(&a);
  } else {
    lv_obj_del(keyBoard);
    keyBoard = NULL;
    if (doneAction) (*doneAction)();
  }
}

//Keyboard is gone, destroy it
void keyboardHiddenAction(lv_anim_t * a) {
  void (*doneAction)() = (void(*)())lv_obj_get_user_data(keyBoard);
  lv_obj_del(keyBoard);
  keyBoard = NULL;
  if (doneAction) (*doneAction)();
}


//==========================================================================
// Debug terminal

lv_obj_t * terminalWindow;
lv_obj_t * termDropdown;
lv_obj_t * term_label = 0;

//#ifdef CONFIG_IDF_TARGET_ESP32S3
//#define LOG_REDIRECT
//#endif
#define TERMINAL_LOG_LENGTH  2047        //Characters
char* txt_log = 0; //[TERMINAL_LOG_LENGTH + 1];

//Logging from ESP32
//Must modify esp-idf
//https://community.platformio.org/t/redirect-esp32-log-messages-to-sd-card/33734/10
char * ets_log_line = 0;      //256 byte buffer in PSRAM
void myputc(char c) { 
  static int index = 0;
  if (!ets_log_line) {
    ets_write_char_uart(c);
  }
  else {
    if (c == '\n') {
      uint8_t lvl = 0;
      char* lvlchrptr = strchr(&ets_log_line[7], '[');
      if (lvlchrptr) {
        char lvlchr = lvlchrptr[1];
        if (lvlchr == 'E') lvl = 1;
        else if (lvlchr == 'W') lvl = 2;
        else if (lvlchr == 'I') lvl = 3;
        else if (lvlchr == 'D') lvl = 4;
        else if (lvlchr == 'V') lvl = 5;
        if (settings) {   //We may get called before settings is available
          if (lvl <= settings->logLevel)
            serial.println(lvlchrptr);
        } else {
          if (lvl <= 2) { //Default to warn if settings not initialised
            lvlchrptr[1] = '*';       //Levels not initialised..
            serial.println(lvlchrptr);
          }
        }
      } else serial.println(ets_log_line);
      index = 0;
    } else if (c != '\r' && index < 255) { 
      ets_log_line[index++] = c;
      ets_log_line[index] = '\0';
    }
  }
}

/*
int log_vprintf(const char *fmt, va_list args) {
  char* str; //printf result will be stored in this     
  int ret;
  size_t size_string=snprintf(NULL,0,fmt,args); //Calculating the size of the formed string 
  str=(char *)ps_malloc(size_string+4);         //Initialising the string 
  ret = vsnprintf(str,size_string,fmt,args);    //Storing the outptut into the string 

    Serial.printf("Converted String is :  %s\n",str);

  free(str);
  return ret;
}
*/

void initTerminalLog() {
  txt_log = (char*)ps_malloc(TERMINAL_LOG_LENGTH + 1);
  ets_log_line = (char*)ps_malloc(256);
  if (!txt_log || !ets_log_line) {
    Serial.println("Allocating PSRAM Failed!");
    Serial.println("Cannot continue, halting.");
    while(1) delay(100);
  }
  txt_log[0] = '\0'; 
  //ESP32 logging
#ifdef LOG_REDIRECT
  ets_install_putc1(myputc);
#endif
#ifdef MONITOR_PORT
  wifiTerminalSetup();
#endif
}

#ifdef LOG_REDIRECT
void loglvlActivated(lv_event_t * event);
void loglvlAction(lv_event_t * event);
#endif

void createTerminalWindow(lv_obj_t *win) {
  static lv_style_t style_bg;
  lv_style_init(&style_bg);
  lv_style_set_bg_color(&style_bg, lv_color_black());
  lv_style_set_bg_grad_color(&style_bg, lv_color_black());
  lv_style_set_bg_opa(&style_bg, LV_OPA_COVER);
  lv_obj_add_style(win, &style_bg, LV_PART_MAIN);
  
  static lv_style_t style1;
  lv_style_init(&style1);
  lv_style_set_text_color(&style1, lv_color_white());    //Make the window's content responsive
#if (TFT_WIDTH == 480)
  lv_style_set_text_font(&style1, &lv_font_terminal12);
#else
  lv_style_set_text_font(&style1, &lv_font_unscii_8);
#endif
  //Create a label for the text of the terminal
  term_label = lv_label_create(win);
  lv_label_set_long_mode(term_label, LV_LABEL_LONG_WRAP);
  lv_label_set_recolor(term_label, true);
  lv_obj_add_flag(term_label, LV_OBJ_FLAG_USER_1);          //Hack to turn off word break
  lv_obj_set_pos(term_label, 0, 0);
  lv_obj_set_width(term_label, lv_obj_get_content_width(win));
  lv_obj_set_height(term_label, LV_SIZE_CONTENT);
  lv_label_set_text_static(term_label, txt_log);               //Use the text array directly
  lv_obj_add_style(term_label, &style1, LV_PART_MAIN);
  terminalWindow = win;

#ifdef LOG_REDIRECT
  termDropdown = lv_dropdown_create(win);
  lv_obj_set_pos(termDropdown, lv_obj_get_content_width(win) - 50, 0);
  lv_obj_set_size(termDropdown, 48, 22);
  lv_obj_add_flag(termDropdown, LV_OBJ_FLAG_FLOATING);
  lv_obj_set_style_bg_color(termDropdown, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_border_width(termDropdown, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(termDropdown, lv_color_hex(0x303030), LV_PART_MAIN);
  lv_dropdown_set_options(termDropdown, "Off\nErr\nWrn\nInf\nDbg");
  lv_dropdown_set_selected(termDropdown, settings->logLevel);
  lv_obj_add_event_cb(termDropdown, loglvlAction, LV_EVENT_VALUE_CHANGED, NULL);                         //Set function to call on new option is chosen
  lv_obj_add_event_cb(termDropdown, loglvlActivated, LV_EVENT_CLICKED, NULL);
#endif
}

#ifdef LOG_REDIRECT
//Log level dropdown opened, set the style of the dropdown
void loglvlActivated(lv_event_t * event) {
  lv_obj_t * dropdown = lv_event_get_target(event);
  lv_obj_t * ddlist = lv_dropdown_get_list(dropdown);
  lv_state_t state = lv_obj_get_state(dropdown);
  if (ddlist && (state & LV_STATE_CHECKED)) {  
    lv_obj_set_style_max_height(ddlist, 300, LV_PART_MAIN);
    lv_obj_set_height(ddlist, LV_SIZE_CONTENT);
  }
}

//Log level dropdown selection made
void loglvlAction(lv_event_t * event) {
  uint16_t lvl = lv_dropdown_get_selected(lv_event_get_target(event));      //Get the id of selected option
  serial.printf("> New log level: %d\r\n", lvl);
  if (settings->logLevel != lvl) {
    settings->logLevel = lvl;
    writeSettings();
  }
}
#endif

void loglvlCloseDropdown() {
#ifdef LOG_REDIRECT
  lv_dropdown_close(termDropdown);
#endif
}

//Keeps the terminal buffer at less than TERMINAL_LOG_LENGTH
//Returns index of insertion point in txt_log
int trimTerminalBuffer(int newTextLength) {
  int bufferLength = strlen(txt_log);
  //If the text become too long 'forget' the oldest lines
  if(bufferLength + newTextLength > TERMINAL_LOG_LENGTH) {
    uint16_t new_start;
    for(new_start = 0; new_start < bufferLength; new_start++) {
      if(txt_log[new_start] == '\n') {
        //If there is enough space break
        if(new_start >= newTextLength) {
          //Ignore line breaks
          while(txt_log[new_start] == '\n' || txt_log[new_start] == '\r') new_start++;
          break;
        }
      }
    }

    // If it wasn't able to make enough space on line breaks
    // simply forget the oldest characters
    if(new_start == bufferLength) {
      new_start = bufferLength - (TERMINAL_LOG_LENGTH - newTextLength);
    }
    //Move the remaining text to the beginning
    uint16_t j;
    for(j = new_start; j < bufferLength; j++) {
      txt_log[j - new_start] = txt_log[j];
    }
    bufferLength = bufferLength - new_start;
    txt_log[bufferLength] = '\0';
  }
  return bufferLength;
}

//Update the cached terminal contents from the main loop
bool invalidateTerminal = false;
void terminalHandle() {
  if (term_label && invalidateTerminal && txt_log) {
    lv_label_set_text_static(term_label, txt_log);
    //Scroll to end
    lv_obj_update_layout(terminalWindow);
    lv_obj_scroll_to_y(terminalWindow, TERMINAL_LOG_LENGTH, LV_ANIM_OFF);
    invalidateTerminal = false;
    if (serial.InSetup) {
      //Pump the handle manually
      pumpLvgl();
    }
  }
}

const char* logColDef[] = {
  "FF0000",  //31 - (E) red
  "00FF00",  //32 - (I) green
  "FFFF00",  //33 - (W) yellow
  "00FFFF",  //36 - (D) cyan
  "7F7F7F"   //37 - (V) grey
};

char logColour(int col, int i) {
  if (i > 5) return 0;
  if (col == 31) return logColDef[0][i];
  if (col == 32) return logColDef[1][i];
  if (col == 33) return logColDef[2][i];
  if (col == 36) return logColDef[3][i];
  if (col == 37) return logColDef[4][i];
  return 255;
}


bool escCode(char ch) {
  static uint8_t esccode = 0;
  static uint8_t color = 0;
  if (esccode == 1) {
    if (ch == '[') esccode = 2;
    else esccode = 0;
    return true;
  }
  if (esccode == 2) {
    color = 0; //Reset color
    if (ch == ';') esccode = 3;
    if (ch >= 0x40) {
      int insert_at = trimTerminalBuffer(1);
      txt_log[insert_at] = '#';
      txt_log[insert_at + 1] = '\0';
      invalidateTerminal = true;      
      esccode = 0;
    }
    return true;
  }
  if (esccode == 3) {
    if (ch >= 0x40) {
      int insert_at = trimTerminalBuffer(8);
      txt_log[insert_at] = '#';
      for (int c = 0; c < 6; c++)
        txt_log[insert_at + c + 1] = logColour(color, c);
      txt_log[insert_at + 7] = ' ';
      txt_log[insert_at + 8] = '\0';
      invalidateTerminal = true;      
      esccode = 0;
    }
    else color = (color * 10) + ch - '0';
    return true;
  }
  if (ch == 27) {
    esccode = 1;
    return true;
  }
  return false;
}


//Custom serial class also prints to the terminal
//mySerial declared in config.h
size_t mySerial::write(uint8_t ch) {
  //if (escCode(ch)) return 1;
  ets_write_char_uart(ch);    // for ets_install_putc1 - don't use Serial.write(ch);
#ifdef MONITOR_PORT
  wifiTerminalWrite(ch);
#endif
  if (txt_log && ch != '\r') {
    int insert_at = trimTerminalBuffer(1);
    txt_log[insert_at] = ch;
    txt_log[insert_at + 1] = '\0';
    invalidateTerminal = true;
  }
  return 1;
}

bool startupError = false;
//Show an error message and carry on
void errorContinue(int time, const char * message) {
  info(NOW, time, LV_SYMBOL_WARNING " - %s", message);
  serial.printf(LV_SYMBOL_WARNING " %s\r\n", message);
  startupError = true;
}


//Show an error message and set the fatal flag
void errorHalt(const char * message) {
  errorContinue(0, message);
  while(1) { lv_task_handler(); }
}

//===========================================================================
//Factory mode

lv_obj_t * factoryWindow;
lv_obj_t * factoryResetBtn;
lv_obj_t * factoryResetLbl;
lv_obj_t * resetEepromBtn;
lv_obj_t * resetEepromLbl;
lv_obj_t * removePodcastsBtn;
lv_obj_t * removePodcastsLbl;
lv_obj_t * transferPodcastsBtn;
lv_obj_t * transferPodcastsLbl;
lv_obj_t * removeStationsBtn;
lv_obj_t * removeStationsLbl;
lv_obj_t * transferStationsBtn;
lv_obj_t * transferStationsLbl;
lv_obj_t * removeFtpPlaylistLbl;
lv_obj_t * removeFtpPlaylistBtn;
lv_obj_t * removeDlnaPlaylistLbl;
lv_obj_t * removeDlnaPlaylistBtn;
lv_obj_t * removeSdPlaylistBtn;
lv_obj_t * removeSdPlaylistLbl;
lv_obj_t * formatFilesystemBtn;
lv_obj_t * formatFilesystemLbl;

void factoryWindow_close_action(lv_event_t * event);
void factoryReset(lv_event_t * event);
void factoryResetEeprom(lv_event_t * event);
void factoryRemovePodcasts(lv_event_t * event);
void factoryTransferPodcasts(lv_event_t * event);
void factoryRemoveStations(lv_event_t * event);
void factoryTransferStations(lv_event_t * event);
void factoryRemoveFtpPlaylist(lv_event_t * event);
void factoryRemoveDlnaPlaylist(lv_event_t * event);
void factoryRemoveSdPlaylist(lv_event_t * event);
void factoryFormatFs(lv_event_t * event);

void activateFactoryMode() {
  if (!factoryMode) {
    factoryMode = true;
    createFactoryWindow();
  }
}

void factoryModeActivated(lv_event_t * event) {
  activateFactoryMode();
}

void createFactoryWindow() {
  lv_obj_t* parent = lv_scr_act();
  factoryWindow = lv_win_create(parent, 40);
  lv_obj_t* headerLbl = lv_win_add_title(factoryWindow, LV_SYMBOL_WARNING " Factory Functions");
  lv_obj_set_style_pad_left(headerLbl, 6, LV_PART_MAIN);
  lv_obj_set_size(factoryWindow, lv_obj_get_content_width(parent), lv_obj_get_content_height(parent));
  lv_obj_set_pos(factoryWindow, 0, 0);
  lv_obj_add_style(factoryWindow, &style_win, LV_PART_MAIN);
  lv_obj_t * win_content = lv_win_get_content(factoryWindow);
  lv_obj_add_style(win_content, &style_win, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(win_content, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(win_content, lv_color_black(), LV_PART_MAIN);

  lv_obj_t * close_btn = lv_win_add_btn(factoryWindow, LV_SYMBOL_CLOSE, 50);           /*Add close button and use built-in close action*/
  lv_obj_add_style(close_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(close_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(close_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(close_btn, factoryWindow_close_action, LV_EVENT_CLICKED, NULL);

  static lv_style_t style_font;
  lv_style_init(&style_font);
#if (TFT_WIDTH == 480)
  lv_style_set_text_font(&style_font, &lv_font_montserrat_16);
#else
  lv_style_set_text_font(&style_font, &lv_font_montserrat_14);
#endif

  factoryResetBtn = lv_btn_create(win_content);
  lv_obj_set_size(factoryResetBtn, 40, 35);
  lv_obj_add_style(factoryResetBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(factoryResetBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(factoryResetBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_set_pos(factoryResetBtn, 20, 10);
  lv_obj_add_event_cb(factoryResetBtn, factoryReset, LV_EVENT_CLICKED, NULL);
  lv_obj_t * label = lv_label_create(factoryResetBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_LOOP);
  factoryResetLbl = lv_label_create(win_content);
  lv_obj_add_style(factoryResetLbl, &style_font, LV_PART_MAIN);
  lv_label_set_text(factoryResetLbl, "Full Factory Reset (All settings)");
  lv_obj_align_to(factoryResetLbl, factoryResetBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

  resetEepromBtn = lv_btn_create(win_content);
  lv_obj_set_size(resetEepromBtn, 40, 35);
  lv_obj_add_style(resetEepromBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(resetEepromBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(resetEepromBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_align_to(resetEepromBtn, factoryResetBtn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
  lv_obj_add_event_cb(resetEepromBtn, factoryResetEeprom, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(resetEepromBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_LOOP);
  resetEepromLbl = lv_label_create(win_content);
  lv_obj_add_style(resetEepromLbl, &style_font, LV_PART_MAIN);
  lv_label_set_text(resetEepromLbl, "Reset EEPROM to default values");
  lv_obj_align_to(resetEepromLbl, resetEepromBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

  removePodcastsBtn = lv_btn_create(win_content);
  lv_obj_set_size(removePodcastsBtn, 40, 35);
  lv_obj_add_style(removePodcastsBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(removePodcastsBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(removePodcastsBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_align_to(removePodcastsBtn, resetEepromBtn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
  lv_obj_add_event_cb(removePodcastsBtn, factoryRemovePodcasts, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(removePodcastsBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_TRASH);
  removePodcastsLbl = lv_label_create(win_content);
  lv_obj_add_style(removePodcastsLbl, &style_font, LV_PART_MAIN);
  lv_label_set_text(removePodcastsLbl, "Delete Podcasts file:\n" PODLIST_PATH);
  lv_obj_align_to(removePodcastsLbl, removePodcastsBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

#ifdef SDPLAYER
  transferPodcastsBtn = lv_btn_create(win_content);
  lv_obj_set_size(transferPodcastsBtn, 40, 35);
  lv_obj_add_style(transferPodcastsBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(transferPodcastsBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(transferPodcastsBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_align_to(transferPodcastsBtn, removePodcastsBtn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
  lv_obj_add_event_cb(transferPodcastsBtn, factoryTransferPodcasts, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(transferPodcastsBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_LOOP);
  transferPodcastsLbl = lv_label_create(win_content);
  lv_obj_add_style(transferPodcastsLbl, &style_font, LV_PART_MAIN);
  lv_label_set_text(transferPodcastsLbl, "Transfer Podcasts to/from SD\n" PODLIST_COPY);
  lv_obj_align_to(transferPodcastsLbl, transferPodcastsBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
#endif

  removeStationsBtn = lv_btn_create(win_content);
  lv_obj_set_size(removeStationsBtn, 40, 35);
  lv_obj_add_style(removeStationsBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(removeStationsBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(removeStationsBtn, &style_bigfont_orange, LV_PART_SELECTED);
#ifdef SDPLAYER
  lv_obj_align_to(removeStationsBtn, transferPodcastsBtn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
#else
  lv_obj_align_to(removeStationsBtn, removePodcastsBtn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
#endif
  lv_obj_add_event_cb(removeStationsBtn, factoryRemoveStations, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(removeStationsBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_TRASH);
  removeStationsLbl = lv_label_create(win_content);
  lv_obj_add_style(removeStationsLbl, &style_font, LV_PART_MAIN);
  lv_label_set_text(removeStationsLbl, "Delete Webstations file:\n" WEBSTATIONS_PATH);
  lv_obj_align_to(removeStationsLbl, removeStationsBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

#ifdef SDPLAYER
  transferStationsBtn = lv_btn_create(win_content);
  lv_obj_set_size(transferStationsBtn, 40, 35);
  lv_obj_add_style(transferStationsBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(transferStationsBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(transferStationsBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_align_to(transferStationsBtn, removeStationsBtn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
  lv_obj_add_event_cb(transferStationsBtn, factoryTransferStations, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(transferStationsBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_LOOP);
  transferStationsLbl = lv_label_create(win_content);
  lv_obj_add_style(transferStationsLbl, &style_font, LV_PART_MAIN);
  lv_label_set_text(transferStationsLbl, "Transfer Webstations to/from SD\n" WEBSTATIONS_COPY);
  lv_obj_align_to(transferStationsLbl, transferStationsBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
#endif

  removeFtpPlaylistBtn = lv_btn_create(win_content);
  lv_obj_set_size(removeFtpPlaylistBtn, 40, 35);
  lv_obj_add_style(removeFtpPlaylistBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(removeFtpPlaylistBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(removeFtpPlaylistBtn, &style_bigfont_orange, LV_PART_SELECTED);
#ifdef SDPLAYER
  lv_obj_align_to(removeFtpPlaylistBtn, transferStationsBtn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
#else
  lv_obj_align_to(removeFtpPlaylistBtn, removeStationsBtn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
#endif
  lv_obj_add_event_cb(removeFtpPlaylistBtn, factoryRemoveFtpPlaylist, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(removeFtpPlaylistBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_TRASH);
  removeFtpPlaylistLbl = lv_label_create(win_content);
  lv_obj_add_style(removeFtpPlaylistLbl, &style_font, LV_PART_MAIN);
  lv_label_set_text(removeFtpPlaylistLbl, "Delete FTP Playlist file:\n" FTPPLAYLIST_PATH);
  lv_obj_align_to(removeFtpPlaylistLbl, removeFtpPlaylistBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

  removeDlnaPlaylistBtn = lv_btn_create(win_content);
  lv_obj_set_size(removeDlnaPlaylistBtn, 40, 35);
  lv_obj_add_style(removeDlnaPlaylistBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(removeDlnaPlaylistBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(removeDlnaPlaylistBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_align_to(removeDlnaPlaylistBtn, removeFtpPlaylistBtn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
  lv_obj_add_event_cb(removeDlnaPlaylistBtn, factoryRemoveDlnaPlaylist, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(removeDlnaPlaylistBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_TRASH);
  removeDlnaPlaylistLbl = lv_label_create(win_content);
  lv_obj_add_style(removeDlnaPlaylistLbl, &style_font, LV_PART_MAIN);
  lv_label_set_text(removeDlnaPlaylistLbl, "Delete DLNA Playlist file:\n" DLNAPLAYLIST_PATH);
  lv_obj_align_to(removeDlnaPlaylistLbl, removeDlnaPlaylistBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

#ifdef SDPLAYER
  removeSdPlaylistBtn = lv_btn_create(win_content);
  lv_obj_set_size(removeSdPlaylistBtn, 40, 35);
  lv_obj_add_style(removeSdPlaylistBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(removeSdPlaylistBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(removeSdPlaylistBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_align_to(removeSdPlaylistBtn, removeDlnaPlaylistBtn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
  lv_obj_add_event_cb(removeSdPlaylistBtn, factoryRemoveSdPlaylist, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(removeSdPlaylistBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_TRASH);
  removeSdPlaylistLbl = lv_label_create(win_content);
  lv_obj_add_style(removeSdPlaylistLbl, &style_font, LV_PART_MAIN);
  lv_label_set_text(removeSdPlaylistLbl, "Remove SD Playlist file\n" SDPLAYLIST_PATH);
  lv_obj_align_to(removeSdPlaylistLbl, removeSdPlaylistBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
#endif

  formatFilesystemBtn = lv_btn_create(win_content);
  lv_obj_set_size(formatFilesystemBtn, 40, 35);
  lv_obj_add_style(formatFilesystemBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(formatFilesystemBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(formatFilesystemBtn, &style_bigfont_orange, LV_PART_SELECTED);
#ifdef SDPLAYER
  lv_obj_align_to(formatFilesystemBtn, removeSdPlaylistBtn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
#else
  lv_obj_align_to(formatFilesystemBtn, removeDlnaPlaylistBtn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
#endif
  lv_obj_add_event_cb(formatFilesystemBtn, factoryFormatFs, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(formatFilesystemBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_SAVE);
  formatFilesystemLbl = lv_label_create(win_content);
  lv_obj_add_style(formatFilesystemLbl, &style_font, LV_PART_MAIN);
  lv_label_set_text(formatFilesystemLbl, "Format C: (SPIFFS)");
  lv_obj_align_to(formatFilesystemLbl, formatFilesystemBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
}

void factoryReset(lv_event_t * event) {
  lv_label_set_text(factoryResetLbl, "Formatting.. wait..");
  lv_obj_align_to(factoryResetLbl, factoryResetBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
  refreshDisplay();
  if (SPIFFS.format())
    lv_label_set_text(formatFilesystemLbl, "Format C: Success " LV_SYMBOL_OK);
  else {
    lv_label_set_text(formatFilesystemLbl, "Format C: Failed! " LV_SYMBOL_WARNING);
    return;
  }
  lv_obj_align_to(factoryResetLbl, factoryResetBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
  delay(500);
  setDefaults();
  writeSettings();
  lv_label_set_text(factoryResetLbl, "EEPROM has been reset " LV_SYMBOL_OK);
  lv_obj_align_to(factoryResetLbl, factoryResetBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
  delay(500);
  lv_label_set_text(factoryResetLbl, "Factory Reset complete " LV_SYMBOL_OK);
  lv_obj_align_to(factoryResetLbl, factoryResetBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
}
void factoryResetEeprom(lv_event_t * event) {
  setDefaults();
  writeSettings();
  lv_label_set_text(resetEepromLbl, "EEPROM reset OK " LV_SYMBOL_OK);
  lv_obj_align_to(resetEepromLbl, resetEepromBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
}
void factoryRemovePodcasts(lv_event_t * event) {
  removePodcasts();
  lv_label_set_text(removePodcastsLbl, "Podcast file deleted " LV_SYMBOL_OK);
  lv_obj_align_to(removePodcastsLbl, removePodcastsBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
}
static void factoryTransferPodAction(lv_event_t * e) {
  lv_obj_t * obj = lv_event_get_current_target(e);
  if (lv_msgbox_get_active_btn_text(obj)[0] == 'T') { // "To SD"
    if (transferPodcasts(false))
      lv_label_set_text(transferPodcastsLbl, "Podcast file copied to SD " LV_SYMBOL_OK);
    else lv_label_set_text(transferPodcastsLbl, "Podcast file not found! " LV_SYMBOL_WARNING);
  }
  else if (lv_msgbox_get_active_btn_text(obj)[0] == 'F') { // "From SD"
    if (transferPodcasts(true))
      lv_label_set_text(transferPodcastsLbl, "Podcast file copied from SD " LV_SYMBOL_OK);
    else lv_label_set_text(transferPodcastsLbl, "Podcast file not found! " LV_SYMBOL_WARNING);
  }
  lv_obj_align_to(transferPodcastsLbl, transferPodcastsBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
  lv_msgbox_close(obj);
}
void factoryTransferPodcasts(lv_event_t * event) {
  static const char * btns[] = {"To SD", "From SD", ""};
  lv_obj_t * mbox1 = lv_msgbox_create(NULL, "Podcast File", "Transfer podcast file to or from SD?", btns, true);
  lv_obj_add_event_cb(mbox1, factoryTransferPodAction, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_center(mbox1);
}
void factoryRemoveStations(lv_event_t * event) {
  removeWebStations();
  lv_label_set_text(removeStationsLbl, "Stations file deleted " LV_SYMBOL_OK);
  lv_obj_align_to(removeStationsLbl, removeStationsBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
}
static void factoryTransferStationAction(lv_event_t * e) {
  lv_obj_t * obj = lv_event_get_current_target(e);
  if (lv_msgbox_get_active_btn_text(obj)[0] == 'T') { // "To SD"
    if (transferPodcasts(false))
      lv_label_set_text(transferStationsLbl, "Station file copied to SD " LV_SYMBOL_OK);
    else lv_label_set_text(transferStationsLbl, "Station file not found! " LV_SYMBOL_WARNING);
  }
  else if (lv_msgbox_get_active_btn_text(obj)[0] == 'F') { // "From SD"
    if (transferPodcasts(true))
      lv_label_set_text(transferStationsLbl, "Station file copied from SD " LV_SYMBOL_OK);
    else lv_label_set_text(transferStationsLbl, "Station file not found! " LV_SYMBOL_WARNING);
  }
  lv_obj_align_to(transferStationsLbl, transferStationsBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
  lv_msgbox_close(obj);
}
void factoryTransferStations(lv_event_t * event) {
  static const char * btns[] = {"To SD", "From SD", ""};
  lv_obj_t * mbox1 = lv_msgbox_create(NULL, "Webstations File", "Transfer webstation file to or from SD?", btns, true);
  lv_obj_add_event_cb(mbox1, factoryTransferStationAction, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_center(mbox1);
}
void factoryRemoveFtpPlaylist(lv_event_t * event) {
  removePlaylist(FTPPLAYLIST_PATH);
  lv_label_set_text(removeFtpPlaylistLbl, "FTP Playlist deleted " LV_SYMBOL_OK);
  lv_obj_align_to(removeFtpPlaylistLbl, removeFtpPlaylistBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
}
void factoryRemoveDlnaPlaylist(lv_event_t * event) {
  removePlaylist(DLNAPLAYLIST_PATH);
  lv_label_set_text(removeDlnaPlaylistLbl, "DLNA Playlist deleted " LV_SYMBOL_OK);
  lv_obj_align_to(removeDlnaPlaylistLbl, removeDlnaPlaylistBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
}
void factoryRemoveSdPlaylist(lv_event_t * event) {
  removePlaylist(SDPLAYLIST_PATH);
  lv_label_set_text(removeSdPlaylistLbl, "SD Playlist deleted " LV_SYMBOL_OK);
  lv_obj_align_to(removeSdPlaylistLbl, removeSdPlaylistBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
}
void factoryFormatFs(lv_event_t * event) {
  lv_label_set_text(formatFilesystemLbl, "Formatting.. wait..");
  lv_obj_align_to(formatFilesystemLbl, formatFilesystemBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
  refreshDisplay();
  if (SPIFFS.format())
    lv_label_set_text(formatFilesystemLbl, "Format C: Success " LV_SYMBOL_OK);
  else
    lv_label_set_text(formatFilesystemLbl, "Format C: Failed! " LV_SYMBOL_WARNING);
}

void factoryWindow_close_action(lv_event_t * event) {
  if(factoryWindow) {
    lv_obj_del(factoryWindow);
    factoryWindow = NULL;
    factoryMode = false;
  }
  setRadioMode(settings->mode);
}

//=============================================================================================
//System monitor

#define CPU_LABEL_COLOR     "FF7F00"
#define MEM_LABEL_COLOR     "007FFF"
#define PS_LABEL_COLOR      "7F7F00"
#define PERF_CHART_POINT_NUM     100

#define VOLT_LABEL_COLOR     "FF0000"
#define AMP_LABEL_COLOR     "007FFF"
#define BATT_CHART_POINT_NUM     200

#define REFR_TIME         500
#define BATTERY_INTERVAL  18000 //18 seconds per sample, 1 hour on graph

//Power source string
const char* sourceString[] = { "Unknown", "Battery", "AC/DC", "Solar", "USB" };

static lv_obj_t * perf_chart;
static lv_chart_series_t * cpu_ser;
static lv_chart_series_t * mem_ser;
static lv_chart_series_t * ps_ser;
static lv_obj_t * info_label;
static lv_obj_t * sysinfo_label;
static lv_obj_t * cpu_label;
static lv_obj_t * mem_label;
static lv_obj_t * ps_label;
#ifdef BATTERYMON
static lv_obj_t * batt_chart;
static lv_chart_series_t * volt_ser;
static lv_chart_series_t * amp_ser;
static lv_obj_t * batt_label;
static lv_obj_t * volt_label;
static lv_obj_t * amp_label;
#endif
static lv_timer_t * sysmonTimer;

static void sysmonTimerAction(lv_timer_t * param);
void fillSysinfoLabel();

//Chart series external buffer in PSRAM 
void mallocSeries(lv_obj_t* chart, lv_chart_series_t* series) {
  int numPoints = lv_chart_get_point_count(chart);
  lv_coord_t* ptr = (lv_coord_t*)ps_malloc(numPoints * sizeof(lv_coord_t));
  memset(ptr, 0, numPoints * sizeof(lv_coord_t));
  lv_chart_set_ext_y_array(chart, series, ptr);
}

void setNext(lv_obj_t* chart, lv_chart_series_t* series, int val) {
  lv_coord_t* vals = lv_chart_get_y_array(chart, series);
  int numPoints = lv_chart_get_point_count(chart);
  for (int i = 0; i < numPoints - 1; i++)
    vals[i] = vals[i + 1];
  vals[numPoints - 1] = val;
  lv_chart_refresh(chart);
}

void createSysmonWindow(lv_obj_t *scr) {
  lv_coord_t hres = lv_obj_get_content_width(scr);
  lv_coord_t vres = lv_obj_get_content_height(scr);
  sysmonTimer = lv_timer_create(sysmonTimerAction, REFR_TIME, NULL);

  static lv_style_t style_speclines;
  lv_style_init(&style_speclines);
  lv_style_set_line_width(&style_speclines, 4);
  lv_style_set_width(&style_speclines, 3);
  lv_style_set_height(&style_speclines, 3);

#ifdef BATTERYMON
  //Create a battery chart with two data lines
  batt_chart = lv_chart_create(scr);
  lv_obj_set_size(batt_chart, hres - 20, vres / 2);
  lv_obj_set_pos(batt_chart, 10, 0);
  //lv_obj_set_click(chart, false);
  lv_chart_set_point_count(batt_chart, BATT_CHART_POINT_NUM);
  lv_chart_set_range(batt_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
  lv_chart_set_type(batt_chart, LV_CHART_TYPE_LINE);
  lv_chart_set_div_line_count(batt_chart, 0, 0);
  lv_obj_add_style(batt_chart, &style_groupbox, LV_PART_MAIN);           //Apply the new style
  lv_obj_add_style(batt_chart, &style_speclines, LV_PART_ITEMS);           //Apply the new style
  lv_obj_add_style(batt_chart, &style_speclines, LV_PART_INDICATOR);           //Apply the new style
  volt_ser =  lv_chart_add_series(batt_chart, lv_color_hex(0xFF0000), LV_CHART_AXIS_PRIMARY_Y);
  mallocSeries(batt_chart, volt_ser);
  amp_ser =  lv_chart_add_series(batt_chart, lv_color_hex(0x007FFF), LV_CHART_AXIS_PRIMARY_Y);
  mallocSeries(batt_chart, amp_ser);

  //Create a label for the details of Memory and CPU usage
  volt_label = lv_label_create(scr);
  lv_label_set_recolor(volt_label, true);
  lv_obj_add_style(volt_label, &style_bigfont, LV_PART_MAIN);           //Apply the new style
  lv_obj_align_to(volt_label, batt_chart, LV_ALIGN_TOP_LEFT, 5, 5);


  //Create a label for the details of Memory and CPU usage
  amp_label = lv_label_create(scr);
  lv_label_set_recolor(amp_label, true);
  lv_obj_add_style(amp_label, &style_bigfont, LV_PART_MAIN);           //Apply the new style
  lv_obj_align_to(amp_label, batt_chart, LV_ALIGN_BOTTOM_LEFT, 5, -5);
  
  //Create a label for the details of Memory and CPU usage
  batt_label = lv_label_create(scr);
  lv_label_set_recolor(batt_label, true);
  lv_obj_add_style(batt_label, &style_biggerfont, LV_PART_MAIN);           //Apply the new style
  lv_obj_align_to(batt_label, batt_chart, LV_ALIGN_OUT_BOTTOM_LEFT, 4, 4);
#endif

  //Create a chart with two data lines
  perf_chart = lv_chart_create(scr);
  lv_obj_set_size(perf_chart, hres - 20, vres / 2);
#ifdef BATTERYMON
  lv_obj_align_to(perf_chart, batt_label, LV_ALIGN_OUT_BOTTOM_LEFT, -4, 40);
#else
  lv_obj_set_pos(perf_chart, 10, 0);
#endif
  //lv_obj_set_click(chart, false);
  lv_chart_set_point_count(perf_chart, PERF_CHART_POINT_NUM);
  lv_chart_set_range(perf_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
  lv_chart_set_type(perf_chart, LV_CHART_TYPE_LINE);
  lv_chart_set_div_line_count(perf_chart, 0, 0);
  lv_obj_add_style(perf_chart, &style_groupbox, LV_PART_MAIN);           //Apply the new style
  lv_obj_add_style(perf_chart, &style_speclines, LV_PART_ITEMS);           //Apply the new style
  lv_obj_add_style(perf_chart, &style_speclines, LV_PART_INDICATOR);           //Apply the new style
  cpu_ser =  lv_chart_add_series(perf_chart, lv_color_hex(0xFF7F00), LV_CHART_AXIS_PRIMARY_Y);
  mallocSeries(perf_chart, cpu_ser);
  
  mem_ser =  lv_chart_add_series(perf_chart, lv_color_hex(0x007FFF), LV_CHART_AXIS_PRIMARY_Y);
  mallocSeries(perf_chart, mem_ser);

  ps_ser =  lv_chart_add_series(perf_chart, lv_color_hex(0x7F7F00), LV_CHART_AXIS_PRIMARY_Y);
  mallocSeries(perf_chart, ps_ser);

  for(int i = 0; i < PERF_CHART_POINT_NUM; i++) {
      lv_chart_set_next_value(perf_chart, cpu_ser, 0);
      lv_chart_set_next_value(perf_chart, mem_ser, 0);
      lv_chart_set_next_value(perf_chart, ps_ser, 0);
  }

  //Create a label for the details of Memory and CPU usage
  cpu_label = lv_label_create(scr);
  lv_label_set_recolor(cpu_label, true);
  lv_obj_add_style(cpu_label, &style_bigfont, LV_PART_MAIN);           //Apply the new style
  lv_obj_align_to(cpu_label, perf_chart, LV_ALIGN_TOP_LEFT, 5, 5);

  //Create a label for the details of Memory and CPU usage
  ps_label = lv_label_create(scr);
  lv_label_set_recolor(ps_label, true);
  lv_obj_add_style(ps_label, &style_bigfont, LV_PART_MAIN);           //Apply the new style
  lv_obj_align_to(ps_label, perf_chart, LV_ALIGN_BOTTOM_LEFT, 5, -5);
  
  //Create a label for the details of Memory and CPU usage
  mem_label = lv_label_create(scr);
  lv_label_set_recolor(mem_label, true);
  lv_obj_add_style(mem_label, &style_bigfont, LV_PART_MAIN);           //Apply the new style
  lv_obj_align_to(mem_label, ps_label, LV_ALIGN_TOP_LEFT, 0, -16);

  //Create a label for the details of Memory and CPU usage
  info_label = lv_label_create(scr);
  lv_label_set_recolor(info_label, true);
  lv_obj_add_style(info_label, &style_bigfont, LV_PART_MAIN);           //Apply the new style
  lv_obj_align_to(info_label, perf_chart, LV_ALIGN_OUT_BOTTOM_LEFT, 4, 4);

  //Create a label for the details of Memory and CPU usage
  sysinfo_label = lv_label_create(scr);
  lv_obj_add_style(sysinfo_label, &style_bigfont, LV_PART_MAIN);           //Apply the new style
  lv_obj_align_to(sysinfo_label, info_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 40);

  fillSysinfoLabel();
}

int freeram() {
  return ESP.getFreeHeap();
}
int totalram() {
  return ESP.getHeapSize();
}

esp_chip_info_t chip_info;
void fillSysinfoLabel() {
  lv_label_set_text_fmt(sysinfo_label, "Processor: %s rev %d\n - %d Cores @ %d Mhz, \nFlash: %dKb @ %dMhz",  
    ESP.getChipModel(),
    ESP.getChipRevision(),
    ESP.getChipCores(),
    ESP.getCpuFreqMHz(),
    ESP.getFlashChipSize()/1024,
    ESP.getFlashChipSpeed()/1000000);
}


static void sysmonTimerAction(lv_timer_t * param) {
    char buf_long[256];
#ifdef BATTERYMON    
    static unsigned long batteryTimer = millis();
    mAseconds += (battAmps * 500); //0.5 seconds, accumulate mA
    int mAh = mAseconds / 3600;

    if (millis() > batteryTimer) {
      batteryTimer = millis() + BATTERY_INTERVAL;
      //Add the voltage and current data to the chart
      setNext(batt_chart, volt_ser, battPercent);
      int amps = chargeI * 100;
      setNext(batt_chart, amp_ser, amps);
    }
    //volt label
    snprintf(buf_long, 255, "%s%s BATTERY: %d%%",
            LV_TXT_COLOR_CMD,
            VOLT_LABEL_COLOR,
            battPercent);
    lv_label_set_text(volt_label, buf_long);
    //amp label
    if (powerSource == SOURCE_SOLAR)
      snprintf(buf_long, 255, "%s%s SOLAR: %.2fV, %.3fmA",
              LV_TXT_COLOR_CMD,
              AMP_LABEL_COLOR,
              solarV,
              chargeI);
    else
      snprintf(buf_long, 255, "%s%s INPUT: %.2fmA",
              LV_TXT_COLOR_CMD,
              AMP_LABEL_COLOR,
              chargeI);
    lv_label_set_text(amp_label, buf_long);

    snprintf(buf_long, 255, "Battery  %.2f V, %.2f A, %d mAh, %s\n"
                      "%s Power  %.2fW In, %.2fW Out",
            battV,
            battAmps,
            mAh,
            battCharging?"Charging":battAmps<-0.05?"Discharging":battPercent>=95?"Float(Full)":"Float",
            sourceString[powerSource],
            inputPower,
            outputPower); 
            
    lv_label_set_text(batt_label, buf_long);
#endif    

    //Get CPU and memory information 
    uint8_t cpu_busy = 100 - lv_timer_get_idle();
    uint8_t mem_used_pct = 0;
    uint8_t ps_used_pct = 0;
#if  LV_MEM_CUSTOM == 0
    lv_mem_monitor_t mem_mon;
    lv_mem_monitor(&mem_mon);
    mem_used_pct = mem_mon.used_pct;
#else
    mem_used_pct = ((totalram() - freeram()) * 100) / totalram();    
#endif
    ps_used_pct = ((ESP.getPsramSize() - ESP.getFreePsram()) * 100) / ESP.getPsramSize();    
    
    //Add the CPU and memory data to the chart
    setNext(perf_chart, cpu_ser, cpu_busy);
    setNext(perf_chart, mem_ser, mem_used_pct);
    setNext(perf_chart, ps_ser, ps_used_pct);

    //Refresh the and windows
    //CPU label
    snprintf(buf_long, 255, "%s%s CPU: %d%%",
            LV_TXT_COLOR_CMD,
            CPU_LABEL_COLOR,
            cpu_busy);
    lv_label_set_text(cpu_label, buf_long);
    //Memory label
    snprintf(buf_long, 255, "%s%s HEAP: %d%%",
            LV_TXT_COLOR_CMD,
            MEM_LABEL_COLOR,
            mem_used_pct);
    lv_label_set_text(mem_label, buf_long);
   //PSRAM label
    snprintf(buf_long, 255, "%s%s PSRAM: %d%%",
            LV_TXT_COLOR_CMD,
            PS_LABEL_COLOR,
            ps_used_pct);
    lv_label_set_text(ps_label, buf_long);

#if LV_MEM_CUSTOM == 0            
    snprintf(buf_long, 255, "Total: %.1fK, Free: %.1fK, Frag: %d%%",
            (int)mem_mon.total_size / 1024.0,
            (int)mem_mon.free_size / 1024.0, 
            (int)mem_mon.frag_pct);
#else
    snprintf(buf_long, 255, "Heap: %dK, Used: %dK, Free: %dK\nWebradio stack: %d of %d\nPSRAM Total: %dK, Used: %dK",
            totalram() / 1024,
            (totalram() - freeram()) / 1024, 
            freeram() / 1024,
            WR_STACK_SIZE - uxTaskGetStackHighWaterMark(radioTaskHandle), WR_STACK_SIZE,
            ESP.getPsramSize() / 1024, (ESP.getPsramSize() - ESP.getFreePsram()) / 1024);
#endif
    lv_label_set_text(info_label, buf_long);
}