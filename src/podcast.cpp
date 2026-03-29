#include "decls.h"
char* podEpisodeName;
char* podEpisodeDescription;

bool podcastsModified = false;
bool podClientActive = false;     //Lockout
bool podGettingLatest = false;    //Another lockout
podInfo* currentPodcast = NULL; 
int autoplay = 0;

void podEditSearchAction(lv_event_t * event);
void keyboardPodSearchKeyAction(lv_event_t * event);
uint8_t parsePodClient(int c);
void parsePodClientTag(uint8_t level, char* tag, char* val);
void CreatePodQueues();

#define POD_TIMEOUT    60000

#define POD_EPISODEMAX   30

//Called from setup() to initialise memory
void initPodcast() {
  podEpisodeName = (char*)ps_malloc(260);
  podEpisodeName[0] = '\0';
  podEpisodeDescription = (char*)ps_malloc(260);
  podEpisodeDescription[0] = '\0';
  CreatePodQueues();
}

lv_obj_t * addPodcastButton(podInfo* info);
void podEpisodesFinished();
void podEpisodesStarted();
void addNewPodcast();
bool readPodcastFile(const char* filename);
void podSearch(String term, lv_obj_t*(*addResult)(podInfo*), void(*finish)());
void podGetEpisodes(String id, int numResults, lv_obj_t*(*addResult)(podInfo*), void(*finish)());
void podGetLatestEpisodes();
void podGetLatestEpisodes2();

//-----------------------------------
// Podcast list window

lv_obj_t * podcastWindow;
lv_obj_t * podMainList;
lv_obj_t * podEpisodeList;
lv_obj_t * podUpBtn;
lv_obj_t * podTopBtn;
lv_obj_t * podLatestBtn;
lv_obj_t * podSaveBtn;
lv_obj_t * podHeaderLabel;
lv_obj_t * podcastSpinner;
bool podcastEpisodes = false;

void podGoUp(lv_event_t * event);
void podWindow_totop_action(lv_event_t * event);
void podWindow_del_action(lv_event_t * event);
void podWindow_save_action(lv_event_t * event);
void podWindow_latest_action(lv_event_t * event);
void podWindow_search_action(lv_event_t * event);

void createPodcastWindow(lv_obj_t * parent) {    
  //Styles first

  podcastWindow = lv_win_create(parent, 40);
  podUpBtn = lv_win_add_btn(podcastWindow, LV_SYMBOL_LEFT, 50);
  lv_obj_add_event_cb(podUpBtn, podGoUp, LV_EVENT_CLICKED, NULL);
  lv_obj_add_style(podUpBtn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(podUpBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(podUpBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_set_hidden(podUpBtn, true);

  podHeaderLabel = lv_win_add_title(podcastWindow, "Podcasts");
  lv_obj_set_style_pad_left(podHeaderLabel, 6, LV_PART_MAIN);
  lv_obj_set_size(podcastWindow, lv_obj_get_content_width(parent), lv_obj_get_content_height(parent));
  lv_obj_set_pos(podcastWindow, 0, 0);
  lv_obj_add_style(podcastWindow, &style_win, LV_PART_MAIN);
  lv_obj_t * win_content = lv_win_get_content(podcastWindow);
  lv_obj_add_style(win_content, &style_win, LV_PART_MAIN);
  lv_obj_set_hidden(podcastWindow, true);

  podSaveBtn = lv_win_add_btn(podcastWindow, LV_SYMBOL_SAVE, 50);          
  lv_obj_add_style(podSaveBtn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(podSaveBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(podSaveBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(podSaveBtn, podWindow_save_action, LV_EVENT_CLICKED, NULL);
  lv_obj_set_hidden(podSaveBtn, true);  
  
  podTopBtn = lv_win_add_btn(podcastWindow, LV_SYMBOL_UP, 50); 
  lv_obj_add_style(podTopBtn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(podTopBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(podTopBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(podTopBtn, podWindow_totop_action, LV_EVENT_CLICKED, NULL);

  podLatestBtn = lv_win_add_btn(podcastWindow, LV_SYMBOL_REFRESH, 50);
  lv_obj_add_style(podLatestBtn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(podLatestBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(podLatestBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(podLatestBtn, podWindow_latest_action, LV_EVENT_CLICKED, NULL);

  lv_obj_t * podSearchBtn = lv_win_add_btn(podcastWindow, LV_SYMBOL_SEARCH, 50);
  lv_obj_add_style(podSearchBtn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(podSearchBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(podSearchBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(podSearchBtn, podWindow_search_action, LV_EVENT_CLICKED, NULL);

  podMainList = lv_list_create(win_content);
  lv_obj_set_style_bg_opa(podMainList, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_add_style(podMainList, &style_biggestfont, LV_PART_MAIN);
  lv_obj_update_layout(win_content);
  lv_obj_set_size(podMainList, lv_obj_get_content_width(win_content), lv_obj_get_content_height(win_content));
  lv_obj_set_pos(podMainList, 0, 0);

  podEpisodeList = lv_list_create(win_content);
  lv_obj_set_style_bg_opa(podEpisodeList, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_add_style(podEpisodeList, &style_biggestfont, LV_PART_MAIN);
  lv_obj_update_layout(win_content);
  lv_obj_set_size(podEpisodeList, lv_obj_get_content_width(win_content), lv_obj_get_content_height(win_content));
  lv_obj_set_pos(podEpisodeList, 0, 0);
  lv_obj_set_hidden(podEpisodeList, true);

    //Loading spinner
  podcastSpinner = lv_spinner_create(win_content, 1000, 60);
  lv_obj_set_size(podcastSpinner, 100, 100);
  lv_obj_align(podcastSpinner, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(podcastSpinner, LV_OBJ_FLAG_FLOATING);
  lv_obj_set_hidden(podcastSpinner, true);

  lv_obj_set_hidden(podcastWindow, true);
}

//Show podcast list for the right modes
void setPodVisibility() {
  lv_obj_set_hidden(podcastWindow, settings->mode != MODE_POD);  
}

//Update the window header and buttons
void updatePodcastHeader() {
  //Set text
  if (podcastEpisodes && currentPodcast) {
    lv_label_set_text_fmt(podHeaderLabel, "%s%s", currentPodcast->name, podcastsModified?" (*)":"");
  } else {
    lv_label_set_text_fmt(podHeaderLabel, "Podcasts%s", podcastsModified?" (*)":"");
  }
  //Show/hide buttons
  lv_obj_set_hidden(podSaveBtn, podcastEpisodes || !podcastsModified);  
  lv_obj_set_hidden(podUpBtn, !podcastEpisodes);
  lv_obj_set_hidden(podLatestBtn, podcastEpisodes);
}

//Activity spinner
void showPodcastSpinner(bool shown) {
  lv_obj_set_hidden(podcastSpinner, !shown);
}

//Close episodes window and return to podcast list
void podCloseEpisodes() {
  if (podcastEpisodes) {
    podcastEpisodes = false;
    lv_obj_set_hidden(podEpisodeList, true);
    lv_obj_set_hidden(podMainList, false);
    clearPodEpisodes();
    updatePodcastHeader();
  }
}

//Previous window
void podGoUp(lv_event_t * event) {
  podStop();
  podCloseEpisodes();
}

//Get latest episodes action Penis
void podWindow_latest_action(lv_event_t * event) {
  podGetLatestEpisodes();
}

//Scroll to top of window action
void podWindow_totop_action(lv_event_t * event) {
  listMenuClose(podMainList, false);
  if (podcastEpisodes) lv_obj_scroll_to(podEpisodeList, 0, 0, LV_ANIM_ON); 
  else lv_obj_scroll_to(podMainList, 0, 0, LV_ANIM_ON);
}

void podWindow_search_action(lv_event_t * event) {
  lv_obj_set_hidden(podcastWindow, true);
  hidePodSearchWindow(false);
}

//Call after list changes to handle autosave
void podcastsChanged() {
  if (settings->autosave) {
    writePodcasts(PODLIST_PATH);
  } else {
    podcastsModified = true;
  }
  updatePodcastHeader();
}

//Delete selected podcast action
void podWindow_del_action(lv_event_t * event) {
  listMenuClose(podMainList, true);
  int index = listGetSelect(podMainList);
  if (index >= 0) {
    lv_obj_t* child = lv_obj_get_child(podMainList, index);
    lv_obj_t* text = lv_obj_get_child(podMainList, index + 1);
    podInfo* info = (podInfo*)lv_obj_get_user_data(child);
    if (info) {
      delete((podInfo*)info);
      lv_obj_del(child);
      lv_obj_del(text);
      podcastsChanged();
    }
  }
}

//Save podcast list action
void podWindow_save_action(lv_event_t * event) {
  if (podcastsModified) {
    writePodcasts(PODLIST_PATH);
    podcastsModified = false;
    updatePodcastHeader();
  }
}

//Clear podcast list (and episodes) and also free the entry's attached userdata
void clearPodcasts() {
  clearPodEpisodes();
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(podMainList); i++) {
    lv_obj_t* child = lv_obj_get_child(podMainList, i);
    void * data = lv_obj_get_user_data(child);
    if (data) delete((podInfo*)data);
  }
  lv_obj_clean(podMainList);
}

//Clear episode list and also free the entry's attached userdata
void clearPodEpisodes() {
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(podEpisodeList); i++) {
    lv_obj_t* child = lv_obj_get_child(podEpisodeList, i);
    void * data = lv_obj_get_user_data(child);
    if (data) delete((podInfo*)data);
  }
  lv_obj_clean(podEpisodeList);
}

void podListMove(int dir) {
  if (!podMainList || podGettingLatest) return;
  lv_obj_t* list = podMainList;
  if (podcastEpisodes) list = podEpisodeList;
  int numItems = ((lv_obj_get_child_cnt(list) - 1) >> 1) << 1;
  int index = ((listGetSelect(list) >> 1) + dir) << 1;
  if (index > numItems) index = numItems;
  if (index < 0) index = 0;
  listSetSelect(list, index);
}

//Context menu
void podcastListMenu(lv_event_t * event) {
  lv_obj_t * item = lv_event_get_target(event);
  lv_obj_t * menu = createListMenu(item);
  lv_obj_t * btn;
  btn = lv_list_add_btn(menu, LV_SYMBOL_TRASH, "Delete Podcast");
  lv_obj_add_event_cb(btn, podWindow_del_action, LV_EVENT_CLICKED, NULL);
  listMenuActivate(menu);
}

void podEntryActivate(lv_obj_t* child) {
  //retrieve pocast info
  podInfo* info = (podInfo*)lv_obj_get_user_data(child);
  if (!info) return;
  if (!info->isEpisode) {
    //This is a podcast entry
    if (currentPodcast) delete(currentPodcast);
    currentPodcast = new podInfo(info);
    if (WiFi.status() != WL_CONNECTED) listWarning(podEpisodeList, "WiFi Not Connected");
    //Retrieve episode list
    podGetEpisodes(String(currentPodcast->id), POD_EPISODEMAX, addPodcastButton, podEpisodesFinished);
    lv_obj_set_hidden(podEpisodeList, false);
    lv_obj_set_hidden(podMainList, true);
    podcastEpisodes = true;
    podEpisodesStarted();
  } else {
    //This is an episode entry
    if (info->url) {  //Play it
      strcpy(podEpisodeName, info->name);
      strcpy(podEpisodeDescription, info->description);
      tabViewShowMain();
      connectToHost(info->url, false);
    }
  }
  updateUrlEditText();    
}

void podActivate() {
  if (!podMainList || podGettingLatest) return;
  lv_obj_t* list = podMainList;
  if (podcastEpisodes) list = podEpisodeList;
  lv_obj_t* entry = listGetSelectObj(list);
  if (entry) podEntryActivate(entry);
}

//When a podcast entry is selected
void podcastAction(lv_event_t * event) {
  lv_obj_t * child = lv_event_get_target(event);  //selected object
  //Return if already active
  if (podGettingLatest || podClientActive) {    //Wait for getting latest..
    listClearSelect(podMainList);
    return;
  }                   
  listSetSelect(child);
  podEntryActivate(child);
} 

char* timeAgo(int32_t ago) {
  static char val[16] = {0};
  int32_t seconds = utf() - ago;
  int32_t interval = seconds / 2592000;
  if (interval >= 1) { sprintf(val, "Old"); return val; }
  interval = seconds / 604800;
  if (interval == 1) { sprintf(val, "Last Week"); return val; }
  if (interval > 1) { sprintf(val, "%d Weeks", interval); return val; }
  interval = seconds / 86400;
  if (interval == 1) { sprintf(val, "Yesterday"); return val; }
  if (interval > 1) { sprintf(val, "%d Days", interval); return val; }
  interval = seconds / 3600;
  if (interval == 1) { sprintf(val, "1 Hour"); return val; };
  if (interval > 1) { sprintf(val, "%d Hours", interval); return val; };
  interval = seconds / 60;
  if (interval <= 1) { sprintf(val, "Just Now"); return val; };
  sprintf(val, "%d Mins", interval); 
  return val;
}

//Add an entry to the podcast list
lv_obj_t * addPodcastButton(podInfo* info) {
  if (!info || !podMainList) return NULL;
  lv_obj_t * list_btn;
  if (info->isEpisode) {  //Add to episode list
    if (info->lastUpdateTime > 0) {
      //Serial.println(info->lastUpdateTime);
      char strbuf[300] = {0};
      sprintf(strbuf, "%s (%s)", info->name, timeAgo(info->lastUpdateTime));
      list_btn = lv_list_add_btn(podEpisodeList, LV_SYMBOL_AUDIO, strbuf);
    } else list_btn = lv_list_add_btn(podEpisodeList, LV_SYMBOL_AUDIO, info->name);
    lv_obj_t * list_txt = lv_list_add_text(podEpisodeList, info->description);
    lv_obj_set_style_text_font(list_txt, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(list_txt, 6, LV_PART_MAIN);
    lv_label_set_long_mode(list_txt, LV_LABEL_LONG_WRAP);
    lv_obj_add_event_cb(list_btn, podcastAction, LV_EVENT_CLICKED, NULL);
  } else { //Insertion sort - main list 
    list_btn = lv_list_add_btn(podMainList, LV_SYMBOL_BULLHORN, info->name);
    lv_obj_t * list_txt = lv_list_add_text(podMainList, "");
    lv_obj_set_style_text_font(list_txt, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(list_txt, 6, LV_PART_MAIN);
    lv_label_set_long_mode(list_txt, LV_LABEL_LONG_WRAP);
    lv_obj_set_hidden(list_txt, true);
    int num = lv_obj_get_child_cnt(podMainList);
    if (num > 1) {
      for (int i = num - 2; i >= 0 ; i-=2) {
        lv_obj_t* btn = lv_obj_get_child(podMainList, i);
        lv_obj_t* txt = lv_obj_get_child(podMainList, i + 1);
        const char * lbl = lv_list_get_btn_text(podMainList, btn); 
        if (strcasecmp(lbl, info->name) < 0) break;
        lv_obj_swap(list_btn, btn);
        lv_obj_swap(list_txt, txt);
      }
    }
    lv_obj_add_event_cb(list_btn, podcastAction, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_event_cb(list_btn, podcastListMenu, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(list_btn, listMenuClicked, LV_EVENT_CLICKED, NULL);
  }
  lv_obj_add_flag(list_btn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_add_style(list_btn, &style_list, LV_PART_MAIN);
  lv_obj_add_style(list_btn, &style_listsel, LV_STATE_CHECKED);
  lv_obj_set_user_data(list_btn, (void*)info);
  return list_btn;
}

//Episode fetch begin
void podEpisodesStarted() {
  showPodcastSpinner(true);
  updatePodcastHeader();
  updateUrlEditText();
}

//Episode fetch end
void podEpisodesFinished() {
  updatePodcastHeader();
  showPodcastSpinner(false);
}

//look to see if podcast id is already in the podcast list
bool podcastInList(uint32_t id) {
  if (!podMainList) return false;
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(podMainList); i++) {
    lv_obj_t* child = lv_obj_get_child(podMainList, i);
    podInfo* data = (podInfo*)lv_obj_get_user_data(child);
    if (data && data->id == id) return true;
  }
  return false;
}

//Called from webradioHandle() when audio file starts playing in MODE_POD
void podShowEpisodeInfo() {
  info(NAME, 0, LV_SYMBOL_AUDIO " - %s - ", podEpisodeName);
  info(NOW, 0, podEpisodeDescription); 
  //Is this a new podcast?
  if (currentPodcast && !podcastInList(currentPodcast->id)) {
    //Shorten name with ellipsis
    char name[64];
    strncpy(name, currentPodcast->name, 60);
    name[61] = '.';
    name[62] = '.';
    name[63] = '\0';
    //construct popup
    char buf[128];
    snprintf(buf, 128, " "  "  Keep '%s'?", name);
    popup(buf, addNewPodcast, true);
  } else closePopup(true);
}

//Add currently playing station to station list
void addNewPodcast() {
  lv_obj_t * btn = addPodcastButton(new podInfo(currentPodcast));
  listSetSelect(btn);
  podcastsChanged();
}

//Called from webradioHandle() when audio file finishes
void podEOF() {
  podEpisodeName[0] = '\0';
  podEpisodeDescription[0] = '\0';
  updateUrlEditText();
  info(NOW, 0, LV_SYMBOL_LEFT " Choose podcasts from the list to the left.. ");
  if (progTimeBar) lv_bar_set_value(progTimeBar, 0, LV_ANIM_OFF);
  clearBufStat();
}

//Queries from other windows
const char* getPodcastName() { return currentPodcast?currentPodcast->name:""; }
const char* getPodcastDescription() { return currentPodcast?currentPodcast->description:""; }
const char* getPodEpisodeName() { return podEpisodeName; }
const char* getPodEpisodeDescription() { return podEpisodeDescription; }


lv_obj_t * podStartPlaying(podInfo* info) {
  if (info) {
    if (info->url) {  //Play it
      strcpy(podEpisodeName, info->name);
      strcpy(podEpisodeDescription, info->description);
      //tabViewShowMain();
      connectToHost(info->url, false);
      updateUrlEditText();    
    }
  }
  return NULL;
}

//Called from presets
void podPlayLatestEpisode(int id) {
  if (!podMainList) return;
  bool found = false;
  podCloseEpisodes();
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(podMainList); i++) {
    lv_obj_t* child = lv_obj_get_child(podMainList, i);
    podInfo* data = (podInfo*)lv_obj_get_user_data(child);
    if (data && data->id == id) {
      found = true;
      lv_obj_add_state(child, LV_STATE_CHECKED);
      if (currentPodcast) delete(currentPodcast);
      currentPodcast = new podInfo(data);
    } else lv_obj_clear_state(child, LV_STATE_CHECKED);
  }
  if (found) podGetEpisodes(String(id), 1, podStartPlaying, nullptr);
}



//----------------------------------------
//Latest Episodes sequencer

int podLatestIndex;               //Sequence counter - also index into podMainList
lv_obj_t* podLatestTxt;           //Pointer to current list label
bool podLatestTimeout = false;    //Timeout

//Callback with latest episode info
lv_obj_t * podGotLatestEpisode(podInfo* info) {
  if (info) {
    if (info->lastUpdateTime > 0) {
      lv_label_set_text_fmt(podLatestTxt, "     " LV_SYMBOL_AUDIO " %s (%s)", info->name, timeAgo(info->lastUpdateTime));
    } else lv_label_set_text_fmt(podLatestTxt, "     " LV_SYMBOL_AUDIO " %s", info->name);
    podLatestTimeout = false;
  }
  return NULL;
}

//Callback to fetch next podcast's episode info
void podNextLatestEpisode() {
  //Timeout?
  if (podLatestTimeout && podLatestTxt)
    lv_label_set_text(podLatestTxt, "     " LV_SYMBOL_WARNING " Timeout!");
  //End of list?
  if (podLatestIndex >= lv_obj_get_child_cnt(podMainList)) {
    hideWebControls();
    showPodcastSpinner(false);
    podGettingLatest = false;
    return;
  }
  //Fetch podcast info and ask for latest episode
  podLatestTimeout = true;
  lv_obj_t* btn = lv_obj_get_child(podMainList, podLatestIndex++);
  podInfo* info = (podInfo*)lv_obj_get_user_data(btn);
  if (info) {    
    podLatestTxt = lv_obj_get_child(podMainList, podLatestIndex++);
    lv_label_set_text(podLatestTxt, "     " LV_SYMBOL_REFRESH " Fetching..");
    lv_obj_set_hidden(podLatestTxt, false);
    podGetEpisodes(String(info->id), 1, podGotLatestEpisode, podNextLatestEpisode);
  }
}

//Start latest episode sequencer
void podGetLatestEpisodes() {
  if (!podGettingLatest) {    //Don't do it twice at once
    podGettingLatest = true;
    info(NOW, 5, LV_SYMBOL_BULLHORN " Fetching latest episodes..");
    showLoadSpinner();
    showPodcastSpinner(true);
    podLatestIndex = 0;
    podNextLatestEpisode();
  }
}

//Faster episode sequencer

int podFindById(int id) {
  for (int i = 0; i < lv_obj_get_child_cnt(podMainList); i++) {
    lv_obj_t* btn = lv_obj_get_child(podMainList, i);
    podInfo* info = (podInfo*)lv_obj_get_user_data(btn);
    if (info && info->id == id) return i;   
  }
  return -1;
}

//Callback with latest episode info
lv_obj_t * podGotLatestEpisodes(podInfo* info) {
  if (info) {
    int index = podFindById(info->feedId);
    serial.printf("name:%s, feedId %d, index %d\r\n", info->name, info->feedId, index);
    if (index >= 0) {
      lv_obj_t* txt = lv_obj_get_child(podMainList, index + 1);
      if (txt) {
        lv_label_set_text_fmt(txt, "     " LV_SYMBOL_AUDIO " %s", info->name);
        lv_obj_set_hidden(txt, false);
      }
    }
  }
  return NULL;
}

//Callback to fetch next podcast's episode info
void podLatestEpisodeDone() {
  hideWebControls();
  showPodcastSpinner(false);
  podGettingLatest = false;
}

void podGetLatestEpisodes2() {
  if (lv_obj_get_child_cnt(podMainList) == 0) return;
  if (!podGettingLatest) {    //Don't do it twice at once
    podGettingLatest = true;
    info(NOW, 2, LV_SYMBOL_BULLHORN " Fetching latest episodes..");
    showLoadSpinner();
    showPodcastSpinner(true);
    lv_obj_t* btn = lv_obj_get_child(podMainList, 0);
    podInfo* info = (podInfo*)lv_obj_get_user_data(btn);
    String idList = String(info->id);
    for (int i = 1; i < lv_obj_get_child_cnt(podMainList); i++) {
      btn = lv_obj_get_child(podMainList, i);
      info = (podInfo*)lv_obj_get_user_data(btn);
      if (info) idList += String(",") + String(info->id);
    }
    podGetEpisodes(idList, PODCAST_LATEST, podGotLatestEpisodes, podLatestEpisodeDone);
  }
}

//-------------------------------------------------
// Pocast file management

//Look for the podcast list file, transfer from SD if needed
void readPodcasts() {
  showLoadSpinner();
  showPodcastSpinner(true);
  serial.print("> Reading podcasts..");
  if (readPodcastFile(PODLIST_PATH)) serial.println("OK."); 
  else if (readPodcastFile(PODLIST_COPY)) {
    serial.println("OK.\n> Podcasts transferred from SD.");
    writePodcasts(PODLIST_PATH);
  }
  else serial.println("Podcast list empty.");
  hideWebControls();
  showPodcastSpinner(false);
  if (autoplay) {
    podPlayLatestEpisode(autoplay);
    autoplay = 0;
  }
    //podGetLatestEpisodes();
}

//Read the podcast list file into the podcast list
bool readPodcastFile(const char* filename) {
  lv_fs_file_t f;
  if (lv_fs_open(&f, filename, LV_FS_MODE_RD) != LV_FS_RES_OK) return false;
  int lbi = 0;
  char linebuf[516];
  int32_t id;
  uint32_t read_num;
  char ch;
  bool search = strlen(searchStationName);
  clearPodcasts();
  while (1) {
    if (fs_err(lv_fs_read(&f, &ch, 1, &read_num), "Read podcast list")) return false;
    if (read_num == 0) break;
    if (ch != '\r' && ch != '\n' ) linebuf[lbi++] = ch;
    if (ch == '\n' || lbi == 511) {
      linebuf[lbi] = 0;
      if (linebuf[0] == '#') {
        char * name = strchr(linebuf, ',');
        char * desc = strchr(linebuf, '\\');
        if (name && desc) {
          *name++ = 0;
          *desc++ = 0;
          id = atol(linebuf+1);
          if (id) {
            podInfo* info = new podInfo(id, name, desc, false, "", -1, 0);
            addPodcastButton(info);
            if (search && strcmp(name, searchStationName) == 0) {
              serial.print(" (found) ");
              autoplay = id;
            }
          }
        }
      }
      lbi = 0;
    }
  }
  lv_fs_close(&f);
  searchStationName[0] = '\0';  
  return true;
}


//Write the current podlist to the podlist file
// format is one entry per line:
// #[podcast id],[podcast name]\[podcast description]
void writePodcasts(const char* path) {
  lv_fs_file_t f;
  char str[516];
  uint32_t byteswrote;
  //Build podlist
  removePodcasts();
  if (fs_err(lv_fs_open(&f, path, LV_FS_MODE_WR), "Open Podcast list")) return;
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(podMainList); i++) {   //every even entry is a button
    lv_obj_t* child = lv_obj_get_child(podMainList, i);
    podInfo* info = (podInfo*)lv_obj_get_user_data(child);
    if (info) {
      snprintf(str, 515, "#%d,%s\\%s\r\n", info->id, info->name, info->description);
      if (fs_err(lv_fs_write(&f, str, strlen(str), &byteswrote), "Write Podcast list Entry")) return;
    }
  }  
  lv_fs_close(&f);
}

//Remove (delete) podlist
void removePodcasts() {
  const char* path = PODLIST_PATH;
  if (path[0] == 'C') { //SD
    if (SD.exists(&path[2])) {
      serial.println("> Removing old podcast list.");
      SD.remove(&path[2]);  
    }
  } else if (path[0] == 'D') { //SPIFFS
    if (SPIFFS.exists(&path[2])) {
      serial.println("> Removing old podcast list.");
      SPIFFS.remove(&path[2]);  
    }
  }
}

//Look for the podcast list file on SD and transfer
// Factory restore function
// tofrom: false = to SD, true = from SD
bool transferPodcasts(bool tofrom) {
  if (tofrom) {
    if (readPodcastFile(PODLIST_COPY)) {
      writePodcasts(PODLIST_PATH);
      return true;
    }
  } else {
    if (readPodcastFile(PODLIST_PATH)) {
      writePodcasts(PODLIST_COPY);
      return true;
    }
  }
  return false;
}



//-----------------------------------------------------------------------------
// Podcast search window

lv_obj_t * podSearchWindow;
lv_obj_t * podSearchEditText;
lv_obj_t * podSearchResultList;
lv_obj_t * podSearchResults;
lv_obj_t * podSearchEpisodeList;
lv_obj_t * podSearchEpisodeCont;
lv_obj_t * podSearchSpinner;
lv_obj_t * podSearchUpBtn;
lv_obj_t * podSearchHeaderLabel;

int podSearchResultCount;   //Number of search results
bool podSearchEpisodes = false;

void podSearchGoUp(lv_event_t * event);
void podSearchWindow_totop_action(lv_event_t * event);
void podSearchWindow_close_action(lv_event_t * event);
void activatePodSearch();
lv_obj_t * addPodSearchResult(podInfo* info);
void podClientStop();
void podSearchAction(lv_event_t * event);
void clearPodSearchResults();
void podSearchFinished();
void podSearchStarted();

void createPodSearchWindow(lv_obj_t * parent) {    
  //Styles first

  podSearchWindow = lv_win_create(parent, 40);
  podSearchUpBtn = lv_win_add_btn(podSearchWindow, LV_SYMBOL_LEFT, 50);
  lv_obj_add_event_cb(podSearchUpBtn, podSearchGoUp, LV_EVENT_CLICKED, NULL);
  lv_obj_add_style(podSearchUpBtn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(podSearchUpBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(podSearchUpBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_set_hidden(podSearchUpBtn, true);

  podSearchHeaderLabel = lv_win_add_title(podSearchWindow, "Podcast Search");
  lv_obj_set_style_pad_left(podSearchHeaderLabel, 6, LV_PART_MAIN);
  lv_obj_set_size(podSearchWindow, lv_obj_get_content_width(parent), lv_obj_get_content_height(parent));
  lv_obj_set_pos(podSearchWindow, 0, 0);
  lv_obj_add_style(podSearchWindow, &style_win, LV_PART_MAIN);
  lv_obj_t * win_content = lv_win_get_content(podSearchWindow);
  lv_obj_add_style(win_content, &style_win, LV_PART_MAIN);
  lv_obj_clear_flag(win_content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_hidden(podSearchWindow, true);

  lv_obj_t * totop_btn = lv_win_add_btn(podSearchWindow, LV_SYMBOL_UP, 50);           /*Add close button and use built-in close action*/
  lv_obj_add_style(totop_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(totop_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(totop_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(totop_btn, podSearchWindow_totop_action, LV_EVENT_CLICKED, NULL);

  lv_obj_t * close_btn = lv_win_add_btn(podSearchWindow, LV_SYMBOL_CLOSE, 50);           /*Add close button and use built-in close action*/
  lv_obj_add_style(close_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(close_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(close_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(close_btn, podSearchWindow_close_action, LV_EVENT_CLICKED, NULL);

  //Search text and result list in a scrollable container
  podSearchResults = lv_obj_create(win_content);
  lv_obj_set_pos(podSearchResults, 0, 0);
  lv_obj_add_style(podSearchResults, &style_groupbox, LV_PART_MAIN);
  lv_obj_set_style_border_width(podSearchResults, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_top(podSearchResults, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(podSearchResults, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_left(podSearchResults, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_right(podSearchResults, 0, LV_PART_MAIN);

  //Search edit textbox
  lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
  podSearchEditText = lv_textarea_create(podSearchResults);
  lv_obj_set_pos(podSearchEditText, 6, 6);
  lv_obj_add_style(podSearchEditText, &style_ta, LV_PART_MAIN);
  lv_textarea_set_text_selection(podSearchEditText, false);
  lv_textarea_set_one_line(podSearchEditText, true);
  lv_obj_set_size(podSearchEditText, lv_obj_get_content_width(win_content) - 12, 36);
  lv_textarea_set_text(podSearchEditText, "");
  lv_obj_add_event_cb(podSearchEditText, podEditSearchAction, LV_EVENT_PRESSED, NULL);

  podSearchResultList = lv_list_create(podSearchResults);
  lv_obj_set_size(podSearchResultList, lv_obj_get_content_width(win_content) - 4, LV_SIZE_CONTENT);
  lv_obj_set_pos(podSearchResultList, 0, 48);
  lv_obj_add_style(podSearchResultList, &style_halfopa, LV_PART_MAIN);
  lv_obj_add_style(podSearchResultList, &style_biggestfont, LV_PART_MAIN);
  //lv_obj_clear_flag(podSearchResultList, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(podSearchResults, lv_obj_get_width(win_content), lv_obj_get_height(win_content));

  //Episodes list in a scrollable container
  podSearchEpisodeCont = lv_obj_create(win_content);
  lv_obj_set_pos(podSearchEpisodeCont, 0, 0);
  lv_obj_add_style(podSearchEpisodeCont, &style_groupbox, LV_PART_MAIN);
  lv_obj_set_style_border_width(podSearchEpisodeCont, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_top(podSearchEpisodeCont, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(podSearchEpisodeCont, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_left(podSearchEpisodeCont, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_right(podSearchEpisodeCont, 0, LV_PART_MAIN);
  lv_obj_set_hidden(podSearchEpisodeCont, true);

  podSearchEpisodeList = lv_list_create(podSearchEpisodeCont);
  lv_obj_set_size(podSearchEpisodeList, lv_obj_get_content_width(win_content) - 4, LV_SIZE_CONTENT);
  lv_obj_set_pos(podSearchEpisodeList, 0, 0);
  lv_obj_add_style(podSearchEpisodeList, &style_halfopa, LV_PART_MAIN);
  lv_obj_add_style(podSearchEpisodeList, &style_biggestfont, LV_PART_MAIN);
  lv_obj_set_size(podSearchEpisodeCont, lv_obj_get_width(win_content), lv_obj_get_height(win_content));

    //Loading spinner
  podSearchSpinner = lv_spinner_create(win_content, 1000, 60);
  lv_obj_set_size(podSearchSpinner, 100, 100);
  lv_obj_align(podSearchSpinner, LV_ALIGN_CENTER, 0, 20);
  lv_obj_add_flag(podSearchSpinner, LV_OBJ_FLAG_FLOATING);
  lv_obj_set_hidden(podSearchSpinner, true);
}

//Visibility of whole window
void hidePodSearchWindow(bool hide) {
  lv_obj_set_hidden(podSearchWindow, hide);
}

//Switch between podcast search result list and episode list
void hidePodResultList(bool hide) {
  lv_obj_set_hidden(podSearchResults, hide);
  lv_obj_set_hidden(podSearchEpisodeCont, !hide);
  lv_obj_set_hidden(podSearchUpBtn, !hide);
}

//Clear podcast search result list
void clearPodSearchResults() {
  lv_obj_scroll_to(podSearchResults, 0, 0, LV_ANIM_OFF);  
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(podSearchResultList); i++) {
    lv_obj_t* child = lv_obj_get_child(podSearchResultList, i);
    void * data = lv_obj_get_user_data(child);
    if (data) delete((podInfo*)data);
  }
  lv_obj_clean(podSearchResultList);
  podSearchResultCount = 0;
}

//Clear episode list
void clearPodSearchEpisodes() {
  lv_obj_scroll_to(podSearchEpisodeCont, 0, 0, LV_ANIM_OFF);  
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(podSearchEpisodeList); i++) {
    lv_obj_t* child = lv_obj_get_child(podSearchEpisodeList, i);
    void * data = lv_obj_get_user_data(child);
    if (data) delete((podInfo*)data);
  }
  lv_obj_clean(podSearchEpisodeList);
}

//Update the window header
void updatePodSearchHeader() {
  if (podSearchEpisodes && currentPodcast) lv_label_set_text(podSearchHeaderLabel, currentPodcast->name);
  else lv_label_set_text_fmt(podSearchHeaderLabel, "Search (%d results)", podSearchResultCount);    
}

//Close and clear the episode list
void podSearchCloseEpisodes() {
  if (podSearchEpisodes) {
    podSearchEpisodes = false;
    hidePodResultList(false);
    clearPodSearchEpisodes();
    updatePodSearchHeader();
  }
}

//Return to search results action
void podSearchGoUp(lv_event_t * event) {
  podStop();
  podSearchCloseEpisodes();
}

//Scroll to top of list action
void podSearchWindow_totop_action(lv_event_t * event) {
  if (podSearchEpisodes) lv_obj_scroll_to(podSearchEpisodeCont, 0, 0, LV_ANIM_ON);
  else lv_obj_scroll_to(podSearchResults, 0, 0, LV_ANIM_ON);
}

//Close window action
void podSearchWindow_close_action(lv_event_t * event) {
  podStop();
  podSearchCloseEpisodes();
  //clearPodSearchResults();
  lv_obj_set_hidden(podSearchWindow, true);
  keyboardHide(false, NULL);
  lv_obj_set_hidden(podcastWindow, false);
}

//Edit the search term action
void podEditSearchAction(lv_event_t * event) {
  lv_obj_t * target = lv_event_get_target(event);
  if (!keyboardShowing()) {
    keyboardShow(lv_scr_act(), target, keyboardPodSearchKeyAction);
  }
}

//Keyboard OK or Cancel on search term edit
void keyboardPodSearchKeyAction(lv_event_t * event) {
  uint32_t res = lv_event_get_code(event);
  if(res == LV_EVENT_READY) {
    if (currentPodcast) {
      delete(currentPodcast);
      currentPodcast = NULL;
    }
    keyboardHide(true, activatePodSearch);
  }
  else if (res == LV_EVENT_CANCEL) {
    keyboardHide(true, NULL);
  }
}

//Start the pod search with keyboard hidden callback
void activatePodSearch() {
  clearPodSearchResults();
  if (WiFi.status() != WL_CONNECTED) listWarning(podSearchResultList, "WiFi Not Connected");
  const char* term = lv_textarea_get_text(podSearchEditText);
  podSearch(term, addPodSearchResult, podSearchFinished);
  podSearchStarted();
}

//Add an entry to the search result list
lv_obj_t * addPodSearchResult(podInfo* info) {
  lv_obj_t * list_btn;
  lv_obj_t * list_txt;
  if (!podSearchResultList || !info) return NULL;
  //if ((++podSearchResultCount % 10) == 0)
  updatePodSearchHeader();
  if (info->isEpisode) {
    //Into episode list
    if (info->lastUpdateTime > 0) {
      char strbuf[300] = {0};
      sprintf(strbuf, "%s (%s)", info->name, timeAgo(info->lastUpdateTime));
      list_btn = lv_list_add_btn(podSearchEpisodeList, LV_SYMBOL_AUDIO, strbuf);
    } else list_btn = lv_list_add_btn(podSearchEpisodeList, LV_SYMBOL_AUDIO, info->name);
    list_txt = lv_list_add_text(podSearchEpisodeList, info->description);
  } else {
    //Into search results list
    ++podSearchResultCount;
    list_btn = lv_list_add_btn(podSearchResultList, LV_SYMBOL_BULLHORN, info->name);
    list_txt = lv_list_add_text(podSearchResultList, info->description);
    //Insertion sort
    int num = lv_obj_get_child_cnt(podSearchResultList);
    if (num > 1) {
      for (int i = num - 2; i >= 0 ; i-=2) {
        lv_obj_t* btn = lv_obj_get_child(podSearchResultList, i);
        lv_obj_t* txt = lv_obj_get_child(podSearchResultList, i + 1);
        const char * lbl = lv_list_get_btn_text(podSearchResultList, btn); 
        if (strcasecmp(lbl, info->name) < 0) break;
        lv_obj_swap(list_btn, btn);
        lv_obj_swap(list_txt, txt);
      }
    }
  }
  lv_obj_set_style_text_font(list_txt, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(list_txt, 6, LV_PART_MAIN);
  lv_label_set_long_mode(list_txt, LV_LABEL_LONG_WRAP);
  lv_obj_add_event_cb(list_btn, podSearchAction, LV_EVENT_CLICKED, NULL);
  lv_obj_add_flag(list_btn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_add_style(list_btn, &style_list, LV_PART_MAIN);
  lv_obj_add_style(list_btn, &style_listsel, LV_STATE_CHECKED);
  lv_obj_set_user_data(list_btn, (void*)info);
  return list_btn;
}

//List entry was selected action
void podSearchAction(lv_event_t * event) {
  lv_obj_t * child = lv_event_get_target(event);
  //If the latest episode sequencer is running, don't do this
  if (podGettingLatest) {
    listClearSelect(podSearchResultList);
    return;
  }
  listSetSelect(child);
  podCloseEpisodes();       //Close the episodes over in the podcast list
  podInfo* info = (podInfo*)lv_obj_get_user_data(child);
  if (!info) return;
  if (!info->isEpisode) {
    //Podcast search results
    if (podClientActive) {
      //if we are still searching, just return
      listClearSelect(podSearchResultList);
      return;                //Wait for search to fully load..
    }
    hidePodResultList(true);  //Don't want to be showing the episodes list
    if (currentPodcast) delete(currentPodcast);
    currentPodcast = new podInfo(info);
    podGetEpisodes(String(currentPodcast->id), POD_EPISODEMAX, addPodSearchResult, podSearchFinished);
    podSearchStarted();
  } else {
    //Episode result
    if (info->url) {
      //Play it
      strcpy(podEpisodeName, info->name);
      strcpy(podEpisodeDescription, info->description);
      tabViewShowMain();
      connectToHost(info->url, false);
    }
  }
  updateUrlEditText();
}

//Search begun
void podSearchStarted() {
  updatePodSearchHeader();
  lv_obj_set_hidden(podSearchSpinner, false);
}

//Search ended
void podSearchFinished() {
  updatePodSearchHeader();
  lv_obj_set_hidden(podSearchSpinner, true);  
}

//------------------------------------------------------------------------------
//Podcastindex client 

HTTPClient podClient; // wifi client object
//triggers
bool startPodSearch = false;
bool startPodEpisodes = false;
//flags
bool podClientEpisodes = false;
//state
String podSearchTerm;
int podEpisodeId;
int podNumResults;
//function pointers
lv_obj_t*(*addPodResult)(podInfo*);
void(*podFinished)();

//Message in podcast queue
struct podcastMessage{
    podInfo*    info;
    const char* term;
    int         numResults;
    uint8_t     cmd;
    bool        episodes;
} podcastTxMessage, podcastRxMessage;

//The podcast message queue handles
QueueHandle_t podcastSetQueue = NULL;
QueueHandle_t podcastGetQueue = NULL;

//Podcast message types
enum : uint8_t { POD_SEARCH, POD_STOP, POD_RESULT, POD_ERROR, POD_FINISHED };

//Start a search for podcasts using 'term'
void podSearch(String term, lv_obj_t*(*addResult)(podInfo*), void(*finish)()) {
  podSearchEpisodes = false;  //"we are searching for episodes" flag
  podSearchTerm = urlencode(term);       //Cache the search term string
  addPodResult = addResult;   //Callback pointer
  podFinished = finish;       //another callback
  //Construct a message
  podcastTxMessage.cmd = POD_SEARCH;
  podcastTxMessage.term = podSearchTerm.c_str();    //use the cached string
  podcastTxMessage.episodes = false;
  xQueueSend(podcastSetQueue, &podcastTxMessage, 0);
}

//Start a request for episode list
void podGetEpisodes(String id, int numResults, lv_obj_t*(*addResult)(podInfo*), void(*finish)()) {
  podSearchEpisodes = true;   //Yes, episodes
  podSearchTerm = id;
  addPodResult = addResult;
  podFinished = finish;
  //Message
  podcastTxMessage.cmd = POD_SEARCH;
  podcastTxMessage.episodes = true;
  podcastTxMessage.term = podSearchTerm.c_str();    //use the cached string
  podcastTxMessage.numResults = numResults; //how many results we want
  xQueueSend(podcastSetQueue, &podcastTxMessage, 0);
}

//Send a stop message to the podcast client
void podStop() {
  podcastTxMessage.cmd = POD_STOP;
  xQueueSend(podcastSetQueue, &podcastTxMessage, 0);
}

//Called from loop()
// Dispatch podcast client callbacks from here
void podcastHandle() {
  while (podcastGetQueue && xQueueReceive(podcastGetQueue, &podcastRxMessage, 0) == pdPASS){
    if (podcastRxMessage.cmd == POD_RESULT && addPodResult) addPodResult(podcastRxMessage.info);
    else if (podcastRxMessage.cmd == POD_FINISHED && podFinished) podFinished();
    else if (podcastRxMessage.cmd == POD_ERROR) {
      serial.printf(LV_SYMBOL_WARNING " Podcast server request failed, response code: %d\r\n", podcastRxMessage.numResults);
      info(NOW, 0, LV_SYMBOL_WARNING "Podcast server request failed, response code: %d", podcastRxMessage.numResults);
    }
  }
}

//--------------------
//Threaded stuff below

//Allocate the message queues
void CreatePodQueues(){
  if(!podcastSetQueue) podcastSetQueue = xQueueCreate(5, sizeof(struct podcastMessage));
  if(!podcastGetQueue) podcastGetQueue = xQueueCreate(61, sizeof(struct podcastMessage));
}

//SHA1 Hash to string of hex bytes
String sha1(String input) {
  char hex[4];
  String result;
  Sha1.init();
  Sha1.print(input);
  uint8_t * bytes = Sha1.result(); 
  for (int n = 0; n < 20; n++) {
    snprintf(hex, 3, "%02x", bytes[n]);
    result += hex;
  }
  return result;
}

//Construct and send a request to Podcastindex.org server
bool podServerRequest(String url) {
  struct podcastMessage podcastTxTaskMessage;
  //serial.println(url);
  String apiKey = String(podcast_key);
  String apiHeaderTime = String(utf(), DEC);
  String hash = sha1(apiKey + String(podcast_secret) + apiHeaderTime);
  if (!podClient.begin(url)) return false;
  podClient.setReuse(false);
  podClient.setUserAgent("SuperWooferPodPlayer/0.1");
  podClient.addHeader("Content-Type", "application/x-www-form-urlencoded;charset=UTF-8");
  podClient.addHeader("Host", PODCAST_HOST);
  podClient.addHeader("X-Auth-Key", apiKey);
  podClient.addHeader("X-Auth-Date", apiHeaderTime);
  podClient.addHeader("Authorization", hash); 
  int httpCode = podClient.GET();
  if (httpCode != 200) {
    podClient.end();
    podcastTxTaskMessage.cmd = POD_ERROR;
    podcastTxTaskMessage.numResults = httpCode;
    while (!xQueueSend(podcastGetQueue, &podcastTxTaskMessage, 0)) vTaskDelay(1);   
    if (podFinished) podFinished();
    return false;
  }
  return true;
}

//Stop and release any active connection
void podClientStop() {
  struct podcastMessage podcastTxTaskMessage;
  if (podClientActive) {
    podClientActive = false;
    podClient.end();
    podcastTxTaskMessage.cmd = POD_FINISHED;
    while (!xQueueSend(podcastGetQueue, &podcastTxTaskMessage, 0)) vTaskDelay(1);   
  }  
}

//Threaded handler - called from services thread
// Returns true if active
bool podClientHandle() {
  static unsigned long timeout = 0;
  static uint8_t level = 0;
  static WiFiClient* client;
  struct podcastMessage podcastRxTaskMessage;
  //Service any active connection
  if (podClientActive) {
    //Check for timeout
    if (millis() > timeout) {
      log_e("Timeout");  
      Serial.println(" timeout!");
      podClientStop();
    } 
    //Read data from server
    if (client && client->available()) {
      while(client->available()) 
        level = parsePodClient(client->read());
      timeout = millis() + POD_TIMEOUT;
    }
    //End of data
    else if(!client || !client->connected()) {
      podClientStop();
      //Check parsing results
      if (level == 0) {
        log_i("Search request completed");
        Serial.println(" OK.");
      }
      else {
        log_e("Response truncated");
        Serial.printf(" truncated! Level = %d\n", level);
      }
    }
  }
  //Service incoming messages IF we are in the right mode AND wifi is connected
  if(settings->mode == MODE_POD && WiFi.status() == WL_CONNECTED && xQueueReceive(podcastSetQueue, &podcastRxTaskMessage, 1) == pdPASS) {
    //Search request
    if(podcastRxTaskMessage.cmd == POD_SEARCH){
      //For podcasts..
      if(!podcastRxTaskMessage.episodes) {
        log_i("Begin podcast search");
        Serial.print("Searching for podcasts.."); // start a new connection
        podClientActive = podServerRequest(String(PODCAST_URL) + podcastRxTaskMessage.term + PODCAST_DEFAULTS);
        podClientEpisodes = false;
      //Or episodes..
      } else {
        log_i("Begin getting podcast episodes");
        Serial.print("Getting Episodes.."); // start a new connection
        if (podcastRxTaskMessage.numResults < 1)    //Only get latest
          podClientActive = podServerRequest(String(PODCAST_IDURL) + podcastRxTaskMessage.term + PODCAST_LATESTRES);
        else
          podClientActive = podServerRequest(String(PODCAST_IDURL) + podcastRxTaskMessage.term + PODCAST_IDRESULTS + String(podcastRxTaskMessage.numResults));
        podClientEpisodes = true;
      }
      //Successful connection?
      if (podClientActive) {
        level = 0;
        timeout = millis() + POD_TIMEOUT;
        client = podClient.getStreamPtr();
      }
      else {
        log_e("Connection failed");
        Serial.println("Failed!");
      }
    //Close connection request
    } else if(podcastRxTaskMessage.cmd == POD_STOP){
      podClientStop();
    }
  }
  return podClientActive;
}


//Turn a single hex digit into it's value
int hexDigit(int c){
  if(c>='0'&&c<='9') return c-'0';
  if(c>='a'&&c<='f') return c-'a'+10;
  if(c>='A'&&c<='F') return c-'A'+10;
  return -1;//error
}

//Fudge some common unicode symbols into ASCII
int fudgeUnicode(int c) {
  if (c < 128) return c;
  if (c == 160) return ' ';
  if (c == 215) return '*';
  if (c == 8211 || c == 8212) return '-';
  if (c == 8216 || c == 8217 || c == 8242) return '\'';
  if (c == 8220 || c == 8221 || c == 8243) return '\"';
  if (c == 8230) return '.'; //unicode ellipsis
  return '.';
}

//Strip escape codes and html formatting
int decodeEscape(int c) {
  static bool inEscape = false;
  static bool inTag = false;
  static uint8_t escCount = 0;
  static uint8_t tagCount = 0;
  static uint32_t codePoint = 0;
  if (inEscape) {
    inEscape = false;
    if (!escCount) {
      codePoint = 0;
      if (c == 'u') escCount = 4;
      else if (c == 'U') escCount = 8;
      else if (c == 'r') return -1;     //"\r"
      else if (c == 'n') return ' ';    //"\n"
      else return c;
    } else {
      int h = hexDigit(c);
      if (h >= 0) codePoint = codePoint * 16 + h;
      else return c; //Serial.println("Escape code decode error!");
      if (--escCount == 0) return fudgeUnicode(codePoint);
    }
    inEscape = true;
    return -1; 
  }
  if (!inTag && c == '\\') {   // ='\' Escape sequence begins
    inEscape = true;
    escCount = 0;
    return -1;
  }
  if (inTag) {
    if (c == '>' || ++tagCount > 4) inTag = false;
    if (c == 'p' && tagCount == 1) return ' ';    //<p> 
    if (c == 'b' && tagCount == 1) return ' ';    //<br>  mostly..
    return -1;
  } 
  if (c == '<') {
    inTag = true;
    tagCount = 0;
    return -1;
  }
  return c;
}


uint8_t parsePodClient(int c) {
  static uint8_t curlyLevel = 0;
  static bool inTag = false;
  static bool getTag = true;
  static bool inValue = false;
  static bool getValue = false;
  static char tag[128];
  static char val[256];
  static uint8_t tagIndex = 0;
  static uint16_t valIndex = 0;
  if (c == -1) return curlyLevel;    //Propagate errors
  //Serial.write((char)c);
  if (curlyLevel) {
    if (getTag) {
      if (c == '"') {
        inTag = !inTag;
        if (inTag) {
          tag[0] = 0;
          tagIndex = 0;
        } else {
          getTag = false;
          inValue = false;
          getValue = false;
          val[0] = 0;
          valIndex = 0;
        }
      }
      else if (inTag) {
        int dc = decodeEscape(c);          
        if (dc >= 0 && tagIndex < 127) {
          tag[tagIndex++] = (char)dc;
          tag[tagIndex] = 0;
        }
      }
    } else {
      if (!getValue) {
        if (c == ':') getValue = true;
      } else {
        if (c == '"') inValue = !inValue;
        else if (!inValue && (c == '{' || c == '[' || c == ',' || c == '}')) {
          parsePodClientTag(curlyLevel, tag, val);
          getValue = false;
          getTag = true;
        }
        else {
          int dc = decodeEscape(c);
          if (dc >= 0 && valIndex < 255) {
            if (valIndex == 253 || valIndex == 254) dc = '.';
            val[valIndex++] = (char)dc;
            val[valIndex] = 0;
          }
        }
      }
    }
  }
  if (c == '{') curlyLevel++;
  else if (c == '}') curlyLevel--;
  return curlyLevel;  
}


void parsePodClientTag(uint8_t level, char* tag, char* val) {
  struct podcastMessage podcastTxTaskMessage;
  //Collect this data
  static String name = "";
  static String description = "";
  static String url = "";
  static int id;
  static int feedId = -1;
  static int lastUpdateTime = 0;
  //Find and store the right tags
  if (strcmp(tag, "id") == 0) id = atoi(val);
  else if (strcmp(tag, "title") == 0) name = val;
  else if (strcmp(tag, "description") == 0) description = val;
  else if (strcmp(tag, "enclosureUrl") == 0) url = val;
  else if (strcmp(tag, "feedId") == 0) feedId = atoi(val);
  else if (strcmp(tag, "datePublished") == 0) lastUpdateTime = atoi(val);
  else if (strcmp(tag, "newestItemPubdate") == 0 || strcmp(tag, "transcriptUrl") == 0) {
    //Final tag in an entry, construct an info object and send it in a message
    podcastTxTaskMessage.cmd = POD_RESULT;
    podcastTxTaskMessage.info = new podInfo(id, name.c_str(), description.c_str(), podClientEpisodes, url.c_str(), feedId, lastUpdateTime);
    while (!xQueueSend(podcastGetQueue, &podcastTxTaskMessage, 0)) vTaskDelay(1);   
    //Clear the data caches to be a bit more memory efficient
    name = description = url = "";
    feedId = -1;
  } 
}

