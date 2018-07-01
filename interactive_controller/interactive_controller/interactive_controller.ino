/*
 * LED lighting controller
 *   - 6 buttons
 *   - 6 bi-color (red/green) LEDs
 *   - Cirque TSM9957 Touchpad in extended mode for HSV control
 *   - djd Touch pot for intensity control
 *   - VL6180X distance sensor for beat control
 *   - dj2 RF 2 Interface (Host mode/TX only)
 *   
 * Run on an Arduino Leonardo clone (Pro Micro)
 *   Pin   Assignment
 *   0     dj2 RF IF RX
 *   1     dj2 RF IF TX (unused)
 *   2     SDA
 *   3     SCL
 *   4     dj2 RF nCTS Input
 *   5     Red LED anode (connected to all 6 LEDs) - PWM capable
 *   6     Green LED anode (connected to all 6 LEDs) - PWM capable
 *   7     Touchpad CLK (internal pullup)
 *   8     Touchpad DAT (internal pullup)
 *   9     LED 1 cathode (through series R)
 *   10    LED 2 cathode (through series R)
 *   14    LED 3 cathode (through series R)
 *   15    LED 4 cathode (through series R)
 *   16    LED 5 cathode (through series R)
 *   18    LED 6 cathode (through series R)
 *   A0    Button 1-2 analog input
 *   A1    Button 3-4 analog input
 *   A2    Button 5-6 analog input
 *  
 * Button Signaling is analog (yeah..I shoulda picked better resistor values...)
 *         VCC 
 *          |
 *          +
 *         | |
 *         | | 1 k-ohm
 *         | |
 *          +
 *          |
 *      ----+---+------> Analog Input
 *      |       |
 *       \ S1    \ S2
 *      |       |
 *      +       +
 *     | |     | |
 *     | |1k   | | 10k
 *     | |     | |
 *      +       +
 *      |       |
 *      |       |
 *     GND     GND
 *     
 *  Sense input voltage
 *    Near 1.0 for both open -> near 1023 counts
 *    Near 0.5 for S1 closed -> near 512 counts
 *    Near 0.91 for S2 closed -> near 930 counts
 *    Near 0.476 for both closed -> near 487 counts
 *  
 */
#include <TimerOne.h>
#include <Wire.h>
#include <VL6180X.h>

// ---------------------------------
// Constants
//

//
// Evaluation interval (mSec) - sets the basic time unit for all aspects of program evaluation
//
#define EVAL_PERIOD_MSEC 10

//
// Eval periods per transmitted packet
//  This period, in multiples of EVAL_PERIOD_MSEC, tells us how many samples from each sensor to
//  average together to match the maximum data rate our transmitter can send (200 pkts/sec in Fast-Tx).
//  Since we have two sources of data streams that can have up to 6 packets per sample then we need
//  an average period of (2 * 6 * 5 mSec) = 60 mSec
//
#define NUM_SAMPLE_PERIODS (60/EVAL_PERIOD_MSEC)

//
// Inactivity Timeout (seconds) - causes us to set the lights into a random changing mode
//
#define INACTIVITY_TIMEOUT 60



// ---------------------------------
// Objects
//
VL6180X sensor;


// ---------------------------------
// Variables
//

//
// Timer variables
//
boolean timer10MsecTick;
uint8_t timer10MsecCount;
boolean Tick10Msec;  // For use by evaluation routines
uint8_t timerTickCount;
boolean TickSec;

uint16_t inactivityTimer;  // Counts seconds since last user interaction


// ---------------------------------
// Arduino environment entry points
//

void setup() {
  Serial.begin(115200);
  Wire.begin();

  InitButtons();
  InitCommand();
  InitDj2();
  InitLeds();
  InitSensor();
  InitTimer();
  InitTouchPad();
  InitTouchPot();
}

void loop() {
  // Evaluate our timer ticks for this evaluation
  EvalTimer();

  if (Tick10Msec) {
    EvalButtons();
    EvalCommand();
    EvalDj2();
    EvalSensor();
    EvalTouchPad();
    EvalTouchPot();
  }

  if (TickSec) {
    EvalInactivityTimeout();
  }
}


// ---------------------------------
// Support routines
//

void InitTimer()
{
  Timer1.initialize(); // 10 mSec evaluation periods
  Timer1.attachInterrupt(EvalTimerHandler, 1000);
  timer10MsecTick = false;
  timer10MsecCount = 0;
  timerTickCount = 0;
  inactivityTimer = INACTIVITY_TIMEOUT;  // Start inactivity timeout upon first user interaction
}


void EvalTimer()
{
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
}


void EvalTimerHandler() {
  // LEDs updated every mSec
  EvalLeds();
  
  // Everything else is at least 10 mSec
  if (++timer10MsecCount == 10) {
    timer10MsecTick =  true; // consumed and cleared in main loop
    timer10MsecCount = 0;
  }
}


void NoteUserActivity() {
  inactivityTimer = 0;
}


void EvalInactivityTimeout() {
  if (inactivityTimer < INACTIVITY_TIMEOUT) {
    if (++inactivityTimer == INACTIVITY_TIMEOUT) {
      // Setup a cool random activity
      Dj2LoadMode(0, 0, 0x21, 0x0C);   // Fast/Random to the universe...
    }
  }
}

