#include "decls.h"
//------------------------------------------------------------------------------------
// Stations list window - sorted list

char webStationName[35] = "";
char searchStationName[35] = "";

bool stationsModified = false;

lv_obj_t * dabStationList;
static lv_obj_t * stationWindow;
static lv_obj_t * stationHeaderLabel;
static lv_obj_t * stationSaveBtn;
static lv_obj_t * nameEditText;
//index of button currently editing
int editIndex;
//deferred station rename
char newStationName[35] = {0};

void stationWindow_totop_action(lv_event_t * event);
void stationWindow_edit_action(lv_event_t * event);
void stationWindow_delete_action(lv_event_t * event);
void stationWindow_save_action(lv_event_t * event);
void stationWindow_search_action(lv_event_t * event);
void closeEditText();
void nameEditAction(lv_event_t * event);
void keyboardNameEditAction(lv_event_t * event);
void nameEditFinished();
int readWebStationFile(const char * path);  
void writeWebStations();
void renameWebStation(int index, const char * name);

void createStationsWindow(lv_obj_t * page) {

  stationWindow = lv_win_create(page, 40);
  stationHeaderLabel = lv_win_add_title(stationWindow, "Web Stations");
  lv_obj_set_style_pad_left(stationHeaderLabel, 6, LV_PART_MAIN);
  lv_obj_set_size(stationWindow, lv_obj_get_content_width(page), 40);
  lv_obj_set_pos(stationWindow, 0, 0);
  lv_obj_add_style(stationWindow, &style_win, LV_PART_MAIN);
  lv_obj_set_hidden(stationWindow, true);  

  stationSaveBtn = lv_win_add_btn(stationWindow, LV_SYMBOL_SAVE, 50);          
  lv_obj_add_style(stationSaveBtn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(stationSaveBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(stationSaveBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(stationSaveBtn, stationWindow_save_action, LV_EVENT_CLICKED, NULL);
  lv_obj_set_hidden(stationSaveBtn, true);  

  lv_obj_t * totop_btn = lv_win_add_btn(stationWindow, LV_SYMBOL_UP, 50);           /*Add close button and use built-in close action*/
  lv_obj_add_style(totop_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(totop_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(totop_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(totop_btn, stationWindow_totop_action, LV_EVENT_CLICKED, NULL);

  lv_obj_t * search_btn = lv_win_add_btn(stationWindow, LV_SYMBOL_SEARCH, 50);           /*Add close button and use built-in close action*/
  lv_obj_add_style(search_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(search_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(search_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(search_btn, stationWindow_search_action, LV_EVENT_CLICKED, NULL);

  //Station list - not much to it
  dabStationList = lv_list_create(page);
  lv_obj_set_pos(dabStationList, 0, 40);
  lv_obj_set_size(dabStationList, lv_obj_get_content_width(page), lv_obj_get_content_height(page)-40);
  
  lv_obj_set_style_bg_opa(dabStationList, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_add_style(dabStationList, &style_biggestfont, LV_PART_MAIN);
}

void hideStationWindow(bool hide) {
  lv_obj_set_hidden(stationWindow, hide);
  lv_obj_set_hidden(dabStationList, hide);
}

uint8_t oldMainWindowIndex = 4; //we started on the terminal
void setStationsVisibility() {
  //Station tab visibility
  bool stationsHidden = (settings->mode == MODE_BT);
#ifdef MONKEYBOARD
  stationsHidden = (settings->mode == MODE_FM || settings->mode == MODE_LINE || stationsHidden);
#endif
#ifdef NXP6686
  stationsHidden = (settings->mode == MODE_NFM || settings->mode == MODE_NMW  || settings->mode == MODE_NLW  || settings->mode == MODE_NSW || stationsHidden);
#endif
  lv_obj_set_hidden(stationsTab, stationsHidden);
  mainWindowIndex = stationsHidden?1:2;
  if (oldMainWindowIndex != mainWindowIndex) {
    oldMainWindowIndex = mainWindowIndex;
    lv_tabview_set_act(tabView, mainWindowIndex, LV_ANIM_OFF);              //Start on the terminal
  }  
  //Station list visibility
  bool hide = (settings->mode != MODE_DAB && settings->mode != MODE_WEB);
  if (settings->mode == MODE_DAB) lv_label_set_text(stationHeaderLabel, "DAB+ Stations"); 
  if (settings->mode == MODE_WEB) lv_label_set_text(stationHeaderLabel, "Web Stations"); 
  hideStationWindow(hide);
}

//Edit entry in list
void createEditText(int index) {
  closeEditText();
  lv_obj_t* child = lv_obj_get_child(dabStationList, index);
  int ypos = lv_obj_get_y(child);
  nameEditText = lv_textarea_create(dabStationList);
  lv_obj_add_flag(nameEditText, LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_set_pos(nameEditText, 0, ypos);
  lv_obj_add_style(nameEditText, &style_listsel, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(nameEditText, LV_OPA_COVER, LV_PART_MAIN);
  lv_textarea_set_text_selection(nameEditText, false);
  lv_textarea_set_one_line(nameEditText, true);
  lv_obj_set_size(nameEditText, lv_obj_get_content_width(dabStationList), 32);
  lv_obj_add_event_cb(nameEditText, nameEditAction, LV_EVENT_PRESSED, NULL);
  lv_textarea_set_text(nameEditText, lv_list_get_btn_text(dabStationList, child));  
  lv_group_t * g = lv_group_create();  
  lv_group_add_obj(g, nameEditText);
  lv_group_focus_obj(nameEditText);
  //lv_obj_add_state(nameEditText, LV_STATE_FOCUSED | LV_STATE_EDITED);
  keyboardShow(dabStationList, nameEditText);
  lv_obj_add_event_cb(keyBoard, keyboardNameEditAction, LV_EVENT_ALL, NULL);
  editIndex = index;
}

//Show the keyboard when edit text is clicked
void nameEditAction(lv_event_t * event) {
  if (keyBoard == NULL) {
    keyboardShow(dabStationList, nameEditText);
    lv_obj_add_event_cb(keyBoard, keyboardNameEditAction, LV_EVENT_ALL, NULL);
  }
}

//Keyboard OK or Cancel on URL edit
void keyboardNameEditAction(lv_event_t * event) {
  uint32_t res = lv_event_get_code(event);
  if(res == LV_EVENT_READY || res == LV_EVENT_CANCEL){
    newStationName[0] = '\0';
    if(res == LV_EVENT_READY) {
      strncpy(newStationName, lv_textarea_get_text(nameEditText), 34);
      newStationName[34] = '\0';
    }
    keyboardHide(true, nameEditFinished);
  }
}

//Called from keyboard hidden event to perform the rename
void nameEditFinished() {
  lv_obj_del(nameEditText);
  nameEditText = NULL;
  if(strlen(newStationName) > 0) 
    renameWebStation(editIndex, newStationName);  
}

//Dismiss the rename textbox and keyboard if present
void closeEditText() {
  keyboardHide(false, NULL);
  if (nameEditText) {
    lv_obj_del(nameEditText);
    nameEditText = NULL;
  }         
}

void stationWindow_totop_action(lv_event_t * event) {
  listMenuClose(dabStationList, false);
  lv_obj_scroll_to(dabStationList, 0, 0, LV_ANIM_ON);
}

void stationWindow_edit_action(lv_event_t * event) {
  listMenuClose(event);
  int index = listGetSelect(dabStationList);
  if (!nameEditText && index >= 0) {
    createEditText(index);
  }
}

void stationWindow_search_action(lv_event_t * event) {
  hideStationWindow(true);
  hideSearchWindow(false);
}

void updateStationHeader() {
  if (stationsModified) {
    lv_label_set_text(stationHeaderLabel, "Web Stations (*)");
    lv_obj_set_hidden(stationSaveBtn, false);  
  } else {
    lv_label_set_text(stationHeaderLabel, "Web Stations");
    lv_obj_set_hidden(stationSaveBtn, true);  
  }
}

//Call after list changes to handle autosave
void stationsChanged() {
  if (settings->autosave) {
    writeWebStations();
  } else {
    stationsModified = true;
  }
  updateStationHeader();
}

void stationWindow_delete_action(lv_event_t * event) {
  listMenuClose(event);
  int index = listGetSelect(dabStationList);
  if (index >= 0) {
    lv_obj_t* child = lv_obj_get_child(dabStationList, index);
    const char* name = lv_list_get_btn_text(dabStationList, child);
    deletePreset(name); //if this station has one..
    //Get the userdata
    char* url = (char*)lv_obj_get_user_data(child);
    //Delete the button
    lv_obj_del(child);
    //Save out the new station list
    stationsChanged();
    //Is it the currently playing station?
    if (url && strcmp(url, settings->server) == 0) {
      //Stop playing
      webradioStop();
      clearProgLbl();
      //Remove it from settings as well
      settings->server[0] = '\0';
      writeSettings();
    }
    if (url) free(url);  //Clean up
  }
}

//Save button pressed
void stationWindow_save_action(lv_event_t * event) {
  if (stationsModified) {
    writeWebStations();
    stationsModified = false;
    updateStationHeader();
  }
}

//Clear a station list and also free the entry's attached userdata
void clearStations() {
  if (!dabStationList) return;
  closeEditText(); //if any
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(dabStationList); i++) {
    lv_obj_t* child = lv_obj_get_child(dabStationList, i);
    //Free all the userdata
    void * data = lv_obj_get_user_data(child);
    if (data) free(data);
  }
  //clear the list
  lv_obj_clean(dabStationList);
}

//Do the actual rename
void renameWebStation(int index, const char * name) {
  lv_obj_t* child = lv_obj_get_child(dabStationList, index);
  const char* oldname = lv_list_get_btn_text(dabStationList, child);
  if (strncmp(name, oldname, 35) != 0) {
    renamePreset(oldname, name);  //Rename this station's preset if one is found..
    //Get the userdata
    char* url = (char*)lv_obj_get_user_data(child);
    if (url) {
      //Replace button
      lv_obj_t * btn = addStationButton(name, webRadioListAction, url, strlen(url)+1);
      listSetSelect(btn);
      //Remove old button
      lv_obj_del(child);
      free(url);            //don't forget to clean up
      //Stations modified
      stationsChanged();
    }
  }
}

//Context menu
void stationListMenu(lv_event_t * event) {
  lv_obj_t * item = lv_event_get_target(event);
  lv_obj_t * menu = createListMenu(item);
  lv_obj_t * btn;
  btn = lv_list_add_btn(menu, LV_SYMBOL_EDIT, "Rename Station");
  lv_obj_add_event_cb(btn, stationWindow_edit_action, LV_EVENT_CLICKED, NULL);
  btn = lv_list_add_btn(menu, LV_SYMBOL_TRASH, "Delete Station");
  lv_obj_add_event_cb(btn, stationWindow_delete_action, LV_EVENT_CLICKED, NULL);
  listMenuActivate(menu);
}

//Add an entry to the station list
lv_obj_t * addStationButton(const char* text, lv_event_cb_t callback, void* data, int datalen) {
  lv_obj_t * list_btn;
  if (!dabStationList) return NULL;
  String name = text;
  list_btn = lv_list_add_btn(dabStationList, LV_SYMBOL_BROADCAST, name.c_str());
  //Insertion sort
  int num = lv_obj_get_child_cnt(dabStationList);
  if (num > 1) {
    for (int i = num - 1; i >= 0 ; i--) {
      lv_obj_t* btn = lv_obj_get_child(dabStationList, i);
      const char * lbl = lv_list_get_btn_text(dabStationList, btn); 
      if (strncasecmp(lbl, text, 34) < 0) break;
      lv_obj_swap(list_btn, btn);
    }
  }
  lv_obj_add_event_cb(list_btn, callback, LV_EVENT_SHORT_CLICKED, NULL);
  lv_obj_add_event_cb(list_btn, stationListMenu, LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_event_cb(list_btn, listMenuClicked, LV_EVENT_CLICKED, NULL);
  lv_obj_add_flag(list_btn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_add_style(list_btn, &style_list, LV_PART_MAIN);
  lv_obj_add_style(list_btn, &style_listsel, LV_STATE_CHECKED);
  if (data) {
    void* ptr = ps_malloc(datalen);
    memcpy(ptr, data, datalen);
    lv_obj_set_user_data(list_btn, ptr);
  }
  return list_btn;
}

//Set the currently highlighted station list entry selection to button object
void listSetSelect(lv_obj_t * btn) {
  lv_obj_t* parent = lv_obj_get_parent(btn);
  closeEditText();
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(parent); i++) {
    lv_obj_t* child = lv_obj_get_child(parent, i);
    if (child == btn) {
      lv_obj_add_state(child, LV_STATE_CHECKED);
      lv_obj_scroll_to_view(child, LV_ANIM_OFF);
    }
    else lv_obj_clear_state(child, LV_STATE_CHECKED);
  }
}

//Clear all selections in list
void listClearSelect(lv_obj_t* parent) {
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(parent); i++) {
    lv_obj_t* child = lv_obj_get_child(parent, i);
    lv_obj_clear_state(child, LV_STATE_CHECKED);
  }
}

//Set the currently highlighted station list entry selection to list index
void listSetSelect(lv_obj_t* list, int index) {
  if (!list) return;
  closeEditText();
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(list); i++) {
    lv_obj_t* child = lv_obj_get_child(list, i);
    if (i == index) {
      lv_obj_add_state(child, LV_STATE_CHECKED);
      lv_obj_scroll_to_view(child, LV_ANIM_OFF);
    }
    else lv_obj_clear_state(child, LV_STATE_CHECKED);
  }
}

//Get the currently highlighted station list entry selection as list index
int listGetSelect(lv_obj_t * list) {
  if (!list) return -1;
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(list); i++) {
    lv_obj_t* child = lv_obj_get_child(list, i);
    if (lv_obj_get_state(child) & LV_STATE_CHECKED) return i;
  }
  return -1;
}

//Get the currently highlighted station list entry selection as object
lv_obj_t* listGetSelectObj(lv_obj_t * list) {
  if (!list) return NULL;
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(list); i++) {
    lv_obj_t* child = lv_obj_get_child(list, i);
    if (lv_obj_get_state(child) & LV_STATE_CHECKED) return child;
  }
  return NULL;
}

//Set the currently highlighted station list entry selection
void listDoSelect() {
  if (!dabStationList) return;
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(dabStationList); i++) {
    lv_obj_t* child = lv_obj_get_child(dabStationList, i);
    if (lv_obj_get_state(child) & LV_STATE_CHECKED) {
      tabViewShowMain();
      if (settings->mode == MODE_WEB) {
        listSetSelect(child);
        strncpy(webStationName, lv_list_get_btn_text(dabStationList, child), 34);
        webStationName[34] = '\0';
        char* url = (char*)lv_obj_get_user_data(child);
        if (strncmp(settings->server, url, 255) != 0) {
          strncpy(settings->server, url, 255);
          settings->server[255] = '\0';
          writeSettings();
        }  
        updateUrlEditText();
        connectToHost(url, true);
      }
    }
  }
}

void listMoveSelect(int offset) {
  if (tabView && lv_tabview_get_tab_act(tabView) != 1) 
    lv_tabview_set_act(tabView, 1, LV_ANIM_ON);
  int numItems = lv_obj_get_child_cnt(dabStationList) - 1;
  int index = listGetSelect(dabStationList);
  if (index < 0) return;
  index += offset;
  if (index > numItems) index = numItems;
  if (index < 0) index = 0;
  listSetSelect(dabStationList, index);  
}


//See if a station with a given url is already in the list
bool stationInList(char * url) {
  if (!dabStationList) return false;
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(dabStationList); i++) {
    lv_obj_t* child = lv_obj_get_child(dabStationList, i);
    void * data = lv_obj_get_user_data(child);
    if (data && strncmp((char*)data, url, 255) == 0) return true;
  }
  return false;
}

//Get the list name of a station given the URL
const char* stationListName(char * url) {
  if (!dabStationList) return NULL;
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(dabStationList); i++) {
    lv_obj_t* child = lv_obj_get_child(dabStationList, i);
    void * data = lv_obj_get_user_data(child);
    if (data && strncmp((char*)data, url, 255) == 0) return lv_list_get_btn_text(dabStationList, child);
  }
  return NULL;
}

//Get the list name of a station given the button object
const char* stationListName(lv_obj_t * obj) {
  if (!dabStationList) return NULL;
  return lv_list_get_btn_text(dabStationList, obj);
}

//Webradio station list controls

int readWebStations() {
  serial.println("> Reading WebRadio playlist..");
  int stations = readWebStationFile(WEBSTATIONS_PATH);
  if (stations) return stations;
  stations = readWebStationFile(WEBSTATIONS_COPY);
  if (stations) {
    serial.println("> Playlist transferred from SD.");
    writeWebStations();
    return stations;
  }
  fs_err(12, "Open playlist");
  return 0;
}

// -- BEWARE! --
//Magic - if searchStationName contains a string it will be found in station names
// and the station's URL transferred to settings->server
// otherwise the default behaviour is to search for the URL in settings->server
// searchStationName is reset after the search to revert to default behaviour.
// This magic is used by the Presets mechanism to match by station name whereas
// normal startup matches by URL. This allows sensible operation with SD card removed.
 
int readWebStationFile(const char * path) {  
  lv_fs_file_t f;
  if (lv_fs_open(&f, path, LV_FS_MODE_RD) != LV_FS_RES_OK) return 0;
  int lbi = 0;
  char linebuf[260];
  char name[35];
  int count = 0;
  lv_obj_t * previous = NULL;
  uint32_t read_num;
  char ch;
  bool search = strlen(searchStationName);
  bool found = false;
  while (1) {
    if (fs_err(lv_fs_read(&f, &ch, 1, &read_num), "Read webstations")) return 0;
    if (read_num == 0) break;
    if (ch != '\r' && ch != '\n' ) linebuf[lbi++] = ch;
    if (ch == '\n' || lbi == 255) {
      linebuf[lbi] = 0;
      //Serial.println(linebuf);
      if (linebuf[0] == '#') {
        char * ptr = strchr(linebuf, ',');
        if (ptr) {
          strncpy(name, ptr+1, 34);
          name[34] = '\0';
          if (search && strcmp(name, searchStationName) == 0) found = true;
        }
      }
      if (strncasecmp("http", linebuf, 4) == 0) {
        lv_obj_t * list_btn = addStationButton(name, webRadioListAction, linebuf, lbi+1);
        if (!search && strncmp(linebuf, settings->server, 255) == 0) found = true;
        if (found) {
          serial.println("> Selected station found.");
          if (search && strncmp(linebuf, settings->server, 255) != 0) {
            strncpy(settings->server, linebuf, 255);
            settings->server[255] = '\0';
            writeSettings();
          } else {
            strncpy(webStationName, name, 34);
            webStationName[34] = '\0';
          }
          previous = list_btn;
          found = false;
        }
        count++;
      }
      lbi = 0;
    }
  }
  if (previous) listSetSelect(previous);
  serial.printf("> Read %d list entries OK.\r\n", count);
  lv_fs_close(&f);
  searchStationName[0] = '\0';
  return count;
}

void writeWebStations() {
  lv_fs_file_t f;
  char str[260];
  uint32_t byteswrote;
  //Build playlist
  if (fs_err(lv_fs_open(&f, WEBSTATIONS_PATH, LV_FS_MODE_WR), "Open Webstations")) return;
  snprintf(str, 259, "#EXTM3U\r\n\r\n");
  if (fs_err(lv_fs_write(&f, str, strlen(str), &byteswrote), "Write File Header")) return;
  
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(dabStationList); i++) {
    lv_obj_t* child = lv_obj_get_child(dabStationList, i);
    const char * name = lv_list_get_btn_text(dabStationList, child);
    snprintf(str, 259, "#EXTINF:-1,%s\r\n", name);
    if (fs_err(lv_fs_write(&f, str, strlen(str), &byteswrote), "Write Station Header")) return;
    const char * path = (const char*)lv_obj_get_user_data(child);
    if (path) {
      snprintf(str, 259, "%s\r\n\r\n", path);
      if (fs_err(lv_fs_write(&f, str, strlen(str), &byteswrote), "Write Station Entry")) return;
    }
  }  
  lv_fs_close(&f);
}

//Remove (delete) stations
void removeWebStations() {
  const char* path = WEBSTATIONS_PATH;
  if (SPIFFS.exists(&path[2])) {
    Serial.println("> Removing stations list");
    SPIFFS.remove(&path[2]);  
  }
}

bool transferWebStations() {
  if (readWebStationFile(WEBSTATIONS_COPY)) {
    writeWebStations();
    return true;
  }
  return false;
}


//Action from stations list
void webRadioListAction(lv_event_t * event) {
  lv_obj_t * obj = lv_event_get_target(event);
  strncpy(webStationName, lv_list_get_btn_text(dabStationList, obj), 34);
  webStationName[34] = '\0';
  char* url = (char*)lv_obj_get_user_data(obj);
  listSetSelect(obj);
  tabViewShowMain();
  if (strncmp(settings->server, url, 255) != 0) {
    strncpy(settings->server, url, 255);
    settings->server[255] = '\0';
    writeSettings();
  }  
  updateUrlEditText();
  connectToHost(url, true);
}
