#include "decls.h"

WiFiClient soapClient;
WiFiUDP    soapUdp;
SoapESP32  soap(&soapClient, &soapUdp);
String     dlnaMetaName;
String     dlnaMetaAlbum;

//DLNA commands
enum : uint8_t { DLNA_SERVERS, DLNA_STARTSEEK, DLNA_ENDSEEK, DLNA_BROWSE, DLNA_RECURSE, DLNA_ITEM, DLNA_FINISHED, DLNA_RECURSE_FINISHED };

//Linked list describing the chain of parents of the current directory
// - allows going back down the tree to the root
dlnaDir* dlnaDirHead = NULL;

//type of information in an dlnaInfo object
enum : uint8_t { DLNA_TYPE_SERVER, DLNA_TYPE_OBJECT };

//DLNA message
struct dlnaMessage{
    dlnaInfo*   info;
    int         serverIndex;
    const char* id;
    uint8_t     cmd;
    bool        refresh;
} dlnaTxMessage, dlnaRxMessage;

//Message queues
QueueHandle_t dlnaSetQueue = NULL;
QueueHandle_t dlnaGetQueue = NULL;

//Allocate the queues
void CreateDLNAQueues(){
  if(!dlnaSetQueue) dlnaSetQueue = xQueueCreate(10, sizeof(struct dlnaMessage));
  if(!dlnaGetQueue) dlnaGetQueue = xQueueCreate(10, sizeof(struct dlnaMessage));
}

//Called once from setup to get things going
void DLNASetup() {
  CreateDLNAQueues();
  if(!dlnaSetQueue || !dlnaGetQueue) {
    errorContinue(0, "DLNA Queues are not initialized");
  }
}


lv_obj_t * dlnaWindow;
lv_obj_t * dlnaHeaderLabel;
lv_obj_t * dlnaList;
lv_obj_t * dlnaUpBtn;
lv_obj_t * dlnaSpinner;

void dlnaTotop(lv_event_t * event);
void dlnaGoUp(lv_event_t * event);
void dlnaRefresh(lv_event_t * event);

void createDlnaWindow(lv_obj_t * parent) {    
  //Styles first

  dlnaWindow = lv_win_create(parent, 40);
  dlnaUpBtn = lv_win_add_btn(dlnaWindow, LV_SYMBOL_LEFT, 50);
  lv_obj_add_event_cb(dlnaUpBtn, dlnaGoUp, LV_EVENT_CLICKED, NULL);
  lv_obj_add_style(dlnaUpBtn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(dlnaUpBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(dlnaUpBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_set_hidden(dlnaUpBtn, true);

  dlnaHeaderLabel = lv_win_add_title(dlnaWindow, "Media Servers");
  lv_obj_set_style_pad_left(dlnaHeaderLabel, 6, LV_PART_MAIN);
  lv_obj_set_size(dlnaWindow, lv_obj_get_content_width(parent), lv_obj_get_content_height(parent));
  lv_obj_set_pos(dlnaWindow, 0, 0);
  lv_obj_add_style(dlnaWindow, &style_win, LV_PART_MAIN);
  lv_obj_t * win_content = lv_win_get_content(dlnaWindow);
  lv_obj_add_style(win_content, &style_win, LV_PART_MAIN);
  lv_obj_set_hidden(dlnaWindow, true);

  lv_obj_t * totop_btn = lv_win_add_btn(dlnaWindow, LV_SYMBOL_UP, 50);           /*Add close button and use built-in close action*/
  lv_obj_add_style(totop_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(totop_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(totop_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(totop_btn, dlnaTotop, LV_EVENT_CLICKED, NULL);

  lv_obj_t * refresh_btn = lv_win_add_btn(dlnaWindow, LV_SYMBOL_HOME, 50);
  lv_obj_add_style(refresh_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(refresh_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(refresh_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(refresh_btn, dlnaRefresh, LV_EVENT_CLICKED, NULL);

  dlnaList = lv_list_create(win_content);
  lv_obj_set_style_bg_opa(dlnaList, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_add_style(dlnaList, &style_biggestfont, LV_PART_MAIN);
  lv_obj_update_layout(win_content);
  lv_obj_set_size(dlnaList, lv_obj_get_content_width(win_content), lv_obj_get_content_height(win_content));
  lv_obj_set_pos(dlnaList, 0, 0);
  
    //Loading spinner
  dlnaSpinner = lv_spinner_create(win_content, 1000, 60);
  lv_obj_set_size(dlnaSpinner, 100, 100);
  lv_obj_align(dlnaSpinner, LV_ALIGN_CENTER, 0, 20);
  lv_obj_add_flag(dlnaSpinner, LV_OBJ_FLAG_FLOATING);
  lv_obj_set_hidden(dlnaSpinner, true);

}

//Show podcast list for the right modes
void setDlnaVisibility() {
  lv_obj_set_hidden(dlnaWindow, settings->mode != MODE_DLNA);  
}

//Activity spinner
void showDlnaSpinner(bool shown) {
  lv_obj_set_hidden(dlnaSpinner, !shown);
}

//Scroll to top of list action
void dlnaTotop(lv_event_t * event) {
  lv_obj_t * win_content = lv_win_get_content(dlnaWindow);
  lv_obj_scroll_to(win_content, 0, 0, LV_ANIM_ON);
}

void updateDlnaHeader(const char* header) {
  lv_label_set_text(dlnaHeaderLabel, header);  
}

//Clear episode list and also free the entry's attached userdata
void clearDlnaList() {
  //Delete attached userdata (dlnaInfo entries)
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(dlnaList); i++) {
    lv_obj_t* child = lv_obj_get_child(dlnaList, i);
    void * data = lv_obj_get_user_data(child);
    if (data) delete((dlnaInfo*)data);
  }
  //empty out the list
  lv_obj_clean(dlnaList);
}

//Start a request to scan for (if refresh true) and retrieve available servers
void DLNAGetServers(bool refresh) {
  if (!dlnaSetQueue) return;
  showDlnaSpinner(true);
  clearDlnaList();
  dlnaTxMessage.cmd = DLNA_SERVERS;
  dlnaTxMessage.refresh = refresh;
  xQueueSend(dlnaSetQueue, &dlnaTxMessage, portMAX_DELAY);    
}

//Start a request to browse for files on server
void DLNABrowseServers(int index, const char* id) {
  if (!dlnaSetQueue) return;
  showDlnaSpinner(true);
  clearDlnaList();
  dlnaTxMessage.cmd = DLNA_BROWSE;
  dlnaTxMessage.serverIndex = index;
  dlnaTxMessage.id = strdup(id);            //Remember to delete this in the client..
  xQueueSend(dlnaSetQueue, &dlnaTxMessage, portMAX_DELAY);    
}

//Action to rescan for servers
void dlnaRefresh(lv_event_t * event) {
  //Clear the dlna directory chain
  while (dlnaDirHead) {
    dlnaDir* dir = dlnaDirHead;
    dlnaDirHead = dlnaDirHead->previous;
    delete(dir);
  }
  DLNAGetServers(true);     //Force a full server scan
}

//Action to return to top of list
void dlnaGoUp(lv_event_t * event) {
  dlnaDir *dirInfo;
  //Remove top entry from directory chain
  if (dlnaDirHead != NULL) { //Pop first
    dirInfo = dlnaDirHead;
    dlnaDirHead = dirInfo->previous;
    delete(dirInfo);
  }
  //Repopulate list using new top entry
  if (dlnaDirHead != NULL) {
    updateDlnaHeader(dlnaDirHead->name);
    DLNABrowseServers(dlnaDirHead->server, dlnaDirHead->id);
  } else {
    //No top entry, back at server list
    updateDlnaHeader("Media Servers");
    lv_obj_set_hidden(dlnaUpBtn, true);
    DLNAGetServers(false);
  }
}

//Set early metadata from DLNA entry in case file has no metadata
void dlnaSetMeta(String name) {
  int ptr1 = name.indexOf(" - ");
  int ptr2 = name.lastIndexOf(" - ");
  if (ptr1 >= 0 && ptr2 > 0 && ptr1 != ptr2) { 
    String artist = name.substring(0, ptr1 - 1);
    dlnaMetaAlbum = name.substring(ptr1 + 3, ptr2 - 1);
    String title = name.substring(ptr2 + 3, name.length());
    dlnaMetaName = artist + " - " + title;
  } else {
    dlnaMetaAlbum = "";
    dlnaMetaName = name;
  }
}

//Another version based on attributes
void dlnaSetMeta(String artist, String album, String name) {
  dlnaMetaAlbum = album;
  dlnaMetaName = artist + " - " + name;
}

//Bring up the early metadata on the display
void showDlnaMetadata() {
  if (dlnaMetaName != "") { 
    String s = (String)LV_SYMBOL_AUDIO + " - " + dlnaMetaName + " -";
    info(NAME, 0, s.c_str()); 
  }
  if (dlnaMetaAlbum != "") { 
    String s = (String)"Album: " + metaAlbum;
    info(NOW, 0, s.c_str());       
  }
}

//Construct an entry name from item attributes
void dlnaMakeName(char* buf, char* artist, char* album, char* name) {
  buf[0] = '\0';
  if (strlen(artist) && strcmp(artist, "[Unknown Artist]") != 0) sprintf(buf, "%s - ", artist);
  if (strlen(album) && strcmp(album, "[Unknown Album]") != 0) sprintf(buf + strlen(buf), "%s - ", album);
  sprintf(buf + strlen(buf), "%s", name);
}

//Add a DLNA playlist entry to an open playlist file, dlnaPlaylistFile
lv_fs_file_t dlnaPlaylistFile;
void addPlaylistEntry(dlnaObject* object) {
  char name[256];
  char url[512];
  dlnaMakeName(name, object->artist, object->album, object->name);
  sprintf(url, "http://%s:%d/%s", object->ip.toString().c_str(), object->port, object->uri);
  addPlaylistEntry(&dlnaPlaylistFile, name, url);      
}

#define DLNA_PLAYACTION_NEW 0
#define DLNA_PLAYACTION_INS 1
#define DLNA_PLAYACTION_ADD 2

//DLNA playlist builder
bool dlnaStartPlaying = false;
int dlnaAppendPos = -1;
void buildDlnaPlaylist(lv_event_t * event) {
  listMenuClose(dlnaList, true);
  lv_obj_t* child = listGetSelectObj(dlnaList);
  if (child == NULL) return;
  dlnaInfo* dlna = (dlnaInfo*)lv_obj_get_user_data(child);
  if (dlna->infoType == DLNA_TYPE_SERVER) return;
  int dlnaPlaylistAction = (bool)lv_event_get_user_data(event);
  dlnaStartPlaying = (!getPlaylistCount() || dlnaPlaylistAction == DLNA_PLAYACTION_NEW);
  const char* path = getPlaylistPath();
  //Build playlist
  if (dlnaStartPlaying) {   //empty playlist
    sdStop();
    sdSongFinished();
    clearAlbumArt(); 
    info(NAME, 0, LV_SYMBOL_DIRECTORY " Building Playlist..");
    showLoadSpinner();
    setPlaylistIndex(1);              //in case we crash, whatever we got of the playlist picks up on reboot  
  } else info(NAME, 3, LV_SYMBOL_DIRECTORY " Appending Playlist..");
  tabViewShowMain();
  removePlaylist(path);
  dlnaAppendPos = -1;
  if (fs_err(lv_fs_open(&dlnaPlaylistFile, path, LV_FS_MODE_WR), "Open Playlist")) return;
  if (dlnaPlaylistAction == DLNA_PLAYACTION_ADD && getPlaylistCount()) writePlaylistToFp(&dlnaPlaylistFile);
  else if (dlnaPlaylistAction == DLNA_PLAYACTION_INS) {
    for (int16_t i = 0; i < getPlaylistIndex(); i++) {
      lv_obj_t* child = getPlaylistChild(i);
      addPlaylistEntry(&dlnaPlaylistFile, (char*)lv_obj_get_user_data(child));
    }
    dlnaAppendPos = getPlaylistIndex();            
  }
  dlnaObject* object = (dlnaObject*)dlna->object;
  if (object->isDir) {
    //start reading directory
    Serial.printf("Start directory %s, level = 1\n", object->name);
    dlnaTxMessage.cmd = DLNA_RECURSE;
    dlnaTxMessage.serverIndex = dlna->index;
    dlnaTxMessage.id = strdup(object->id);
    xQueueSend(dlnaSetQueue, &dlnaTxMessage, portMAX_DELAY);
  } else {
    //add single file
    addPlaylistEntry(object);
    if (dlnaAppendPos >= 0) {
      for (uint16_t i = dlnaAppendPos; i < getPlaylistCount(); i++) {
        lv_obj_t* child = getPlaylistChild(i);
        addPlaylistEntry(&dlnaPlaylistFile, (char*)lv_obj_get_user_data(child));            
      }  
    }
    lv_fs_close(&dlnaPlaylistFile);
    if (dlnaStartPlaying) {
      hideWebControls();
      playFile(getPlaylistPath());
    }
  }
}

//Context menu (long press)
void dlnaListMenu(lv_event_t * event) {
  lv_obj_t * item = lv_event_get_target(event);
  lv_obj_t * menu = createListMenu(item);
  lv_obj_t * btn;
  btn = lv_list_add_btn(menu, LV_SYMBOL_PLAY, "Play Now");
  lv_obj_add_event_cb(btn, buildDlnaPlaylist, LV_EVENT_CLICKED, (void*)DLNA_PLAYACTION_NEW);
  btn = lv_list_add_btn(menu, LV_SYMBOL_NEXT, "Play Next");
  lv_obj_add_event_cb(btn, buildDlnaPlaylist, LV_EVENT_CLICKED, (void*)DLNA_PLAYACTION_INS);
  btn = lv_list_add_btn(menu, LV_SYMBOL_PLUS, "Add to Queue");
  lv_obj_add_event_cb(btn, buildDlnaPlaylist, LV_EVENT_CLICKED, (void*)DLNA_PLAYACTION_ADD);
  listMenuActivate(menu);
}

//List item selected action (short click)
void dlnaAction(lv_event_t * event) {
  lv_obj_t * child = lv_event_get_target(event);  //selected object
  listSetSelect(child);
  //selected a server - retrieve server info
  dlnaInfo* info = (dlnaInfo*)lv_obj_get_user_data(child);
  if (info->infoType == DLNA_TYPE_SERVER) {
    dlnaDirHead = new dlnaDir(dlnaDirHead, info->server->name, info->index, "0");
    updateDlnaHeader(info->server->name);
    DLNABrowseServers(info->index, "0");
  }
  //Selected an item
  else if (info->infoType == DLNA_TYPE_OBJECT) {
    //item is directory - browse into it
    if (info->object->isDir) {
      updateDlnaHeader(info->object->name);
      dlnaDirHead = new dlnaDir(dlnaDirHead, info->object->name, info->index, info->object->id);
      DLNABrowseServers(info->index, info->object->id);
    } else {
      //item is a file - construct the access URL and send it to a handler function if we know what to do with it
      String URL = "http://" + info->object->ip.toString() + ":" + info->object->port + "/" + info->object->uri;
      //Audio file
      if (info->object->type == fileTypeAudio) {
        dlnaSetMeta(info->object->artist, info->object->album, info->object->name);
        tabViewShowMain();
        connectToHost(URL.c_str(), false);  
      }
      //Image file
      else if (info->object->type == fileTypeImage) {
        tabViewShowMain();
        loadAlbumArt(URL.c_str());  
      }
      //Ignore video files
    }
  }
  lv_obj_set_hidden(dlnaUpBtn, false);
}

//Add a list button to the media list
lv_obj_t * addDlnaItem(dlnaInfo* info) {
  if (!info || !dlnaList) return NULL;
  lv_obj_t * list_btn;
  char buf[512];
  //Server entry
  if (info->infoType == DLNA_TYPE_SERVER) {  //Add to episode list
    sprintf(buf, "%s [%s]", info->server->name, info->server->ip.toString().c_str());
    list_btn = lv_list_add_btn(dlnaList, LV_SYMBOL_DRIVE, buf);
    lv_obj_add_event_cb(list_btn, dlnaAction, LV_EVENT_CLICKED, NULL);
  }
  else { //if (info->infoType == DLNA_TYPE_OBJECT) {
    //Directory entry
    if (info->object->isDir) {
      dlnaMakeName(buf, info->object->artist, info->object->album, info->object->name);
      list_btn = lv_list_add_btn(dlnaList, LV_SYMBOL_DIRECTORY, buf);
    }
    //Audio file entry
    else if (info->object->type == fileTypeAudio) {
      dlnaMakeName(buf, info->object->artist, info->object->album, info->object->name);
      list_btn = lv_list_add_btn(dlnaList, LV_SYMBOL_AUDIO, buf);
    }
    //Image file entry
    else if (info->object->type == fileTypeImage) {
      list_btn = lv_list_add_btn(dlnaList, LV_SYMBOL_IMAGE, info->object->name);
    }
    //Video file entry
    else if (info->object->type == fileTypeVideo) {
      list_btn = lv_list_add_btn(dlnaList, LV_SYMBOL_VIDEO, info->object->name);
    }
    //Anything else.. is there anything else?
    else { 
      list_btn = lv_list_add_btn(dlnaList, LV_SYMBOL_FILE, info->object->name);
    }
    //Add events to button
    lv_obj_add_event_cb(list_btn, dlnaAction, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_event_cb(list_btn, dlnaListMenu, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(list_btn, listMenuClicked, LV_EVENT_CLICKED, NULL);
  }
  lv_obj_add_flag(list_btn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_add_style(list_btn, &style_list, LV_PART_MAIN);
  lv_obj_add_style(list_btn, &style_listsel, LV_STATE_CHECKED);
  lv_obj_set_user_data(list_btn, (void*)info);
  return list_btn;
}

//Called from loop()
// Get messages from the DLNA client and dispatch actions from here
void DLNAHandle() {
  while (xQueueReceive(dlnaGetQueue, &dlnaRxMessage, 0) == pdPASS){
    //Got DLNA item - add to list
    if (dlnaRxMessage.cmd == DLNA_ITEM) addDlnaItem(dlnaRxMessage.info);
    //Starting a server seek - long wait - put up a spinner
    else if (dlnaRxMessage.cmd == DLNA_STARTSEEK) {
      info(NAME, 4, LV_SYMBOL_DRIVE " Scan for Media Servers..");
      showLoadSpinner();
    }
    //Server seek finished, hide the spinner
    else if (dlnaRxMessage.cmd == DLNA_ENDSEEK) hideWebControls();
    //DLNA action completed, hide the DLNA window spinner
    else if (dlnaRxMessage.cmd == DLNA_FINISHED) showDlnaSpinner(false);

    //DLNA item directory recursion
    //Got DLNA item from recursion - add into open playlist
    else if (dlnaRxMessage.cmd == DLNA_RECURSE) {
      addPlaylistEntry(dlnaRxMessage.info->object);
      delete(dlnaRxMessage.info);   //Delete this - information is now stored
    }
    //End of DLNA recursion - finish writing playlist, close it and start playing it
    else if (dlnaRxMessage.cmd == DLNA_RECURSE_FINISHED) {
      //Do we want to append the rest of the existing playlist? (insertion mode)
      if (dlnaAppendPos >= 0) {
        for (uint16_t i = dlnaAppendPos; i < getPlaylistCount(); i++) {
          lv_obj_t* child = getPlaylistChild(i);
          addPlaylistEntry(&dlnaPlaylistFile, (char*)lv_obj_get_user_data(child));            
        }  
      }
      //All done here
      Serial.println("Recurse finished.");
      lv_fs_close(&dlnaPlaylistFile);
      //Should we begin playing? (new playlist mode) 
      if (dlnaStartPlaying) {
        hideWebControls();
        playFile(getPlaylistPath());
      }
    }
  }
}

//--------------------------------------------------------------
// Threaded below

//Recursive DLNA file lister
void dlnaTaskRecurse(int index, const char* id) {
  soapObjectVect_t browseResult;                //Local list of results on the stack
  struct dlnaMessage dlnaTxTaskMessage;
  Serial.println("Browse server..");
  soap.browseServer(index, id, &browseResult);  //Load up the list
  if(browseResult.size() == 0) return; // empty list? then the directory is empty
  //Go through the results list
  for (int i = 0; i < browseResult.size(); i++) {
    soapObject_t obj = browseResult[i];
    if (obj.isDirectory) {
      //start reading directory - recursive call
      Serial.printf("Start directory %s\n", obj.name.c_str());
      dlnaTaskRecurse(index, obj.id.c_str()); //and back in we go for another round
    } else {
      //Only audio files needed in playlist please
      if (obj.fileType == fileTypeAudio) {  //reduce traffic
        //send file description
        dlnaTxTaskMessage.cmd = DLNA_RECURSE;
        dlnaTxTaskMessage.info = new dlnaInfo(DLNA_TYPE_OBJECT, index, &obj);
        xQueueSend(dlnaGetQueue, &dlnaTxTaskMessage, portMAX_DELAY);
      }
    }
  }
  browseResult.clear();     //Free the list
}


//Called from services task
bool DLNAClientHandle() {
  struct dlnaMessage dlnaRxTaskMessage;
  struct dlnaMessage dlnaTxTaskMessage;
  if(settings->mode == MODE_DLNA && WiFi.status() == WL_CONNECTED && xQueueReceive(dlnaSetQueue, &dlnaRxTaskMessage, 1) == pdPASS) {
    //Get servers
    if(dlnaRxTaskMessage.cmd == DLNA_SERVERS) {
      int numServers = soap.getServerCount();     //Have we got any cached servers?
      //Scan for servers
      if (dlnaRxTaskMessage.refresh || numServers == 0) { //No, or forced refresh
        dlnaTxTaskMessage.cmd = DLNA_STARTSEEK;
        xQueueSend(dlnaGetQueue, &dlnaTxTaskMessage, portMAX_DELAY);
        soap.seekServer();                        //Do the server scan
        numServers = soap.getServerCount();       //now how many?
        dlnaTxTaskMessage.cmd = DLNA_ENDSEEK;
        xQueueSend(dlnaGetQueue, &dlnaTxTaskMessage, portMAX_DELAY);
      }
      //Send server entries
      for(int i = 0; i < numServers; i++){
        soapServer_t srv;
        dlnaTxTaskMessage.cmd = DLNA_ITEM;
        soap.getServerInfo(i, &srv);
        Serial.println("got info");
        dlnaTxTaskMessage.info = new dlnaInfo(DLNA_TYPE_SERVER, i, &srv);
        xQueueSend(dlnaGetQueue, &dlnaTxTaskMessage, portMAX_DELAY);
      }
      //and we are done
      dlnaTxTaskMessage.cmd = DLNA_FINISHED;
      xQueueSend(dlnaGetQueue, &dlnaTxTaskMessage, portMAX_DELAY);
    }
    if(dlnaRxTaskMessage.cmd == DLNA_BROWSE) {
      soapObjectVect_t browseResult;      //local copy of results on stack
      Serial.println("Browse server..");
      soap.browseServer(dlnaRxTaskMessage.serverIndex, dlnaRxTaskMessage.id, &browseResult);
      if(browseResult.size() == 0) Serial.println("no content!"); // then the directory is empty
      //go through results
      for (int i = 0; i < browseResult.size(); i++) {
        soapObject_t obj = browseResult[i];
        dlnaTxTaskMessage.cmd = DLNA_ITEM;
        dlnaTxTaskMessage.info = new dlnaInfo(DLNA_TYPE_OBJECT, dlnaRxTaskMessage.serverIndex, &obj);
        xQueueSend(dlnaGetQueue, &dlnaTxTaskMessage, portMAX_DELAY);
      }
      browseResult.clear();     //free the results
      free((void*)dlnaRxTaskMessage.id);    //Don't forget to free the ID char array in the message
      //Done here
      dlnaTxTaskMessage.cmd = DLNA_FINISHED;
      xQueueSend(dlnaGetQueue, &dlnaTxTaskMessage, portMAX_DELAY);
    }
    if(dlnaRxTaskMessage.cmd == DLNA_RECURSE) {
      //Start the recursion here
      dlnaTaskRecurse(dlnaRxTaskMessage.serverIndex, dlnaRxTaskMessage.id);
      //finished with the ID field, must delete it
      free((void*)dlnaRxTaskMessage.id);
      //Recurse is done
      dlnaTxTaskMessage.cmd = DLNA_RECURSE_FINISHED;
      xQueueSend(dlnaGetQueue, &dlnaTxTaskMessage, portMAX_DELAY);
    }
  }
  return false;  //Does not require boost mode
}