//
// Code related to the rotary encoder
//

// Period, in mSec, for long-press detection
#define ROT_BUT_LONG_PERIOD     2000

// Period, in mSec, between debounce samples of the rotary encoder button
#define ROT_BUT_DEBOUNCE_PERIOD 10

//
// Internal variables
//
const int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
uint8_t old_AB = 0;
int8_t rotCount = 0;  // Contains the relative position since the last time the encoder was read (must be single-byte so it
                      // will be atomically used by the main code's call to GetRotaryCount)

boolean rotButtonDown = false;
boolean rotButtonPrev = false;
uint16_t rotButtonDownTimer = 0;
uint16_t rotButtonEvalTimer = 0;
boolean sawShortPress = false;
boolean sawLongPress = false;

uint8_t rotLedColor = ROT_LED_OFF;


//
// Rotary Control API routines
//
void InitRotaryEncoder() {
  // Initialize our IO
  pinMode(ROT_A, INPUT);
  pinMode(ROT_B, INPUT);
  pinMode(ROT_BUTTON_N, INPUT);
  pinMode(ROT_GREEN, OUTPUT);
  digitalWrite(ROT_GREEN, LOW);
  pinMode(ROT_RED, OUTPUT);
  digitalWrite(ROT_RED, LOW);
}


// Designed to be called on 1 mSec intervals
void EvalRotaryEncoder() {
  _EvalRotaryEnc();
  _EvalRotaryBut();
}


int8_t GetRotaryCount() {
  int8_t c = rotCount;
  
  rotCount = 0;
  return c;
}


int GetRotaryButton() {
  int v = ROT_BUT_NONE;
  
  if (sawShortPress) {
    v = ROT_BUT_SHORT;
    sawShortPress = false;
  } else if (sawLongPress) {
    v = ROT_BUT_LONG;
    sawLongPress = false;
  }
  
  return v;
}


void SetRotaryLed(uint8_t c) {
  if (c != rotLedColor) {
    rotLedColor = c;
    switch (c) {
      case ROT_LED_GREEN:
        digitalWrite(ROT_GREEN, HIGH);
        digitalWrite(ROT_RED, LOW);
        break;
        
      case ROT_LED_RED:
        digitalWrite(ROT_GREEN, LOW);
        digitalWrite(ROT_RED, HIGH);
        break;
        
      case ROT_LED_YELLOW:
        digitalWrite(ROT_GREEN, HIGH);
        digitalWrite(ROT_RED, HIGH);
        break;
        
      default: // off
        digitalWrite(ROT_GREEN, LOW);
        digitalWrite(ROT_RED, LOW);
        break;
    }
  }
}


uint8_t GetRotaryLed() {
  return rotLedColor;
}



//
// Rotary Control Internal routines
//
void _EvalRotaryEnc() {
  uint8_t curMask = 0x00;
  
  // Get current values
  //if (digitalRead(ROT_A) == HIGH) curMask |= 0x02;
  //if (digitalRead(ROT_B) == HIGH) curMask |= 0x01;
  curMask = PIND & 0x03; // ROT_B is D[0], ROT_A is D[1]
  
  // Based on Oleg Mazurov's code - read at 1 mSec intervals
  old_AB <<= 2;   // remember previous state
  old_AB |= curMask;
  rotCount += enc_states[(old_AB & 0x0F)];
}


void _EvalRotaryBut() {
  boolean curBut;
  boolean keyPressed;
  boolean keyReleased;
  
  if (++rotButtonEvalTimer == ROT_BUT_DEBOUNCE_PERIOD) {
    rotButtonEvalTimer = 0;
    curBut = digitalRead(ROT_BUTTON_N) == 0;
    
    // Compute buttons state
    keyPressed = curBut && rotButtonPrev && !rotButtonDown;
    keyReleased = rotButtonDown && !curBut && !rotButtonPrev;
    if (curBut && rotButtonPrev) rotButtonDown = true;
    if (!curBut && !rotButtonPrev) rotButtonDown = false;
    rotButtonPrev = curBut;
    
    // Evaluate button down timer
    if (keyPressed) {
      rotButtonDownTimer = 0;
    } else if (rotButtonDown) {
      if (rotButtonDownTimer < (ROT_BUT_LONG_PERIOD / ROT_BUT_DEBOUNCE_PERIOD)) {
        if (++rotButtonDownTimer == (ROT_BUT_LONG_PERIOD / ROT_BUT_DEBOUNCE_PERIOD)) {
          sawShortPress = false;
          sawLongPress = true;
        }
      }
    } else if (keyReleased) {
      if (rotButtonDownTimer < (ROT_BUT_LONG_PERIOD / ROT_BUT_DEBOUNCE_PERIOD)) {
        sawShortPress = true;
        sawLongPress = false;
      }
    }
  }
}
