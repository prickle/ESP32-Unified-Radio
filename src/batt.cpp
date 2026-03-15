#include "decls.h"
//Battery monitor
//Uses ADS1115 ADC to monitor charger and INA219 current shunt to monitor system input power rail

volatile bool ShuntFound = false;
volatile bool ADCFound = false;
volatile bool touchFound = false;
volatile bool nxpFound = false;

#ifdef I2C_SDA
//Look for I2C devices
void i2cScan() {
  serial.print("> Scanning I2C Bus: ");
  terminalHandle();
  byte error, address;
  int nDevices = 0;
  Wire.begin(I2C_SDA, I2C_SCL);
  for(address = 1; address < 127; address++ )
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission(); 
    if (error == 0) {
      //Known addresses
      if (address == NXP_ADDR) nxpFound = true;
      else if (address == FT62XX_ADDR) touchFound = true;
      else if (address == INA_ADDR) ShuntFound = true;
      else if (address == ADS1_ADDR) ADCFound = true;
      else {
        serial.print("Unknown @ 0x");
        if (address<16) serial.print("0");
        serial.print(address, HEX);
      }
      nDevices++;
    }
    else if (error==4) {
      serial.print("Error @ 0x");
      if (address<16) serial.print("0");
      serial.print(address, HEX);
    }    
  }
  if (nDevices == 0) serial.print("No");
  else serial.print(nDevices);
  serial.printf(" device%s found.\r\n", nDevices == 1?"":"s");
  terminalHandle();
}
#endif

#ifdef BATTERYMON
//Battery monitor, ADC and current shunt
ADS1115 ads(ADS1_ADDR);
Adafruit_INA219 ina219;

float battV;
float solarV;
float chargeI;
float dischargeI;
int8_t battPercent;
int8_t powerSource;
float inputPower;
float outputPower;
float battAmps;
int64_t mAseconds;
bool battCharging = false;           //when charging


void initAdsStateMachine();

//Called from setup
void ADCInit() {
  //I2C devices
  serial.print("> Start Battery Monitor: ");
  if (ADCFound) {
    //Whole unit is present
    ads.set_data_rate(ADS1115_DATA_RATE_16_SPS);
    ads.set_mode(ADS1115_MODE_SINGLE_SHOT);
    ads.set_pga(ADS1115_PGA_TWO_THIRDS);
    initAdsStateMachine();
    serial.println("OK.");
    ina219.begin();
  } else {
    //Just the controller is connected to USB
    powerSource = SOURCE_USB;    
    serial.println("Not Found!");
  }
  terminalHandle();
}

//Begin ADC read
void startReadADS(ads1115_mux channel) {
  ads.set_mux(channel);
  ads.trigger_sample();
}

//Top line battery icon
void updateBatteryDisplay() {
  char s[25] = {0};
  if (powerSource == SOURCE_USB)
    snprintf(s, 24, " " LV_SYMBOL_USB " ");
  else if (powerSource == SOURCE_UNKNOWN)
    snprintf(s, 24, " " LV_SYMBOL_BATTERY_EMPTY " ?");
  else if (battCharging) 
    snprintf(s, 24, " " LV_SYMBOL_CHARGE " %d%%", battPercent);
  else {
    if (battPercent < 20) 
      snprintf(s, 24, LV_SYMBOL_BATTERY_EMPTY " %d%%", battPercent);    
    else if (battPercent < 40) 
      snprintf(s, 24, LV_SYMBOL_BATTERY_1 " %d%%", battPercent);    
    else if (battPercent < 60) 
      snprintf(s, 24, LV_SYMBOL_BATTERY_2 " %d%%", battPercent);    
    else if (battPercent < 80) 
      snprintf(s, 24, LV_SYMBOL_BATTERY_3 " %d%%", battPercent);    
    else 
      snprintf(s, 24, LV_SYMBOL_BATTERY_FULL " %d%%", battPercent);    
  }
  setBattChgLbl(s);  
}

void resetAmpHours() {
  mAseconds = 0;
}

//------------------------------------------------------------------------
// ADC state machine

uint8_t adsState = ADCSTATE_IDLE;
#define MUX_MODE     ADS1115_MUX_GND_AIN0
#define MUX_SOLAR    ADS1115_MUX_GND_AIN1
#define MUX_CURRENT  ADS1115_MUX_GND_AIN2
#define MUX_BATTERY  ADS1115_MUX_GND_AIN3

void initAdsStateMachine() {
  startReadADS(MUX_BATTERY);
  adsState = ADCSTATE_BATTERY;
}

float hardLimit(float val, float low, float high) {
  if (val > high) val = high;
  if (val < low) val = low;
  return val;
}

int16_t adc0, adc1, adc2, adc3;


//Input controls: Non-blocking state machine
// Also samples battery
// returns true on new data set, can be free-wheeling unsynchronised
bool handleAdsStateMachine() {
  if (!ADCFound || adsState == ADCSTATE_IDLE || ads.is_sample_in_progress()) return false;
  int16_t val = ads.read_sample();
  switch (adsState) {
    case ADCSTATE_BATTERY:
      adc0 = val;
      battV = (float)val / 1667;
      battPercent = (battPercent * 0.8) + (hardLimit(((battV - FLAT_BATT) * 100) / (FULL_BATT - FLAT_BATT), 0, 100) * 0.2);  //Slight filtering
      startReadADS(MUX_CURRENT); //Current
      adsState = ADCSTATE_CURRENT;
      dischargeI = ina219.getCurrent_mA();      //Try to synchronise readings to some degree
      break;
    case ADCSTATE_CURRENT:
      adc1 = val - 13409;   //zero point
      chargeI = (float)(val - 13409) / 531; //amps
      if (chargeI < 0) chargeI = 0;         //only input current makes sense
      startReadADS(MUX_SOLAR); //Solar
      adsState = ADCSTATE_SOLAR;
      break;
    case ADCSTATE_SOLAR:
      adc2 = val;
      solarV = (float)val / 765;  //volts
      startReadADS(MUX_MODE); //Mode
      adsState = ADCSTATE_MODE;
      break;
    case ADCSTATE_MODE:
      adc3 = val;
      if (val > 18300 && val < 18500) {
        powerSource = SOURCE_BATTERY;
      }
      else if ((val > 10900 && val < 11100) || (val > 12960 && val < 13160)) {
        powerSource = SOURCE_SOLAR;
      }
      else if ((val > 5280 && val < 5480) || (val > 7310 && val < 7510)) {
        powerSource = SOURCE_AC;
      }
      else {
        powerSource = SOURCE_UNKNOWN;
      }
      inputPower = battV * chargeI;
      //From calibration, takes into account diode voltage drop and boost/charger inefficiency
      if (powerSource == SOURCE_AC) inputPower = inputPower * 0.661 - 0.0486;
      else if (powerSource == SOURCE_SOLAR) inputPower = inputPower * 1.648 - 0.15384;
      outputPower = battV * dischargeI / 1000.0;
      if (battV < 0.1) battAmps = 0;
      else battAmps = (battAmps * 0.8) + (((inputPower - outputPower) / battV) * 0.2); //Slight filtering
      battCharging = (battAmps > 0.08);  //80mA
      startReadADS(MUX_BATTERY); //Battery
      adsState = ADCSTATE_BATTERY;
      return true;
  }
  return false;
}

unsigned long adcTimer;

//Called from loop
void ADCHandle() {  
  static uint8_t oldBattPercent = 101;
  static uint8_t oldPowerSource = SOURCE_UNKNOWN;
  static bool oldBattState = false;
  if (adcTimer <= millis()) {
    adcTimer = millis() + 100;      //Not too often, can make I2C very busy
    handleAdsStateMachine();        //Don't care when finished, just look for changes below
    if (oldBattState != battCharging || oldBattPercent != battPercent || oldPowerSource != powerSource) {
      oldBattState = battCharging;
      oldBattPercent = battPercent;
      oldPowerSource = powerSource;
      updateBatteryDisplay();
    }
  }
}

//Debugging - Battery report on Serial
void batteryReport() {
  Serial.print("out: ");
  Serial.print(battV);
  Serial.print("v, ");
  Serial.print(dischargeI);
  Serial.print("A, in: ");
  //Serial.print(outputPower);
  //Serial.print("W, ");
  Serial.print(solarV);
  Serial.print("v, ");
  Serial.print(chargeI * 1000);
  Serial.print("A, ");
  if (powerSource == SOURCE_BATTERY) Serial.print("Battery "); 
  else if (powerSource == SOURCE_SOLAR) Serial.print("Solar "); 
  else if (powerSource == SOURCE_AC) Serial.print("AC/DC "); 
  else if (powerSource == SOURCE_UNKNOWN) Serial.print("Unknown? ");
  if (battCharging) Serial.print("[charging] :"); 
  else Serial.print(" :"); 
  Serial.println(adc3);  
}

#endif
