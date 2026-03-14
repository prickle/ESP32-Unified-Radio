
#include "decls.h"
//Experimental - read only - not used

//---------------------------------------------------
// LVGL FTP Filesystem - drive "E"
/*
static lv_fs_drv_t ftpfs_drv; 

void FTPFileSystemInit() {
  lv_fs_drv_init(&ftpfs_drv);                     //Basic initialization  
  ftpfs_drv.letter = 'E';                         //An uppercase letter to identify the drive
  ftpfs_drv.open_cb = ftpfs_open;                 //Callback to open a file 
  ftpfs_drv.close_cb = ftpfs_close;               //Callback to close a file 
  ftpfs_drv.read_cb = ftpfs_read;                 //Callback to read a file 
  ftpfs_drv.tell_cb = ftpfs_tell;                 //Callback to tell the cursor position    
  ftpfs_drv.dir_close_cb = ftpfs_dir_close;
  ftpfs_drv.dir_open_cb = ftpfs_dir_open;
  ftpfs_drv.dir_read_cb = ftpfs_dir_read;
  lv_fs_drv_register(&ftpfs_drv);                 //Finally register the drive
  serial.println("Driver installed.");
}


struct ftpFile {
  size_t              size;
  size_t              pos;
  WiFiClient          cmd;
  WiFiClient          data;
  FTPBasicAPI         ftp;  
  bool                isDir;
  ftpFile(): size(0), pos(0), isDir(false) {}
  bool open(const char * p) { 
    IPAddress addr = settings->ftpAddress;  
    return ftp.open(&cmd, &data, addr, 21, 1000, settings->ftpUser, settings->ftpPass);
  }
  bool read(const char * p) {
    if (!(size = ftp.size(p))) return false;  //No or empty file
    if (!(ftp.read(p))) return false;
    while(data.connected() && !data.available()) delay(10);
    return true;
  }
  bool dir(const char * p) {
    if (!(ftp.dir(p))) return false;
    while(data.connected() && !data.available()) delay(10);
    isDir = true;
    return true;
  }
};

static void* ftpfs_open(lv_fs_drv_t *drv, const char * fn, lv_fs_mode_t mode) {
  const char* flags;
  if(mode == LV_FS_MODE_RD) flags = FILE_READ;
  else return NULL;   //No writing yet
  ftpFile* f = new ftpFile(); 
  if (!f->open(fn)) return NULL;
  if (!f->read(fn)) return NULL;
  return (void*)f;
}

uint32_t lv_ftpfs_size(lv_fs_file_t * file) {
  return ((ftpFile*)(file->file_d))->size;
}

static lv_fs_res_t ftpfs_close(lv_fs_drv_t *drv, void * file_p) {
  ftpFile* f = ((ftpFile*)file_p);
  const char* ok[] = {"221", "226", "426", nullptr};
  f->ftp.checkResult(ok, "close", false);
  f->ftp.closeData();
  f->ftp.quit();
  delete(f);
  return LV_FS_RES_OK;
}

static lv_fs_res_t ftpfs_read(lv_fs_drv_t *drv, void * file_p, void * buf, uint32_t btr, uint32_t * br) {
  ftpFile * f = ((ftpFile*)file_p);
  if ((!f->data.connected() && !f->data.available()) || f->pos >= f->size) *br = -1;         //eof
  else {
    *br = f->data.read((uint8_t*)buf, btr);
    f->pos += *br;
  }
  return LV_FS_RES_OK;
}

static lv_fs_res_t ftpfs_tell(lv_fs_drv_t *drv, void * file_p, uint32_t * pos_p) {
    *pos_p = ((ftpFile*)file_p)->pos;
    return LV_FS_RES_OK;
}

//Directories
static void * ftpfs_dir_open (lv_fs_drv_t * drv, const char *path) {
  ftpFile* f = new ftpFile(); 
  if (!f->open(path)) return NULL;
  if (!f->dir(path)) return NULL;
  Serial.println("Open Directory");
  return (void*)f;
}

static lv_fs_res_t ftpfs_dir_read (lv_fs_drv_t * drv, void * dir_p, char *fn) {
  ftpFile * f = ((ftpFile*)dir_p);
  int dataCount = 0;
  int namePtr = 0;
  while (f->data.connected() || f->data.available())  {
    if(f->data.available()) {
      char c = f->data.read();
      dataCount++;
      if (c == '\n') break;
      else if (c != '\r') {   //ignore '\r'
        //put characters in line buffer, ignore most of the file info
        if (dataCount == 1 && c == 'd') fn[namePtr++] = '/';   //leading slash for directories
        else if (dataCount > 56) fn[namePtr++] = c;
      }
    }
  }
  fn[namePtr] = '\0';
  return LV_FS_RES_OK;
}

static lv_fs_res_t ftpfs_dir_close (lv_fs_drv_t * drv, void * dir_p) {
  ftpFile* f = ((ftpFile*)dir_p);
  const char* ok[] = {"226", nullptr};
  f->ftp.checkResult(ok, "dir", false);
  f->ftp.closeData();
  f->ftp.quit();
  delete(f);
  return LV_FS_RES_OK;
}

*/