#include "decls.h"
WiFiClient searchClient; // wifi client object
lv_obj_t * searchWindow;
lv_obj_t * searchEditText1;
lv_obj_t * searchEditText2;
lv_obj_t * searchEditText3;
lv_obj_t * addTermBtn;
lv_obj_t * delTermBtn2;
lv_obj_t * delTermBtn3;
lv_obj_t * termTypeList1;
lv_obj_t * termTypeList2;
lv_obj_t * termTypeList3;
lv_obj_t * searchResultList;
lv_obj_t * searchSpinner;
lv_obj_t * searchHeaderLabel;
int searchResultCount;
uint8_t searchPage;

#define SEARCH_PAGESIZE   50
#define SEARCH_TIMEOUT    30000

const char* termTypes[] = { "tag=", "name=", "bitrateMin=", "bitrateMax=", "countrycode=", "language=" };
bool searchTerm2Shown = false;
bool searchTerm3Shown = false;

void searchWindow_totop_action(lv_event_t * event);
void searchWindow_close_action(lv_event_t * event);
void searchListActivated(lv_event_t * event);
void editSearchAction(lv_event_t * event);
static void addTermBtnAction(lv_event_t * event);
static void delTermBtnAction2(lv_event_t * event);
static void delTermBtnAction3(lv_event_t * event);
void cancelSearch();
void keyboardSearchKeyAction(lv_event_t * event);
void activateSearch();
void clearSearchResults();
void search(String term);
void searchAction(lv_event_t * event);
uint8_t parseSearch(int c);
void parseSearchTag(uint8_t level, char* tag, char* val);
void CreateStationQueues();
void searchPreviousPageAction(lv_event_t * event);


void createSearchWindow(lv_obj_t * parent) {    
  //Styles first

  searchWindow = lv_win_create(parent, 40);
  searchHeaderLabel = lv_win_add_title(searchWindow, "Webstation Search");
  lv_obj_set_style_pad_left(searchHeaderLabel, 6, LV_PART_MAIN);
  lv_obj_set_size(searchWindow, lv_obj_get_content_width(parent), lv_obj_get_content_height(parent));
  lv_obj_set_pos(searchWindow, 0, 0);
  lv_obj_add_style(searchWindow, &style_win, LV_PART_MAIN);
  lv_obj_t * win_content = lv_win_get_content(searchWindow);
  lv_obj_add_style(win_content, &style_win, LV_PART_MAIN);
  lv_obj_set_hidden(searchWindow, true);

  lv_obj_t * totop_btn = lv_win_add_btn(searchWindow, LV_SYMBOL_UP, 50);           /*Add close button and use built-in close action*/
  lv_obj_add_style(totop_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(totop_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(totop_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(totop_btn, searchWindow_totop_action, LV_EVENT_CLICKED, NULL);

  lv_obj_t * close_btn = lv_win_add_btn(searchWindow, LV_SYMBOL_CLOSE, 50);           /*Add close button and use built-in close action*/
  lv_obj_add_style(close_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(close_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(close_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(close_btn, searchWindow_close_action, LV_EVENT_CLICKED, NULL);

  lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

  //Search entry 1

  termTypeList1 = lv_dropdown_create(win_content);            //Create a drop down list
  lv_obj_add_style(termTypeList1, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(termTypeList1, &style_biggerfont, LV_PART_MAIN);
  lv_obj_set_pos(termTypeList1, 2, 6);  
  lv_dropdown_set_options(termTypeList1, "Genre\nName\nBitrateMin\nBitrateMax\nCountry\nLanguage"); //Set the options
  lv_obj_set_size(termTypeList1, 108, 36);
  lv_obj_add_event_cb(termTypeList1, searchListActivated, LV_EVENT_CLICKED, NULL);

  //Search edit textbox
  searchEditText1 = lv_textarea_create(win_content);
  lv_obj_set_pos(searchEditText1, 116, 6);
  lv_obj_add_style(searchEditText1, &style_ta, LV_PART_MAIN);
  lv_textarea_set_text_selection(searchEditText1, false);
  lv_textarea_set_one_line(searchEditText1, true);
  lv_obj_set_size(searchEditText1, lv_obj_get_content_width(win_content) - 45 - 116, 36);
  lv_textarea_set_text(searchEditText1, "");
  lv_obj_add_event_cb(searchEditText1, editSearchAction, LV_EVENT_PRESSED, NULL);

  //Add term button
  addTermBtn = lv_btn_create(win_content);
  lv_obj_set_pos(addTermBtn, lv_obj_get_content_width(win_content) - 44, 6);
  lv_obj_set_size(addTermBtn, 40, 36);
  lv_obj_add_style(addTermBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(addTermBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(addTermBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(addTermBtn, addTermBtnAction, LV_EVENT_CLICKED, NULL);
  lv_obj_t * label = lv_label_create(addTermBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_PLUS);

  //Search entry 2

  termTypeList2 = lv_dropdown_create(win_content);            //Create a drop down list
  lv_obj_add_style(termTypeList2, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(termTypeList2, &style_biggerfont, LV_PART_MAIN);
  lv_obj_align_to(termTypeList2, termTypeList1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);  
  lv_dropdown_set_options(termTypeList2, "Genre\nName\nBitrateMin\nBitrateMax\nCountry\nLanguage"); //Set the options
  lv_dropdown_set_selected(termTypeList2, 2);
  lv_obj_set_size(termTypeList2, 108, 36);
  lv_obj_add_event_cb(termTypeList2, searchListActivated, LV_EVENT_CLICKED, NULL);
  lv_obj_set_hidden(termTypeList2, true);

  //Search edit textbox
  searchEditText2 = lv_textarea_create(win_content);
  lv_obj_align_to(searchEditText2, searchEditText1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);
  lv_obj_add_style(searchEditText2, &style_ta, LV_PART_MAIN);
  lv_textarea_set_text_selection(searchEditText2, false);
  lv_textarea_set_one_line(searchEditText2, true);
  lv_obj_set_size(searchEditText2, lv_obj_get_content_width(win_content) - 45 - 116, 36);
  lv_textarea_set_text(searchEditText2, "");
  lv_obj_add_event_cb(searchEditText2, editSearchAction, LV_EVENT_PRESSED, NULL);
  lv_obj_set_hidden(searchEditText2, true);

  //Delete term button
  delTermBtn2 = lv_btn_create(win_content);
  lv_obj_align_to(delTermBtn2, addTermBtn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);
  lv_obj_set_size(delTermBtn2, 40, 36);
  lv_obj_add_style(delTermBtn2, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(delTermBtn2, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(delTermBtn2, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(delTermBtn2, delTermBtnAction2, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(delTermBtn2);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_MINUS);
  lv_obj_set_hidden(delTermBtn2, true);

  //Search entry 3

  termTypeList3 = lv_dropdown_create(win_content);            //Create a drop down list
  lv_obj_add_style(termTypeList3, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(termTypeList3, &style_biggerfont, LV_PART_MAIN);
  lv_obj_align_to(termTypeList3, termTypeList2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);  
  lv_dropdown_set_options(termTypeList3, "Genre\nName\nBitrateMin\nBitrateMax\nCountry\nLanguage"); //Set the options
  lv_dropdown_set_selected(termTypeList3, 3);
  lv_obj_set_size(termTypeList3, 108, 36);
  lv_obj_add_event_cb(termTypeList3, searchListActivated, LV_EVENT_CLICKED, NULL);
  lv_obj_set_hidden(termTypeList3, true);

  //Search edit textbox
  searchEditText3 = lv_textarea_create(win_content);
  lv_obj_align_to(searchEditText3, searchEditText2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);
  lv_obj_add_style(searchEditText3, &style_ta, LV_PART_MAIN);
  lv_textarea_set_text_selection(searchEditText3, false);
  lv_textarea_set_one_line(searchEditText3, true);
  lv_obj_set_size(searchEditText3, lv_obj_get_content_width(win_content) - 45 - 116, 36);
  lv_textarea_set_text(searchEditText3, "");
  lv_obj_add_event_cb(searchEditText3, editSearchAction, LV_EVENT_PRESSED, NULL);
  lv_obj_set_hidden(searchEditText3, true);

  //Delete term button
  delTermBtn3 = lv_btn_create(win_content);
  lv_obj_align_to(delTermBtn3, delTermBtn2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);
  lv_obj_set_size(delTermBtn3, 40, 36);
  lv_obj_add_style(delTermBtn3, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(delTermBtn3, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(delTermBtn3, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(delTermBtn3, delTermBtnAction3, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(delTermBtn3);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_MINUS);
  lv_obj_set_hidden(delTermBtn3, true);


  searchResultList = lv_list_create(win_content);
  lv_obj_set_size(searchResultList, lv_obj_get_content_width(win_content) - 4, LV_SIZE_CONTENT);
  lv_obj_set_pos(searchResultList, 0, 0);
  lv_obj_align_to(searchResultList, termTypeList1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);
  lv_obj_add_style(searchResultList, &style_halfopa, LV_PART_MAIN);
  lv_obj_add_style(searchResultList, &style_listfont, LV_PART_MAIN);
  lv_obj_clear_flag(searchResultList, LV_OBJ_FLAG_SCROLLABLE);
  //lv_obj_set_scrollbar_mode(dabStationList, LV_SCROLLBAR_MODE_OFF);

    //Loading spinner
  searchSpinner = lv_spinner_create(win_content, 1000, 60);
  lv_obj_set_size(searchSpinner, 100, 100);
  lv_obj_align(searchSpinner, LV_ALIGN_CENTER, 0, 20);
  lv_obj_add_flag(searchSpinner, LV_OBJ_FLAG_FLOATING);
  lv_obj_set_hidden(searchSpinner, true);

  CreateStationQueues(); //Do it here because we have no init function     
}

//Search dropdown opened, set the style of the dropdown
void searchListActivated(lv_event_t * event) {
  lv_obj_t * dropdown = lv_event_get_target(event);
  lv_obj_t * ddlist = lv_dropdown_get_list(dropdown);
  lv_state_t state = lv_obj_get_state(dropdown);
  if (ddlist && (state & LV_STATE_CHECKED)) {  
    lv_obj_add_style(ddlist, &style_biggerfont, LV_PART_MAIN);
    lv_obj_set_style_max_height(ddlist, 300, LV_PART_MAIN);
    lv_obj_set_height(ddlist, LV_SIZE_CONTENT);
  }
}

void searchCloseDropdown() {
  lv_dropdown_close(termTypeList1);
  lv_dropdown_close(termTypeList2);
  lv_dropdown_close(termTypeList3);
}

//Realign terms in window as visibility changes
void searchWindowAlign() {
  if (searchTerm2Shown) {
    if (searchTerm3Shown) {
      lv_obj_align_to(termTypeList3, termTypeList2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);  
      lv_obj_align_to(searchEditText3, searchEditText2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);
      lv_obj_align_to(delTermBtn3, delTermBtn2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);
      lv_obj_align_to(searchResultList, termTypeList3, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);  
    } else lv_obj_align_to(searchResultList, termTypeList2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);  
  } else {
    if (searchTerm3Shown) {
      lv_obj_align_to(termTypeList3, termTypeList1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);  
      lv_obj_align_to(searchEditText3, searchEditText1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);
      lv_obj_align_to(delTermBtn3, addTermBtn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);
      lv_obj_align_to(searchResultList, termTypeList3, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);
    } else lv_obj_align_to(searchResultList, termTypeList1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);  
  }
}

//Term 2 visibility
void showSearchTerm2(bool shown) {
  lv_obj_set_hidden(termTypeList2, !shown);
  lv_obj_set_hidden(searchEditText2, !shown);
  lv_obj_set_hidden(delTermBtn2, !shown);
  searchTerm2Shown = shown;
}

//Term 3 visibility
void showSearchTerm3(bool shown) {
  lv_obj_set_hidden(termTypeList3, !shown);
  lv_obj_set_hidden(searchEditText3, !shown);
  lv_obj_set_hidden(delTermBtn3, !shown);
  searchTerm3Shown = shown;
}

//Add Term button clicked
void addTermBtnAction(lv_event_t * event) {
  if (!searchTerm2Shown) showSearchTerm2(true);
  else if (!searchTerm3Shown) showSearchTerm3(true);
  searchWindowAlign();
}

//Remove term 2 action
void delTermBtnAction2(lv_event_t * event) {
  showSearchTerm2(false);
  searchWindowAlign();
}

//Remove term 3 action
void delTermBtnAction3(lv_event_t * event) {
  showSearchTerm3(false);
  searchWindowAlign();
}

//Search window visibility
void hideSearchWindow(bool hide) {
  lv_obj_set_hidden(searchWindow, hide);
}

//Scroll to top of list action
void searchWindow_totop_action(lv_event_t * event) {
  lv_obj_t * win_content = lv_win_get_content(searchWindow);
  lv_obj_scroll_to(win_content, 0, 0, LV_ANIM_ON);
}

//Close search window action
void searchWindow_close_action(lv_event_t * event) {
  cancelSearch();
  hideSearchWindow(true);
  keyboardHide(false, NULL);
  hideStationWindow(false);
}

//Edit search term action
void editSearchAction(lv_event_t * event) {
  lv_obj_t * target = lv_event_get_target(event);
  if (!keyboardShowing()) {
    keyboardShow(lv_scr_act(), target, keyboardSearchKeyAction);
  }
}

//Keyboard OK or Cancel on URL edit
void keyboardSearchKeyAction(lv_event_t * event) {
  uint32_t res = lv_event_get_code(event);
  if(res == LV_EVENT_READY || res == LV_EVENT_CANCEL){
    if (res == LV_EVENT_READY) {
      searchPage = 0;
      keyboardHide(true, activateSearch);
    }
    else keyboardHide(true, NULL);
  }
}

//Update window header
void updateSearchHeader(bool searching) {
  if (searching || (searchResultCount < SEARCH_PAGESIZE && searchPage == 0)) {
    lv_label_set_text_fmt(searchHeaderLabel, "Search (%d results)", searchResultCount);    
  }
  else {
    lv_label_set_text_fmt(searchHeaderLabel, "Search (Page %d)", searchPage + 1);    
  }
}

//Callback to start search from keyboard hidden action
void activateSearch() {
  String request;
  clearSearchResults();
  if (WiFi.status() != WL_CONNECTED) listWarning(searchResultList, "WiFi Not Connected");
  updateSearchHeader(false);
  if (searchPage != 0) {
    lv_obj_t* list_btn = lv_list_add_btn(searchResultList, LV_SYMBOL_LEFT, "Previous Page");
    lv_obj_add_event_cb(list_btn, searchPreviousPageAction, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(list_btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_style(list_btn, &style_list, LV_PART_MAIN);
    lv_obj_add_style(list_btn, &style_listsel, LV_STATE_CHECKED);
  }
  //Construct a search request
  const char* term = lv_textarea_get_text(searchEditText1);
  const char* type = termTypes[lv_dropdown_get_selected(termTypeList1)];
  request = String(type) + urlencode(term);
  if (searchTerm2Shown) {
    term = lv_textarea_get_text(searchEditText2);
    type = termTypes[lv_dropdown_get_selected(termTypeList2)];
    request += String("&") + type + urlencode(term);
  }
  if (searchTerm3Shown) {
    term = lv_textarea_get_text(searchEditText3);
    type = termTypes[lv_dropdown_get_selected(termTypeList3)];
    request += String("&") + type + urlencode(term);
  }
  request += String("&offset=") + String(SEARCH_PAGESIZE * searchPage);
  serial.print("Search for: ");
  serial.println(request);
  search(request);
}

void searchNextPageAction(lv_event_t * event) {
  searchPage++;
  activateSearch();
}

void searchPreviousPageAction(lv_event_t * event) {
  searchPage--;
  activateSearch();
}

//Add an entry to the search result list
lv_obj_t * addSearchResult(stationInfo* info) {
  lv_obj_t * list_btn;
  char buf[300];
  if (!searchResultList || !info) return NULL;
  ++searchResultCount;
  updateSearchHeader(true);
  //Entry text
#if (TFT_WIDTH == 480)
  if (info->bitrate > 0)
    sprintf(buf, " %s [%dk %s]", info->name, info->bitrate, info->codec);
  else
    sprintf(buf, " %s [%s]", info->name, info->codec);
#else
    sprintf(buf, "%s", info->name);
#endif    
  list_btn = lv_list_add_btn(searchResultList, LV_SYMBOL_AUDIO, buf);
  lv_obj_add_event_cb(list_btn, searchAction, LV_EVENT_CLICKED, NULL);
  lv_obj_add_flag(list_btn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_add_style(list_btn, &style_list, LV_PART_MAIN);
  lv_obj_add_style(list_btn, &style_listsel, LV_STATE_CHECKED);
  lv_obj_set_user_data(list_btn, info);
  return list_btn;
}

//Search result selected action
void searchAction(lv_event_t * event) {
  lv_obj_t * child = lv_event_get_target(event);
  listSetSelect(child);
  //Get station info
  stationInfo* info = (stationInfo*)lv_obj_get_user_data(child);  
  strncpy(webStationName, info->name, 34);
  webStationName[34] = '\0';
  //Update EEPROM server setting
  if (strncmp(settings->server, info->url, 255) != 0) {
    strncpy(settings->server, info->url, 255);
    settings->server[255] = '\0';
    writeSettings();
  }
  //Play the station  
  tabViewShowMain();
  connectToHost(settings->server, true);
  updateUrlEditText();
}

//Clear the results list
void clearSearchResults() {
  lv_obj_t * win_content = lv_win_get_content(searchWindow);
  lv_obj_scroll_to(win_content, 0, 0, LV_ANIM_OFF);
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(searchResultList); i++) {
    lv_obj_t* child = lv_obj_get_child(searchResultList, i);
    void * data = lv_obj_get_user_data(child);
    if (data) {
      delete((stationInfo*)data);
      data = NULL;
    }
  }
  lv_obj_clean(searchResultList);
  searchResultCount = 0;
}

//Called when search started
void searchStarted() {
  lv_obj_set_hidden(searchSpinner, false);
  //lv_task_handler();    
}

//Called when search ends
void searchFinished() {
  updateSearchHeader(false);
  if(searchResultCount == SEARCH_PAGESIZE) {
    lv_obj_t* list_btn = lv_list_add_btn(searchResultList, LV_SYMBOL_RIGHT, "Next Page");
    lv_obj_add_event_cb(list_btn, searchNextPageAction, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(list_btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_style(list_btn, &style_list, LV_PART_MAIN);
    lv_obj_add_style(list_btn, &style_listsel, LV_STATE_CHECKED);
  }
  lv_obj_set_hidden(searchSpinner, true);  
}

//------------------------------------------------------------------------------
//Search client

bool searching = false;
String searchTerm;

//Message in queue
struct stationMessage{
    stationInfo* info;
    const char*  term;
    uint8_t      cmd;
} stationTxMessage, stationRxMessage;

//Station search message types
enum : uint8_t { STN_SEARCH, STN_CANCEL, STN_RESULT, STN_FINISHED };

//Station queue handles
QueueHandle_t stationSetQueue = NULL;
QueueHandle_t stationGetQueue = NULL;

//activate the station search
void search(String term) {
  if (!stationSetQueue) return;
  searchTerm = term;    //Cache the string
  //Construct the message
  stationTxMessage.cmd = STN_SEARCH;
  stationTxMessage.term = searchTerm.c_str(); //Send pointer to the cached string
  xQueueSend(stationSetQueue, &stationTxMessage, 0);
  searchStarted();
}

//Send a message to cancel the search
void cancelSearch() {
  if (!stationSetQueue) return;
  stationTxMessage.cmd = STN_CANCEL;
  xQueueSend(stationSetQueue, &stationTxMessage, 0);
}

//Allocate the message queues
void CreateStationQueues(){
  if(!stationSetQueue) stationSetQueue = xQueueCreate(5, sizeof(struct stationMessage));
  if(!stationGetQueue) stationGetQueue = xQueueCreate(50, sizeof(struct stationMessage));
}

//Called from loop()
// Get messages from the search client and dispatch actions from here
void searchHandle() {
  while (stationGetQueue && xQueueReceive(stationGetQueue, &stationRxMessage, 0) == pdPASS){
    if (stationRxMessage.cmd == STN_RESULT) addSearchResult(stationRxMessage.info);
    else if (stationRxMessage.cmd == STN_FINISHED) searchFinished();
  }
}

//-------------------------------
//Threaded stuff below

//Connect to server and send the search request
bool searchRequest(String terms) {
  const int httpPort = 80;
  searchClient.stop();  // Clear any current connections
  if (!searchClient.connect(SEARCH_HOST, httpPort)) return false;
  String url = String(SEARCH_URL) + terms + SEARCH_DEFAULTS;  
  log_d("Search URL: %s", url.c_str());
  searchClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
  "Host: " + SEARCH_HOST + "\r\n" +
  "Connection: close\r\n\r\n");  
  return true;
}

//Send message that search has finished
void searchClientFinished() {
  struct stationMessage stationTxTaskMessage;
  stationTxTaskMessage.cmd = STN_FINISHED;
  while (!xQueueSend(stationGetQueue, &stationTxTaskMessage, 0)) vTaskDelay(1);   
}

//Cancel any current search
void stopSearchClient() {
  if (searching) {
    searching = false;
    searchClient.stop();
    searchClientFinished();
  }  
}

//Called from services task
bool searchClientHandle() {
  static unsigned long timeout = 0;
  static uint8_t level = 0;
  struct stationMessage stationRxTaskMessage;

  //Service incoming messages IF we are in the right mode AND wifi is connected
  if (settings->mode == MODE_WEB && WiFi.status() == WL_CONNECTED && xQueueReceive(stationSetQueue, &stationRxTaskMessage, 1) == pdPASS) {
    //Start a search please..
    if(stationRxTaskMessage.cmd == STN_SEARCH){
      log_i("Starting search request");
      Serial.print("Searching.."); // start a new connection
      searching = searchRequest(searchTerm);
      if (searching) timeout = millis() + SEARCH_TIMEOUT;
      else {
        log_e("Connection failed");
        Serial.println(" Failed!");
        searchClientFinished();
      }
    }
    //Stop searching please..
    else if (stationRxMessage.cmd == STN_CANCEL) stopSearchClient();
  }  

  //If client is active
  if (searching) {
    //Timeout?
    if (millis() > timeout) {
      log_e("Timeout");  
      Serial.println(" timeout!");
      stopSearchClient();
    }
    //Data available?
    if (searchClient.available()) {
      while(searchClient.available())
        level = parseSearch(searchClient.read());
      timeout = millis() + SEARCH_TIMEOUT;
    }
    //End of data?
    else if(!searchClient.connected()) {
      searching = false;
      if (level == 0) {
        log_i("Search request completed");
        Serial.println(" OK.");
      }
      else {
        log_e("Response truncated");
        Serial.println(" truncated!");
      }
      searchClientFinished();
    }
  }
  return searching;
}

//Cheap-style JSON parser
uint8_t parseSearch(int c) {
  static uint8_t curlyLevel = 0;
  static bool inTag = false;
  static bool getTag = true;
  static bool inValue = false;
  static bool getValue = false;
  static char tag[128];
  static char val[256];
  static uint8_t tagIndex = 0;
  static uint16_t valIndex = 0;
  if (c == -1) return 1;    //Propagate errors
  //Serial.print((char)c);
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
        if (tagIndex < 127) {
          tag[tagIndex++] = (char)c;
          tag[tagIndex] = 0;
        }
      }
    } else {
      if (!getValue) {
        if (c == ':') getValue = true;
      } else {
        if (c == '"') inValue = !inValue;
        else if (!inValue && (c == '{' || c == '[' || c == ',' || c == '}')) {
          parseSearchTag(curlyLevel, tag, val);
          getValue = false;
          getTag = true;
        }
        else if (valIndex < 255) {
          val[valIndex++] = (char)c;
          val[valIndex] = 0;
        }
      }
    }
  }
  if (c == '{') curlyLevel++;
  else if (c == '}') curlyLevel--;
  return curlyLevel;  
}

//tag=val pairs come flooding in here
void parseSearchTag(uint8_t level, char* tag, char* val) {
  struct stationMessage stationTxTaskMessage;
  //Cache useful data
  static String name;
  static String url;
  static String codec;
  //Look for known tags and cache their values
  if (strcmp(tag, "name") == 0) name = val;
  else if (strcmp(tag, "url_resolved") == 0) url = val;
  else if (strcmp(tag, "codec") == 0) codec = val;
  else if (strcmp(tag, "bitrate") == 0) {
    //Last tag in an entry, construct a stationInfo object and send it in a message
    if (strlen(val) < 255) {  //Ignore stations with too large URLs
      stationTxTaskMessage.cmd = STN_RESULT;
      Serial.println(name);
      stationTxTaskMessage.info = new stationInfo(name.c_str(), url.c_str(), codec.c_str(), atoi(val));
      while (!xQueueSend(stationGetQueue, &stationTxTaskMessage, 0)) vTaskDelay(1);   
    }
    //Clear the caches for efficiency
    name = url = codec = "";
  } 
}
