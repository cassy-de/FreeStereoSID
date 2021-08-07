
#ifndef _adsr_
#define _adsr_


#include <stdint.h>


class ADSR {

  public:
    ADSR(void) {

    }

    void calc(uint8_t lfoNr, uint8_t sidRegisters[]);

    float volume                = 1.0f;

    int32_t attackTime          = 0;
    int32_t decayTime           = 0;
    float sustainLevel          = 0;
    int32_t releaseTime         = 0;


  private:

    bool debug = false;

    void soundOn(uint8_t lfoNr, uint8_t sidRegisters[]);
    void soundOff();
    void doADSR(uint8_t lfoNr, uint8_t sidRegisters[]);


    // * Rising/Falling Times in Milliseconds *
    uint32_t attackTimes[16]         = {3, 8, 16, 24, 38, 56, 68, 80, 100, 240, 500, 800, 1000, 3000, 5000, 8000};
    uint32_t decayTimes[16]          = {6, 24, 48, 72, 114, 168, 204, 240, 300, 750, 1500, 2400, 3000, 9000, 15000, 24000};
    uint32_t releaseTimes[16]        = {6, 24, 48, 72, 114, 168, 204, 240, 300, 750, 1500, 2400, 3000, 9000, 15000, 24000};

    
    float adsrTimeCounterMillis = 0;
        
    int8_t off                 = -1;
    int8_t attack              = 0;
    int8_t decay               = 1;
    int8_t release             = 2;
    int8_t adsr                = off;

    bool releaseFlag            = false;
    

    float attackRisingSteps       = 0;
    float decayFallingSteps       = 0;
    float releaseFallingSteps     = 0;

};

#endif
