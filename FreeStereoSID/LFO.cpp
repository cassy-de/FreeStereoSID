
#include "LFO.h"
#include <stdlib.h>
#include <math.h>

#include <Arduino.h>

void LFO::oscillate(int lfoNr, uint8_t sidRegisters[]) {


  // * Calculate the ADSR *
  adsr.calc(lfoNr, sidRegisters);


  // * Voice Frequency Calculation (SID-Register 1 and 0, High and Low byte) *
  toneFreq    = (((sidRegisters[lfoNr * 7 + 1] & 0xff) << 8) + (sidRegisters[lfoNr * 7 + 0] & 0xff)) * (985248.0f / 16777216.0f);
  sampleSteps = (float)sampleWaveformLength / (44100.0f / toneFreq);


  // * Fill Audio Block with SampleWaveform Array Values *
  for (int i = 0; i < 128; i++) { // Teensy AUDIO_BLOCK_SAMPLES = 128


    // * Check/Update "Rectangle" (Pulse) Waveform (Duty Value) *
    int _duty = (((sidRegisters[lfoNr * 7 + 3] & 0xff) << 8) + (sidRegisters[lfoNr * 7 + 2] & 0xff)) / 40.96f;  
    if(_duty != duty) {      
      duty = _duty;
      createRectangleWave(duty);
    }    

    // * Use Waveforms *
    uint8_t waveformType = (sidRegisters[lfoNr * 7 + 4] & 0xff);

    if((waveformType & 16) == 16)         audioBlockData[i] = triangleWaveform[(int)sampleWaveformPositionPointer] * adsr.volume;
    else if((waveformType & 32) == 32)    audioBlockData[i] = sawtoothWaveform[(int)sampleWaveformPositionPointer] * adsr.volume;
    else if((waveformType & 64) == 64)    audioBlockData[i] = rectangleWaveform[(int)sampleWaveformPositionPointer] * adsr.volume;
    // Noise with Array is not working, so here directly rand()
    else if((waveformType & 128) == 128)  audioBlockData[i] = random(-32766,32766) * adsr.volume;  // rand()  is also not working!
    else                                  audioBlockData[i] = 0;

    
    sampleWaveformPositionPointer += sampleSteps;

    // * if the end was reached (>256), begin from the start (but not 0! ... we have to stay on the right Step Size, so -256) *
    if (sampleWaveformPositionPointer > sampleWaveformLength) sampleWaveformPositionPointer -= sampleWaveformLength; 
  }
}




// **********************
// * Generate Waveforms *
// **********************


void LFO::createTriangleWave() {

  int len = sampleWaveformLength/4;

  float steps = (float)32766/len;
  int level = 0;
  for (int i = 0; i < len; i++)  {
    triangleWaveform[i] = level;
    level +=steps;
  }
  for (int i = 0; i < len*2; i++) {
    triangleWaveform[len+i] = level;
    level -=steps;
  }
  for (int i = 0; i < len; i++)  {
    triangleWaveform[len*3+i] = level;  
    level +=steps;
  }
}

void LFO::createSawtoothWave() {

  int steps = 65532/sampleWaveformLength;
  int level   = 32766;  
  for (int i = 0; i < sampleWaveformLength; i++) {   
    sawtoothWaveform[i] = level;
    level -= steps;
  }
}

void LFO::createRectangleWave(int _duty) {

  duty = _duty;

  int firstLen = ((float)sampleWaveformLength)*((float)duty/100.0f);
  for (int i = 0; i < firstLen; i++) rectangleWaveform[i] = 32765;
 
  int lastLen = sampleWaveformLength-firstLen;  
  for (int i = 0; i < lastLen; i++) rectangleWaveform[firstLen+i] = -32765;
}

void LFO::createNoiseWave() {
  for (int i = 0; i < sampleWaveformLength; i++) noiseWaveform[i] = (rand() * 65532) - 32766; 
}

void LFO::createSineWave() {

  for (int i = 0; i < sampleWaveformLength; i++) {
    float v = ((360.0f / sampleWaveformLength) * i) * 3.14159265f / 180.0f; // making deg in radians
    int level = (int)(sin(v) * 32765);
    sineWaveform[i] = level;
  }
}
