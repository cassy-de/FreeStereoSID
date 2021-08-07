
#ifndef _lfo_
#define _lfo_

#define sampleWaveformLength 256


#include "ADSR.h"
#include <stdint.h>

class LFO {

  public:
    LFO(void) {
      createTriangleWave();
      createSawtoothWave();
      createRectangleWave(50);
      createNoiseWave();
      createSineWave();      
    }

    void oscillate(int lfoNr, uint8_t reg[]);

    int32_t audioBlockData[128];
   
  private:

    float sampleWaveformPositionPointer;
    float sampleSteps;   
    float toneFreq  = 0.0f;     // not initialized (no sound if no Frequency was set)
    uint8_t duty    = 50;       // 0...100
    
    int32_t triangleWaveform[sampleWaveformLength];
    int32_t sawtoothWaveform[sampleWaveformLength];
    int32_t rectangleWaveform[sampleWaveformLength];
    int32_t noiseWaveform[sampleWaveformLength];
    int32_t sineWaveform[sampleWaveformLength];
    
    void createTriangleWave();
    void createSawtoothWave();
    void createRectangleWave(int duty);
    void createNoiseWave();
    void createSineWave();  

    ADSR adsr; 
};

#endif
