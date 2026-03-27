#include "decls.h"

#ifdef USING_TFTESPI
TFT_eSPI tft = TFT_eSPI();
#else
static LGFX tft;            // declare display variable
#endif

#define LVGL_TICK_PERIOD 5
Ticker tick; /* timer for interrupt handler */

//LVGL display object
static lv_disp_t * display;
static lv_disp_draw_buf_t disp_buf;

//LVGL driver objects
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;
static lv_fs_drv_t spiffs_drv; 
#ifdef SDPLAYER
static lv_fs_drv_t sd_drv; 
#endif
bool touched = false; //Touchscreen currently touched
bool ScreenSaverActive = false;

void LVdispFlush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p );
static void LVtickHandler(void);
void LVinputRead(lv_indev_drv_t * drv, lv_indev_data_t*data);
#if LV_USE_LOG != 0
void LVlogPrint(const char * buf);
#endif

//Display driver
void initLVGL()  {
  //Hook up LVGL
  lv_init();
#if LV_USE_LOG != 0
  lv_log_register_print_cb(LVlogPrint); /* register print function for debugging */
#endif
#ifdef USE_DMA
  tft.initDMA();
  lv_color_t * buf1 = (lv_color_t *)heap_caps_malloc(tft.width() * (LVGL_BUFF_SIZE / 2) * sizeof(lv_color_t),MALLOC_CAP_DMA);
  lv_color_t * buf2 = (lv_color_t *)heap_caps_malloc(tft.width() * (LVGL_BUFF_SIZE / 2) * sizeof(lv_color_t),MALLOC_CAP_DMA);
  lv_disp_draw_buf_init(&disp_buf, buf1, buf2, tft.width() * (LVGL_BUFF_SIZE / 2));    
#else
  lv_color_t * buf1 = (lv_color_t *)heap_caps_malloc(tft.width() * LVGL_BUFF_SIZE * sizeof(lv_color_t),MALLOC_CAP_DMA);
  lv_disp_draw_buf_init(&disp_buf, buf1, NULL, tft.width() * LVGL_BUFF_SIZE);
#endif

  /*Initialize the display*/
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = tft.width();
  disp_drv.ver_res = tft.height();
  disp_drv.flush_cb = LVdispFlush;
  disp_drv.draw_buf = &disp_buf;
  display = lv_disp_drv_register(&disp_drv);
  //Initialize the graphics library's tick handler
  tick.attach_ms(LVGL_TICK_PERIOD, LVtickHandler);
  //Set up LVGL device drivers
  inputDriverInit();
}

void resetScreen() {
  //Toggle the common reset line
#ifdef TFT_ILI9341  
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, LOW);
  delay(5);
  digitalWrite(TFT_RST, HIGH);
#endif  
}

//TODO: brightness control
void initScreen() {
  // Setup the LCD
  tft.init();
  tft.setRotation(TFT_ROTATION);
  tft.fillScreen(TFT_BLACK); 
  setBrightness(settings->brightness);
  //Start LVGL
  initLVGL();
  screenInit();
}

void setBrightness(uint8_t bright) {
#ifdef USING_TFTESPI   
#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, bright << 2);
#endif
#else
  tft.setBrightness(settings->brightness);
#endif
}

//*** LVGL : Setup & Initialize the input device driver ***
void inputDriverInit() {
  serial.print("> Start Touch: ");
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = LVinputRead;
  indev_drv.long_press_time = 1500;
  lv_indev_drv_register(&indev_drv);
  serial.println("Driver OK.");  
}

/* Interrupt driven periodic handler */
static void LVtickHandler(void) {
  lv_tick_inc(LVGL_TICK_PERIOD);
}

//Logging from LVGL
#if LV_USE_LOG != 0
/* Serial debugging */
void LVlogPrint(const char * buf) {
  serial.printf("> %s", buf);
}
#endif

//void ets_install

void refreshDisplay() {
  lv_refr_now(display);
}

 /* Display flushing */
void LVdispFlush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p ) {
  if (!ScreenSaverActive) {
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );
#ifdef USE_DMA    
    if (tft.getStartCount() == 0) tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushPixelsDMA( ( uint16_t * )&color_p->full, w * h);
#else
    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );    
    tft.pushPixels( ( uint16_t * )&color_p->full, w * h);//, true );
    tft.endWrite();
#endif
  }
//#ifdef USE_DMA    
  //if (ScreenSaverActive)
//#endif
  lv_disp_flush_ready( disp );
}

// This function will be called during decoding of the jpeg file to
// render each block directly to the TFT fullscreen
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  tft.startWrite();
  tft.setAddrWindow( x, y, w, h );
#if LV_COLOR_16_SWAP == 0
  /*Pixel format: Red: 5 bit, Green: 6 bit, Blue: 5 bit*/
  tft.pushPixels(bitmap, w * h);
#else
  /*Pixel format: Red: 5 bit, Green: 6 bit, Blue: 5 bit BUT the 2 bytes are swapped*/
#ifdef USING_TFTESPI
  tft.pushColors(bitmap, w * h, true);
#else
  tft.pushPixels(bitmap, w * h, true);
#endif
#endif
  tft.endWrite();
  return 1;
}

#ifdef FUNKYTOUCH
// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 320
#define TS_MINY 220
#define TS_MAXX 3840
#define TS_MAXY 3670
//My touchscreen is funky and needs extra conditioning
void LVinputRead(lv_indev_drv_t * drv, lv_indev_data_t*data) {
  int16_t rx = 0, ry = 0;
  static int16_t x, y;
  static bool touched = false;
  bool rz = tft.getTouchRaw(&rx, &ry);
  //Serial.printf("x:%i, y:%i, z:%d \n", rx, ry, rz);
  data->state = rz? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
  if (rz) screenSaverInteraction();
  // Scale from ~0->4000 to tft.width using the calibration #'s
  int px = map(ry, TS_MINY, TS_MAXY, 0, tft.width());
  int py = map(rx, TS_MINX, TS_MAXX, 0, tft.height());
  px = constrain(px, 0, tft.width());
  py = constrain(py, 0, tft.height());
  if (!rz && touched) {
    data->point.x = x;
    data->point.y = y;
  } else {
    data->point.x = px;
    data->point.y = py;
  }
  x = px;
  y = py;
  touched = rz;
}

#elif defined(TOUCH_VOLUME)
//My touchscreen is noisy and needs extra conditioning
//that's an understatement, now using an arduino to read the touch, see "touch" 
void LVinputRead(lv_indev_drv_t * drv, lv_indev_data_t*data)
{
  //uint16_t rx, ry;
  int16_t x, y;
  static int16_t lx = 0, ly = 0;
  static bool touched = false;
  data->state = tz? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
  if (tz) screenSaverInteraction();
  // Scale from ~0->4000 to tft.width using the calibration #'s
  x = map(tx, 0, 1024, 0, tft.width() - 1);
  y = map(ty, 0, 1024, 0, tft.height() - 1);
  if (!tz && touched) {
    data->point.x = lx;
    data->point.y = ly;
  } else {
    data->point.x = x;
    data->point.y = y;
  }
  lx = x;
  ly = y;
  touched = tz;
}

#else
void LVinputRead(lv_indev_drv_t * drv, lv_indev_data_t*data) {
  static int16_t lx = 0, ly = 0;
  touched = tft.getTouch(&lx, &ly);
//  touched = tft.getTouchRaw(&lx, &ly);
  if (touched) {
    screenSaverInteraction();
    data->state = LV_INDEV_STATE_PR;
    data->point.x = lx;
    data->point.y = ly;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}
#endif

//Filesystem error reporter
bool fs_err(lv_fs_res_t err, const char * msg) {
  char buf[64];
  if (err == LV_FS_RES_OK) return false;
  else if (err == LV_FS_RES_INV_PARAM) snprintf(buf, 63, "Invalid Parameter");
  else if (err == LV_FS_RES_NOT_EX) snprintf(buf, 63, "Drive doesn't exist");
  else if (err == LV_FS_RES_HW_ERR) snprintf(buf, 63, "Hardware error");
  else if (err == LV_FS_RES_NOT_IMP) snprintf(buf, 63, "Not implemented");
  else if (err == 12) snprintf(buf, 63, "File Not Found");
  else snprintf(buf, 63, "Unknown error: %d", err);
  info(NOW, 0, LV_SYMBOL_WARNING " Error: %s - %s ", msg, buf);  
  serial.print("! ");
  serial.print(msg);
  serial.print(": ");
  serial.println(buf);
  hideWebControls();
  showPodcastSpinner(false);
  showBrowserSpinner(false);
  return true;
}

//bodge..easier migration from lvgl 6 to 8
void lv_obj_set_hidden(lv_obj_t * obj, bool isHidden) {
  if (obj == NULL) return;
  if (isHidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

//Fully update LVGL screen while loop is not running
// Used by mySerial
void pumpLvgl() {
  uint64_t timer = millis();
  while(millis() < timer + 20) {
    delay(1);
    lv_task_handler();  
  }
}

//Can be removed in later versions of lvgl
void lv_obj_swap(lv_obj_t * obj1, lv_obj_t * obj2) {
  lv_obj_t * parent = lv_obj_get_parent(obj1);
  lv_obj_t * parent2 = lv_obj_get_parent(obj2);
  uint_fast32_t index1 = lv_obj_get_child_id(obj1);
  uint_fast32_t index2 = lv_obj_get_child_id(obj2);
  parent->spec_attr->children[index1] = obj2;
  parent2->spec_attr->children[index2] = obj1;
  lv_event_send(parent, LV_EVENT_CHILD_CHANGED, obj2);
  lv_event_send(parent2, LV_EVENT_CHILD_CHANGED, obj1);
  lv_obj_invalidate(parent);
  if( parent != parent2) {
    lv_obj_invalidate(parent2);
  }
}

//Color conversion
lv_color_t lv_col(uint16_t color) {
  lv_color16_t col;
  col.full = color;
  return col;
}


//----------------------------------------------------
// SPIFFS filesystem - drive "C"

void initSPIFFS() {
  serial.print("> Start Internal Filesystem: ");
  if (!SPIFFS.begin()) {
    serial.println("Uninitialised. \n> Formatting SPIFFS, please wait..");
    terminalHandle();
    SPIFFS.format();
    serial.print("> Format Complete, ");
  }
  SPIFFSfileSystemInit();
  terminalHandle();
}

static void* spiffs_open(lv_fs_drv_t *drv, const char * fn, lv_fs_mode_t mode) {
  const char* flags;
  if(mode == LV_FS_MODE_RD) flags = FILE_READ;
  else flags = FILE_WRITE;
  if (mode != LV_FS_MODE_RD || SPIFFS.exists(fn)) {
    File * f = new File(SPIFFS.open(fn, flags));
    return (void*)f;
  }
  return NULL;
}

static lv_fs_res_t spiffs_close(lv_fs_drv_t *drv, void * file_p) {
  ((File*)file_p)->close();
  delete(((File*)file_p));
  return LV_FS_RES_OK;
}

static lv_fs_res_t spiffs_read(lv_fs_drv_t *drv, void * file_p, void * buf, uint32_t btr, uint32_t * br) {
  *br = ((File*)file_p)->read((uint8_t*)buf, btr);
  return LV_FS_RES_OK;
}

static lv_fs_res_t spiffs_write(lv_fs_drv_t *drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw) {
  *bw = ((File*)file_p)->write((uint8_t*)buf, btw);
  return LV_FS_RES_OK;
}

static lv_fs_res_t spiffs_seek(lv_fs_drv_t *drv, void * file_p, uint32_t pos, lv_fs_whence_t whence) {
  if (whence == LV_FS_SEEK_SET) ((File*)file_p)->seek(pos);
  else if (whence == LV_FS_SEEK_CUR) ((File*)file_p)->seek(pos + ((File*)file_p)->position());
  else if (whence == LV_FS_SEEK_END) ((File*)file_p)->seek(pos + ((File*)file_p)->size());
  else return LV_FS_RES_INV_PARAM;
  return LV_FS_RES_OK;
}

static lv_fs_res_t spiffs_tell(lv_fs_drv_t *drv, void * file_p, uint32_t * pos_p) {
  *pos_p = ((File*)file_p)->position();
  return LV_FS_RES_OK;
}

// File system driver
void SPIFFSfileSystemInit() {
  lv_fs_drv_init(&spiffs_drv);                     /*Basic initialization*/
  spiffs_drv.letter = 'C';                         /*An uppercase letter to identify the drive */
  spiffs_drv.open_cb = spiffs_open;                 /*Callback to open a file */
  spiffs_drv.close_cb = spiffs_close;               /*Callback to close a file */
  spiffs_drv.read_cb = spiffs_read;                 /*Callback to read a file */
  spiffs_drv.write_cb = spiffs_write;                 /*Callback to read a file */
  spiffs_drv.seek_cb = spiffs_seek;                 /*Callback to seek in a file (Move cursor) */
  spiffs_drv.tell_cb = spiffs_tell;                 /*Callback to tell the cursor position  */  
  lv_fs_drv_register(&spiffs_drv);                 /*Finally register the drive*/  
  serial.println("Driver OK.");
}


#ifdef SDPLAYER
//---------------------------------------------------
// SD Card Filesystem - drive "D"

void initSDCard() {
  serial.print("> Start SD card: ");
  myspi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!(SDFound = SD.begin(SD_CS, myspi, 40000000))) {
    serial.println("Failed! Card not inserted?");
  } else {
    serial.print("OK, ");
    SDfileSystemInit();
    setSdcardLbl(LV_SYMBOL_SD_CARD);
  }
  terminalHandle();
}

static void* sd_open(lv_fs_drv_t *drv, const char * fn, lv_fs_mode_t mode) {
    const char* flags;
    if(mode == LV_FS_MODE_RD) flags = FILE_READ;
    else flags = FILE_WRITE;
    if (mode != LV_FS_MODE_RD || SD.exists(fn)) {
      File *f = new File(SD.open(fn, flags));
      return (void*)f;
    }
    return NULL;
}

//Hack to get size of file
uint32_t lv_fs_size(lv_fs_file_t * file) {
//  if (file->drv->letter == 'D')
    return ((File*)(file->file_d))->size();
//  else if (file->drv->letter == 'E')
//    return ((ftpFile*)(file->file_d))->size;
}

static lv_fs_res_t sd_close(lv_fs_drv_t *drv, void * file_p) {
    ((File*)file_p)->close();
    delete(((File*)file_p));
    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_read(lv_fs_drv_t *drv, void * file_p, void * buf, uint32_t btr, uint32_t * br) {
    *br = ((File*)file_p)->read((uint8_t*)buf, btr);
    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_write(lv_fs_drv_t * drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw) {
    *bw = ((File*)file_p)->write((uint8_t*)buf, btw);
    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_seek(lv_fs_drv_t *drv, void * file_p, uint32_t pos, lv_fs_whence_t whence) {
    if (whence == LV_FS_SEEK_SET) ((File*)file_p)->seek(pos);
    else if (whence == LV_FS_SEEK_CUR) ((File*)file_p)->seek(pos + ((File*)file_p)->position());
    else if (whence == LV_FS_SEEK_END) ((File*)file_p)->seek(pos + ((File*)file_p)->size());
    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_tell(lv_fs_drv_t *drv, void * file_p, uint32_t * pos_p) {
    *pos_p = ((File*)file_p)->position();
    return LV_FS_RES_OK;
}

//Directories
static void * sd_dir_open (lv_fs_drv_t * drv, const char *path) {
  if (strlen(path) == 0) path = "/";
  File *f = new File(SD.open(path));
  return (void*)f;
}

static lv_fs_res_t sd_dir_read (lv_fs_drv_t * drv, void * dir_p, char *fn) {
  File entry =  ((File*)dir_p)->openNextFile();
  if (!entry) strcpy(fn, "");
  else {
    char * str = fn;
    const char * name = entry.name();
    const char * slsh = strrchr(name, '/'); 
    if (slsh) name = slsh+1;
    if (entry.isDirectory()) *str++ = '/';
    strcpy(str, name);
    entry.close(); 
  }
  return LV_FS_RES_OK;
}

static lv_fs_res_t sd_dir_close (lv_fs_drv_t * drv, void * dir_p) {
  ((File*)dir_p)->close();
  delete(((File*)dir_p)); 
  return LV_FS_RES_OK;
}

void SDfileSystemInit() {
  lv_fs_drv_init(&sd_drv);                     /*Basic initialization*/  
  sd_drv.letter = 'D';                         /*An uppercase letter to identify the drive */
  sd_drv.open_cb = sd_open;                 /*Callback to open a file */
  sd_drv.close_cb = sd_close;               /*Callback to close a file */
  sd_drv.read_cb = sd_read;                 /*Callback to read a file */
  sd_drv.write_cb = sd_write;               /*Callback to write a file */
  sd_drv.seek_cb = sd_seek;                 /*Callback to seek in a file (Move cursor) */
  sd_drv.tell_cb = sd_tell;                 /*Callback to tell the cursor position  */  
  sd_drv.dir_close_cb = sd_dir_close;
  sd_drv.dir_open_cb = sd_dir_open;
  sd_drv.dir_read_cb = sd_dir_read;
  lv_fs_drv_register(&sd_drv);                 /*Finally register the drive*/
  serial.println("Driver installed.");
}

#endif
//--------------------------------------------------
//Screen Saver handler
unsigned long ScreenSaverTimer;
bool ScreenSaverPending = false;
bool wasTouched = false;

void screenSaverHandle() {
  if (!ScreenSaverPending) {
    if (millis() > ScreenSaverTimer) {
      ScreenSaverPending = true;
      int debugWindowIndex = mainWindowIndex + 1;
      if (mainWindowIndex > 0) debugWindowIndex++;
      if (activeTab() < debugWindowIndex) {
        tabViewShowMain();              
      }
    }
  } else if (ScreenSaverActive) {
    static long lastms = 0;
    if (millis() > lastms) {
      lastms = millis() + 100;
      if (!touched && wasTouched) 
        screenSaverInteraction();
      wasTouched = touched;
    }
  }
}

void screenSaverInteraction() {
  if (ScreenSaverPending) {
    ScreenSaverPending = false;
    if (ScreenSaverActive) {
      ScreenSaverActive = false;
      wasTouched = false;
      lv_obj_invalidate(lv_scr_act());
    }
  }
  ScreenSaverTimer = millis() + SCREENSAVER_TIMEOUT;  
}


