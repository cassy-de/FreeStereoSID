
#include "FreeStereoSID.h"
#include<Arduino.h>

void FreeStereoSID::setSIDAddress(uint16_t sidAddr) {
  sidAddress = sidAddr;
}


// * AudioConnection streaming functions: begin(), reset(), stop(), update() *

void FreeStereoSID::begin(void) {

  for (int v = 0; v < 3; v++) lfos[v] = new LFO();
  playing = true;
}

void FreeStereoSID::update(void) {
  if (!playing) return;

  audio_block_t *audioBlock; // * Create a new Audio Block  (Teensy AUDIO_BLOCK_SAMPLES = 128, 2.9ms) *

  audioBlock = allocate();
  if (audioBlock == NULL) return;


  // * Oscillate the 3 LFOs (Voices) for 128 Steps *
  for (int v = 0; v < 3; v++) lfos[v]->oscillate(v, memory);


  // * Mix and Normalize the 3 LFOs *
  for (int i = 0; i < 128; i++) {

    // * Mix *
    float sum = (lfos[0]->audioBlockData[i] + lfos[1]->audioBlockData[i] + lfos[2]->audioBlockData[i]) / 3;
    
    sum *= (1.0f/15.0*(memory[24] & 0xf));  // Volume-Reg 24
            
    audioBlock->data[i] = sum;       
  }

  // * Send the Audio Block to the Audio Output *
  transmit(audioBlock);
  release(audioBlock);
}

void FreeStereoSID::reset(void) {

}

void FreeStereoSID::stop(void) {
  __disable_irq();
  playing = false;
  __enable_irq();
}

void FreeStereoSID::writeReg(uint16_t address, uint8_t data) {  
  memory[address] = data;
}
