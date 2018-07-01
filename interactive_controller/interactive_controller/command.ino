/*
 * Main program logic
 *   - Detects button operation to select addresses
 *   - Controls LEDs
 *   - Detects changes from sensor to create packets to send
 *   
 */

// ---------------------------------
// Constants
//

// Local constant - should match number of buttons
#define NUM_ADDRS 6

//
// Button channel state mask values
//
#define CMD_MASK_IDLE  0x00
#define CMD_MASK_COLOR 0x01
#define CMD_MASK_BEAT  0x02
#define CMD_MASK_BOTH  0x03

//
// LED colors
//
#define LED_BLACK  0
#define LED_RED    1
#define LED_GREEN  2 
#define LED_YELLOW 3

//
// dj2 color packet HSV flags (mask for each bit position)
//
#define HSV_FLAG_COLORVALUES  0x20
#define HSV_FLAG_DELTAVALUES  0x10
#define HSV_FLAG_VALID_HUE    0x08
#define HSV_FLAG_VALID_SAT    0x04
#define HSV_FLAG_VALID_VAL    0x02
#define HSV_FLAG_SCALE_V      0x01

//
// Beat pulse states
//
#define BEAT_IDLE    0
#define BEAT_FADE_UP 1
#define BEAT_FADE_DN 2

//
// Beat pulse brightness values
//
#define BEAT_UP_V 0x0100
#define BEAT_DN_V 0x0010

//
// Beat pulse fade times (in units of 100 mSec)
//
#define BEAT_FADE_T 1

//
// Beat timer preset (eval units) - Set this to 10 mSec less than fade time
//
//#define BEAT_TO (BEAT_FADE_T * 100 / EVAL_PERIOD_MSEC)
#define BEAT_TO (90 / EVAL_PERIOD_MSEC)

//
// Button Address assignment
//
struct AddrEntry {
  uint16_t z;
  uint16_t a2;
  boolean isUnit;
};

const AddrEntry buttonAddrs[NUM_ADDRS] = {
  {0, 1, false},
  {0, 2, false},
  {0, 50, false},
  {0, 51, false},
  {301, 0, true},
  {302, 0, true}
};


// ---------------------------------
// Variables
//
uint8_t commandState[NUM_ADDRS];
uint8_t commandStatePrev[NUM_ADDRS];
uint8_t commandBeatState[NUM_ADDRS];
uint8_t commandBeatTimers[NUM_ADDRS];


// ---------------------------------
// API
//

void InitCommand()
{
  uint8_t i;

  for (i=0; i<NUM_ADDRS; i++) {
    commandState[i] = CMD_MASK_IDLE;
    commandStatePrev[i] = CMD_MASK_IDLE;
    commandBeatState[i] = BEAT_IDLE;
    commandBeatTimers[i] = BEAT_TO;
  }
}


void EvalCommand()
{
    // Look for state changes based on button presses and update LEDs
    if (_CommandEvalButtons()) {
      _CommandUpdateLEDs();
      _CommandCheckBeatExit();
    }

    // Look for sensor activity and load packets
    _CommandEvalColor();
    _CommandEvalBeat();
}


// ---------------------------------
// Internal Routines
//

// Returns true if any button was detected and state updated
boolean _CommandEvalButtons()
{
  uint8_t i;
  boolean addrChange = false;

  for (int i=0; i<NUM_ADDRS; i++) {
    if (ButtonSingleClick(i+1)) {
      // Color
      commandState[i] ^= CMD_MASK_COLOR;
      addrChange = true;
    }
    if (ButtonDoubleClick(i+1)) {
      // Beat
      commandState[i] ^= CMD_MASK_BEAT;
      addrChange = true;
    }
  }

  return addrChange;
}


void _CommandUpdateLEDs()
{
  uint8_t i;
  uint8_t ledColor;
  
  for (i=0; i<NUM_ADDRS; i++) {
      switch (commandState[i]) {
        case CMD_MASK_IDLE:
          ledColor = LED_BLACK;
          break;
        case CMD_MASK_COLOR:
          ledColor = LED_GREEN;
          break;
        case CMD_MASK_BEAT:
          ledColor = LED_RED;
          break;
        case CMD_MASK_BOTH:
          ledColor = LED_YELLOW;
          break;
      }
      SetLed(i+1, ledColor);
    }
}


void _CommandEvalColor()
{
  uint8_t i;
  uint8_t flags = 0;
  uint16_t h, s, v;

  // Check touchpot for intensity changes
  if (GetTpChanged()) {
    v = GetTpValue();
    if (v == 255) v = 0x0100;  // Just a little hack to ensure absolute maximum brightness
    flags |= HSV_FLAG_VALID_VAL | HSV_FLAG_COLORVALUES;
  }

  // Check touchpad for color changes
  if (GetTouchColor(&h, &s)) {
    flags |= HSV_FLAG_VALID_HUE | HSV_FLAG_VALID_SAT | HSV_FLAG_COLORVALUES;
  }

  // Load packet(s) for any enabled address (we throw away packets if the fifo is already full)
  if (flags != 0) {
    for (i=0; i<NUM_ADDRS; i++) {
      if (((commandState[i] & CMD_MASK_COLOR) == CMD_MASK_COLOR) && Dj2Available()) {
        Dj2LoadSetHsv(buttonAddrs[i].z, buttonAddrs[i].a2, buttonAddrs[i].isUnit, flags, h, s, v);
      }
    }

    NoteUserActivity();
  }
}


void _CommandCheckBeatExit()
{
  uint8_t i;
  uint8_t f = HSV_FLAG_VALID_VAL | HSV_FLAG_COLORVALUES;
  
  for (i=0; i<NUM_ADDRS; i++) {
    if (((commandState[i] & CMD_MASK_BEAT) == 0x00) && ((commandStatePrev[i] & CMD_MASK_BEAT) == CMD_MASK_BEAT)) {
      // Channel exited beat mode so turn the lights associated with it back up
      Dj2LoadFadeHsv(buttonAddrs[i].z, buttonAddrs[i].a2, buttonAddrs[i].isUnit, f, 0, 0, BEAT_UP_V, BEAT_FADE_T);
    }

    commandStatePrev[i] = commandState[i];
  }
}


void _CommandEvalBeat()
{
  uint8_t i;
  uint8_t f = HSV_FLAG_VALID_VAL | HSV_FLAG_COLORVALUES;
  boolean beat = SensorBeat();

  for (i=0; i<NUM_ADDRS; i++) {
    if ((commandState[i] & CMD_MASK_BEAT) == 0x00) {
      // Skip
      continue;
    }
    switch (commandBeatState[i]) {
      case BEAT_IDLE:
        if (beat) {
          if (Dj2Available()) {
            // Start fade up
            Dj2LoadFadeHsv(buttonAddrs[i].z, buttonAddrs[i].a2, buttonAddrs[i].isUnit, f, 0, 0, BEAT_UP_V, BEAT_FADE_T);
            commandBeatTimers[i] = BEAT_TO;
            commandBeatState[i] = BEAT_FADE_UP;
          }
        }
        break;
      case BEAT_FADE_UP:
        if (--commandBeatTimers[i] == 0) {
          // Start fade down
          if (Dj2Available()) {
            Dj2LoadFadeHsv(buttonAddrs[i].z, buttonAddrs[i].a2, buttonAddrs[i].isUnit, f, 0, 0, BEAT_DN_V, BEAT_FADE_T);
            commandBeatTimers[i] = BEAT_TO;
            commandBeatState[i] = BEAT_FADE_DN;
          } else {
            // Reset in case we had to throw away this command
            commandBeatState[i] = BEAT_IDLE;
          }
        }
        break;
      case BEAT_FADE_DN:
        if (--commandBeatTimers[i] == 0) {
          // Done with this beat
          commandBeatState[i] = BEAT_IDLE;
        }
        break;
      default:
        commandBeatState[i] = BEAT_IDLE;
    }
  }

  if (beat) {
    NoteUserActivity();
  }
}


