//ESP32 Radio
//Nick Metcalfe, 4/2024

//Look in here for hardware defs
#include "decls.h"

//SD Card SPI channel
SPIClass myspi(HSPI);

//My serial class that writes to the terminal as well as serial..
mySerial serial;

//Services task runs at higher priority than webradio task
// and handles network connections
TaskHandle_t servicesTaskHandle;
#define SRV_STACK_SIZE 8000 
void startServicesTask();

void setup(void)
{
  //first init..
  Serial.begin(115200);           //Get arduino serial device going
  resetScreen();                  //Toggle the common reset line now
  initTerminalLog();              //Start buffering mySerial calls
  disableCore0WDT();              //Core 0 is saturated during large searches, quiet! bad dog!

  //try to start up
  Serial.println("---------------------");
#ifndef BOARD_HAS_PSRAM
  errorHalt("*** WARNING - PSRAM Must be enabled!");
#endif  
  serial.println("> Radio v6.0 Nick Metcalfe 4/2024.");
  //Get the EEPROM going, we need settings early in the startup process
  initSettings();
  setupModes();                   //Construct mode lists

  // Setup the LCD
  initScreen();
  setupInfoLabels();  //allocate buffers for info label stack

  //Startup message
  serial.println("> LittleVGL GUI Started.");    
  terminalHandle();       //need to call this after every print when in setup

#if defined(BATTERYMON)
  //Scan I2C and find devices
  i2cScan();
#endif

  //Start internal filesystem
  initSPIFFS();

#ifdef SDPLAYER
  //Find and start SD card
  initSDCard();
#endif

  //FTPFileSystemInit();
#ifdef FFTMETER
  initFFT();
#endif

  //Set up GUI objects
  initBrowser();
  initPlayer();
  initAlbumArt();

  //Set up the rest of the radio
#if defined(TWINSCROLL) || defined(PANASCROLL)
  prepareEncoders();
#endif
#ifdef MINIBUTTONS
  prepareButtons();
#endif
#ifdef MONKEYBOARD
  initDab();
#endif
#ifdef NXP6686
  initNXP();
#endif
  setupWifi();
  initNTP();
  initFTP();
  initPodcast();
  DLNASetup();
#ifdef BATTERYMON
  ADCInit();
#endif
  webradioSetup();
  setupWeather();
  setRadioMode(settings->mode);
  startServicesTask();

  //All done, get ready for loop()
  if (!startupError) serial.println("> Setup OK - entering main loop.");
  else serial.println(LV_SYMBOL_WARNING " Error during setup, continuing..");
  terminalHandle();
  serial.InSetup = false;   //Turn off mySerial pumping
  screenSaverInteraction();
#ifdef BATTERYMON
  resetAmpHours();
#endif
}

//Just call the handlers for each feature
void loop() {
  lv_task_handler(); /* let the GUI do its work */  
#ifdef MONITOR_PORT
  wifiTerminalHandle();
#endif
  terminalHandle();
  screenSaverHandle();
  wifiHandle();
  webradioHandle();
  NTPHandle();
  FTPHandle();
#ifdef BATTERYMON
  ADCHandle();
#endif
  timeHandle();
  searchHandle();
  podcastHandle();
  artHandle();
#ifdef VUMETER
  VUHandle();
#endif
  passwordHandle();
  weatherHandle();
  infoLabelHandle();
  DLNAHandle();
#ifdef USE_OTA  
  ArduinoOTA.handle();
#endif
#ifdef MONKEYBOARD
  dabHandle();
#endif
#if defined(TWINSCROLL) || defined(PANASCROLL)
  encodersHandle();
#endif
#ifdef MINIBUTTONS
  buttonsHandle();
#endif
  weatherClientHandle();
#ifdef NXP6686
  NXPHandle();
#endif
}

//Network services
// This task runs at higher priority than the audio task
// Heavy stuff happening here will cause audio stutter
// just service the network clients else they are too slow and stall up
void servicesTask( void * pvParameters ) {
  for(;;) {
    bool active = false;
    DLNAClientHandle();   //Blocking
    if (podClientHandle()) active = true;
    if (searchClientHandle()) active = true;
    //if (weatherClientHandle()) active = true;
    if (active) taskYIELD();
    else vTaskDelay(1);
  }
}

//Start services task
void startServicesTask() {
  xTaskCreatePinnedToCore(
             servicesTask,         // Task function. 
             "NetServices",          // name of task. 
             SRV_STACK_SIZE,               // Stack size of task 
             NULL,                // parameter of the task 
             17,                   // priority of the task 
             &servicesTaskHandle,    // Task handle to keep track of created task 
             0);                  // pin task to core 0
}
