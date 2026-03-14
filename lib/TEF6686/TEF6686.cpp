#include "TEF6686.h"

const char* ptyLUT[51] = { 
      "",
      "News",
      "Information",
      "Sports",
      "Talk",
      "Rock",
      "Classic Rock",
      "Adult Hits",
      "Soft Rock",
      "Top 40",
      "Country",
      "Oldies",
      "Soft",
      "Nostalgia",
      "Jazz",
      "Classical",
      "Rhythm and Blues",
      "Soft R & B",
      "Foreign Language",
      "Religious Music",
      "Religious Talk",
      "Personality",
      "Public",
      "College",
      "Reserved  -24-",
      "Reserved  -25-",
      "Reserved  -26-",
      "Reserved  -27-",
      "Reserved  -28-",
      "Weather",
      "Emergency Test",
      "ALERT",
      "Current Affairs",
      "Education",
      "Drama",
      "Cultures",
      "Science",
      "Varied Speech",
      "Easy Listening",
      "Light Classics",
      "Serious Classics",
      "Other Music",
      "Finance",
      "Children's Progs",
      "Social Affairs",
      "Phone In",
      "Travel & Touring",
      "Leisure & Hobby",
      "National Music",
      "Folk Music",
      "Documentary"};

const char* ptyLUTEU[32] = { 
      "",
      "News",
      "Current affairs",
      "Information",
      "Sport",
      "Education",
      "Drama",
      "Culture",
      "Science",
      "Varied",
      "Popular Music",
      "Rock Music",
      "Easy Listening",
      "Light Classical",
      "Serious Classical",
      "Other Music",
      "Weather",
      "Finance",
      "Children's Programmes",
      "Social Affairs",
      "Religion",
      "Phone-in",
      "Travel",
      "Leisure",
      "Jazz Music",
      "Country Music",
      "National Music",
      "Oldies Music",
      "Folk Music",
      "Documentary",
      "Alarm Test",
      "Alarm"};

TEF6686::TEF6686() {
}

void TEF6686::reset() {
  unsigned char patchTab[6] = {5, 0x1E, 0x5A, 0x01, 0x5A, 0x5A};
  Tuner_WriteBuffer((unsigned char *)&patchTab[1], patchTab[0]);
}

uint8_t TEF6686::init(bool initI2C) {
  uint8_t result;
  uint8_t counter = 0;
  uint8_t status;

  if (initI2C) Tuner_I2C_Init();
  
  delay(5);
  while (true) {
    result = devTEF668x_APPL_Get_Operation_Status(&status);
    if (result == 1) {
      if (status <= 1) {
        Tuner_Init();
        powerOff();
        return 1; //Ok
      } else {
        powerOff();
        return 0; //Already running
      }
    }
    if (++counter > 50) {
      return 2; //Doesn't exist
    }
    delay(5);
  } 
}

void TEF6686::powerOn() {
  devTEF668x_APPL_Set_OperationMode(0);
}

void TEF6686::powerOff() {
  devTEF668x_APPL_Set_OperationMode(1);
}

void TEF6686::setFrequency(int frequency, uint8_t band) {
  setFrequency(Radio_PRESETMODE, frequency, band);
}

void TEF6686::setFrequency(uint8_t mode, int frequency, uint8_t band) {
  if (band <= FM3_BAND) frequency /= 10;
  Radio_SetFreq(mode, band, frequency);
  clearRDS();
}

void TEF6686::setFrequencyQuick(int frequency, uint8_t band) {
  setFrequency(Radio_JUMPMODE, frequency, band);
}

void TEF6686::setBandwidth(bool automatic, uint16_t bandwidth) {
  devTEF668x_Radio_Set_Bandwidth((Radio_GetCurrentBand() <= FM3_BAND), automatic, bandwidth, 1000, 1000);
}

//agc = 0(low), 1(mid), 2(high), 3(max)
void TEF6686::setRFAGC(int level) {
  const int am[] = {1020, 990, 970, 940};
  const int fm[] = {920, 890, 870, 840};
  if (level > 3) level = 3;
  if ((Radio_GetCurrentBand() <= FM3_BAND))
    devTEF668x_Radio_Set_RFAGC(true, fm[level], 0);
  else
    devTEF668x_Radio_Set_RFAGC(false, am[level], 0);
}

int TEF6686::getFrequency() {
  int frequency = Radio_GetCurrentFreq();
  if (Radio_GetCurrentBand() <= FM3_BAND) frequency *= 10;
  return frequency;
}

int16_t TEF6686::getLevel() {
  return Radio_Get_Level((Radio_GetCurrentBand() <= FM3_BAND));
}

// range 0=off, 1-10=60-150% sensitivity, default 5
void TEF6686::setNoiseBlanker(uint16_t sensitivity) {
  if (!sensitivity) {
    devTEF668x_Radio_Set_NoiseBlanker((Radio_GetCurrentBand() <= FM3_BAND), 0, 1000);
    devTEF668x_Radio_Set_NoiseBlanker_Audio((Radio_GetCurrentBand() <= FM3_BAND), 0, 1000);
  }
  else {
    devTEF668x_Radio_Set_NoiseBlanker((Radio_GetCurrentBand() <= FM3_BAND), 1, sensitivity * 100 + 500);
    devTEF668x_Radio_Set_NoiseBlanker_Audio((Radio_GetCurrentBand() <= FM3_BAND), 1, sensitivity * 100 + 500);
  }
}

uint8_t TEF6686::getStereoStatus() {
  return Radio_CheckStereo();
}

uint16_t TEF6686::seekUp() {
	return seek(1);
}

uint16_t TEF6686::seekDown() {
	return seek(0);
}

uint16_t TEF6686::tuneUp() {
  return tune(1);
}

uint16_t TEF6686::tuneDown() {
  return tune(0);
}

void TEF6686::setVolume(uint16_t volume) {
  devTEF668x_Audio_Set_Volume(volume);
}

void TEF6686::setInput(uint16_t source) {
  devTEF668x_Audio_Set_Input(source);
}

uint16_t TEF6686::setDigIO(uint16_t signal,uint16_t mode,uint16_t format,uint16_t operation,uint16_t samplerate) {
  return devTEF668x_Audio_Set_Dig_IO(signal,mode,format,operation,samplerate);
}

uint16_t TEF6686::setOutputSource(uint16_t signal,uint16_t source) {
  return devTEF668x_Audio_Set_Output_Source(signal,source);
}

void TEF6686::seekSensHigh(bool high) {
  Radio_SetSeekSenLevel(high);
}

void TEF6686::setMute() {
  devTEF668x_Audio_Set_Mute(1);
}

void TEF6686::setUnMute() {
  devTEF668x_Audio_Set_Mute(0);
}

void TEF6686::forceMono(bool mono) {
  if (mono) devTEF668x_Radio_Set_Stereo_Min(1, 2, 400);
  else devTEF668x_Radio_Set_Stereo_Min(1, 0, 400);
}

bool TEF6686::checkRDS(uint16_t* status) {
  uint16_t result = devTEF668x_Radio_Get_RDS_Status(1, status);
  return result && ((*status & 0x8000) != 0x0);
}

void TEF6686::clearRDS() {
  for (uint8_t i = 0; i < 64; i++) rdsRadioText[i] = ' ';
  rdsRadioText[64] = '\0';     
  for (uint8_t i = 0; i < 8; i++) rdsProgramService[i] = ' ';
  rdsProgramService[8] = '\0';     
}

uint8_t TEF6686::readRDS() {
  uint8_t rdsBHigh, rdsBLow, rdsCHigh, rdsCLow, rdsDHigh, isReady = 0, rdsDLow;
  uint16_t rdsStat, rdsA, rdsB, rdsC, rdsD, rdsErr;
  uint16_t result = devTEF668x_Radio_Get_RDS_Data(1, &rdsStat, &rdsA, &rdsB, &rdsC, &rdsD, &rdsErr);

  uint8_t rdsErrB = (rdsErr >> 12) & 0x03; 
  uint8_t rdsErrC = (rdsErr >> 10) & 0x03; 
  uint8_t rdsErrD = (rdsErr >> 8) & 0x03; 
  bool err = (rdsErrB > 1) || (rdsErrC > 1) || (rdsErrD > 1);
  bool noerr = (rdsErrB == 0) && (rdsErrC == 0) && (rdsErrD == 0);

  //if (!(result && (rdsB != 0x0) && ((rdsStat & 0x8000) != 0x0) && ((rdsErr & 0x0a00) == 0x0))) {
  if (!(result && (rdsB != 0x0) && ((rdsStat & 0x8000) != 0x0) && !err)) {
    return isReady; 
  }

  rdsBHigh = (uint8_t)(rdsB >> 8);
  rdsBLow = (uint8_t)rdsB;
  rdsCHigh = (uint8_t)(rdsC >> 8);
  rdsCLow = (uint8_t)rdsC;
  rdsDHigh = (uint8_t)(rdsD >> 8);
  rdsDLow = (uint8_t)rdsD;
 
  uint8_t programType = ((rdsBHigh & 3) << 3) | ((rdsBLow >> 5) & 7);
  strcpy(rdsProgramType, (programType < 32) ? ptyLUTEU[programType] : "PTY ERROR");
  if (RDS_ProgramType) RDS_ProgramType(rdsProgramType);

  uint8_t type = (rdsBHigh >> 4) & 15;
  uint8_t version = (rdsBHigh >> 4) & 1;
  
  // Groups 0A & 0B
  // Basic tuning and switching information only
  if (type == 0) {
    uint8_t address = rdsBLow & 3;
    // Groups 0A & 0B: to extract PS segment we need blocks 1 and 3
    if (address <= 3) {
      if (rdsDHigh != '\0') {
        rdsProgramService[address * 2] = rdsDHigh;
      }  
      if (rdsDLow != '\0') {
        rdsProgramService[address * 2 + 1] = rdsDLow;
      }  
      isReady = (address == 3) ? 1 : 0;
    } 
    rdsFormatString(rdsProgramService, 8);
    if (isReady) {
      if (RDS_ProgramService) RDS_ProgramService(rdsProgramService);
      if (RDS_RadioText) RDS_RadioText(rdsRadioText);      
    }
  }
  // Groups 2A & 2B
  // Radio Text
  else if (type == 2) {
    uint16_t addressRT = rdsBLow & 15;
    uint8_t ab = (rdsBLow >> 4) & 1;
    if (version == 0) {
      if (addressRT <= 15) {
        if (rdsCHigh != 0x0D) rdsRadioText[addressRT*4] = rdsCHigh;
        else rdsRadioText[addressRT * 4] = '\0';
        if (rdsCLow != 0x0D) rdsRadioText[addressRT * 4 + 1] = rdsCLow;
        else rdsRadioText[addressRT * 4 + 1] = '\0';
        if (rdsDHigh != 0x0D) rdsRadioText[addressRT * 4 + 2] = rdsDHigh;
        else rdsRadioText[addressRT * 4 + 2] = '\0';
        if (rdsDLow != 0x0D) rdsRadioText[addressRT * 4 + 3] = rdsDLow;
        else rdsRadioText[addressRT * 4 + 3] = '\0';
      }
    }
    else {
      if (addressRT <= 7) {
        if (rdsDHigh != '\0') {
          rdsRadioText[addressRT * 2] = rdsDHigh;
        }  
        if (rdsDLow != '\0') {
          rdsRadioText[addressRT * 2 + 1] = rdsDLow;
        }
      }
    }
    if (ab != rdsAb) {
      for (uint8_t i = 0; i < 64; i++) {
        rdsRadioText[i] = ' ';
      }
      rdsRadioText[64] = '\0';     
      isRdsNewRadioText = 1;
    }
    else {
      isRdsNewRadioText = 0;
    }
    rdsAb = ab;
    rdsFormatString(rdsRadioText, 64);
  }

  //TOD
  else if (type == 4 && RDS_Time && noerr) {
    unsigned long MJD, CT, ys;
    uint16_t yp;
    uint8_t k, mp;

    CT = ((unsigned long)rdsC << 16) | rdsD;
    //The standard mandates that CT must be all zeros if no time
    //information is being provided by the current station.
    if(CT) {
      MJD = (unsigned long)(rdsB & uint16_t(0x0003)) << 15;
      MJD |= (CT & 0xFFFE0000UL) >> 17;
      uint16_t Hours = (CT & 0x0001F000UL) >> 12;
      uint16_t offset = CT & 0x0000001FUL;
      if (CT & 0x00000020UL)
        offset = -offset;
      uint16_t Minutes = (CT & 0x00000FC0UL) >> 6;
      //Use integer arithmetic at all costs, Arduino lacks an FPU
      yp = (MJD * 10 - 150782) * 10 / 36525;
      ys = yp * 36525 / 100;
      mp = (MJD * 10 - 149561 - ys * 10) * 1000 / 306001;
      uint16_t Day = MJD - 14956 - ys - mp * 306001 / 10000;
      k = (mp == 14 || mp == 15) ? 1 : 0;
      uint16_t Year = 1900 + yp + k;
      uint16_t Month = mp - 1 - k * 12;
      RDS_Time(Hours + offset / 2, Minutes, Day, Month, Year);
    }
  }
  return isReady; 
}
 
void TEF6686::getRDS(RdsInfo* rdsInfo) {
  strcpy(rdsInfo->programType, rdsProgramType);
  strcpy(rdsInfo->programService, rdsProgramService);
  strcpy(rdsInfo->radioText, rdsRadioText); 
}

void TEF6686::rdsFormatString(char* str, uint16_t length) {  
  for (uint16_t i = 0; i < length; i++) {    
    if ((str[i] != 0 && str[i] < 32) || str[i] > 126 ) {
      str[i] = ' ';  
    }
  }
}

uint16_t TEF6686::seek(uint8_t up) {
  uint16_t mode = 20;
  uint16_t startFrequency = Radio_GetCurrentFreq();

  while (true) {
    switch(mode){
      case 20:
        Radio_ChangeFreqOneStep(up);
        Radio_SetFreq(Radio_SEARCHMODE, Radio_GetCurrentBand(), Radio_GetCurrentFreq());
      
        mode = 30;
        Radio_CheckStationInit();
        Radio_ClearCurrentStation();
        
        break;
      
      case 30:
        delay(20);
        Radio_CheckStation();
        if (Radio_CheckStationStatus() >= NO_STATION) {
          mode = 40;
        }   
        
        break;

      case 40:
        if (Radio_CheckStationStatus() == NO_STATION) {        
          mode = (startFrequency == Radio_GetCurrentFreq()) ? 50 : 20;
        }
        else if (Radio_CheckStationStatus() == PRESENT_STATION) {
          mode = 50;
        }
        
        break;
      
      case 50:
        Radio_SetFreq(Radio_PRESETMODE, Radio_GetCurrentBand(), Radio_GetCurrentFreq());
        return Radio_GetCurrentFreq();
    }
  }
  return 0;
}

uint16_t TEF6686::seekSync(uint8_t up) {
  if (seekMode == 0) {	
	seekMode = 20;
	seekStartFrequency = Radio_GetCurrentFreq();
  }
  switch(seekMode) {
    case 20:
	  Radio_ChangeFreqOneStep(up);
      Radio_SetFreq(Radio_SEARCHMODE, Radio_GetCurrentBand(), Radio_GetCurrentFreq());
      
      seekMode = 30;
      Radio_CheckStationInit();
      Radio_ClearCurrentStation();
        
      return 0;
      
    case 30:
      delay(20);
      Radio_CheckStation();
      if (Radio_CheckStationStatus() >= NO_STATION) {
        seekMode = 40;
      }   
        
      return 0;

    case 40:
      if (Radio_CheckStationStatus() == NO_STATION) {        
        seekMode = (seekStartFrequency == Radio_GetCurrentFreq()) ? 50 : 20;
      }
      else if (Radio_CheckStationStatus() == PRESENT_STATION) {
        seekMode = 50;
      }
        
      return 0;
      
    case 50:
	  seekMode = 0;
      Radio_SetFreq(Radio_PRESETMODE, Radio_GetCurrentBand(), Radio_GetCurrentFreq());
      return 1;  
  }
  return 0;
}	

uint16_t TEF6686::tune(uint8_t up) {
  Radio_ChangeFreqOneStep(up);

  Radio_SetFreq(Radio_PRESETMODE, Radio_GetCurrentBand(), Radio_GetCurrentFreq());
  Radio_ClearCurrentStation();
  return Radio_GetCurrentFreq();
}
