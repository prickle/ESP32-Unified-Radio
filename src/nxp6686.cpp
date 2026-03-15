#include "decls.h"

#ifdef NXP6686
#include "TEF6686.h"

uint8_t isRDSReady;  
TEF6686 radio;
RdsInfo rdsInfo;
bool nxpI2SMode = false;
bool nxpFine = false;
uint32_t nxpI2SRate = 44100;
uint8_t nxpBand = FM1_BAND;

#define RDS_INTERVAL 80
unsigned long rdsTimer = millis() + RDS_INTERVAL;

// 0-FM1,       1-FM2,       2-FM3,       3-MW,      4-LW,     5-SW
// {6500,10800},{6500,10800},{6500,10800},{522,1710},{144,288},{2300,27000}
int nxpFreqTable[][9] = {
  { 88000, 108000, 100, 0, Radio_JUMPMODE , 25, 50, 88, 108},
  { 88000, 108000, 100, 0, Radio_JUMPMODE , 25, 50, 88, 108},
  { 88000, 108000, 100, 0, Radio_JUMPMODE , 25, 50, 88, 108},
  {   522,   1710,   9, 1, Radio_PRESETMODE, 35, 100, 52, 171 },
  {   144,    288,   1, 0, Radio_PRESETMODE, 40, 60, 144, 288 },
  {  2300,  27000,  10, 1, Radio_PRESETMODE, 35, 50, 2, 27}
};

const char* nxpAmBandWidthDsc = "3Khz\n4Khz\n6Khz\n8Khz";
int nxpAmBandWidthVal[] = { 30, 40, 60, 80 };

//56 / 64 / 72 / 84 / 97 / 114 / 133 / 151 / 168 / 184 / 200 / 217 / 236 / 254 / 287 / 311
const char* nxpFmBandWidthDsc = "Auto\n56Khz\n64Khz\n72Khz\n84Khz\n97Khz\n114Khz\n133Khz\n151Khz\n168Khz\n184Khz\n200Khz\n217Khz\n236Khz\n254Khz\n287Khz\n311Khz";
int nxpFmBandWidthVal[] = { 0, 560, 640, 720, 840, 970, 1140, 1330, 1510, 1680, 1840, 2000, 2170, 2360, 2540, 2870, 3110 };

const char* nxpAGCDsc = "Low\nMid\nHigh\nMax";

typedef struct nxpBandRange {
  int start;
  int end;
  const char* desc;
} nxpBandRange;

nxpBandRange nxpAMBandTable[] = {
  {621, 621, "ABC Radio National"},
  {693, 693, "3AW Nine Radio"},
  {774, 774, "774 ABC Melbourne"},
  {855, 855, "3CR Community radio"},
  {927, 927, "RSN Victorian racing industry"},
  {1026, 1026, "ABC NewsRadio"},
  {1116, 1116, "SEN 1116 Pacific Star Network Limited"},
  {1179, 1179, "3RPH Radio Print Handicapped Network"},
  {1224, 1224, "SBS Radio 1"},
  {1278, 1278, "Magic 1278"},
  {1377, 1377, "3MP Ace Radio"},
  {1422, 1422, "3XY Radio Hellas"},
  {1503, 1503, "3KND South Eastern Indigenous Media Association"},
  {1593, 1593, "Niche Radio Network Narrowcast multicultural radio"},
  {1611, 1611, "Vision Radio Network Narrowcast Christian Radio - Vermont"},
  {1629, 1629, "3CW 1629AM Program - Narrowcast Chinese language music service - Williamstown"},
  {1638, 1638, "2ME Arabic Radio Narrowcast Arabic language Radio"},
  {1656, 1656, "Rythmos 1656 Narrowcast Greek language Radio"},
  {1665, 1665, "Vision Radio Network Narrowcast Christian radio - Hoppers Crossing"},
  {1674, 1674, "Lion 1674/Surf City Sound"},
  {1701, 1701, "Islamic Voice Radio Narrowcast Islamic Radio"},
};
#define NXPAMBANDTABLE_NUM 21

nxpBandRange nxpSWBandTable[] = {
  {0, 0, ""},
  {2300, 2495, "120 Meters"},
  {3200, 3400, "90 Meters"},
  {3500, 3800, "80 Meters (Amateur)"},
  {3900, 4000, "75 Meters"},
  {4750, 4995, "60 Meters"},
  {5351, 5367, "60 Meters (Amateur)"},
  {5900, 6200, "49 Meters"},
  {7000, 7199, "40 Meters (Amateur)"},
  {7200, 7450, "41 Meters"},
  {9400, 9900, "31 Meters"},
  {10100, 10150, "30 Meters (Amateur)"},
  {11600, 12100, "25 Meters"},
  {13570, 13870, "22 Meters"},
  {14000, 14350, "20 Meters"},
  {15100, 15800, "19 Meters"},
  {17480, 17900, "16 Meters"},
  {18068, 18168, "17 Meters (Amateur)"},
  {18900, 19020, "15 Meters"},
  {18900, 19020, "15 Meters"},
  {21000, 21450, "15 Meters (Amateur)"},
  {24890, 24990, "12 Meters (Amateur)"},
  {25670, 26100, "11 Meters"},
};
#define NXPSWBANDTABLE_NUM 23
int nxpBandTableIndex = 0;

//char programTypePrevious[17] = "                ";
char programServicePrevious[9];
char radioTextPrevious[65];

bool NXPSearching = false;
bool NXPSearchUp = false;

#define NXPSCAN_IDLE  0
#define NXPSCAN_START  1
#define NXPSCAN_SETFRQ  2
#define NXPSCAN_CHECK  3
#define NXPSCAN_INCFRQ  4
#define NXPSCAN_DONE  5
uint8_t nxpScanState = NXPSCAN_IDLE;
int nxpScanOriginalFreq;

void initNXP() {
  radio.reset();
  delay(100);
  int result = radio.init(false);
  if (result == 0) log_i("Radio already started");
  else if (result == 1) log_i("Radio started OK");
  else {
    errorContinue(3, "NXP6686 Radio not found");
    return;
  }
  setVolume(settings->dabVolume);
  radio.seekSensHigh(false);
  scanAGCOptions(nxpAGCDsc);
}

void NXPShowFineTune() {
  if (nxpBand == FM1_BAND) return;
  if (nxpFine) setStmoLbl("<>");
  else setStmoLbl("");
}

void NXPUpdateInfo(int freq) {
  static int oldBand = -1;
  int band = 0;
  const char* desc = "";
  if (nxpBand == SW_BAND) {
    for (int n = 1; n < NXPSWBANDTABLE_NUM; n++) {
      if (freq >= nxpSWBandTable[n].start && freq <= nxpSWBandTable[n].end) {
        band = n;
        desc = nxpSWBandTable[band].desc;
        break;
      }
    }
  }
  else if (nxpBand == MW_BAND) {
    for (int n = 1; n < NXPAMBANDTABLE_NUM; n++) {
      if (freq >= nxpAMBandTable[n].start && freq <= nxpAMBandTable[n].end) {
        band = n;
        desc = nxpAMBandTable[band].desc;
        break;
      }
    }
  }
  if (band != oldBand) {
    oldBand = band;
    info(NAME, 0, desc);
  }
}

void NXPUpdateDisplay(int freq) {
  updateFMDisplay(freq);
  scanCursor(freq);
  NXPUpdateInfo(freq);
  //Clear FM RDS info on change of frequency
  if (nxpBand <= FM3_BAND) {
    info(NAME, 0, "");
    info(NOW, 0, "");
  }
}

void NXPSetNoiseBlanker(int val) {
  radio.setNoiseBlanker(val);
}

//range 0-21
void setVolume(uint8_t volume) {
  radio.setVolume(volume * 4 - 60);
}

void NXPSetBand(uint8_t band) {
  scanClear(nxpFreqTable[(nxpBand = band)]);
}

// 0-FM1,       1-FM2,       2-FM3,       3-LW,      4-MW,     5-SW
// {6500,10800},{6500,10800},{6500,10800},{522,1710},{144,288},{2300,27000}
void NXPSetFrequency(uint32_t frequency) {
  if (nxpScanState != NXPSCAN_IDLE) nxpScanOriginalFreq = frequency;
  else {
    radio.setFrequency(frequency, nxpBand);
    NXPUpdateDisplay(frequency);
  }
  rdsTimer = millis() + RDS_INTERVAL;
  programServicePrevious[0] = '\0';
  NXPShowFineTune();
}

void NXPSetBandwidth(int index) {
  bool aut = false;
  int bw = (nxpBand<=FM3_BAND)?nxpFmBandWidthVal[index]:nxpAmBandWidthVal[index];
  if (bw == 0) { //Automatic
    aut = true;
    bw = 236;
  }
  radio.setBandwidth(aut, bw);
}

void NXPSetAGC(int index) {
  radio.setRFAGC(index);
}

void NXPFineTune() {
  if (!nxpFreqTable[nxpBand][NXPTABLE_FINE]) return;
  nxpFine = !nxpFine;
  NXPShowFineTune();
}

#define NXP_DIAL_DELAY  500
//Fired when tuning dial changes
unsigned long NXPDialTimer = 0;
void NXPChangeFrequency(bool up) {
  if (nxpScanState != NXPSCAN_IDLE) return;
  int step = nxpFreqTable[nxpBand][NXPTABLE_STEP];
  if (nxpFine && nxpFreqTable[nxpBand][NXPTABLE_FINE]) step = 1;
  int freq = (radio.getFrequency() / step) * step + ((up * 2 - 1) * step);
  freq = constrain(freq, nxpFreqTable[nxpBand][NXPTABLE_FMIN], nxpFreqTable[nxpBand][NXPTABLE_FMAX]);
  if (nxpScanState != NXPSCAN_IDLE) nxpScanOriginalFreq = freq;
  else radio.setFrequency(nxpFreqTable[nxpBand][NXPTABLE_MODE], freq, nxpBand);
  NXPUpdateDisplay(freq);
  rdsTimer = millis() + RDS_INTERVAL;
  NXPDialTimer = millis() + NXP_DIAL_DELAY;
  programServicePrevious[0] = '\0';
}

void NXPStartFM() {
  radio.powerOn();
  radio.setInput(TEF_SETINPUT_RADIO);
  nxpI2SMode = false;
  NXPSetBand(FM1_BAND);
  scanBWOptions(nxpFmBandWidthDsc, settings->bwFM);
  scanAGCSelected(settings->agcFM);
  NXPSetFrequency(settings->dabFM);
  NXPSetBandwidth(settings->bwFM);
  NXPSetAGC(settings->agcFM);
  log_d("Set FM mode");
#ifdef FORCE_MONO
  radio.forceMono(true);
#endif
}

void NXPStartMW() {
  radio.powerOn();
  radio.setInput(TEF_SETINPUT_RADIO);
  nxpI2SMode = false;
  NXPSetBand(MW_BAND);
  scanBWOptions(nxpAmBandWidthDsc, settings->bwMW);
  scanAGCSelected(settings->agcMW);
  NXPSetFrequency(settings->freqMW);
  NXPSetBandwidth(settings->bwMW);
  NXPSetAGC(settings->agcMW);
  log_d("Set AM mode");
}

void NXPStartLW() {
  radio.powerOn();
  radio.setInput(TEF_SETINPUT_RADIO);
  nxpI2SMode = false;
  NXPSetBand(LW_BAND);
  scanBWOptions(nxpAmBandWidthDsc, settings->bwLW);
  scanAGCSelected(settings->agcLW);
  NXPSetFrequency(settings->freqLW);
  NXPSetBandwidth(settings->bwLW);
  NXPSetAGC(settings->agcLW);
  log_d("Set LW mode");
}

void NXPStartSW() {
  radio.powerOn();
  radio.setInput(TEF_SETINPUT_RADIO);
  nxpI2SMode = false;
  NXPSetBand(SW_BAND);
  scanBWOptions(nxpAmBandWidthDsc, settings->bwSW);
  scanAGCSelected(settings->agcSW);
  NXPSetFrequency(settings->freqSW);
  NXPSetBandwidth(settings->bwSW);
  NXPSetAGC(settings->agcSW);
  log_d("Set SW mode");
}

void NXPStartI2S() {
  //radio.powerOn();
  radio.setInput(TEF_SETINPUT_I2S);
  radio.setDigIO(TEF_SETDIGIO_SD0, TEF_SETDIGIO_INPUT, TEF_SETDIGIO_I2S32, TEF_SETDIGIO_SLAVE, nxpI2SRate / 10);
  nxpI2SMode = true;
  log_i("Start I2S: Sample rate %d", nxpI2SRate);
}

void NXPStop() {
  radio.powerOff();
  nxpI2SMode = false;
  nxpScanState = NXPSCAN_IDLE;
}

void NXPSetI2SRate(uint32_t rate) {
  if (nxpI2SMode && rate != nxpI2SRate) {
    //radio.powerOff();
    nxpI2SRate = rate;
    NXPStartI2S();
  }
}

void NXPSearch(uint8_t dir) {
  if (NXPSearching || nxpScanState != NXPSCAN_IDLE) return;
  log_i("Seek begin");
  NXPSearching = true;
  NXPSearchUp = dir;
}

void NXPSearchHandle() {
  if (NXPSearching) {
    if (radio.seekSync(NXPSearchUp)) {
      NXPSearching = false;
      log_i("> Seek stopped @ %d\r\n", radio.getFrequency());
      NXPDialTimer = millis(); //Save now
    }
    NXPUpdateDisplay(radio.getFrequency());
  }
}

void NXPScan() {
  if (nxpScanState == NXPSCAN_IDLE) nxpScanState = NXPSCAN_START;
  else nxpScanState = NXPSCAN_DONE;
}

// 0-FM1,       1-FM2,       2-FM3,       3-LW,      4-MW,     5-SW
// {6500,10800},{6500,10800},{6500,10800},{522,1710},{144,288},{2300,27000}
void NXPScanHandle() {
  static int nxpScanFreq;
  static int nxpScanStep;
  static unsigned long delayMs = 0;
  if (delayMs) {
    if (millis() < delayMs) return;
    delayMs = 0;
  }
  switch(nxpScanState) {
    case NXPSCAN_IDLE:
      break;
    case NXPSCAN_START:
      nxpScanOriginalFreq = radio.getFrequency();
      nxpScanFreq = nxpFreqTable[nxpBand][NXPTABLE_FMIN];
      nxpScanStep = nxpFreqTable[nxpBand][NXPTABLE_STEP];
      //scanClear(nxpFreqTable[nxpBand]);
      nxpScanState = NXPSCAN_SETFRQ;
      setSigStrengthLbl(LV_SYMBOL_WIFI " ..");
      delayMs = millis() + 10;
      break;
    case NXPSCAN_SETFRQ:
      radio.setFrequency(Radio_SEARCHMODE, nxpScanFreq, nxpBand);
      //updateFMDisplay(radio.getFrequency());
      NXPUpdateDisplay(radio.getFrequency());
      nxpScanState = NXPSCAN_CHECK;
      delayMs = millis() + (nxpBand <= FM3_BAND? 32 : 60); //Settling time      
      break;
    case NXPSCAN_CHECK:
      scanAdd(nxpScanFreq, radio.getLevel());
      nxpScanState = NXPSCAN_INCFRQ;
      break;
    case NXPSCAN_INCFRQ:
      nxpScanFreq = (nxpScanFreq / nxpScanStep) * nxpScanStep + nxpScanStep;
      if (nxpScanFreq >= nxpFreqTable[nxpBand][NXPTABLE_FMAX]) nxpScanState = NXPSCAN_DONE;
      else nxpScanState = NXPSCAN_SETFRQ;
      break;
    case NXPSCAN_DONE:
      radio.setFrequency(nxpScanOriginalFreq, nxpBand);
      NXPUpdateDisplay(radio.getFrequency());
      nxpScanState = NXPSCAN_IDLE;
      break;
  }
}


void RDS_ProgramType(char* type) {
  //strcpy(programTypePrevious, type);
  info(TEXT, 0, "%s", type);

}

void RDS_ProgramService(char* service) {
  //if ((strlen(service) == 8) && strncmp(service, programServicePrevious, 8) != 0) {
  //  if (programTypePrevious[0] != '\0')
  //    info(NOW, 0, "%s (%s)", service, programTypePrevious);
  //  else
      info(NOW, 0, "%s", service);
  //  strcpy(programServicePrevious, service);
  //}
}

void RDS_RadioText(char* text) {
  if (strncmp(text, radioTextPrevious, 65) != 0){
    String txt = String(text);
    txt.trim();
    info(NAME, 0, LV_SYMBOL_AUDIO " - %s - ", txt.c_str());
    strcpy(radioTextPrevious, text);
  }
}

void RDS_Time(uint16_t hour, uint16_t min, uint16_t day, uint16_t month, uint16_t year) {
  //Set RTC if not set
  if (!RTCSet) {
    serial.print("> Got time from RDS: ");
    setTime(0, min, hour, day, month, year);
    serial.println(getTime("%A, %B %d %Y %H:%M:%S"));
    RTCSet = true;
  }
}

void NXPUpdateLevel() {
  if (nxpScanState != NXPSCAN_IDLE) return;
  char s[25] = {0};
  int16_t lvl = radio.getLevel();
  setSignalMeter(constrain(lvl, 0, 100));
  scanAdd(radio.getFrequency(), lvl);
  snprintf(s, 24, LV_SYMBOL_WIFI " %d%%", lvl);
  setSigStrengthLbl(s);    
}

void NXPHandle() {
  if (settings->mode != MODE_NFM && settings->mode != MODE_NMW && settings->mode != MODE_NLW && settings->mode != MODE_NSW) return;
  //FM tuning dial movement ceased, save new frequency
  if (NXPDialTimer && NXPDialTimer < millis()) {
    NXPDialTimer = 0;
    if (settings->mode == MODE_NFM) settings->dabFM = radio.getFrequency();
    else if (settings->mode == MODE_NLW) settings->freqLW = radio.getFrequency();
    else if (settings->mode == MODE_NMW) settings->freqMW = radio.getFrequency();
    else if (settings->mode == MODE_NSW) settings->freqSW = radio.getFrequency();
    writeSettings();
  }

  if (rdsTimer <= millis()) {
    rdsTimer = millis() + RDS_INTERVAL;
    if (settings->mode == MODE_NFM) {
      uint16_t status;
      if (radio.checkRDS(&status)) radio.readRDS();
    }
    NXPUpdateLevel();
  }
  NXPSearchHandle();
  NXPScanHandle();
}

#endif