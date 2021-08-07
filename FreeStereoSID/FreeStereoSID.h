
#ifndef _free
#define _free


#include <EEPROM.h>
#include "LFO.h"
#include <AudioStream.h>

class FreeStereoSID : public AudioStream {

  public:
  
    FreeStereoSID(void) : AudioStream(0, NULL) { begin(); }

    void setSIDAddress(uint16_t sidAddr);    
    void writeReg(uint16_t address, uint8_t data);

    int  sidAddress;
    
  private:
    
    void begin(void);
    void update(void);  
    void reset(void);
    void stop(void);

    bool playing   = false; 

    uint8_t memory[0x1F];
    LFO* lfos[3];     
};

#endif
