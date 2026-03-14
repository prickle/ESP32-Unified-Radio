#include "decls.h"
//=======================================================================================
//Playlist window

static lv_obj_t * playlistWindow;
static lv_obj_t * playlistHeaderLabel;
static lv_obj_t * playlistMainList;

static void playlistWindow_close_action(lv_event_t * event);
static void playlistWindow_clear_action(lv_event_t * event);
static void playlistWindow_shuffle_action(lv_event_t * event);
static void playlistWindow_reload_action(lv_event_t * event);
static void playlistWindow_totop_action(lv_event_t * event);

#if (TFT_WIDTH == 480) 
#define BTNWIDTH  40
#else
#define BTNWIDTH  30
#endif

void createPlaylistWindow(lv_obj_t * parent) {
  playlistWindow = lv_win_create(parent, 40);
  playlistHeaderLabel = lv_win_add_title(playlistWindow, "Playlist");
  lv_obj_set_style_pad_left(playlistHeaderLabel, 6, LV_PART_MAIN);
  lv_obj_set_size(playlistWindow, lv_obj_get_content_width(parent), lv_obj_get_content_height(parent));
  lv_obj_set_pos(playlistWindow, 0, 0);
  lv_obj_add_style(playlistWindow, &style_win, LV_PART_MAIN);
  lv_obj_t * win_content = lv_win_get_content(playlistWindow);
  lv_obj_add_style(win_content, &style_win, LV_PART_MAIN);
  lv_obj_set_hidden(playlistWindow, true);
  
  lv_obj_t * totop_btn = lv_win_add_btn(playlistWindow, LV_SYMBOL_UP, BTNWIDTH);           /*Add close button and use built-in close action*/
  lv_obj_add_style(totop_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(totop_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(totop_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(totop_btn, playlistWindow_totop_action, LV_EVENT_CLICKED, NULL);

  lv_obj_t * reload_btn = lv_win_add_btn(playlistWindow, LV_SYMBOL_REFRESH, BTNWIDTH);           /*Add close button and use built-in close action*/
  lv_obj_add_style(reload_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(reload_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(reload_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(reload_btn, playlistWindow_reload_action, LV_EVENT_CLICKED, NULL);

  lv_obj_t * clear_btn = lv_win_add_btn(playlistWindow, LV_SYMBOL_TRASH, BTNWIDTH);           /*Add close button and use built-in close action*/
  lv_obj_add_style(clear_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(clear_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(clear_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(clear_btn, playlistWindow_clear_action, LV_EVENT_CLICKED, NULL);

  lv_obj_t * shuffle_btn = lv_win_add_btn(playlistWindow, LV_SYMBOL_SHUFFLE, BTNWIDTH);           /*Add close button and use built-in close action*/
  lv_obj_add_style(shuffle_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(shuffle_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(shuffle_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(shuffle_btn, playlistWindow_shuffle_action, LV_EVENT_CLICKED, NULL);

  lv_obj_t * close_btn = lv_win_add_btn(playlistWindow, LV_SYMBOL_CLOSE, BTNWIDTH);           /*Add close button and use built-in close action*/
  lv_obj_add_style(close_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(close_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(close_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(close_btn, playlistWindow_close_action, LV_EVENT_CLICKED, NULL);

  playlistMainList = lv_list_create(win_content);
  lv_obj_add_style(playlistMainList, &style_halfopa, LV_PART_MAIN);
  lv_obj_add_style(playlistMainList, &style_biggestfont, LV_PART_MAIN);
  lv_obj_update_layout(win_content);
  lv_obj_set_size(playlistMainList, lv_obj_get_content_width(win_content), lv_obj_get_content_height(win_content));
  lv_obj_set_pos(playlistMainList, 0, 0);
}

void setPlaylistVisibility() {
#ifdef SDPLAYER  
  lv_obj_set_hidden(playlistWindow, settings->mode != MODE_SD && settings->mode != MODE_FTP && settings->mode != MODE_DLNA);  
#else
  lv_obj_set_hidden(playlistWindow, settings->mode != MODE_FTP && settings->mode != MODE_DLNA);  
#endif
}

void hidePlaylist(bool hide) {
  lv_obj_set_hidden(playlistWindow, hide);
}

void playlistWindow_totop_action(lv_event_t * event) {
  lv_obj_t * win_content = lv_win_get_content(playlistWindow);
  lv_obj_scroll_to(win_content, 0, 0, LV_ANIM_ON);
}

//Close button
static void playlistWindow_close_action(lv_event_t * event) {
  hidePlaylist(true);
  showPresets(true);
}

//Clear button
static void playlistWindow_clear_action(lv_event_t * event) {
  sdStop();
  sdSongFinished();
  setPlaylistIndex(0);
  clearPlaylist();
}

//Shuffle button
static void playlistWindow_shuffle_action(lv_event_t * event) {
  Serial.println("> Shuffling playlist..");
  shufflePlaylist();
  setPlaylistIndex(1);
  playPlaylistFile();    
}

//Reload button
static void playlistWindow_reload_action(lv_event_t * event) {
  if (loadPlaylist(getPlaylistPath())) {
    setPlaylistIndex(1);
    playPlaylistFile();
  }    
}

//Playlist header text showing currently playing index
void updatePlaylistHeader() {
  lv_label_set_text_fmt(playlistHeaderLabel, "Playlist (%d of %d)", getPlaylistIndex(), (int)lv_obj_get_child_cnt(playlistMainList));
}

//Clear playlist and also free the entry's attached userdata - updates header too
void clearPlaylist() {
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(playlistMainList); i++) {
    lv_obj_t* child = lv_obj_get_child(playlistMainList, i);
    void * data = lv_obj_get_user_data(child);
    if (data) {
      free(data);
      data = NULL;
    }
  }
  lv_obj_clean(playlistMainList);
  updatePlaylistHeader();
}

void playlistRemoveItem(lv_event_t * event) {
  listMenuClose(event);
  int index = listGetSelect(playlistMainList);
  if (index >= 0) {
    lv_obj_t* child = lv_obj_get_child(playlistMainList, index);
    lv_obj_del(child);
    writePlaylist();
    updatePlaylistHeader();
  }
}

//Context menu
void playlistMenu(lv_event_t * event) {
  lv_obj_t * item = lv_event_get_target(event);
  lv_obj_t * menu = createListMenu(item);
  lv_obj_t * btn;
  btn = lv_list_add_btn(menu, LV_SYMBOL_TRASH, "Remove Item");
  lv_obj_add_event_cb(btn, playlistRemoveItem, LV_EVENT_CLICKED, NULL);
  listMenuActivate(menu);
}

//When a playlist entry is selected
void playlistAction(lv_event_t * event) {
  lv_obj_t * child = lv_event_get_target(event);
  int index = lv_obj_get_child_id(child);
  listSetSelect(child);
  setPlaylistIndex(index+1);
  playPlaylistFile();
}

//Add an entry to the station list
lv_obj_t * addPlaylistButton(const char* text, void* data, int datalen) {
  lv_obj_t * list_btn;
  if (!playlistMainList) return NULL;
  list_btn = lv_list_add_btn(playlistMainList, LV_SYMBOL_AUDIO, text);
  lv_obj_add_event_cb(list_btn, playlistAction, LV_EVENT_SHORT_CLICKED, NULL);
  lv_obj_add_event_cb(list_btn, playlistMenu, LV_EVENT_LONG_PRESSED, NULL);
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

int getPlaylistCount() { 
  return lv_obj_get_child_cnt(playlistMainList);
}

lv_obj_t * getPlaylistChild(int index) {
  return lv_obj_get_child(playlistMainList, index);
}

const char* getPlaylistItem(int index) {
  lv_obj_t* child = lv_obj_get_child(playlistMainList, index);
  if (child) return lv_list_get_btn_text(playlistMainList, child);
  else return "";
}

//------------------------------------------------------------
//Playlist file management

//Start playing most recently played file in list
void resumePlaylist() {
  if (getPlaylistIndex()) {
    if (loadPlaylist(getPlaylistPath()))
      playPlaylistFile();
  }
}

//Mode-aware playlist info
const char* getPlaylistPath() {
  if (settings->mode == MODE_FTP) return FTPPLAYLIST_PATH;
#ifdef SDPLAYER  
  else if (settings->mode == MODE_SD) return SDPLAYLIST_PATH;
#endif
  else if (settings->mode == MODE_DLNA) return DLNAPLAYLIST_PATH;
  return "";
}

int getPlaylistIndex() {
  if (settings->mode == MODE_FTP) return settings->ftpPlaylistIndex;
#ifdef SDPLAYER
  else if (settings->mode == MODE_SD) return settings->playlistIndex;
#endif
  else if (settings->mode == MODE_DLNA) return settings->dlnaPlaylistIndex;
  return 0;  
}

void setPlaylistIndex(int index) {
  if (settings->mode == MODE_FTP) settings->ftpPlaylistIndex = index;
#ifdef SDPLAYER
  else if (settings->mode == MODE_SD) settings->playlistIndex = index;
#endif
  else if (settings->mode == MODE_DLNA) settings->dlnaPlaylistIndex = index;
  writeSettings();
}

//Remove (delete) playlist
void removePlaylist(const char* path) {
  if (path[0] == 'C') { //SPIFFS
    if (SPIFFS.exists(&path[2])) {
      Serial.println("> Removing old playlist");
      SPIFFS.remove(&path[2]);  
    }
  }
  else if (path[0] == 'D') { //SD
    if (SD.exists(&path[2])) {
      Serial.println("> Removing old playlist");
      SD.remove(&path[2]);  
    }
  }
}

//Shuffle the playlist and write it back out
void shufflePlaylist() {
  srand(millis());
  int N = lv_obj_get_child_cnt(playlistMainList);
  for (int i = 0; i < N-1; ++i)
  {
    lv_obj_t* btn1 = lv_obj_get_child(playlistMainList, i);
    int j = rand() % (N-i) + i;
    lv_obj_t* btn2 = lv_obj_get_child(playlistMainList, j);
    lv_obj_swap(btn1, btn2);
  }
  writePlaylist();
}

//Read an M3U file into the playlist
bool loadPlaylist(const char * file) {
  Serial.print("> Reading playlist..");
  lv_fs_file_t f;
  if (lv_fs_open(&f, file, LV_FS_MODE_RD) != LV_FS_RES_OK) {
    setPlaylistIndex(0);
    return false;
  }
  int lbi = 0;
  char linebuf[516];
  char name[128];
  int count = 0;
  lv_obj_t * current = NULL;
  uint32_t read_num;
  char ch;
  info(NAME, 3, LV_SYMBOL_DIRECTORY " Loading Playlist..");
  showLoadSpinner();
  clearPlaylist();
  while (1) {
    if (fs_err(lv_fs_read(&f, &ch, 1, &read_num), "Read playlist")) return false;
    if (read_num == 0) break;
    if (ch != '\r' && ch != '\n' ) linebuf[lbi++] = ch;
    if (ch == '\n' || lbi == 511) {
      linebuf[lbi] = 0;
      if (linebuf[0] == '#') {
        char * ptr = strchr(linebuf, ',');
        if (ptr) strncpy(name, ptr+1, 127);
        else strncpy(name, &linebuf[1], 127);
        name[127] = '\0';
      } else {
        lv_obj_t * list_btn = addPlaylistButton(name, linebuf, lbi+1);
        if (count++ == getPlaylistIndex() - 1) current = list_btn;
        if (count % 10 == 0) {
          info(NOW, 2, "Files: %d", count);
          refreshDisplay();
        }
      }
      lbi = 0;
    }
  }
  if (current) listSetSelect(current);
  lv_fs_close(&f);
  hideWebControls();
  Serial.println("OK.");
  info(NOW, 2, "Files: %d", count);
  updatePlaylistHeader();
  return true;
}

void writePlaylistToFp(lv_fs_file_t* f) {
  uint32_t byteswrote;
  char str[FTP_NAME_LENGTH + 5];
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(playlistMainList); i++) {
    lv_obj_t* child = lv_obj_get_child(playlistMainList, i);
    const char * name = lv_list_get_btn_text(playlistMainList, child);
    snprintf(str, FTP_NAME_LENGTH + 4, "#%s\r\n", name);
    if (fs_err(lv_fs_write(f, str, strlen(str), &byteswrote), "Write Playlist Header")) return;
    const char * path = (const char*)lv_obj_get_user_data(child);
    if (path) {
      snprintf(str, FTP_NAME_LENGTH + 4, "%s\r\n", path);
      if (fs_err(lv_fs_write(f, str, strlen(str), &byteswrote), "Write Playlist Entry")) return;
    }
  }  
}

//Write the current playlist to the playlist file
void writePlaylist() {
  lv_fs_file_t f;
  const char* path = getPlaylistPath();
  //Build playlist
  removePlaylist(path);
  if (fs_err(lv_fs_open(&f, path, LV_FS_MODE_WR), "Open Playlist")) return;
  writePlaylistToFp(&f);
  lv_fs_close(&f);
}

void addPlaylistEntry(lv_fs_file_t* f, const char* name, const char* path) {
  uint32_t byteswrote;
  char str[FTP_NAME_LENGTH + 5];
  snprintf(str, FTP_NAME_LENGTH + 4, "#%s\r\n", name);
  if (fs_err(lv_fs_write(f, str, strlen(str), &byteswrote), "Write Playlist Header")) return;
  snprintf(str, FTP_NAME_LENGTH + 4, "%s\r\n", path);
  if (fs_err(lv_fs_write(f, str, strlen(str), &byteswrote), "Write Playlist Entry")) return;
}


//Add a playlist entry to an open file
// used by playlistRecurseDirectory below to build playlists from browser selections
void addPlaylistEntry(lv_fs_file_t * f, char * path) {
  if (isKnownMusicFile(path)) {  
    char name[LV_FS_MAX_PATH_LENGTH] = {0};
    char * pos;
    if ((pos = strrchr(path, '/'))) {
      strncpy(name, pos+1, LV_FS_MAX_PATH_LENGTH - 1);
      name[LV_FS_MAX_PATH_LENGTH-1] = '\0';
    }
    addPlaylistEntry(f, name, path);
  }
}

//Append a name to a path string, adding slash as required
void appendFileName(char * path, const char * name, int maxlen) {
  uint32_t len = strnlen(path, maxlen - 1);
  if (name[0] != '/') {
    strncat(path, "/", maxlen - len - 1);
    len++;
  }
  strncat(path, name, maxlen - len - 1);
}

//Recursive directory tree traversal
// Builds a .m3u playlist from known audio files
void playlistRecurseDirectory(lv_fs_file_t * f, char * orig_path) {
  //char fn[LV_FS_MAX_PATH_LENGTH];
  //char path[LV_FS_MAX_PATH_LENGTH];
  char *fn = (char*)ps_malloc(LV_FS_MAX_PATH_LENGTH);
  char *path = (char*)ps_malloc(LV_FS_MAX_PATH_LENGTH);
  if (!fn || !path) {
    errorContinue(0, "Out of memory in playlistRecurseDirectory()");
    if (fn) free(fn);
    return;
  }
  lv_fs_dir_t dir;
  showFilename(orig_path);
  if (fs_err(lv_fs_dir_open(&dir, orig_path), "Playlist Recurse Directory")) return;
  do {
    fn[0] = 0;
    lv_fs_dir_read(&dir, fn);
    if(fn[0] != 0) {
      strncpy(path, orig_path, LV_FS_MAX_PATH_LENGTH-1);
      path[LV_FS_MAX_PATH_LENGTH-1] = '\0';
      appendFileName(path, fn, LV_FS_MAX_PATH_LENGTH);
      if (fn[0] == '/') playlistRecurseDirectory(f, path);    
      else addPlaylistEntry(f, path);              
    } else break;
  } while(1);
  free(path);
  free(fn);
  lv_fs_dir_close(&dir);
}
