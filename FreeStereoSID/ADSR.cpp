
#include "ADSR.h"

#include<Arduino.h>

void ADSR::calc(uint8_t lfoNr, uint8_t sidRegisters[]) {

  uint8_t waveformValue = (sidRegisters[lfoNr * 7 + 4] & 0xff);

  if ((waveformValue & 1) == 1) soundOn(lfoNr, sidRegisters);
  if ((waveformValue & 1) == 0) releaseFlag = true;

  doADSR(lfoNr, sidRegisters);


  // Debugging only 1 Voice  (Hint: set SID-Count to "1" in "FreeStereoSID.cpp")
  if (debug && lfoNr == 0) { // lfoNr = SID Voice Number 0,1,2

    Serial.print("Voice: ");
    Serial.println(lfoNr);
    Serial.print("Volume: ");
    Serial.println(volume);
    Serial.print("Wave: ");
    Serial.println(waveformValue);
    Serial.print("releaseFlag: ");
    Serial.println(releaseFlag);
    Serial.print("AttackTime: ");
    Serial.println(attackTime);
    Serial.print("AttackRisingSteps: ");
    Serial.println(attackRisingSteps);
    Serial.print("DecayTime: ");
    Serial.println(decayTime);
    Serial.print("SustainLevel: ");
    Serial.println(sustainLevel);
    Serial.print("ReleaseTime: ");
    Serial.println(releaseTime);
    Serial.println();
  }

}


void ADSR::soundOn(uint8_t lfoNr, uint8_t sidRegisters[]) {

  releaseFlag = false;

  adsrTimeCounterMillis   = 0;

  sustainLevel            = (1.0f / 15) * ((sidRegisters[lfoNr * 7 + 6] & 0xf0) >> 4);


  attackTime              = attackTimes[(sidRegisters[lfoNr * 7 + 5] & 0xf0) >> 4];
  attackRisingSteps       = 1.0f / (attackTime / 3);

  decayTime               = decayTimes[(sidRegisters[lfoNr * 7 + 5] & 0xf)];
  decayFallingSteps       = (1.0f - sustainLevel) / (decayTime / 3);

  releaseTime             = releaseTimes[(sidRegisters[lfoNr * 7 + 6] & 0xf)];
  releaseFallingSteps     = sustainLevel / (releaseTime / 3);

  adsr                    = attack;
  volume = 1.0f; 
}


void ADSR::doADSR(uint8_t lfoNr, uint8_t sidRegisters[]) {

  adsrTimeCounterMillis += 3; // 2.9ms = 128 Audio Block Steps


  if (adsr == attack) { // ATTACK

    // Time to rise to Amp 1.0
    volume += attackRisingSteps;
    if (volume > 1.0f) {
      adsrTimeCounterMillis = attackTime;
      volume = 1.0f;
    }

    if (adsrTimeCounterMillis >= attackTime) {
      adsrTimeCounterMillis = 0;
      adsr = decay;
    }
  }
  if (adsr == decay) { // DECAY

    // Time to fall from Amp 1.0 to Sustain Level (x.x)
    volume -= decayFallingSteps;
    if (volume < 0.0f) {
      adsrTimeCounterMillis = decayTime;
      volume = 0.0f;
    }

    if (adsrTimeCounterMillis >= decayTime) {
      adsrTimeCounterMillis = 0;
      if (releaseFlag) adsr = release;
      volume = sustainLevel;
    }
  }
  if (adsr == release) { // RELEASE

    volume -= releaseFallingSteps;
    if (volume < 0.0f) {
      adsrTimeCounterMillis = releaseTime;
      volume = 0.0f;
    }

    // Time to fall from Sustain Level (x.x)  ... to Amp 0.0
    if (adsrTimeCounterMillis >= releaseTime) {
     
      adsrTimeCounterMillis = 0;
      adsr = off;
    }
  }
}
