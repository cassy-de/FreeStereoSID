
/*************************************************************************

  "FreeStereoSID" Teensy 4.0 Firmware, 
  
  Arduino IDE Settings : Tools-> "Serial, 600/816Mhz, Optimized: Fastest"

  Designed by people who think differently

 *************************************************************************


   LICENSE: There is no License, no GPL, no MIT, no BSD, ...

   ...only free software: Public Domain

   Creative freedom means 100% freedom, not pseudo free :-)

*/



#include <EEPROM.h>
#include <Audio.h>
#include "FreeStereoSID.h"

int pinLED            = 1;
int pinCS             = 6;
int pinRW             = 5;
int pinA5             = 2;
int pinA8             = 3;
int pinIO1            = 23;
int pinPOTX           = 22;
int pinPOTY           = 21;
int pinPHI2           = 0;

boolean cs_state      = 0;
boolean rw_state      = 0;
boolean sid_A5_state  = 0;
boolean sid_A8_state  = 0;
boolean phi2_state    = 0;

uint8_t portPinsC[5]  = {7, 8, 9, 4, 11};                   // A0, A1, A2, A3, A4
uint8_t portPinsD[8]  = {13, 14, 15, 16, 17, 18, 19, 20};   // D0, D1, D2, D3, D4, D5, D6, D7


#define sidCount 1

int sidAddresses[]          = {0xD400, 0xD420, 0xD500, 0xD520, 0xDE00};


FreeStereoSID   sidChip[sidCount];

AudioMixer4     mixerSID1;
AudioMixer4     mixerSID2;
AudioOutputMQS  mqs1;

AudioConnection patchCord1(sidChip[0], 0, mixerSID1, 0);
#if sidCount == 2
  AudioConnection patchCord2(sidChip[1], 0, mixerSID2, 0);
#endif

AudioConnection patchCord3(mixerSID1, 0, mqs1, 0); // , msq1, X);  (X=0 -> Pin 12,  X=1 -> Pin 10)
AudioConnection patchCord4(mixerSID2, 0, mqs1, 1);
AudioConnection patchCord5(mixerSID2, 0, mqs1, 0);

AudioAnalyzePeak mixerVUPeak;
AudioConnection  patchCord6(mixerSID1, 0, mixerVUPeak, 0);

//float sidOutputVolume = 0.25f;  // Do not use more than 0.3!
float sidOutputVolume = 0.05f;    // ...0.05f Headphones Level for "Developing"



volatile uint8_t data_val   = 0;

boolean waitingForRegValue  = false;
int regNr                   = -1;
int regVal                  = -1;

int eepromCopy[256];  // EEPROM.read() is to slow... so, read from the Array


boolean ledAnimation = false;

void setup() {

  Serial.begin(115200);

  AudioMemory(12);
  mixerSID1.gain(0, sidOutputVolume);
  mixerSID1.gain(1, sidOutputVolume);
  mixerSID2.gain(0, sidOutputVolume);
  mixerSID2.gain(1, sidOutputVolume);
    
#if sidCount == 2
  routeSID2ToPinOut2(true); // Route the SID2 Output to external "OUT2" Pin
#endif


  // * Set Default SID Addresses: 0xD400, 0xD420, 0xD500, 0xD520, 0xDE00 *
  for (int i = 0; i < sidCount; i++) sidChip[i].setSIDAddress(sidAddresses[i]);


  // * Copy all EEPROM Values into Array (direct EEPROM accces time is to long) *
  for (int i = 0; i < 256; i++) eepromCopy[i] = EEPROM.read(i);

  // * Address and Data Pins direction: INPUT *
  for (int a = 0; a < 5; a++) pinMode(portPinsC[a], INPUT);
  for (int d = 0; d < 8; d++) pinMode(portPinsD[d], INPUT);

  // * Aditional Pins direction: INPUT *
  pinMode(pinPHI2, INPUT);
  pinMode(pinRW, INPUT);
  pinMode(pinCS, INPUT);

  pinMode(pinA5, INPUT);
  pinMode(pinA8, INPUT);
  pinMode(pinIO1, INPUT);

  // * Disable PULLUPs for A5,A8,IO1  because normally would be HIGH and then never D400 happen! *
  CORE_PIN2_CONFIG = CONFIG_NOPULLUP;
  CORE_PIN3_CONFIG = CONFIG_NOPULLUP;
  CORE_PIN23_CONFIG = CONFIG_NOPULLUP;

  pinMode(pinPOTX, INPUT);
  pinMode(pinPOTY, INPUT);


  // * Set Interrupt for Chip Select Pin *
  attachInterrupt(digitalPinToInterrupt(pinCS), chipSelected, LOW);


  for (int i = 0; i < sidCount; i++) {

    // *** Init SID Registers (is needed on Power Up for Jupiter Lander Module) ***

    // Volume
    sidChip[i].writeReg(24, 15);

    // Waveform Recangle (PCM)
    sidChip[i].writeReg(18, 64);
    sidChip[i].writeReg(11, 64);
    sidChip[i].writeReg(4, 64);

    // PCM Duty cycle
    sidChip[i].writeReg(17, 8);
    sidChip[i].writeReg(10, 8);
    sidChip[i].writeReg(3, 8);

    // * Hold/Release *
    sidChip[i].writeReg(20, 208);
    sidChip[i].writeReg(13, 208);
    sidChip[i].writeReg(6, 208);
  }

}


void loop() {

  // * Later here POTX and POTY Code *


  delayMicroseconds(250);

}


void routeSID2ToPinOut2(boolean out2) {

  if (!out2) {
    patchCord4.connect();
    patchCord5.disconnect();
  }
  else {
    patchCord4.disconnect();
    patchCord5.connect();
  }
}



FASTRUN void chipSelected() {

  
  digitalWrite(pinLED, LOW);

  rw_state      = digitalReadFast(pinRW);
  uint16_t regAddr   = (readAddressPins() & B00011111);


  if (rw_state == LOW) {

    // *** WRITE requested by CPU into SID Chip ***

    sid_A5_state  = digitalReadFast(pinA5);
    sid_A8_state  = digitalReadFast(pinA8);
    data_val      = readDataPins();

    int busAdr       = 0xD400;
    bitWrite(busAdr, 5, sid_A5_state ? 1 : 0); // D0x20
    bitWrite(busAdr, 8, sid_A8_state ? 1 : 0); // D05xx


    // * Debug: only for a short check of the Incoming Data Values (normally not needed) *
    /*    
    Serial.print("Address: ");
    Serial.print((busAdr + regAddr), HEX);
    Serial.print("    Data: ");
    Serial.println(data_val);
    */
    

    // * To the SID Chip... *
    if (regAddr >= 0 && regAddr <= 24) {  // SID Registers 0-24;

      // * Check whether the 2nd SID is at this busAdr *
      for (int i = 0; i < sidCount; i++) {
        if (busAdr == sidChip[i].sidAddress) {
          sidChip[i].writeReg(regAddr, data_val);
        }
      }
    }

    // * EEPROM write... *
    else {

      if (regAddr == 0x1D) {   // 54301, Set EEPROM Register Nr
        waitingForRegValue = true;
        regNr = data_val;
      }
      else if (waitingForRegValue && regAddr == 0x1E) { // 54302, Write Value into EEPROM Register Nr
        waitingForRegValue = false;
        regVal = data_val;

        if (regNr >= 0 && regNr <= 255) {
          eepromCopy[regNr] = regVal;
          EEPROM.write(regNr, regVal);
        }

        regNr = -1;
        regVal = -1;
      }
    }

  }
  else {

    // *** READ requested by CPU from SID Chip ***

    switch (regAddr) {

      case 0x19: // POTX
        setDataPins(0);
        break;
      case 0x1A: // POTY
        setDataPins(0);
        break;
      case 0x1B:
        // outputData(sidChip[sidNr].getreg(0x1B));  // Oszillator Stimme 3
        break;
      case 0x1C:
        // outputData(sidChip[sidNr].getreg(0x1C));  // Hüllkurve Stimme 3
        break;

      // * EEPROM read ... *
      case 0x1F:
        if (waitingForRegValue) { // 54303,  Read Value from EEPROM Register Nr
          waitingForRegValue = false;
          if (regNr >= 0 && regNr <= 255)  setDataPins(eepromCopy[regNr]);
          regNr = -1;
        }
        else setDataPins(0);
        break;
      default:
        setDataPins(0);
    }
  }
}


void setDataPins(uint8_t val) {

  // * Set Pins to "OUTPUT" *
  CORE_PIN13_DDRREG |= CORE_PIN13_BITMASK;
  CORE_PIN14_DDRREG |= CORE_PIN14_BITMASK;
  CORE_PIN15_DDRREG |= CORE_PIN15_BITMASK;
  CORE_PIN16_DDRREG |= CORE_PIN16_BITMASK;
  CORE_PIN17_DDRREG |= CORE_PIN17_BITMASK;
  CORE_PIN18_DDRREG |= CORE_PIN18_BITMASK;
  CORE_PIN19_DDRREG |= CORE_PIN19_BITMASK;
  CORE_PIN20_DDRREG |= CORE_PIN20_BITMASK;

  // * Set Pins States *
  if ((val & B00000001) == 1)    CORE_PIN13_PORTSET    = CORE_PIN13_BITMASK;
  else                           CORE_PIN13_PORTCLEAR  = CORE_PIN13_BITMASK;
  if ((val & B00000010) == 2)    CORE_PIN14_PORTSET    = CORE_PIN14_BITMASK;
  else                           CORE_PIN14_PORTCLEAR  = CORE_PIN14_BITMASK;
  if ((val & B00000100) == 4)    CORE_PIN15_PORTSET    = CORE_PIN15_BITMASK;
  else                           CORE_PIN15_PORTCLEAR  = CORE_PIN15_BITMASK;
  if ((val & B00001000) == 8)    CORE_PIN16_PORTSET    = CORE_PIN16_BITMASK;
  else                           CORE_PIN16_PORTCLEAR  = CORE_PIN16_BITMASK;
  if ((val & B00010000) == 16)   CORE_PIN17_PORTSET    = CORE_PIN17_BITMASK;
  else                           CORE_PIN17_PORTCLEAR  = CORE_PIN17_BITMASK;
  if ((val & B00100000) == 32)   CORE_PIN18_PORTSET    = CORE_PIN18_BITMASK;
  else                           CORE_PIN18_PORTCLEAR  = CORE_PIN18_BITMASK;
  if ((val & B01000000) == 64)   CORE_PIN19_PORTSET    = CORE_PIN19_BITMASK;
  else                           CORE_PIN19_PORTCLEAR  = CORE_PIN19_BITMASK;
  if ((val & B10000000) == 128)  CORE_PIN20_PORTSET    = CORE_PIN20_BITMASK;
  else                           CORE_PIN20_PORTCLEAR  = CORE_PIN20_BITMASK;


  // * Set Pins to "INPUT" (Tri State) *
  CORE_PIN13_DDRREG &= ~CORE_PIN13_BITMASK;
  CORE_PIN14_DDRREG &= ~CORE_PIN14_BITMASK;
  CORE_PIN15_DDRREG &= ~CORE_PIN15_BITMASK;
  CORE_PIN16_DDRREG &= ~CORE_PIN16_BITMASK;
  CORE_PIN17_DDRREG &= ~CORE_PIN17_BITMASK;
  CORE_PIN18_DDRREG &= ~CORE_PIN18_BITMASK;
  CORE_PIN19_DDRREG &= ~CORE_PIN19_BITMASK;
  CORE_PIN20_DDRREG &= ~CORE_PIN20_BITMASK;
}

// * Maybe this is usefull later for "INPUT" TriState *
void pinDisable(const uint8_t pin) {
  volatile uint32_t *config;
  if (pin >= CORE_NUM_TOTAL_PINS) return;
  config = portConfigRegister(pin);
  *config = 0;
}


uint8_t readAddressPins() {

  uint8_t ret = 0;
  if ((CORE_PIN7_PINREG & CORE_PIN7_BITMASK) ? 1 : 0) ret |= (1 << 0);
  if ((CORE_PIN8_PINREG & CORE_PIN8_BITMASK) ? 1 : 0) ret |= (1 << 1);
  if ((CORE_PIN9_PINREG & CORE_PIN9_BITMASK) ? 1 : 0) ret |= (1 << 2);
  if ((CORE_PIN4_PINREG & CORE_PIN4_BITMASK) ? 1 : 0) ret |= (1 << 3);
  if ((CORE_PIN11_PINREG & CORE_PIN11_BITMASK) ? 1 : 0) ret |= (1 << 4);
  return ret;
}

uint8_t readDataPins() {

  uint8_t ret = 0;
  if ((CORE_PIN13_PINREG & CORE_PIN13_BITMASK) ? 1 : 0)  ret |= (1 << 0);
  if ((CORE_PIN14_PINREG & CORE_PIN14_BITMASK) ? 1 : 0)  ret |= (1 << 1);
  if ((CORE_PIN15_PINREG & CORE_PIN15_BITMASK) ? 1 : 0)  ret |= (1 << 2);
  if ((CORE_PIN16_PINREG & CORE_PIN16_BITMASK) ? 1 : 0)  ret |= (1 << 3);
  if ((CORE_PIN17_PINREG & CORE_PIN17_BITMASK) ? 1 : 0)  ret |= (1 << 4);
  if ((CORE_PIN18_PINREG & CORE_PIN18_BITMASK) ? 1 : 0)  ret |= (1 << 5);
  if ((CORE_PIN19_PINREG & CORE_PIN19_BITMASK) ? 1 : 0)  ret |= (1 << 6);
  if ((CORE_PIN20_PINREG & CORE_PIN20_BITMASK) ? 1 : 0)  ret |= (1 << 7);
  return ret;
}
