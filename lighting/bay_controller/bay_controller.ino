// SSD Lighting Control program
//
// Designed to run on an Arduino Leonardo.  Implements a basic control panel for the dj2-based lighting system installed
// at Solid State Depot.  Controls both white and color-wash lighting with special buttons dedicated to turning on/off the
// bay white lighting (as well as setting them to classroom mode).  Supports:
//   - Modified dj2_sc-based touch control for color wash, intensity and random-program initiation.  The dj2_sc firmware
//     has been modified to pass all packets to its serial interface for manipulation by this firwmare.  This firmware passes
//     packets to the dj2_sc serial RX for transmission out the RF.
//   - Rotary encoder to select between addresses in a set
//   - Rotary encoder button to select between address sets
//   - Rotary encoder red/green LEDs to display mode information
//   - DOG-M 2x16 LCD display for address set/address selection display
//   - 6 active low trigger outputs for a sound generator
//
//  IO Assignments
//    D0 (TX) : TX to Remote Controller RX
//    D1 (RX) : RX from Remote Controller TX
//    D2 (input): From Rotary Encoder Channel A (Leonardo ATmega 32U4 PORTD[1])
//    D3 (input): From Rotary Encoder Channel B (Leonardo ATmega 32U4 PORTD[0])
//    D4 (output) : Active Low Sound trigger 1
//    D5 (output) : Active Low Sound trigger 2
//    D6 (output) : Active High Rotary Encoder Green LED
//    D7 (output) : Active High Rotary Encoder Red LED
//    D8 (input) : Active Low Rotary Encoder Button input
//    D9 (output): DOG-M LCD RS
//    D10 (output): DOG-M LCD CSB
//    D14 (input): Unused, but reserved for hardware SPI MISO
//    D15 (output): DOG-M LCD CLK
//    D16 (output): DOG-M LCD SI (MOSI)
//    A0 (D18) (output): Active Low Sound trigger 3
//    A1 (D19) (output): Active Low Sound trigger 4
//    A2 (D20) (output): Active Low Sound trigger 5
//    A3 (D21) (output): Active Low Sound trigger 6
//
// Notes
//   1. Rotary Encode Channels pulled up by 10kohm resistors with 0.1 uF debounce caps to ground
//
#include <DogLcd.h>
#include <TimerOne.h>
#include <avr/pgmspace.h>


//
// IO Port Assignments
//
#define ROT_A          2
#define ROT_B          3
#define SOFT_TX        5
#define SOFT_RX        11
#define ROT_GREEN      6
#define ROT_RED        7
#define ROT_BUTTON_N   8
#define DOG_RS         9
#define DOG_CSB        10
#define DOG_CLK        15
#define DOG_SI         16
#define SOUND_1_PIN    4
#define SOUND_2_PIN    5
#define SOUND_3_PIN    18
#define SOUND_4_PIN    19
#define SOUND_5_PIN    20
#define SOUND_6_PIN    21


//
// Rotary Encoder Button values
//
#define ROT_BUT_NONE   0
#define ROT_BUT_SHORT  1
#define ROT_BUT_LONG   2

//
// Rotary Encoder LED settings
//
#define ROT_LED_OFF    0
#define ROT_LED_GREEN  1
#define ROT_LED_RED    2
#define ROT_LED_YELLOW 3

//
// Inactivity Timeout (seconds) - causes us to return our user interface to a default setup
//
#define INACTIVITY_TIMEOUT 60*60*4

//
// Object instantiations
//
DogLcd lcd(DOG_SI, DOG_CLK, DOG_RS, DOG_CSB);


//
// Global variables
//
boolean timer10MsecTick;
uint8_t timer10MsecCount;
boolean Tick10Msec;  // For use by evaluation routines

uint8_t timerTickCount;
boolean TickSec;

uint16_t inactivityTimer;  // Counts seconds since last user interaction



void setup() {
  InitRotaryEncoder();
  InitAddress();
  InitDj2();
  InitEffects();
  InitSound();
  
  Timer1.initialize(); // 1 mSec evaluation periods
  Timer1.attachInterrupt(evalTimerHandler, 1000);
  timer10MsecTick = false;
  timer10MsecCount = 0;
  timerTickCount = 0;
  inactivityTimer = INACTIVITY_TIMEOUT;  // Start inactivity timeout upon first user interaction
  
  // Diagnostic output
  Serial.begin(9600);
}


void loop() {
  // Setup "time" flags for this evaluation
  if (timer10MsecTick) {
    Tick10Msec = true;
    timer10MsecTick = false;
    if (++timerTickCount == 100) {
      timerTickCount = 0;
      TickSec = true;
    }
  } else {
    Tick10Msec = false;
    TickSec = false;
  }
  
  // Evaluate our state
  EvalAddress();
  
  if (Dj2ValidRxPkt()) {
    EvalDj2();
  }
  
  if (Tick10Msec) {
    EvalEffects();
    EvalSound();
  }
  
  if (TickSec) {
    EvalInactivityTimeout();
  }

}


void evalTimerHandler() {
  // Rotary encoder is evaluated every 1 mSec
  EvalRotaryEncoder();
  
  // Everything else is at least 10 mSec
  if (++timer10MsecCount == 10) {
    timer10MsecTick =  true; // consumed and cleared in main loop
    timer10MsecCount = 0;
  }
}


void serialEvent1() {
  while (Serial1.available()) {
    Dj2ProcessRxChar(Serial1.read());
  }
}


void NoteUserActivity() {
  inactivityTimer = 0;
}


void EvalInactivityTimeout() {
  if (inactivityTimer < INACTIVITY_TIMEOUT) {
    if (++inactivityTimer == INACTIVITY_TIMEOUT) {
      SetDefaultAddress();
    }
  }
}

