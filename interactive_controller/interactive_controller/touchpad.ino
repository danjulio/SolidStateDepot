/*
 * Cirque TSM9957 PS/2 compatible touchpad strapped for extended mode (outputs absolute X/Y coordinates).
 * Implements software UART based on PS/2 CLK falling edges.
 *
 * Packet Layout:
 *   Byte 1: Bit 7 = 1, Bit 6:0 = TpX[6:0]
 *   Byte 2: Bit 7 = 1, Bit 6:0 = TpY[6:0]
 *   Byte 3: Bit 7 = 1, Bit 6:3 = TpX[10:7], Bit 2 = Middle Button, Bit 1 = Right, Bit 0 = Left
 *   Byte 4: Bit 7 = 1, Bit 6:3 = TpY[10:7], Bit 2:0 reserved
 *   Byte 5: Bit 7 = 0, Bit 6:0 = TpZ[6:0]
 *
 * Notes:
 *  1. Touchpad outputs one 5-byte packet every 10 mSec (packet period ~5.15 mSec)
 *  2. Clock period is 78 uSec (37 uSec high/41 uSec low) -> 12.82 kbaud
 *  3. There is about 15 uSec setup of data to clock falling edge
 *  4. Clock high period between bytes in a packet is about 311 uSec
 *  5. Typical values
 *     Lower-right: (70, 70)
 *     Upper-left: (1940, 1440)
 *
 * Hue/Saturation
 *  Hue mapped to X range 256 - 1791 (values above and below are clipped)\
 *     L->R : R -> Y -> G -> C -> B -> M -> R (0x0000 - 0x05FF)
 *  Sat mapped to Y range 256 - 1279 (values above and below are clipped)
 *     Bot -> Top : Full Color -> White (0x0100 - 0x0000)
*/

// ---------------------------------
// Constants
//

//
// Pins
//
#define TPAD_CLK 7
#define TPAD_DAT 8

//
// Minimum good touch (Z) value
//
#define Z_THRESHOLD 10

//
// Minimum number of samples to average
//
#define TP_MIN_SAMPLES 2

//
// Receiver states
//
#define TPRX_IDLE   0
#define TPRX_DATA   1
#define TPRX_PARITY 2
#define TPRX_STOP   3

//
// Receiver inner-packet, inner-byte timeouts (uSec)
//
#define TPRX_BIT_TO 150

//
// Fifo depth - should be sized large enough to receive all packets between sample periods
// and be a multiple of 5 bytes
//
#define TP_FIFO_DEPTH (4*5)


// ---------------------------------
// Variables
//

// Receiver
volatile uint8_t tprxState = TPRX_IDLE;
volatile uint8_t tprxBit = 0;
volatile uint8_t tprxData = 0;
volatile boolean tprxCurParity;
volatile uint32_t tprxCurClkT;
volatile uint32_t tprxPrevClkT;
// Packet processor
volatile uint8_t tpPktByteNum = 0;
volatile uint8_t tpPktBuf[5];
volatile boolean tpPktInvalid = false;

// Data Fifo - always loaded in complete 5-byte packets
volatile uint8_t tpFifo[TP_FIFO_DEPTH];
volatile uint8_t tpPushIndex = 0;
uint8_t tpPopIndex = 0;

// Averaging and change detection buffer
uint16_t tpXAvgBuf[NUM_SAMPLE_PERIODS];
uint16_t tpYAvgBuf[NUM_SAMPLE_PERIODS];
uint8_t tpAvgIndex = 0;


// ---------------------------------
// API
//

void InitTouchPad()
{
  // External device will drive both data and clock
  pinMode(TPAD_CLK, OUTPUT);
  digitalWrite(TPAD_CLK, HIGH);  // Force clock high initially
  pinMode(TPAD_CLK, INPUT_PULLUP);
  pinMode(TPAD_DAT, INPUT_PULLUP);

  // Initialize timer
  tprxPrevClkT = micros();

  // Setup interrupt handler
  attachInterrupt(digitalPinToInterrupt(TPAD_CLK), _TouchpadIsr, FALLING);
}

void EvalTouchPad()
{
  uint16_t x, y;

  if (_GetTouch(&x, &y)) {
    // Add to our averaging buffer
    tpXAvgBuf[tpAvgIndex] = x;
    tpYAvgBuf[tpAvgIndex] = y;
    if (tpAvgIndex < NUM_SAMPLE_PERIODS-1) tpAvgIndex++;

    //if (_ValidTouchAverage(&x, &y)) {
    //    Serial.print(x); Serial.print(" "); Serial.println(y);
    //}
  } else {
    // Hold in reset
    tpAvgIndex = 0;
  }
}


boolean GetTouchColor(uint16_t* h, uint16_t* s)
{
  uint16_t x, y;

  if (_ValidTouchAverage(&x, &y)) {
    // Hue
    if (x < 256) {
      *h = 0x05FF;
    } else if (x >= 1792) {
      *h = 0x0000;
    } else {
      *h = 0x05FF - (x - 256);
    }

    // Sat
    if (y < 256) {
      *s = 0x0100;
    } else if (y >= 1280) {
      *s = 0x0000;
    } else {
      *s = 0x0100 - ((y - 256)/4);
    }

    return true;
  } else {
    return false;
  }
}


// ---------------------------------
// Internal Routines
//

// Falling clock edge ISR
void _TouchpadIsr()
{
  boolean curBit;

  // Get the time of this edge
  tprxCurClkT = micros();

  // Get the data bit
  curBit = digitalRead(TPAD_DAT) == HIGH;

  switch (tprxState) {
    case TPRX_IDLE:
      // Make sure this is the start of a byte
      if (_DeltaTimeGreater(TPRX_BIT_TO)) {
        if (!curBit) {
          // Saw low start bit --> setup to receive a byte
          tprxBit = 0;
          tprxData = 0;
          tprxCurParity = true;  // Parity starts high for correct odd-parity calculation
          tprxState = TPRX_DATA;
        }
      }
      break;

    case TPRX_DATA:
      if (_DeltaTimeGreater(TPRX_BIT_TO)) {
        // Time between clocks too long, must have missed an edge (shouldn't happen)
        tprxState = TPRX_IDLE;
      } else {
        // Store data bit
        tprxData = tprxData >> 1;  // Make room
        if (curBit) {
          tprxData |= 0x80;
          tprxCurParity = !tprxCurParity;  // Flip parity for each "1" bit
        }

        // Check if we are done
        if (++tprxBit == 8) {
          tprxState = TPRX_PARITY;
        }
      }
      break;

    case TPRX_PARITY:
      if (_DeltaTimeGreater(TPRX_BIT_TO)) {
        // Time between clocks too long, must have missed an edge (shouldn't happen)
        tprxState = TPRX_IDLE;
      } else {
        // Check parity against calculated
        if (tprxCurParity == curBit) {
          // Good Byte - load it in the current packet buffer and validate
          if (!tpPktInvalid) {
            tpPktBuf[tpPktByteNum] = tprxData;
          }

          // Look for a complete packet
          if ((tprxData & 0x80) == 0x00) {
            // This is the last byte of a packet, make sure we're in sync
            if (tpPktByteNum == 4) {
              // Store this complete packet
              for (uint8_t i = 0; i < 5; i++) {
                tpFifo[tpPushIndex] = tpPktBuf[i];
                if (++tpPushIndex == TP_FIFO_DEPTH) tpPushIndex = 0;
              }
            }
            // Reset our packet index at end of each packet
            tpPktByteNum = 0;
            tpPktInvalid = false;
          } else {
            // Make sure this is not the final expected byte (because it has bit 7 set)
            if (tpPktByteNum == 4) {
              // Setup to skip all bytes until we see the end of a packet to resync
              tpPktInvalid = true;
            } else {
              // Byte ok, setup for next byte
              ++tpPktByteNum;
            }
          }
        } else {
          // Bad parity - set to resync at next valid end-of-packet
          tpPktInvalid = true;
        }
        tprxState = TPRX_STOP;
      }
      break;

    case TPRX_STOP:
      tprxState = TPRX_IDLE;
      break;

    default:
      tprxState = TPRX_IDLE;
  }

  // Save this bit time for the next clock edge
  tprxPrevClkT = tprxCurClkT;
}


boolean _GetTouch(uint16_t* x, uint16_t* y)
{
  uint8_t packet[5];
  uint8_t i;

  // Unload complete packet
  for (i = 0; i < 5; i++) {
    if (tpPopIndex != tpPushIndex) {
      packet[i] = tpFifo[tpPopIndex];
      if (++tpPopIndex == TP_FIFO_DEPTH) tpPopIndex = 0;
    } else {
      // Ran out of data in fifo!  Should not happen
      return false;
    }
  }

  // Parse good data
  if (packet[4] < Z_THRESHOLD) {
    // No sufficient z touch
    return false;
  } else {
    *x = packet[0] & 0x7F;          // Bits 6:0
    *x |= (packet[2] & 0x78) << 4;  // Bits 10:7
    *y = packet[1] & 0x7F;
    *y |= (packet[3] & 0x78) << 4;
    return true;
  }
}


boolean _ValidTouchAverage(uint16_t* x, uint16_t* y)
{
  uint16_t sumX = 0;
  uint16_t sumY = 0;
  uint8_t i;
  
  if (tpAvgIndex < TP_MIN_SAMPLES) {
    return false;
  } else {
    for (i=0; i<= tpAvgIndex; i++) {
      sumX += tpXAvgBuf[i];
      sumY += tpYAvgBuf[i];
    }

    *x = sumX / i;
    *y = sumY / i;

    return true;
  }
}


boolean _DeltaTimeGreater(uint32_t deltaUsec)
{
  if (tprxCurClkT >= tprxPrevClkT) {
    return ((tprxCurClkT - tprxPrevClkT) >= deltaUsec);
  } else {
    return ((tprxCurClkT - tprxPrevClkT + 0xFFFFFFFF) >= deltaUsec);
  }
}



