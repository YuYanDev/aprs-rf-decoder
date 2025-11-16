/*
   RadioLib SX127x APRS Receive Example

   This example receives APRS messages using
   SX1278's FSK modem. The data is modulated
   as AFSK at 1200 baud using Bell 202 tones.

   DO NOT transmit in APRS bands unless
   you have a ham radio license!

   Other modules that can be used for APRS:
    - SX127x/RFM9x
    - RF69
    - SX1231
    - CC1101
    - nRF24
    - Si443x/RFM2x

   For default module settings, see the wiki page
   https://github.com/jgromes/RadioLib/wiki/Default-configuration

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// SX1278 has the following connections:
// NSS pin:   10
// DIO0 pin:  2
// RESET pin: 9
// DIO1 pin:  3
SX1278 radio = new Module(5, 2, 9, 3);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//SX1278 radio = RadioShield.ModuleA;

// create AFSK client instance using the FSK module
// pin 5 is connected to SX1278 DIO2
AFSKClient audio(&radio, 4);

// create AX.25 client instance using the AFSK instance
AX25Client ax25(&audio);

// create APRS client instance using the AX.25 client
APRSClient aprs(&ax25);

/*volatile bool gotSample = false;
volatile uint32_t rawSample = 0;
volatile byte lastSample = 0;
volatile int rawIndex = 0;
volatile int sampleIndex = 0;

void sample(void) {
  rawSample |= digitalRead(4) << rawIndex++;
  if(rawIndex > 3) {
    rawIndex = 0;
    if((rawSample & 0x0F) == 0b0011) {
      lastSample |= (0 << sampleIndex++);
    } else if((rawSample & 0x0F) == 0b0101) {
      lastSample |= (1 << sampleIndex++);
    } else {
      rawSample = 0;
    }
  }

  if(sampleIndex > 7) {
    sampleIndex = 0;
    gotSample = true;
  }
}*/

void setup() {
  Serial.begin(115200);

  // initialize SX1278 with default settings
  // NOTE: moved to ISM band on purpose
  //       DO NOT transmit in APRS bands without ham radio license!
  Serial.print(F("[SX1278] Initializing ... "));
  int state = radio.beginFSK(434.0, 26.4);

  // when using one of the non-LoRa modules for Morse code
  // (RF69, CC1101, Si4432 etc.), use the basic begin() method
  // int state = radio.begin();
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  radio.setOOK(true);
  //attachInterrupt(digitalPinToInterrupt(3), sample, RISING);

  // initialize AX.25 client
  Serial.print(F("[AX.25] Initializing ... "));
  // source station callsign:     "N7LEM"
  // source station SSID:         0
  // preamble length:             8 bytes
  //state = ax25.begin("N7LEM");
  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }

  // initialize APRS client
  Serial.print(F("[APRS] Initializing ... "));
  // symbol:                      '>' (car)
  //state = aprs.begin('>');
  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }

  //radio.setDirectSyncWord(0xD3333335, 32);
  //Serial.println(radio.setDirectSyncWord(0xACCCCCCA, 32));
  //radio.setDirectSyncWord(0xF0F0F0F0, 32);
  radio.setDirectSyncWord(0x3F03F03F, 32);

  // set function that will be called each time a bit is received
  radio.setDirectAction(readBit);

  // start direct mode reception
  radio.receiveDirect();
}

// this function is called when a new bit is received
void readBit(void) {
  // read the data bit
  radio.readBit(4);
}

#define MARK_SRC_LEN   11
#define SPAC_SRC_LEN   3
const uint8_t mark_i_src[MARK_SRC_LEN] = { 0xFF, 0xE0, 0x03, 0xFF, 0x80, 0x0F, 0xFE, 0x00, 0x3F, 0xF8, 0x00 };
const uint8_t mark_q_src[MARK_SRC_LEN] = { 0x0F, 0xFE, 0x00, 0x3F, 0xF8, 0x00, 0xFF, 0xE0, 0x03, 0xFF, 0x80 };
/*const uint8_t spac_i_src[SPAC_SRC_LEN] = { 0xFC, 0x0F, 0xC0 };
const uint8_t spac_q_src[SPAC_SRC_LEN] = { 0x1F, 0x81, 0xF8 };*/
uint32_t spac_i = 0xFC0FC0FC;
uint32_t spac_q = 0x1F81F81F;
uint8_t mark_i_pos = 0;
uint8_t mark_q_pos = 0;
uint8_t spac_i_pos = 0;
uint8_t spac_q_pos = 0;

uint32_t lastSamples = 0;
uint8_t sampleCnt = 0;

uint8_t popcnt(uint32_t n) {
  uint32_t val = n;
  uint8_t count = 0;
  while(val) {
    count += (val & 0x1);
    val >>= 1;
  }
  return(count);
}

uint32_t local_oscillator(uint8_t* src, uint8_t len, uint8_t* pos) {
  uint32_t res = 0;
  for(int8_t i = 3; i >= 0; i--) {
    res |= (uint32_t)src[*pos] << 8*i;
    (*pos)++;
    if((*pos) >= len) {
      (*pos) = 0;
    }
  }
  return(res);
}

int mavg_buff[22] = { 0 };
uint8_t mavg_pos = 0;
int movingAvg(int newSample) {
  // save the new sample
  mavg_buff[mavg_pos++] = newSample;

  // roll over
  if(mavg_pos >= 21) {
    mavg_pos = 0;
  }
  
  // do the average
  int sum = 0;
  for(uint8_t i = 0; i < 22; i++) {
    sum += mavg_buff[i];
  }
  return(sum/22);
}

uint8_t longestSequence(uint32_t sample, uint8_t numBits) {
  uint8_t prevBit = (sample & (uint32_t)1 << 31) >> 31;
  uint8_t len = 1;
  uint8_t maxLen = len;
  for(uint8_t i = 1; i < numBits; i++) {
    uint8_t newBit = (sample & (uint32_t)1 << (31 - i)) >> (31 - i);
    if(newBit == prevBit) {
      len++;
    } else {
      if(len > maxLen) {
        maxLen = len;
      }
      len = 1;
    }
    prevBit = newBit;
  }
  if(len > maxLen) {
    maxLen = len;
  }
  return(maxLen);
}

uint8_t longestSequence8(uint8_t sample) {
  uint8_t prevBit = (sample & (uint32_t)1 << 7) >> 7;
  uint8_t len = 1;
  uint8_t maxLen = len;
  for(uint8_t i = 1; i < 8; i++) {
    uint8_t newBit = (sample & (uint32_t)1 << (7 - i)) >> (7 - i);
    if(newBit == prevBit) {
      len++;
    } else {
      if(len > maxLen) {
        maxLen = len;
      }
      len = 1;
    }
    prevBit = newBit;
  }
  if(len > maxLen) {
    maxLen = len;
  }
  return(maxLen);
}

void demodulate(uint8_t sample) {
  // load the new sample into the buffer
  sampleCnt += 8;
  lastSamples <<= 8;
  lastSamples |= (uint32_t)sample;
  char buff[128];
  sprintf(buff, "x=%02X X=%08lX Sc=%d", sample, lastSamples, sampleCnt);
  Serial.println(buff);
  //uint32_t int_mask = 0xFFFFFC00;

  if(sampleCnt >= 22) {
    /*for(uint8_t i = 0; i < 8; i++) {
      uint32_t int_samp = int_mask & (lastSamples << i);
      uint8_t len = longestSequence(int_samp, 22);
      sprintf(buff, "Xi=%08lX l=%d", int_samp, len);
      Serial.println(buff);
    }*/
    uint32_t mask = 0xFFFFFC00 >> (32 - sampleCnt);
    uint32_t proc = (lastSamples & mask) << (32 - sampleCnt);
    uint8_t len = longestSequence(proc, 22);
    lastSamples &= ~(mask);
    sampleCnt-=22;
    sprintf(buff, "M=%08lX Xi=%08lX l=%d", mask, proc, len);
    Serial.println(buff);
    Serial.println("----------------------------");
    
    // start by building the correlator waveforms
    /*uint32_t mark_i = local_oscillator(mark_i_src, MARK_SRC_LEN, &mark_i_pos);
    uint32_t mark_q = local_oscillator(mark_q_src, MARK_SRC_LEN, &mark_q_pos);
    //uint32_t spac_i = local_oscillator(spac_i_src, SPAC_SRC_LEN, &spac_i_pos);
    //uint32_t spac_q = local_oscillator(spac_q_src, SPAC_SRC_LEN, &spac_q_pos);

    // enough bits in buffer, correlate first
    uint32_t corr_mark_i = ~(lastSamples ^ mark_i);
    uint32_t corr_mark_q = ~(lastSamples ^ mark_q);
    uint32_t corr_spac_i = ~(lastSamples ^ spac_i);
    uint32_t corr_spac_q = ~(lastSamples ^ spac_q);

    // now integrate
    for(uint8_t i = 0; i < 8; i++) {
      uint32_t int_mask = 0xFFFFFC00 >> i;
      uint8_t int_mark_i = popcnt(corr_mark_i & int_mask);
      uint8_t int_mark_q = popcnt(corr_mark_q & int_mask);
      uint8_t int_spac_i = popcnt(corr_spac_i & int_mask);
      uint8_t int_spac_q = popcnt(corr_spac_q & int_mask);

      // sum the squares and return the result
      int sum_mark = int_mark_i + int_mark_q;
      int sum_spac = int_spac_i + int_spac_q;
      int res = sum_mark - sum_spac;
  
      char buff[128];
      sprintf(buff, "M=%08lX|%08lX S=%08lX|%08lX",
        mark_i, mark_q, spac_i, spac_q);
      Serial.println(buff);
      sprintf(buff, "X=%08lX C=%08lX|%08lX,%08lX|%08lX", 
        lastSamples, corr_mark_i & int_mask, corr_mark_q & int_mask, corr_spac_i & int_mask, corr_spac_q & int_mask);
      Serial.println(buff);
      sprintf(buff, "Im=%08lX I=%2d|%2d,%2d|%2d S=%2d,%2d Y=%d\n",
        int_mask, int_mark_i, int_mark_q, int_spac_i, int_spac_q, sum_mark, sum_spac, res);
      Serial.println(buff);
    }
    Serial.println("----------------------------");

    spac_i = ~spac_i;
    spac_q = ~spac_q;
    mark_i_pos -= 3;
    if(mark_i_pos < 0) {
      mark_i_pos += MARK_SRC_LEN;
    }
    mark_q_pos -= 3;
    if(mark_q_pos < 0) {
      mark_q_pos += MARK_SRC_LEN;
    }*/
  }
}

uint8_t prevBit = 0;
uint8_t pulseLen = 0;
uint8_t bitTimer = 0;
uint8_t cntMark = 0;
uint8_t cntSpac = 0;
void demodulate2(uint8_t sample) {
  char buff[128];
  /*sprintf(buff, "X=%02X", sample);
  Serial.println(buff);*/
  for(int8_t i = 7; i >= 0; i--) {
    uint8_t currBit = (sample & (uint32_t)1 << i) >> i;
    if(currBit == prevBit) {
      pulseLen++;
    } else {
      if(pulseLen <= 8) {
        cntSpac++;
      } else {
        cntMark++;
      }
      pulseLen = 1;
    }
    prevBit = currBit;

    bitTimer++;
    if(bitTimer >= 22) {
      uint8_t bitval = 1;
      if(cntMark < cntSpac) {
        bitval = 0;
      }
      
      //sprintf(buff, "M=%02d S=%02d, l=%02d, b=%d", cntMark, cntSpac, pulseLen, bitval);
      //Serial.print(buff);
      Serial.print(bitval);
      cntMark = 0;
      cntSpac = 0;
      bitTimer = 0;
    }
  }
}

void demodulate3(uint8_t sample) {
  
}

uint8_t prev = 0;
uint8_t ctr = 0;

void loop() {
  /*if(radio.available() > 176) {
    uint32_t symbol = 0;
    int8_t bitPos = 22 - 8;
    while(radio.available()) {
      uint8_t b = radio.read();
      Serial.println(b, HEX);

      if(bitPos < 0) {
        symbol |= (uint32_t)b >> (-1*bitPos);
        Serial.println(symbol & 0x3FFFFF, BIN);
        symbol = 0;
        bitPos += 22;
      }
      
      symbol |= (uint32_t)b << bitPos;
      bitPos -= 8;
      
      //Serial.println(radio.read(), HEX);
    }
  }*/

  //uint8_t prevBit = 0;
  if(radio.available() > 22) {
    radio.standby();
    while(radio.available()) {
      uint8_t b = radio.read();
      /*uint8_t len = longestSequence8(b);
      uint8_t newBit = 0;
      if(len == 6) {
        newBit = 0;
      } else if(len == 8) {
        newBit = 1;
      } else {
        newBit = prevBit;
      }
      Serial.println(newBit);
      prevBit = newBit;*/
      //Serial.println(b, HEX);
      demodulate2(b);
    }
    radio.receiveDirect();
  }

  
  /*while(!radio.available()) {
    //delay(1);
  }
  
  uint8_t decoded = 0;
  for(uint8_t j = 0; j < 8; j++) {
    uint8_t b = radio.read(false);
    Serial.println(b, HEX);
    int8_t sum = 0;
    for(uint8_t i = 0; i < 8; i++) {
      uint8_t sample = (prev << (i + 1)) | (b >> (7 - i));
      sum += demodulate(sample, i);
    }
    bool bit = (sum < 0);
    //Serial.println(bit, HEX);
    decoded |= (bit << (7 - j));
    prev = b;
    
    while(!radio.available()) {
      delay(1);
    }
  }

  //Serial.println(decoded, HEX);
  ctr++;

  if(ctr >= 20) {
    while(true);
  }*/
}
