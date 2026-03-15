#include "decls.h"
//------------------------------------------------------------------------------
//Presets window
// Not using a button map as the label control is insufficient

static lv_obj_t * presetsWindow;
static lv_obj_t * urlEditText;
static lv_obj_t * playlistBtn;
static lv_obj_t * playlistBtnLbl;

//A map of actual buttons
lv_obj_t * presetButtons[NUM_PRESETS];
lv_obj_t * presetLabels[NUM_PRESETS];

uint64_t presetTimer = 0;

void editTextAction(lv_event_t * event);
static void playlistBtnAction(lv_event_t * event);
void presetClickAction(lv_event_t * event);
void presetLongAction(lv_event_t * event);
void clearPreset(uint16_t index);
void savePreset(uint16_t index);
void loadPreset(uint16_t index);
void keyboardPresetKeyAction(lv_event_t * event);

void createPresetsWindow(lv_obj_t * parent) {
  presetsWindow = parent;
  //URL edit textbox
  lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
  urlEditText = lv_textarea_create(parent);
  lv_obj_set_pos(urlEditText, 0, 6);
  lv_obj_add_style(urlEditText, &style_ta, LV_PART_MAIN);
  lv_textarea_set_text_selection(urlEditText, false);
  lv_textarea_set_one_line(urlEditText, true);
  lv_obj_set_size(urlEditText, lv_obj_get_content_width(parent), 38);
  lv_textarea_set_text(urlEditText, "");
  lv_obj_add_event_cb(urlEditText, editTextAction, LV_EVENT_PRESSED, NULL);

  //Playlist button
  playlistBtn = lv_btn_create(parent);
  lv_obj_set_pos(playlistBtn, lv_obj_get_content_width(parent) - 56, 6);
  lv_obj_set_size(playlistBtn, 48, 40);
  lv_obj_add_style(playlistBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(playlistBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(playlistBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(playlistBtn, playlistBtnAction, LV_EVENT_CLICKED, NULL);
  playlistBtnLbl = lv_label_create(playlistBtn);
  lv_obj_align(playlistBtnLbl, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(playlistBtnLbl, LV_SYMBOL_LIST2);
  lv_obj_set_hidden(playlistBtn, true);

  //Presets button "map"
  for (int ind = 0; ind < NUM_PRESETS; ind++) {
    presetButtons[ind] = lv_btn_create(parent);
    lv_obj_set_size(presetButtons[ind], (lv_obj_get_content_width(parent) - 20) / 4, PRESET_HEIGHT);
    if (ind == 0) lv_obj_align_to(presetButtons[ind], urlEditText, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 14);
    else if (ind == 4 || ind == 8 || ind == 12) 
      lv_obj_align_to(presetButtons[ind], presetButtons[ind-4], LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    else lv_obj_align_to(presetButtons[ind], presetButtons[ind-1], LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_obj_add_style(presetButtons[ind], &style_wp, LV_PART_MAIN);
    //lv_obj_add_style(presetButtons[ind], &style_listsel, LV_PART_MAIN);
    lv_obj_set_user_data(presetButtons[ind], (void*)ind);
    lv_obj_add_event_cb(presetButtons[ind], presetClickAction, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(presetButtons[ind], presetLongAction, LV_EVENT_LONG_PRESSED, NULL);
    //lv_obj_add_event_cb(presetButtons, presetAction, LV_EVENT_VALUE_CHANGED, NULL);
    presetLabels[ind] = lv_label_create(presetButtons[ind]);
    lv_label_set_text(presetLabels[ind], settings->presets[ind].name);      
    lv_label_set_long_mode(presetLabels[ind], LV_LABEL_LONG_WRAP);
    lv_obj_set_width(presetLabels[ind], lv_obj_get_content_width(presetButtons[ind]));
    lv_obj_set_style_text_align(presetLabels[ind], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(presetLabels[ind], LV_ALIGN_CENTER, 0, 0);      
  }
}

void showPlaylistBtn(bool yesno) {
  if (urlEditText && playlistBtn) {
    if (yesno) {
      lv_obj_set_size(urlEditText, lv_obj_get_content_width(presetsWindow) - 60, 38);
      lv_obj_set_hidden(playlistBtn, false);  
    } else {
      lv_obj_set_size(urlEditText, lv_obj_get_content_width(presetsWindow), 38);
      lv_obj_set_hidden(playlistBtn, true);
    }
  }
}

void setPlaylistBtnVisibility() {
#ifdef SDPLAYER
  showPlaylistBtn(settings->mode == MODE_SD || settings->mode == MODE_FTP || settings->mode == MODE_DLNA);  
#else
  showPlaylistBtn(settings->mode == MODE_FTP || settings->mode == MODE_DLNA);  
#endif
}

//Playlist button clicked
static void playlistBtnAction(lv_event_t * event) {
  showPresets(false);
  hidePlaylist(false);
}

//Show or hide the presets matrix
void showPresets(bool yesno) {
  for (int ind = 0; ind < NUM_PRESETS; ind++) {
    lv_obj_set_hidden(presetButtons[ind], !yesno);
  }
  lv_obj_set_hidden(urlEditText, !yesno);
}

//Read preset names from settings onto the buttons
void updatePresetButtons() {
  for (int ind = 0; ind < NUM_PRESETS; ind++) {
    if (presetLabels[ind]) 
      lv_label_set_text(presetLabels[ind], settings->presets[ind].name);      
  }  
}

//Remember a long press
bool lastPresetLong = false;
void presetLongAction(lv_event_t * event) {
  if (!tabViewIsScrolling()) {
    lastPresetLong = true;
    presetTimer = millis();
  }
}

//Preset button was activated - save, load or clear based on long or very long press
void presetClickAction(lv_event_t * event) {
  lv_obj_t * btn = lv_event_get_target(event);
  int ind = (int)lv_obj_get_user_data(btn);
  if (!tabViewIsScrolling()) {
    if (lastPresetLong) {
      if (presetTimer + 5000 < millis()) 
        clearPreset(ind);
      else savePreset(ind);
    }
    else loadPreset(ind);
  }
  lastPresetLong = false;
}

void clearPreset(uint16_t index) {
  settings->presets[index].mode = 0;
  strcpy(settings->presets[index].name, "<Empty>");
  serial.printf("> Clear preset %d\r\n", index);
  writeSettings();
  updatePresetButtons();
}

//Save out a preset to settings
void savePreset(uint16_t index) {
  settings->presets[index].mode = settings->mode;
  const char* name = settings->presets[index].name;
  if (settings->mode == MODE_WEB) {
    name = stationListName(settings->server);
    if (name) {
      strncpy(settings->presets[index].name, name, 34);
      settings->presets[index].name[34] = '\0';
    } else return;  
  }
  else if (settings->mode == MODE_POD) {
    if (currentPodcast) strncpy(settings->presets[index].name, currentPodcast->name, 34);
  }
#ifdef MONKEYBOARD  
  else if (settings->mode == MODE_DAB) strncpy(settings->presets[index].name, settings->dabChannel, 34);
  else if (settings->mode == MODE_FM) {
    if (strlen(fmStationName)) snprintf(settings->presets[index].name, 34, "%s FM %.1f", fmStationName, dabFrequency / 1000.0);
    else snprintf(settings->presets[index].name, 34, "FM %.1f", dabFrequency / 1000.0);
  }
#endif
#ifdef NXP6686  
  else if (settings->mode == MODE_NFM) {
    if (strlen(stationName)) snprintf(settings->presets[index].name, 34, "%s FM %.1f", stationName, settings->dabFM / 1000.0);
    else snprintf(settings->presets[index].name, 34, "FM %.1f", settings->dabFM / 1000.0);
  }
  else if (settings->mode == MODE_NMW) {
    snprintf(settings->presets[index].name, 34, "AM %d", settings->freqMW);
  }
  else if (settings->mode == MODE_NLW) {
    snprintf(settings->presets[index].name, 34, "LW %d", settings->freqLW);
  }
  else if (settings->mode == MODE_NSW) {
    snprintf(settings->presets[index].name, 34, "SW %d", settings->freqSW);
  }
#endif
  else return;  
  serial.printf("> Save preset %d as %s: %s\r\n", index, modeString[settings->mode], name);
  writeSettings();
  updatePresetButtons();
}

//Called from station rename to keep presets aligned
void renamePreset(const char* oldname, const char* newname) {
  for (int n = 0; n < NUM_PRESETS; n++) {
    if (strncmp(settings->presets[n].name, oldname, 34) == 0) {
      //Name match
      strncpy(settings->presets[n].name, newname, 34);
      settings->presets[n].name[34] = '\0';
      writeSettings();
      updatePresetButtons();
      return;
    }
  }
}

//Called from station delete to keep presets aligned
void deletePreset(const char* name) {
  for (int n = 0; n < NUM_PRESETS; n++) {
    if (strncmp(settings->presets[n].name, name, 34) == 0) {
      //Name match
      clearPreset(n);
      return;
    }
  }
}

//Load in a preset from settings
void loadPreset(uint16_t index) {
  if (strcmp(settings->presets[index].name, "<Empty>") == 0) return;  //Empty..
  uint8_t mode = settings->presets[index].mode;
  char * data = settings->presets[index].name;
  serial.printf("> Load preset %d [%s]: %s\r\n", index, modeString[mode], data);
  if (mode == MODE_WEB || mode == MODE_POD) {
    strncpy(searchStationName, data, 34);
    searchStationName[34] = '\0';
  }
#ifdef MONKEYBOARD
  else if (mode == MODE_DAB) strncpy(settings->dabChannel, data, 34);
  else if (mode == MODE_FM) settings->dabFM = atof(strrchr(data, ' ')+1) * 1000.0;
#endif
#ifdef NXP6686
  else if (mode == MODE_NFM) settings->dabFM = atof(strrchr(data, ' ')+1) * 1000.0;
  else if (mode == MODE_NMW) settings->freqMW = atof(strrchr(data, ' ')+1);
  else if (mode == MODE_NLW) settings->freqLW = atof(strrchr(data, ' ')+1);
  else if (mode == MODE_NSW) settings->freqSW = atof(strrchr(data, ' ')+1);
#endif
  else return;
  tabViewShowMain();
  setSelectedMode(mode);
  setRadioMode(mode);
}

//Show the keyboard when edit text is clicked
void editTextAction(lv_event_t * event) {  
  if (!keyboardShowing()) {
    keyboardShow(lv_scr_act(), urlEditText, keyboardPresetKeyAction);
  }
}

//Change the text in the URL editor to suit mode
void updateUrlEditText() {
  if (urlEditText) {
    //Appearance
    lv_obj_t * label = lv_textarea_get_label(urlEditText);
    if (settings->mode == MODE_WEB) {
      //Editable text   
      lv_obj_clear_state(urlEditText, LV_STATE_DISABLED);
#ifdef THEME_BLUE
      lv_obj_set_style_border_color(urlEditText, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
#else      
      lv_obj_set_style_border_color(urlEditText, lv_color_hex(0xFF7F50), LV_PART_MAIN);
#endif
      lv_textarea_set_one_line(urlEditText, true);
      lv_obj_set_width(label, LV_SIZE_CONTENT);
      lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    } else {
      //Display only
      lv_obj_add_state(urlEditText, LV_STATE_DISABLED);
      lv_obj_set_style_border_color(urlEditText, lv_color_hex(0xC0C0C0), LV_PART_MAIN);
      lv_obj_set_width(label, lv_obj_get_content_width(urlEditText));
      lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
    }
    //Content
    char str[20+FTP_NAME_LENGTH] = "";
    if (settings->mode == MODE_FTP) {
      IPAddress addr = settings->ftpAddress;
      snprintf(str, FTP_NAME_LENGTH + 19, "FTP://%d.%d.%d.%d/%s", addr[0],addr[1],addr[2],addr[3], sdFileName);
      lv_textarea_set_text(urlEditText, str);    
    } 
#ifdef MONKEYBOARD
    else if (settings->mode == MODE_FM) {
      sprintf(str, "FM://%.3f", settings->dabFM / 1000.0);
      lv_textarea_set_text(urlEditText, str);    
    }
    else if (settings->mode == MODE_DAB) {
      sprintf(str, "DAB://%s", settings->dabChannel);
      lv_textarea_set_text(urlEditText, str);    
    }
#endif
#ifdef NXP6686
    else if (settings->mode == MODE_NFM) {
      sprintf(str, "FM://%.1fMhz", settings->dabFM / 1000.0);
      lv_textarea_set_text(urlEditText, str);    
    }
    else if (settings->mode == MODE_NMW) {
      sprintf(str, "AM://%dKhz", settings->freqMW);
      lv_textarea_set_text(urlEditText, str);    
    }
    else if (settings->mode == MODE_NLW) {
      sprintf(str, "LW://%dKhz", settings->freqLW);
      lv_textarea_set_text(urlEditText, str);    
    }
    else if (settings->mode == MODE_NSW) {
      sprintf(str, "SW://%dKhz", settings->freqSW);
      lv_textarea_set_text(urlEditText, str);    
    }
#endif
    else if (settings->mode == MODE_WEB) {
      lv_textarea_set_text(urlEditText, settings->server);  
      lv_obj_scroll_to(urlEditText, 0, 0, LV_ANIM_ON);
    } 
#ifdef SDPLAYER      
    else if (settings->mode == MODE_SD) {
      lv_textarea_set_text(urlEditText, sdFileName);  
    } 
#endif
    else if (settings->mode == MODE_POD) {
      snprintf(str, FTP_NAME_LENGTH + 19, "Podcast://%s - %s", getPodcastName(), getPodEpisodeName());
      lv_textarea_set_text(urlEditText, str);  
      lv_obj_scroll_to(urlEditText, 0, 0, LV_ANIM_ON);
    } 
    else if (settings->mode == MODE_DLNA) {
      lv_textarea_set_text(urlEditText, sdFileName);  
    }
#ifdef LINEIN     
    else if (settings->mode == MODE_LINE) {
      lv_textarea_set_text(urlEditText, "LINE://IN");  
    }
#endif     
    else lv_textarea_set_text(urlEditText, "Huh? What mode am I in?");
  }
}

//Keyboard OK or Cancel on URL edit
void keyboardPresetKeyAction(lv_event_t * event) {
  uint32_t res = lv_event_get_code(event);
  if(res == LV_EVENT_READY || res == LV_EVENT_CANCEL){
    keyboardHide(true, NULL);
    if(res == LV_EVENT_READY) {
      const char* url = lv_textarea_get_text(urlEditText);
      if (strncmp(settings->server, url, 255) != 0) {
        strncpy(settings->server, url, 255);
        settings->server[255] = '\0';
        writeSettings();
      }  
      tabViewShowMain();
      webStationName[0] = '\0';
      //closeStream();
      connectToHost(settings->server, true);
    }
    else if (res == LV_EVENT_CANCEL){
      lv_textarea_set_text(urlEditText, settings->server);  
      lv_obj_scroll_to(urlEditText, 0, 0, LV_ANIM_ON);
    }
  }
}

