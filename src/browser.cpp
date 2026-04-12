#include "decls.h"
//File browser window
// and SD Card browsing routines
//File type checks at bottom

char * browserCurrentDir;
bool browserSelectingMode = false;
//char * browserSelectEntry;
//static bool populateBrowserFlag;
lv_obj_t * browserWindow;
lv_obj_t * browserMainList;
lv_obj_t * browserSelectBtn = NULL;
lv_obj_t * browserCancelBtn = NULL;
lv_obj_t * browserWinTitle;
lv_obj_t * browserSpinner;


void browserGoUp(lv_event_t * event);
void browserGoHome(lv_event_t * event);
void browserTotop(lv_event_t * event);

void initBrowser() {
  // Start browser in the music directory
  browserCurrentDir = (char*)ps_malloc(LV_FS_MAX_PATH_LENGTH);
  strcpy(browserCurrentDir, MUSIC_PATH);//FTPTEST_PATH);//MUSIC_PATH);  
}

//make the window
void createFileBrowserWindow(lv_obj_t * page) {
  browserWindow = lv_win_create(page, 40);
  lv_obj_set_size(browserWindow, lv_obj_get_content_width(page), lv_obj_get_content_height(page));
  lv_obj_add_style(browserWindow, &style_win, LV_PART_MAIN);
  lv_obj_t * up_btn = lv_win_add_btn(browserWindow, LV_SYMBOL_LEFT, 50);
  lv_obj_add_event_cb(up_btn, browserGoUp, LV_EVENT_CLICKED, NULL);
  lv_obj_add_style(up_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(up_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(up_btn, &style_bigfont_orange, LV_PART_SELECTED);
  browserWinTitle = lv_win_add_title(browserWindow, "File Manager");
  lv_obj_set_style_pad_left(browserWinTitle, 6, LV_PART_MAIN);
  lv_obj_t * totop_btn = lv_win_add_btn(browserWindow, LV_SYMBOL_UP, 50);           /*Add close button and use built-in close action*/
  lv_obj_add_style(totop_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(totop_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(totop_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(totop_btn, browserTotop, LV_EVENT_CLICKED, NULL);
  lv_obj_t * home_btn = lv_win_add_btn(browserWindow, LV_SYMBOL_HOME, 50);
  lv_obj_add_style(home_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(home_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(home_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(home_btn, browserGoHome, LV_EVENT_CLICKED, NULL);
  lv_obj_t * win_content = lv_win_get_content(browserWindow);
  lv_obj_add_style(win_content, &style_win, LV_PART_MAIN);
  
  /* Create the file list */
  browserMainList = lv_list_create(win_content);
  //lv_obj_add_style(browserMainList, &style_win, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(browserMainList, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_add_style(browserMainList, &style_listfont, LV_PART_MAIN);
  lv_obj_update_layout(win_content);
  lv_obj_set_size(browserMainList, lv_obj_get_content_width(win_content), lv_obj_get_content_height(win_content));
  lv_obj_set_pos(browserMainList, 0, 0);
  lv_obj_set_hidden(browserWindow, true);

    //Loading spinner
  browserSpinner = lv_spinner_create(win_content, 1000, 60);
  lv_obj_set_size(browserSpinner, 100, 100);
  lv_obj_align(browserSpinner, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(browserSpinner, LV_OBJ_FLAG_FLOATING);
  lv_obj_set_hidden(browserSpinner, true);

  lv_obj_set_hidden(browserWindow, true);

}

//Show browser for the right modes
void setBrowserVisibility() {
#ifdef SDPLAYER
  lv_obj_set_hidden(browserWindow, settings->mode != MODE_SD && settings->mode != MODE_FTP);  
#else
  lv_obj_set_hidden(browserWindow, settings->mode != MODE_FTP);  
#endif
}

void showBrowserSpinner(bool shown) {
  if (browserSpinner) lv_obj_set_hidden(browserSpinner, !shown);
  refreshDisplay();   //Ensure shown
}

//Set the current directory in the title bar
void setBrowserTitle(const char* path) {
  if (!browserWinTitle) return;
  char *firstSlash = strchr(path, '/');
  char *lastSlash = strrchr(path, '/');
  if (firstSlash && firstSlash != lastSlash) { //Two slashes?
    lv_label_set_text_fmt(browserWinTitle, "..%s", lastSlash);
  } else {
    if (settings->mode == MODE_FTP) {
      IPAddress ip = settings->ftpAddress;
      String root = ip.toString() + path;
      lv_label_set_text(browserWinTitle, root.c_str());  
    } else lv_label_set_text(browserWinTitle, path);  
  }
}

//Show just the filename from a path in the progNow label for a bit
void showFilename(const char * path) {
  const char* ptr = strrchr(path, '/');
  if (!ptr) ptr = path; //no slash, use whole string
  else ptr += 1;        //move past the slash
  info(NOW, 3, "/%s..", ptr);
  refreshDisplay();
}

//Remove all selections from file browser
void browserClearSelections() {
  for (uint16_t i = 0; i < lv_obj_get_child_cnt(browserMainList); i++) {
    lv_obj_t* child = lv_obj_get_child(browserMainList, i);
    lv_obj_clear_state(child, LV_STATE_CHECKED);
  }  
}

//Back out a directory level - SD and FTP
void browserGoUp(lv_event_t * event) {
  if (settings->mode == MODE_FTP) {
    ftpGoUp();
    return;
  }
  if(strlen(browserCurrentDir) == 2) {
    // We cannot go up further 
    return;
  }
  char *last_slash = strrchr(browserCurrentDir, '/');
  if (last_slash != NULL) {
    *last_slash = 0;
    sdPopulateBrowser(last_slash + 1);
  }
}

void browserTotop(lv_event_t * event) {
  listMenuClose(browserMainList, false);
  lv_obj_scroll_to(browserMainList, 0, 0, LV_ANIM_ON);
}

//Return to base location  - SD and FTP
void browserGoHome(lv_event_t * event) {
  listMenuClose(browserMainList, false);
  if (settings->mode == MODE_FTP) {
    ftpGoHome();
    return;
  }
  strcpy(browserCurrentDir, MUSIC_PATH);
  sdPopulateBrowser(NULL);
}

//File entry selected - SD only
void sdFileListButtonEvent(lv_event_t * event) {
  lv_obj_t * list_btn = lv_event_get_target(event);
  // Get some basic information about the file
  const char *filename = lv_list_get_btn_text(browserMainList, list_btn);
  bool is_dir = (bool)lv_obj_get_user_data(list_btn);  
  if(is_dir) {
    // Change to new directories
    uint32_t len = strnlen(browserCurrentDir, LV_FS_MAX_PATH_LENGTH);
    strncat(browserCurrentDir, "/", LV_FS_MAX_PATH_LENGTH - len - 1);
    len++;
    strncat(browserCurrentDir, filename, LV_FS_MAX_PATH_LENGTH - len - 1);
    sdPopulateBrowser(NULL);  
  } else {
    //Play single file
    lv_obj_add_state(list_btn, LV_STATE_CHECKED);
    tabViewShowMain();
    char fname[LV_FS_MAX_PATH_LENGTH];
    strncpy(fname, browserCurrentDir, LV_FS_MAX_PATH_LENGTH-1);
    uint32_t len = strnlen(fname, LV_FS_MAX_PATH_LENGTH-1);
    strncat(fname, "/", LV_FS_MAX_PATH_LENGTH - len - 1);
    strncat(fname, filename, LV_FS_MAX_PATH_LENGTH - len - 2);
    browserClearSelections();
    clearAlbumArt();
    if (isJPGFile(fname)) loadAlbumArt(fname);
    else {
      setPlaylistIndex(0);
      playFile(fname);
    }
  }    
}

void listWarning(lv_obj_t* list, const char* msg) {
  char buf[128];
  snprintf(buf, 127, "\n" LV_SYMBOL_WARNING " %s!", msg);
  lv_obj_t* txt = lv_list_add_text(list, buf);
  lv_obj_set_style_text_font(txt, &lv_font_mymontserrat_20, LV_PART_MAIN);
  lv_obj_set_style_text_align(txt, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(txt, LV_OPA_0, LV_PART_MAIN);
}


void buildPlaylist(lv_fs_file_t* f, lv_obj_t* child) {
  char filepath[FTP_NAME_LENGTH+1];
  const char *filename = lv_list_get_btn_text(browserMainList, child);
#ifdef SDPLAYER
  char* currentDir = (settings->mode == MODE_SD)?browserCurrentDir:ftpCurrentDir;
#else
  char* currentDir = ftpCurrentDir;
#endif
  bool is_dir = (bool)lv_obj_get_user_data(child);
  strncpy(filepath, currentDir, FTP_NAME_LENGTH-1);
  filepath[FTP_NAME_LENGTH-1] = '\0';
  appendFileName(filepath, filename, FTP_NAME_LENGTH);   
  showFilename(filename);
  if(is_dir) {
    //recurse into directories
#ifdef SDPLAYER
    if (settings->mode == MODE_SD) playlistRecurseDirectory(f, filepath);
    else if (settings->mode == MODE_FTP) ftpPlaylistRecurseDirectory(f, filepath);
#else
    ftpPlaylistRecurseDirectory(f, filepath);
#endif
  } else {
    //add single file
    addPlaylistEntry(f, filepath);      
  }
}

void newPlaylist(lv_event_t * event) {
  lv_fs_file_t f;
  listMenuClose(browserMainList, true);
  lv_obj_t* child = listGetSelectObj(browserMainList);
  if (child == NULL) return;
  const char* path = getPlaylistPath();
  //Build playlist
  sdStop();
  sdSongFinished();
  clearAlbumArt(); 
  info(NAME, 0, LV_SYMBOL_DIRECTORY " Building Playlist..");
  showLoadSpinner();
  setPlaylistIndex(1);              //in case we crash, whatever we got of the playlist picks up on reboot  
  tabViewShowMain();
  removePlaylist(path);
  if (fs_err(lv_fs_open(&f, path, LV_FS_MODE_WR), "Open Playlist")) return;
  buildPlaylist(&f, child);
  lv_fs_close(&f);
  hideWebControls();
  playFile(path);
}

void insertPlaylist(lv_event_t * event) {
  lv_fs_file_t f;
  bool startPlaying = false;
  listMenuClose(browserMainList, true);
  lv_obj_t* child = listGetSelectObj(browserMainList);
  if (child == NULL) return;
  if (!getPlaylistCount()) startPlaying = true;
  const char* path = getPlaylistPath();
  if (startPlaying) {   //empty playlist
    //Build playlist
    sdStop();
    sdSongFinished();
    clearAlbumArt(); 
    info(NAME, 0, LV_SYMBOL_DIRECTORY " Building Playlist..");
    showLoadSpinner();
    setPlaylistIndex(1);              //in case we crash, whatever we got of the playlist picks up on reboot  
  }
  tabViewShowMain();
  removePlaylist(path);
  if (fs_err(lv_fs_open(&f, path, LV_FS_MODE_WR), "Open Playlist")) return;
  for (int16_t i = 0; i < getPlaylistIndex(); i++) {
    lv_obj_t* child = getPlaylistChild(i);
    addPlaylistEntry(&f, (char*)lv_obj_get_user_data(child));            
  }
  buildPlaylist(&f, child);
  for (int16_t i = getPlaylistIndex(); i < getPlaylistCount(); i++) {
    lv_obj_t* child = getPlaylistChild(i);
    addPlaylistEntry(&f, (char*)lv_obj_get_user_data(child));            
  }
  lv_fs_close(&f);
  hideWebControls();
  if (startPlaying) playFile(path);
  else if (!loadPlaylist(path)) log_w("Playlist file missing");
}

void addToPlaylist(lv_event_t * event) {
  lv_fs_file_t f;
  bool startPlaying = false;
  listMenuClose(browserMainList, true);
  lv_obj_t* child = listGetSelectObj(browserMainList);
  if (child == NULL) return;
  if (!getPlaylistCount()) startPlaying = true;
  const char* path = getPlaylistPath();
  //Build playlist
  if (startPlaying) {   //empty playlist
    sdStop();
    sdSongFinished();
    clearAlbumArt(); 
    info(NAME, 0, LV_SYMBOL_DIRECTORY " Building Playlist..");
    showLoadSpinner();
    setPlaylistIndex(1);              //in case we crash, whatever we got of the playlist picks up on reboot  
  } else info(NAME, 3, LV_SYMBOL_DIRECTORY " Appending Playlist..");
  tabViewShowMain();
  removePlaylist(path);
  if (fs_err(lv_fs_open(&f, path, LV_FS_MODE_WR), "Open Playlist")) return;
  writePlaylistToFp(&f);
  buildPlaylist(&f, child);
  lv_fs_close(&f);
  hideWebControls();
  if (startPlaying) playFile(getPlaylistPath());
  else if (!loadPlaylist(getPlaylistPath())) log_w("Playlist file missing");
}

//Context menu
void browserListMenu(lv_event_t * event) {
  lv_obj_t * item = lv_event_get_target(event);
  lv_obj_t * menu = createListMenu(item);
  lv_obj_t * btn;
  btn = lv_list_add_btn(menu, LV_SYMBOL_PLAY, "Play Now");
  lv_obj_add_event_cb(btn, newPlaylist, LV_EVENT_CLICKED, NULL);
  btn = lv_list_add_btn(menu, LV_SYMBOL_NEXT, "Play Next");
  lv_obj_add_event_cb(btn, insertPlaylist, LV_EVENT_CLICKED, NULL);
  btn = lv_list_add_btn(menu, LV_SYMBOL_PLUS, "Add to Queue");
  lv_obj_add_event_cb(btn, addToPlaylist, LV_EVENT_CLICKED, NULL);
  listMenuActivate(menu);
}

//Read a directory from disk into the file browser window - SD only
void sdPopulateBrowser(char *selectEntry) {
  char fn[LV_FS_MAX_PATH_LENGTH];
  lv_fs_dir_t dir;
  bool is_dir;
  lv_obj_clean(browserMainList);
  if(!SDFound) {
    listWarning(browserMainList, "SD Card not found");
    return;
  }
  setBrowserTitle(browserCurrentDir);
  if (fs_err(lv_fs_dir_open(&dir, browserCurrentDir), "Browser Open Directory")) return;  
  fn[1] = 0;
  do {
    lv_fs_dir_read(&dir, fn);
    if(fn[0] != 0) {
      is_dir = (fn[0] == '/');
      const char* symbol = LV_SYMBOL_DIRECTORY;
      if (!is_dir) {
        if (isKnownMusicFile(fn)) symbol = LV_SYMBOL_AUDIO;
        else if (isJPGFile(fn)) symbol = LV_SYMBOL_IMAGE;
        else symbol = LV_SYMBOL_FILE;  
      }
      String name = &fn[is_dir];
      lv_obj_t * list_btn = lv_list_add_btn(browserMainList, symbol, name.c_str());
      lv_obj_add_style(list_btn, &style_list, LV_PART_MAIN);
      lv_obj_add_style(list_btn, &style_listsel, LV_STATE_CHECKED);
      lv_obj_set_user_data(list_btn, (void*)is_dir);
      lv_obj_add_event_cb(list_btn, sdFileListButtonEvent, LV_EVENT_SHORT_CLICKED, NULL);
      lv_obj_add_event_cb(list_btn, browserListMenu, LV_EVENT_LONG_PRESSED, NULL);
      lv_obj_add_event_cb(list_btn, listMenuClicked, LV_EVENT_CLICKED, NULL);
      //Scroll to selected entry, if specified
      if (selectEntry && strcmp(&fn[is_dir], selectEntry) == 0)
        lv_obj_scroll_to_view(list_btn, LV_ANIM_OFF);
    } else break;
  } while(1);
  lv_fs_dir_close(&dir);
}

//------------------------
// File Type checks

boolean isMP3File(const char* fileName) {
  return (strlen(fileName) > 4) && !strcasecmp(fileName + strlen(fileName) - 4, ".mp3");
}

boolean isMP4File(const char* fileName) {
  return ((strlen(fileName) > 4) && (
            !strcasecmp(fileName + strlen(fileName) - 4, ".mp4") ||
            !strcasecmp(fileName + strlen(fileName) - 4, ".m4a") ||
            !strcasecmp(fileName + strlen(fileName) - 4, ".3gp") ||
            !strcasecmp(fileName + strlen(fileName) - 4, ".3g2") ||
            !strcasecmp(fileName + strlen(fileName) - 4, ".aac"))
          );
}

boolean isOGGFile(const char* fileName) {
  return (strlen(fileName) > 4) && !strcasecmp(fileName + strlen(fileName) - 4, ".ogg");
}

boolean isFlacFile(const char* fileName) {
  return (strlen(fileName) > 5) && !strcasecmp(fileName + strlen(fileName) - 5, ".flac");
}

boolean isKnownMusicFile(const char * filename) {
  if (isMP3File(filename)) return true;
  if (isMP4File(filename)) return true;
  if (isOGGFile(filename)) return true;
  if (isFlacFile(filename)) return true;
  return false;  
}

boolean isJPGFile(const char* fileName) {
  return (strlen(fileName) > 4) && !strcasecmp(fileName + strlen(fileName) - 4, ".jpg");
}

boolean isPlaylistFile(const char* fileName) {
  return (strlen(fileName) > 4) && !strcasecmp(fileName + strlen(fileName) - 4, ".m3u");
}
