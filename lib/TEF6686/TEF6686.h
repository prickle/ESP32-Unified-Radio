#ifndef TEF6686_h
#define TEF6686_h

#include <Wire.h>

#include "Tuner_Api.h"
#include "Tuner_Drv_Lithio.h"
#include "Tuner_Interface.h"

#define I2C_PORT 2
#define I2C_ADDR 0x64

#define TEF_SETINPUT_RADIO		0
#define TEF_SETINPUT_I2S		32
#define TEF_SETINPUT_SINE		240
#define TEF_SETDIGIO_SD0    32  //= I²S digital audio IIS_SD_0 (input)
#define TEF_SETDIGIO_INPUT  1    //1 = input (only available for signal = 32)
#define TEF_SETDIGIO_I2S16  16   //16 = I²S 16 bits (fIIS_BCK = 32 * samplerate)
#define TEF_SETDIGIO_I2S32  32   //16 = I²S 16 bits (fIIS_BCK = 32 * samplerate)
#define TEF_SETDIGIO_SLAVE  0    //0 = slave mode; IIS_BCK and IIS_WS input defined by source (default)

struct RdsInfo {
  char programType[17]; 
  char programService[9];
  char radioText[65];
  bool newRadioText;
};

extern __attribute__((weak)) void RDS_ProgramType(char* type);
extern __attribute__((weak)) void RDS_ProgramService(char* name);
extern __attribute__((weak)) void RDS_RadioText(char* text);
extern __attribute__((weak)) void RDS_Time(uint16_t hour, uint16_t minute, uint16_t day, uint16_t month, uint16_t year);

class TEF6686 {
  public:
    TEF6686();
    void reset();
    uint8_t init(bool initI2C);
    void powerOn();					// call in setup
    void powerOff();				
    void setFrequency(int frequency, uint8_t band = FM1_BAND);    // frequency as int, i.e. 100.00 as 10000
    void setFrequency(uint8_t mode, int frequency, uint8_t band = FM1_BAND);    // frequency as int, i.e. 100.00 as 10000
    void setFrequencyQuick(int frequency, uint8_t band = FM1_BAND);    // frequency as int, i.e. 100.00 as 10000
    void setBandwidth(bool automatic, uint16_t bandwidth);
    void setRFAGC(int level);
    void setNoiseBlanker(uint16_t sensitivity);
    int getFrequency(); // returns the current frequency
    int16_t getLevel();
    uint8_t getStereoStatus();
    uint16_t seekUp();      // returns the tuned frequency or 0
    uint16_t seekDown();    // returns the tuned frequency or 0
	uint16_t seekSync(uint8_t up); // returns 1 if seek was completed and 0 if seek in progress
    uint16_t tuneUp();      // returns the tuned frequency or 0
    uint16_t tuneDown();    // returns the tuned frequency or 0
    void setVolume(uint16_t volume); 	//-60 -- +24 dB volume
    void setInput(uint16_t source);
    uint16_t setDigIO(uint16_t signal,uint16_t mode,uint16_t format,uint16_t operation,uint16_t samplerate);
    uint16_t setOutputSource(uint16_t signal,uint16_t source);
    void seekSensHigh(bool high);
    void setMute();
    void setUnMute();
    void forceMono(bool mono);
    bool checkRDS(uint16_t* status);
    uint8_t readRDS();
    void getRDS(RdsInfo* rdsInfo);
     
  private:
    uint16_t seek(uint8_t up);
    uint16_t tune(uint8_t up);
    void rdsFormatString(char* str, uint16_t length);
    void clearRDS();

    char rdsRadioText[65];
    char rdsRadioTextBuf[65];
    char rdsProgramService[9];
    uint8_t rdsAb;
    char rdsProgramType[17];
    uint8_t isRdsNewRadioText;
	
	uint16_t seekMode;
	uint16_t seekStartFrequency;
};

#endif
