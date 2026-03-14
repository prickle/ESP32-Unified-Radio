#include "decls.h"
//------------------------------------------------------------
//File player

volatile bool SDFound = false;
char * sdFileName;
String metaArtist, metaAlbum, metaTitle, metaYear;

//Called from setup
void initPlayer() {
  //SD current filename
  sdFileName = (char*)ps_malloc(FTP_NAME_LENGTH);
  strcpy(sdFileName, "<None>");
}

//Play a file - recursive if playlist
boolean playFile(const char *file) {
  strncpy(sdFileName, file, FTP_NAME_LENGTH-1);
  sdFileName[FTP_NAME_LENGTH-1] = '\0';
  updateUrlEditText();
  if (isPlaylistFile(sdFileName)) {
    if (loadPlaylist(sdFileName)) {
      setPlaylistIndex(1);
      playPlaylistFile();
    }
    return false;
  }
  sdStop();
  sdSongFinished();
  //Handle DLNA playlists
  if (strncmp(sdFileName, "http://", 7) == 0) {
    connectToHost(sdFileName, false);
  }
  //normal file-based playlist
  else connectToFS(sdFileName, 0);
  //Reset state
  return true;  
}

//Used by playFile() above
void playPlaylistFile() {
  uint16_t index = getPlaylistIndex();
  if (index) {
    lv_obj_t* child = getPlaylistChild(index-1);
    if (child) {
      const char* name = getPlaylistItem(index-1);
      listSetSelect(child);
      dlnaSetMeta(name);
      void * data = lv_obj_get_user_data(child);
      if (data) playFile((const char*)data);
    } else setPlaylistIndex(0);
  }
  updatePlaylistHeader();
}

//Called from webradio audio handler
void metadataSD() {
  if (settings->mode == MODE_POD) return;   //Ignore metadata from podcasts
  //Show the metadata
  if (metaArtist != "" || metaTitle != "") { 
    String s = (String)LV_SYMBOL_AUDIO + " - ";
    if (metaArtist != "") s += metaArtist + " -";
    if (metaTitle != "") s += " " + metaTitle + " -";  
    info(NAME, 0, s.c_str()); 
  }
  
  if (metaAlbum != "" || metaYear != "") { 
    String s = "";
    if (metaAlbum != "") s += (String)"Album: " + metaAlbum;
    if (metaYear != "") s += (String)" (" + metaYear + ")";
    info(NOW, 0, s.c_str());       
  }
}

//Called from webradio audio handler
void sdEOF() {
  strcpy(sdFileName, "<None>");  
  updateUrlEditText();
  //Trigger the next song in the playlist here.
  uint16_t index = getPlaylistIndex();
  if (index) {
    if (++index > getPlaylistCount()) {
      //End of playlist
      index = 0;
      info(NOW, 0, "End of playlist.");
      clearPlaylist();      
    }
    setPlaylistIndex(index); //Next song in playlist
  }  
  if (index == 0) sdSongFinished();
  playPlaylistFile();
}

//Stop playback, close the file and turn off decoding
void sdStop() {
  webradioStop();
}

//Clear the GUI
void sdSongFinished() {
  if (progTimeBar) lv_bar_set_value(progTimeBar, 0, LV_ANIM_OFF);
  clearBufStat();
  clearAlbumArt();
}

#ifdef SDPLAYER
//Try to init newly-inserted SD card - fairly useless when using cassette mechanism
void sdReinit() {
  showLoadSpinner();
  if ((SDFound = SD.begin(SD_CS, myspi, 40000000))) {
    SDfileSystemInit();
    setSdcardLbl(LV_SYMBOL_SD_CARD);
    hideWebControls();
    sdPopulateBrowser(NULL);          //kick off the file browser
    info(NAME, 0, LV_SYMBOL_STOP " Stopped");
    info(NOW, 0, LV_SYMBOL_LEFT " Choose files from the list to the left.. ");
    resumePlaylist();
  } else showReloadBtn();
}
#endif