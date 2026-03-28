#include "decls.h"
//FTP handling routines
ftpFSFS ftpFS; 

//FTP Client
WiFiClient ftpcmd;
WiFiClient ftpdata;
FTPBasicAPI ftp;

//Client pointer
Client* ftp_client;

char * ftpCurrentDir;
char * ftpNameBuffer;
bool ftpPopulateFlag = false;
char * ftpSelectEntry = nullptr;

//Messages from ftpFS
QueueHandle_t ftpMsgQueue = NULL;


void ftpDirectoryEntry(char* fn);

//-------------------------------------------------------
//FTP Browser routines
// Uses the File Browser window

//Called from setup
void initFTP() {
  ftpNameBuffer = (char*)ps_malloc(FTP_NAME_LENGTH);
  ftpNameBuffer[0] = '\0';
  ftpCurrentDir = (char*)ps_malloc(FTP_NAME_LENGTH);
  strcpy(ftpCurrentDir, "/");  
  ftpMsgQueue = xQueueCreate(3, 64);
  // optional logging
  FTPLogger::setOutput(Serial);
  //FTPLogger::setLogLevel(LOG_DEBUG);
}

//Called from loop
void FTPHandle() {
  static int dataCount = 0;
  static int namePtr = 0;
  if (WiFi.status() == WL_CONNECTED) {
    //Read in an active directory listing
    if (ftp_client) {
      //Read data if client is connected
      if (ftp_client->connected() || ftp_client->available()) {
        //Accumulate lines of text, each one a directory entry
        while(ftp_client->available()) {
          char c = ftp_client->read();
          dataCount++;
          if (c == '\n') {
            //Got a line, call out to construct a browser button
            ftpNameBuffer[namePtr] = '\0';
            ftpDirectoryEntry(ftpNameBuffer);
            dataCount = 0;
            namePtr = 0;
          } else if (c != '\r') {   //ignore '\r'
            //put characters in line buffer, ignore most of the file info
            if (dataCount == 1 && c == 'd')
              ftpNameBuffer[namePtr++] = '/';   //leading slash for directories
            else if (dataCount > 56)
              ftpNameBuffer[namePtr++] = c;
          }
        }
      } else {
        //client exists but no longer connected, get ftp result and close server, null out client
        const char* ok_result[] = {"226", nullptr};
        ftp.checkResult(ok_result, "LIST", false);
        ftp.quit();
        ftp_client = NULL;
        showBrowserSpinner(false);
      }
    }
    //Start a listing - Wait for current listing to finish before starting another
    else if (settings->mode == MODE_FTP && ftpPopulateFlag) {
      ftpPopulateFlag = false;
      lv_obj_clean(browserMainList);    //Remove any lingering messages
      showBrowserSpinner(true);
      // open connection
      IPAddress addr = settings->ftpAddress;
      if (!ftp.open(&ftpcmd, &ftpdata, addr, 21, 1000, settings->ftpUser, settings->ftpPass)) {
        errorContinue(0, "FTP Open server Failed!");
        showBrowserSpinner(false);
      } else {
        //So far so good, get the listing
        ftp_client = ftp.dir(ftpCurrentDir);
        if (!ftp_client) errorContinue(0, "FTP 'ls' Command Failed!");
      }
    }
  }
  char msg[64];
  if(ftpMsgQueue && xQueueReceive(ftpMsgQueue, &msg, 0) == pdPASS) info(NOW, 0, msg);
}

//Trigger for ftpPopulateAction()
// Fills file browser with current directory
// selectEntry - if matched to a filename, scroll to that list entry.
void ftpPopulateBrowser(char *selectEntry) {
  setBrowserTitle(ftpCurrentDir[0] == 0?"/":ftpCurrentDir);
  lv_obj_clean(browserMainList);
  if (WiFi.status() != WL_CONNECTED) listWarning(browserMainList, "WiFi Not Connected");
  ftpPopulateFlag = true;
  ftpSelectEntry = selectEntry;
}

//Up a directory level
void ftpGoUp() {
  if(strlen(ftpCurrentDir) <= 1) { /* just forward slash */
    /* We cannot go up further */
    return;
  }
  char *last_slash = strrchr(ftpCurrentDir, '/');
  if (last_slash != NULL) {
    *last_slash = 0;
    ftpPopulateBrowser(last_slash + 1);
  }  
}

//Back to the root
void ftpGoHome() {
  strcpy(ftpCurrentDir, "/");
  ftpPopulateBrowser(NULL);
}

//File entry selected
static void ftpFileListButtonEvent(lv_event_t * event) {
  lv_obj_t * list_btn = lv_event_get_target(event);
  // Get some basic information about the file
  const char *filename = lv_list_get_btn_text(browserMainList, list_btn);
  bool is_dir = (bool)lv_obj_get_user_data(list_btn);
  // Only change to new directories
  if(is_dir) {
    uint32_t len = strnlen(ftpCurrentDir, FTP_NAME_LENGTH);
    if (len != 1) strncat(ftpCurrentDir, "/", FTP_NAME_LENGTH - len - 1);
    len++;
    strncat(ftpCurrentDir, filename, FTP_NAME_LENGTH - len - 1);
    ftpPopulateBrowser(NULL);  
  } else {
    //Play single file
    lv_obj_add_state(list_btn, LV_STATE_CHECKED);
    tabViewShowMain();
    char fname[FTP_NAME_LENGTH] = "";
    strncat(fname, ftpCurrentDir, FTP_NAME_LENGTH-1);
    uint32_t len = strnlen(fname, FTP_NAME_LENGTH-1);
    strncat(fname, "/", FTP_NAME_LENGTH - len - 1);
    strncat(fname, filename, FTP_NAME_LENGTH - len - 2);
    browserClearSelections();
    if (isJPGFile(fname)) loadAlbumArt(fname);
    else {
      setPlaylistIndex(0);
      playFile(fname);
    }
  }    
}

//Place a file/directory entry in the directory list
void ftpDirectoryEntry(char* fn) {
  bool is_dir = (fn[0] == '/');
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
  lv_obj_add_event_cb(list_btn, ftpFileListButtonEvent, LV_EVENT_SHORT_CLICKED, NULL);
  lv_obj_add_event_cb(list_btn, browserListMenu, LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_event_cb(list_btn, listMenuClicked, LV_EVENT_CLICKED, NULL);
  //Scroll to selected entry, if specified
  if (ftpSelectEntry && strcmp(&fn[is_dir], ftpSelectEntry) == 0)
    lv_obj_scroll_to_view(list_btn, LV_ANIM_OFF);
}

//Recursive directory tree traversal
// Builds a .m3u playlist from known audio files
// Each level of recursion a new FTPBasicAPI with both clients on stack
// Blocking, but calls out to update display
//  Can you believe this actually works? And does the job quite nicely..
void ftpPlaylistRecurseDirectory(lv_fs_file_t * f, char * orig_path) {
  int dataCount = 0;
  int namePtr = 0;
  //char fn[FTP_NAME_LENGTH];
  //char path[FTP_NAME_LENGTH];
  char *fn = (char*)ps_malloc(FTP_NAME_LENGTH); //[LV_FS_MAX_PATH_LENGTH];
  char *path = (char*)ps_malloc(FTP_NAME_LENGTH); //[LV_FS_MAX_PATH_LENGTH];
  if (!fn || !path) {
    errorContinue(0, "Out of memory in ftpPlaylistRecurseDirectory()");
    if (fn) free(fn);
    return;
  }
  //We have our own FTP client so recursion works
  WiFiClient dirClient;
  WiFiClient dirData;
  FTPBasicAPI dirFtp;
  Client* client;
  showFilename(orig_path);
  // open connection
  IPAddress addr = settings->ftpAddress;
  if (!dirFtp.open(&dirClient, &dirData, addr, 21, 1000, settings->ftpUser, settings->ftpPass)) {
    errorContinue(0, "FTP Playlist Open server Failed!");
    free(path);
    free(fn);
    return;
  }
  //So far so good, get the listing
  if (!(client = dirFtp.dir(orig_path))) { 
    errorContinue(0, "FTP Playlist 'ls' Command Failed!");
    free(path);
    free(fn);
    return;
  }
  //Wait for client to respond..
  while(client->connected() && !client->available()) delay(10);
  //Read the directory listing
  do {
    if (client->connected() || client->available()) {
      //Accumulate lines of text, each one a directory entry
      while(client->available()) {
        char c = client->read();
        dataCount++;
        if (c == '\n') {
          fn[namePtr] = '\0';
          strncpy(path, orig_path, FTP_NAME_LENGTH-1);
          path[FTP_NAME_LENGTH-1] = '\0';
          appendFileName(path, fn, FTP_NAME_LENGTH);
          if (fn[0] == '/') ftpPlaylistRecurseDirectory(f, path);    
          else addPlaylistEntry(f, path);              
          dataCount = 0;
          namePtr = 0;
        } else if (c != '\r') {   //ignore '\r'
          //put characters in line buffer, ignore most of the file info
          if (dataCount == 1 && c == 'd')
            fn[namePtr++] = '/';   //leading slash for directories
          else if (dataCount > 56)
            fn[namePtr++] = c;
        }
      }
    } else break;
  } while(1);
  free(path); 
  free(fn);
  const char* ok_result[] = {"226", nullptr};
  dirFtp.checkResult(ok_result, "LIST", false);
  dirFtp.quit();
}


///------------------------------------------------------------
/// FTP VFS Hack
/// Read only, tailored for Audio library use, just enough here to play files
/// - called from Audio thread

#include "FS.h"
#include "FSImpl.h"
using namespace fs;

void ftpMsg(const char* chr) {
  char msg[64];
  strncpy(msg, chr, 63);
  msg[63] = '\0';
  xQueueSend(ftpMsgQueue, &msg, 0);
}

bool ftpErrChk(bool not_ok, const char* msg) {
  if (not_ok) {
    char buf[80];
    snprintf(buf, 79, LV_SYMBOL_WARNING " FTP Error: %s", msg);
    ftpMsg(msg);
  }
  return not_ok;
}

class ftpFSFileImpl;

//FS Implementation
class ftpFSImpl : public FSImpl {
protected:
    friend class ftpFSFileImpl;
public:
    FileImplPtr open(const char* path, const char* mode, const bool create);
    bool        exists(const char* path) override;
    bool        rename(const char* pathFrom, const char* pathTo) override;
    bool        remove(const char* path) override;
    bool        mkdir(const char *path) override;
    bool        rmdir(const char *path) override;
};
ftpFSFS::ftpFSFS() : FS(std::shared_ptr<FSImpl>(new ftpFSImpl())) { }
ftpFSFS::~ftpFSFS() { }

//FS File implementation
class ftpFSFileImpl : public FileImpl {
protected:
    ftpFSImpl*          _fs;
    char *              _path;
    size_t              _size;
    size_t              _pos;
    FTPBasicAPI         _ftp;
    WiFiClient          _cmd;
    WiFiClient          _data;
    Client*             _client;
public:
    ftpFSFileImpl(ftpFSImpl* fs, const char* path, const char* mode);
    ~ftpFSFileImpl() override;
    size_t      write(const uint8_t *buf, size_t size) override;
    size_t      read(uint8_t* buf, size_t size) override;
    void        flush() override;
    bool        seek(uint32_t pos, SeekMode mode) override;
    size_t      position() const override;
    size_t      size() const override;
    bool        setBufferSize(size_t size);
    void        close() override;
    const char* name() const override;
    const char* path() const override;
    time_t      getLastWrite()  override;
    boolean     isDirectory(void) override;
    FileImplPtr openNextFile(const char* mode) override;
    boolean seekDir(long position) override;
    String getNextFileName(void) override;
    String getNextFileName(bool *isDir) override;    
    void        rewindDirectory(void) override;
    operator    bool();
};
//FS open() calls the file constructor
FileImplPtr ftpFSImpl::open(const char* path, const char* mode, const bool create) { return std::make_shared<ftpFSFileImpl>(this, path, mode); }
//Construct a VFS file object from an FTP path
ftpFSFileImpl::ftpFSFileImpl(ftpFSImpl* fs, const char* fpath, const char* mode)
    : _fs(fs)
    , _path(NULL)
    , _size(0)
    , _pos(0) 
    , _client(NULL) {
  if (ftpErrChk(!(_path = (char *)ps_malloc(strlen(fpath)+1)), "Out of memory")) return;
  strcpy(_path, fpath);
  ftpMsg("Connecting to FTP server..");
  IPAddress addr = settings->ftpAddress;
  if (ftpErrChk(!_ftp.open(&_cmd, &_data, addr, 21, 1000, settings->ftpUser, settings->ftpPass), "Failed to open server")) return;
  ftpMsg("Reading file..");
  if (ftpErrChk((_size = _ftp.size(_path)) == 0, "Failed to find file")) return;
  ftpErrChk((_client = _ftp.read(_path)) == NULL, "Failed to read file");
  //Wait for client to respond..
  ftpMsg("Waiting for server..");
  while(_client && _client->connected() && !_client->available()) delay(10);
  const char* ptr = strrchr(_path, '/');
  if (!ptr) ptr = _path; //no slash, use whole string
  ftpMsg(ptr);             //Clear the status line
}
//File destructor calls close
ftpFSFileImpl::~ftpFSFileImpl() { close(); }
//File close stops any transfer in progress, closes the client and quits out the server, then frees memory.
void ftpFSFileImpl::close() {
  const char* ok[] = {"221", "226", "426", nullptr};
  _ftp.checkResult(ok, "close", false);
  _ftp.closeData();
  _ftp.quit();
  if(_path) {
    free(_path);
    _path = NULL;
  }
  _client = NULL;
}
//Transfer from the ftp's data WiFiClient to the read buffer
size_t ftpFSFileImpl::read(uint8_t* buf, size_t size) { 
  int count = 0;
  if (!_client || (!_client->connected() && !_client->available()) || _pos >= _size) return -1;         //eof
  if ((count = _client->read(buf, size)) > 0) _pos += count;
  return count;
}
//misc FS and File functions
ftpFSFileImpl::operator bool() { return _client != NULL; }
bool ftpFSImpl::exists(const char* path) { return true; } //Assume yes
size_t ftpFSFileImpl::position() const { return _pos; }
size_t ftpFSFileImpl::size() const { return _size; }
const char* ftpFSFileImpl::name() const { return _path; }
const char* ftpFSFileImpl::path() const { return _path; }

//Ignoring and stubbing out all these unused FS and File functions
bool ftpFSFileImpl::seek(uint32_t pos, SeekMode mode) { Serial.println(">>> VFS SEEK Called! Implement it now!"); return 0; }
void ftpFSFileImpl::flush() { }
time_t ftpFSFileImpl::getLastWrite() { return 0; }
size_t ftpFSFileImpl::write(const uint8_t *buf, size_t size) { return 0; }
bool ftpFSImpl::rename(const char* pathFrom, const char* pathTo) { Serial.println(">>>VFS Rename!"); return true; }
bool ftpFSImpl::remove(const char* path) { Serial.println(">>>VFS Remove!"); return true; }
bool ftpFSImpl::mkdir(const char *path) { Serial.println(">>>VFS Mkdir!"); return true; }
bool ftpFSImpl::rmdir(const char *path) { Serial.println(">>>VFS Rmdir!"); return true; }
bool ftpFSFileImpl::setBufferSize(size_t size) { Serial.println(">>>VFS Setbuffersize!"); return 0; }
FileImplPtr ftpFSFileImpl::openNextFile(const char* mode) { return FileImplPtr(); }
boolean ftpFSFileImpl::seekDir(long position) { return false; }
String ftpFSFileImpl::getNextFileName(void) { return ""; }
String ftpFSFileImpl::getNextFileName(bool *isDir) { return ""; }    
void ftpFSFileImpl::rewindDirectory(void) { }
boolean ftpFSFileImpl::isDirectory(void) { return false; }
