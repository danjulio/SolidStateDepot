/*
 * Contains routines to send dj2 packets
 *  - Sets the dj2 RF IF to Fast/TX mode to enable maximum (~200) packets/sec (cannot have a repeater)
 *  - Sense CTSn from the RF IF to prevent overflowing its buffers
 *  - Implements a packet FIFO to allow controlling code to load multiple packets/event
 *
 * Implements the following packets
 *    Command 0x31 - FadeHSVUnit
 *    Command 0x33 - SetHSVUnit
 *    Command 0x22 - Set Mode 
 *
 */

// ---------------------------------
// Constants
//

//
// nCTS input
//
#define DJ2_CTS_PIN    4

//
// Max packets (allow for 6 color + 6 beat)
//
#define DJ2_MAX_PACKETS 12


//
// dj2 packet byte masks
//
#define DJ2_MARKB      0x20
#define DJ2_MARKE_MASK 0x30
#define DJ2_DATA_MASK  0x40

//
// dj2 Packet Command Bytes
//
#define DJ2_SETMODE      0x22
#define DJ2_FADEHSV_GRP  0x30
#define DJ2_FADEHSV_UNIT 0x31
#define DJ2_SETHSV_GRP   0x32
#define DJ2_SETHSV_UNIT  0x33
#define DJ2_SETRND       0x3B

//
// dj2 SetMode Mode Types
//
#define DJ2_MODE_OP      0x21


// ---------------------------------
// Variables
//

//
// Packet fifo
//
uint8_t dj2txFifo[DJ2_MAX_PACKETS][16];
uint8_t dj2txFifoPktLen[DJ2_MAX_PACKETS];
uint8_t dj2txFifoPushIndex = 0;
uint8_t dj2txFifoPopIndex = 0;
uint8_t dj2txFifoCount = 0;


// ---------------------------------
// API
//
void InitDj2() {
  Serial1.begin(57600);
  
  pinMode(DJ2_CTS_PIN, INPUT);
}


void EvalDj2() {
  boolean done;
  
  // Look for packets to push
  if (dj2txFifoCount != 0) {
    done = false;
    while (!done) {
      // Push as many packets as possible
      if (digitalRead(DJ2_CTS_PIN) == LOW) {
        // Load one packet
        _Dj2SendPkt();
        _Dj2IncPopIndex();

        // See if the fifo is empty now
        done = (dj2txFifoCount == 0);
      } else {
        // dj2 IF is full - done for now
        done = true;
      }
    }
  }
}


boolean Dj2Available() {
  return (dj2txFifoCount < DJ2_MAX_PACKETS);
}


void Dj2LoadMode(uint16_t z, uint16_t a2, uint8_t m, uint8_t v)
{
  uint8_t* pushPtr = &dj2txFifo[dj2txFifoPushIndex][0];

  // Command
  *pushPtr++ = DJ2_SETMODE;
  
  // Address
  *pushPtr++ = z >> 8;
  *pushPtr++ = z & 0xFF;
  *pushPtr++ = a2 >> 8;
  *pushPtr++ = a2 & 0xFF;

  // Mode
  *pushPtr++ = m;

  // Value
  *pushPtr++ = v;

  // Length
  dj2txFifoPktLen[dj2txFifoPushIndex] = 7;

  // Increment push pointer
  _Dj2IncPushIndex();
}


// h is 0 - 0x5FFF
// s is 0 - 0x0100
// v is 0 - 0x0100
// fT is units of 100 mSec
void Dj2LoadFadeHsv(uint16_t z, uint16_t a2, boolean isUnit, uint8_t f, uint16_t h, uint16_t s, uint16_t v, uint16_t fT) {
  uint8_t* pushPtr = &dj2txFifo[dj2txFifoPushIndex][0];

  // Command
  if (isUnit) {
    *pushPtr++ = DJ2_FADEHSV_UNIT;
  } else {
    *pushPtr++ = DJ2_FADEHSV_GRP;
  }
  
  // Address
  *pushPtr++ = z >> 8;
  *pushPtr++ = z & 0xFF;
  if (isUnit) {
    *pushPtr++ = a2 >> 8;
  }
  *pushPtr++ = a2 & 0xFF;
  
  // HSV Flags
  *pushPtr++ = f;
  
  // HSV
  *pushPtr++ = h >> 8;
  *pushPtr++ = h & 0xFF;
  *pushPtr++ = s >> 8;
  *pushPtr++ = s & 0xFF;
  *pushPtr++ = v >> 8;
  *pushPtr++ = v & 0xFF;
  
  // Fade Time
  *pushPtr++ = fT >> 8;
  *pushPtr++ = fT & 0xFF;
  
  // Length
  dj2txFifoPktLen[dj2txFifoPushIndex] = (isUnit) ? 14 : 13;

  // Increment push pointer
  _Dj2IncPushIndex();
}


// h is 0 - 0x5FFF
// s is 0 - 0x0100
// v is 0 - 0x0100
void Dj2LoadSetHsv(uint16_t z, uint16_t a2, boolean isUnit, uint8_t f, uint16_t h, uint16_t s, uint16_t v) {
  uint8_t* pushPtr = &dj2txFifo[dj2txFifoPushIndex][0];

  // Command
  if (isUnit) {
    *pushPtr++ = DJ2_SETHSV_UNIT;
  } else {
    *pushPtr++ = DJ2_SETHSV_GRP;
  }
  
  // Address
  *pushPtr++ = z >> 8;
  *pushPtr++ = z & 0xFF;
  if (isUnit) {
    *pushPtr++ = a2 >> 8;
  }
  *pushPtr++ = a2 & 0xFF;
  
  // HSV Flags
  *pushPtr++ = f;
  
  // HSV
  *pushPtr++ = h >> 8;
  *pushPtr++ = h & 0xFF;
  *pushPtr++ = s >> 8;
  *pushPtr++ = s & 0xFF;
  *pushPtr++ = v >> 8;
  *pushPtr++ = v & 0xFF;
  
  // Length
  dj2txFifoPktLen[dj2txFifoPushIndex] = (isUnit) ? 12 : 11;

  // Increment push pointer
  _Dj2IncPushIndex();
}
  


// ---------------------------------
// Internal Routines
//
void _Dj2SendPkt() {
  uint8_t* popPtr = &dj2txFifo[dj2txFifoPopIndex][0];
  uint8_t parity = 0;
  uint8_t c;
  
  Serial1.write(DJ2_MARKB);
  for (int i=0; i<dj2txFifoPktLen[dj2txFifoPopIndex]; i++) {
    c = *popPtr++;
    Serial1.write(DJ2_DATA_MASK | (c >> 4));
    parity ^= c >> 4;
    Serial1.write(DJ2_DATA_MASK | (c & 0x0F));
    parity ^= c & 0x0F;
  }
  Serial1.write(DJ2_MARKE_MASK | (parity & 0x0F));
}


void _Dj2IncPushIndex()
{
  if (++dj2txFifoPushIndex == DJ2_MAX_PACKETS) dj2txFifoPushIndex = 0;
  ++dj2txFifoCount;
}


void _Dj2IncPopIndex()
{
  if (++dj2txFifoPopIndex == DJ2_MAX_PACKETS) dj2txFifoPopIndex = 0;
  --dj2txFifoCount;
}

