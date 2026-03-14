#include "decls.h"
//-----------------------------------------------------------------------
// Album art

//Note that FTP is case sensitive, SD is not..

//Paths
char* artCurrentFile = NULL;    //artCurrentFile is the path of a newly playing audio file or image file for display
char* artPath = NULL;           //artPath is the directory part of previous path, with try files copied in to the end
char* artPathPtr = NULL;        //artPathPtr is pointer to the end of the directory part of artPath

//JPG file storage
uint8_t* jpgBuffer = NULL;
uint32_t jpgBufferSize;
uint32_t jpgBufferIndex;

//Decoded bitmap storage
uint16_t* jpgImgBuffer = NULL;
static lv_img_dsc_t jpg_img_dsc;

//States
#define ART_IDLE     0
#define ART_DOWNLOAD 1
#define ART_DISPLAY  2
#define ART_OPEN     3
#define ART_TRY_1    4
#define ART_TRY_2    5
#define ART_TRY_3    6
#define ART_TRY_4    7
#define ART_TRY_5    8
#define ART_TRY_6    9
uint8_t artHandlerState = ART_IDLE;     //Machine State
//State timer
#define ARTHANDLER_DELAYTIME 5000;
#define ARTHANDLER_FILECHECKTIME 500;
int64_t artTimer;
//State triggers
bool doFindArt = false;   //Find art file Trigger
bool doLoadArt = false;   //Load art file Trigger
bool doShowArt = false;   //Show art file Trigger
//Flags
bool artFileIsOpen = false;
bool artFullscreen = false;


bool artFilePrepare();
bool artFileOpen(const char* file);
void artFileClose();
bool artFileDownload();

bool ftpOpenAlbumArt(char* path);
#ifdef SDPLAYER
bool sdOpenAlbumArt(char* path);
#endif
bool httpOpenAlbumArt(char* path);
bool clientDownloadAlbumArt();
bool sdDownloadAlbumArt();
bool renderAlbumArt(int16_t bx, int16_t by, uint16_t bw, uint16_t bh, uint16_t* bitmap);


//Error handling
// Some condition caused us to bail out before displaying.. explain to user
// This appears instead of the art image itself
void artError(const char* msg) {
  char buf[64];
  snprintf(buf, 63, LV_SYMBOL_WARNING " %s", msg);
  updateArtImg(buf);
  Serial.print("Album Art: ");
  Serial.println(msg);
}

//Called from setup
// Allocate space for path strings
void initAlbumArt() {
  artCurrentFile = (char*)ps_malloc(FTP_NAME_LENGTH);
  if (!artCurrentFile) {
    errorContinue(0, "artCurrentFile: Out of memory!");
    return;
  }
  artPath = (char*)ps_malloc(FTP_NAME_LENGTH);
  if (!artPath) {
    free(artCurrentFile);
    artCurrentFile = NULL;
    errorContinue(0, "artPath: Out of memory!");
    return;
  }
  artPath[0] = '\0';  
}

//Entry point called with current filepath
// Will search for art by trying to open likely files in same directory
void findAlbumArt(const char* path) {
  if(!artCurrentFile) return;
  strncpy(artCurrentFile, path, FTP_NAME_LENGTH - 1);  
  artCurrentFile[FTP_NAME_LENGTH - 1] = '\0';
  doFindArt = true;
}

//Entry point called with file to load
void loadAlbumArt(const char* path) {
  if(!artCurrentFile) return;
  strncpy(artCurrentFile, path, FTP_NAME_LENGTH - 1);  
  artCurrentFile[FTP_NAME_LENGTH - 1] = '\0';
  doLoadArt = true;
}

//Artwork touched
void artAction() {
  artFullscreen = !artFullscreen;
  doShowArt = true;
}

//Art state machine
void artHandle() {
#ifdef SDPLAYER  
  if (settings->mode != MODE_SD && WiFi.status() != WL_CONNECTED) return;    //Wait for WiFi if not SD
#else
  if (WiFi.status() != WL_CONNECTED) return;    //Wait for WiFi
#endif
  //State change delay
  if (artTimer && millis() < artTimer) return;
  artTimer = 0;
  //State machine
  if (artHandlerState == ART_IDLE) {
    //Action trigger flags
    if (doFindArt) {
      doFindArt = false;
      if (artFilePrepare())
        artHandlerState = ART_TRY_1;
    }
    else if (doShowArt) {
      doShowArt = false;
      artHandlerState = ART_DISPLAY;   
    }
    else if (doLoadArt) {
      doLoadArt = false;
      clearAlbumArt();  
      strncpy(artPath, artCurrentFile, FTP_NAME_LENGTH);
      artHandlerState = ART_OPEN;   
    }
  }
  //Art file attempts
  else if (artHandlerState == ART_TRY_1) {
    if (artFileOpen("/folder.jpg"))
      artHandlerState = ART_DOWNLOAD;
    else artHandlerState = ART_TRY_2;
  }
  else if (artHandlerState == ART_TRY_2) {
    if (artFileOpen("/Folder.jpg"))
      artHandlerState = ART_DOWNLOAD;
    else artHandlerState = ART_TRY_3;
  }
  else if (artHandlerState == ART_TRY_3) {
    if (artFileOpen("/cover.jpg"))
      artHandlerState = ART_DOWNLOAD;
    else artHandlerState = ART_TRY_4;
  }
  else if (artHandlerState == ART_TRY_4) {
    if (artFileOpen("/Cover.jpg"))
      artHandlerState = ART_DOWNLOAD;
    else artHandlerState = ART_TRY_5;
  }
  else if (artHandlerState == ART_TRY_5) {
    if (artFileOpen("/front.jpg"))
      artHandlerState = ART_DOWNLOAD;
    else artHandlerState = ART_TRY_6;
  }
  else if (artHandlerState == ART_TRY_6) {
    if (artFileOpen("/Front.jpg"))
      artHandlerState = ART_DOWNLOAD;
    else {
      Serial.println("None found.");
      artFileClose();
      artHandlerState = ART_IDLE;
    }
  }
  else if (artHandlerState == ART_OPEN) {
    if (artFileOpen(NULL))
      artHandlerState = ART_DOWNLOAD;
    else {
      artHandlerState = ART_IDLE;      
    }
  }
  //Found something, download
  else if (artHandlerState == ART_DOWNLOAD) {
    if (artFileDownload())    //Download finished?
      artHandlerState = ART_DISPLAY;
    else {
      //Progress bar
      static uint8_t oldpct = 0;
      uint8_t pct = ((jpgBufferIndex * 100) / jpgBufferSize);
      if (oldpct != pct) {
        oldpct = pct;
        setArtBar(pct);
      }
    }
  }
  //Got file, display it
  else if (artHandlerState == ART_DISPLAY) {
    setArtBar(0);
    showAlbumArt(artFullscreen);
    artHandlerState = ART_IDLE;
  }
}

//Clear buffers and state
void clearAlbumArt() {
  if (jpgBuffer) {
    free(jpgBuffer);
    jpgBuffer = NULL;
  }
  if (jpgImgBuffer) {
    free(jpgImgBuffer);
    jpgImgBuffer = NULL;
  }
  artFileClose();
  artHandlerState = ART_IDLE;
  artPath[0] = '\0';
  artPathPtr = NULL;
  updateArtImg("");
}

//State machine - Set up search path
bool artFilePrepare() {
  Serial.print("> Looking for album art: ");
  //directory part of new path same as directory part of current path?
  if (artPathPtr && strncmp(artPath, artCurrentFile, artPathPtr - artPath) == 0) {
    //Art already showing
    if (!jpgBuffer) Serial.println("Already found none.");
    else Serial.println("Already showing.");
    return false;
  }
  clearAlbumArt();
  //Copy the path over and set up the directory pointer  
  strncpy(artPath, artCurrentFile, FTP_NAME_LENGTH);
  artPathPtr = strrchr(artPath, '/');
  if (!artPathPtr) {
    errorContinue(0, "artFilePrepare: Path malformed!");
    return false;
  }
  //Start the long timer so the search does not begin until after music begins playing
  artTimer = millis() + ARTHANDLER_DELAYTIME;
  return true;  
}

//State machine - Try to open prospective file
bool artFileOpen(const char* file) {
  char* path = artCurrentFile; //If file is null, path is just the current file
  if (artPathPtr && file) {
    strcpy(artPathPtr, file);  //else copy filename to try to end of current directory
    path = artPath;            // and that is our path
  }
  artTimer = millis() + ARTHANDLER_FILECHECKTIME;  //Set the short timer so file checks are not too rapid
  //Try opening the path
  if (settings->mode == MODE_FTP)
    return ftpOpenAlbumArt(path);
#ifdef SDPLAYER
  else if (settings->mode == MODE_SD)
    return sdOpenAlbumArt(path);
#endif
  else if (settings->mode == MODE_DLNA)
    return httpOpenAlbumArt(path);
  return false;  
}

//State machine - Download file
bool artFileDownload() {
  if (settings->mode == MODE_FTP)
    return clientDownloadAlbumArt();
#ifdef SDPLAYER
  else if (settings->mode == MODE_SD)
    return sdDownloadAlbumArt();
#endif
  else if (settings->mode == MODE_DLNA)
    return clientDownloadAlbumArt();
  else return true;  
}

//State machine - Decode JPG file in memory into new bitmap
void showAlbumArt(bool fullscreen) {
  if (!jpgBuffer || !jpgBufferSize) return;
  uint16_t w = 0, h = 0, sw = 0, sh = 0;
  //Find size of image
  Serial.print("> Show Image ");
  if (fullscreen) Serial.print("fullscreen: ");
  else Serial.print("windowed: ");
  TJpgDec.getJpgSize(&w, &h, jpgBuffer, jpgBufferSize);
  if (w == 0 || h == 0) {
    clearAlbumArt();
    artError("Decode failed!");
    return;     //Problem with image  
  }
  Serial.printf("Image is %dx%d, ", w, h);
  //Set scale factor
  sw = w; 
  sh = h;
  if (fullscreen) {
    if (sw <= 480 && sh <= 280) TJpgDec.setJpgScale(1);
    else if (w <= 960 && h <= 560) { sw /= 2; sh /= 2; TJpgDec.setJpgScale(2); }
    else if (w <= 1980 && h <= 1120) { sw /= 4; sh /= 4; TJpgDec.setJpgScale(4); }
    else { sw /= 8; sh /= 8; TJpgDec.setJpgScale(8); }
  } else {
    if (sw <= 320 && sh <= 150) TJpgDec.setJpgScale(1);
    else if (w <= 640 && h <= 300) { sw /= 2; sh /= 2; TJpgDec.setJpgScale(2); }
    else if (w <= 1280 && h <= 600) { sw /= 4; sh /= 4; TJpgDec.setJpgScale(4); }
    else { sw /= 8; sh /= 8; TJpgDec.setJpgScale(8); }
  }
  Serial.printf("scaled to %dx%d\n", sw, sh);
  //Create an LVGL bitmap in PSRAM
  if (jpgImgBuffer) free(jpgImgBuffer);
  jpgImgBuffer = (uint16_t*)ps_malloc(sw * sh * 2);
  memset(jpgImgBuffer, 0, sw * sh * 2);  
  jpg_img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR,          //Set the color format
  jpg_img_dsc.header.reserved = 0,
  jpg_img_dsc.header.always_zero = 0,
  jpg_img_dsc.header.w = sw; 
  jpg_img_dsc.header.h = sh; 
  jpg_img_dsc.data_size = sw * sh * LV_COLOR_DEPTH / 8; 
  jpg_img_dsc.data = (uint8_t*)jpgImgBuffer;
  updateArtImg(&jpg_img_dsc, fullscreen);
  //Decode the JPG into the bitmap
  TJpgDec.setCallback(renderAlbumArt);
  TJpgDec.setSwapBytes(true);
  TJpgDec.drawJpg(0, 0, jpgBuffer, jpgBufferSize);
  updateArtImg(&jpg_img_dsc, fullscreen);
}

// This next function will be called during decoding of the jpeg file to
// render each block to the bitmap
bool renderAlbumArt(int16_t bx, int16_t by, uint16_t bw, uint16_t bh, uint16_t* bitmap) {
  static uint16_t oldy = 0;
  uint16_t* imgPtr = jpgImgBuffer;
  imgPtr += by * jpg_img_dsc.header.w + bx;
  for (int h = 0; h < bh; h++ ) {
    for (int w = 0; w < bw; w++ ) 
      *imgPtr++ = *bitmap++;
    imgPtr += (jpg_img_dsc.header.w - bw);
  }
  if (by != oldy) {
    oldy = by;
    updateArtImg(&jpg_img_dsc, artFullscreen);
  }
  return 1;
}

//-----------------------------------------------
// FS Specific art loaders
#define ART_BLOCK_SIZE 512

//FTP Album Art client
Client * art_client;
WiFiClient artcmd;
WiFiClient artdata;
FTPBasicAPI artftp;

//Don't reopen the server if it's not been closed yet
bool artFtpIsOpen = false;  //Big speedup from this simple optimisation

bool ftpOpenAlbumArt(char* path) {
  IPAddress addr = settings->ftpAddress;
  if (!artFtpIsOpen && !artftp.open(&artcmd, &artdata, addr, 21, 1000, settings->ftpUser, settings->ftpPass)) {
    artError("Open failed!");
    return false;
  }
  artFtpIsOpen = true;
  //Try get filesize first   
  jpgBufferSize = artftp.size(path);
  if (jpgBufferSize == 0) return false; //File not found
  char* ptr = strrchr(path, '/');
  if (ptr) {
    Serial.print("Downloading: ");
    Serial.println(ptr);
  }
  art_client = artftp.read(path);
  if (!art_client) {
    artError("No client!");
    return false;
  }
  //Create the JPG file buffer
  if (jpgBuffer) free(jpgBuffer);       //Shouldn't happen
  jpgBuffer = (uint8_t*)ps_malloc(jpgBufferSize);
  if (!jpgBuffer) {
    artftp.closeData();
    artError("Image too big!");
    return false;
  }
  artFileIsOpen = true;
  artTimer = 0;
  jpgBufferIndex = 0;
  return true;
}

HTTPClient httpArtClient;
bool httpOpenAlbumArt(char* url) {
  if (!artFileIsOpen && !httpArtClient.begin(url)) {
    artError("Open failed!");
    return false;
  }
  httpArtClient.setReuse(false);
  //Try get filesize first   
  int httpCode = httpArtClient.GET();
  if (httpCode != 200) {
    artError("Failed!");
    httpArtClient.end();
    return false;
  }
  jpgBufferSize = httpArtClient.getSize();
  if (jpgBufferSize == 0) {
    artError("Empty File!");
    httpArtClient.end();
    return false;
  }
  Serial.print("Downloading: ");
  Serial.println(url);
  art_client = httpArtClient.getStreamPtr();
  if (!art_client) {
    artError("No client!");
    httpArtClient.end();
    return false;
  }
  //Create the JPG file buffer
  if (jpgBuffer) free(jpgBuffer);       //Shouldn't happen
  jpgBuffer = (uint8_t*)ps_malloc(jpgBufferSize);
  if (!jpgBuffer) {
    httpArtClient.end();
    artError("Image too big!");
    return false;
  }
  artFileIsOpen = true;
  artTimer = 0;
  jpgBufferIndex = 0;
  return true;
}

//Both HTTP Client and FTP Client use this 
bool clientDownloadAlbumArt() {
  uint32_t read_num = 0;
  if (art_client->available() || art_client->connected()) {
    uint32_t readSize = ART_BLOCK_SIZE;
    if (jpgBufferIndex + readSize > jpgBufferSize)
      readSize = jpgBufferSize - jpgBufferIndex;
    read_num = art_client->read(&jpgBuffer[jpgBufferIndex], readSize);
    jpgBufferIndex += read_num;
    if (jpgBufferIndex >= jpgBufferSize) {
      artFileClose();
      return true;
    }
  } else {
    if (jpgBufferIndex != jpgBufferSize) {
      clearAlbumArt();
      artError("File truncated!");
    }
    artFileClose();
    return true;      
  }
  return false;
}

#ifdef SDPLAYER
//SD Album Art file object
lv_fs_file_t sdArtFile;

bool sdOpenAlbumArt(char* path) {
  //Try opening file
  if (lv_fs_open(&sdArtFile, path, LV_FS_MODE_RD) != LV_FS_RES_OK) return false;  //File not found
  char* ptr = strrchr(path, '/');
  if (ptr) {
    Serial.print("Reading: ");
    Serial.println(ptr);
  }
  //Get the filesize
  jpgBufferSize = lv_fs_size(&sdArtFile);
  if (jpgBufferSize == 0) {
    lv_fs_close(&sdArtFile);
    artError("Zero length file!");
    return false;
  }
  //Create the JPG file buffer
  if (jpgBuffer) free(jpgBuffer);       //Shouldn't happen
  jpgBuffer = (uint8_t*)ps_malloc(jpgBufferSize);
  if (!jpgBuffer) {
    lv_fs_close(&sdArtFile);
    artError("Image too big!");
    return false;
  }
  jpgBufferIndex = 0;
  artTimer = 0;
  artFileIsOpen = true;
  return true;
}


bool sdDownloadAlbumArt() {
  //Read in the file
  uint32_t read_num;
  if (lv_fs_read(&sdArtFile, &jpgBuffer[jpgBufferIndex], ART_BLOCK_SIZE, &read_num) != LV_FS_RES_OK) {
    clearAlbumArt();
    artError("Error reading album art!");
    artFileClose();
    return true;      
  }
  jpgBufferIndex += read_num;
  if (read_num != ART_BLOCK_SIZE) {
    if (jpgBufferIndex != jpgBufferSize) {
      clearAlbumArt();
      artError("File truncated!");
    }
    artFileClose();
    return true;      
  }
  return false;
}
#endif

//Close file if open
void artFileClose() {
  if (artFileIsOpen) {
    if (settings->mode == MODE_FTP) {
      const char* ok[] = {"221", "226", "426", nullptr};
      artftp.checkResult(ok, "close", false);
      artftp.closeData();
    }
#ifdef SDPLAYER
    else if (settings->mode == MODE_SD)
      lv_fs_close(&sdArtFile);
#endif
    else if (settings->mode == MODE_DLNA)
      httpArtClient.end();
    artFileIsOpen = false;
  }
  if (artFtpIsOpen) {
    //do something?
    artFtpIsOpen = false;
  }
}
