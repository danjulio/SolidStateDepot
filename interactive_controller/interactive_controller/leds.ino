/*
 * LED indicators
 * 
 * Control 6 bi-color (red/green) LEDs with the ability to make yellow too
 *   - Multiplexed (at 1 mSec intervals) to allow individual color control
 *   - Common anode (individual cathode control for multiplexing, anode for color)
 *   - PWM used to make better yellow (reduce red).  Arduino PWM on red channel
 *     manually increased to 31.25 kHz to be much faster than multiplex frequency
 *     to avoid beat flickering.
 */

// ---------------------------------
// Constants
//

#define NUM_LEDS 6

//
// LED pins
//
#define LED_R_ANODE 5
#define LED_G_ANODE 6
#define LED_1_CATHODE 9
#define LED_2_CATHODE 10
#define LED_3_CATHODE 14
#define LED_4_CATHODE 15
#define LED_5_CATHODE 16
#define LED_6_CATHODE 18

const uint8_t ledPins[NUM_LEDS] = {
  LED_1_CATHODE,
  LED_2_CATHODE,
  LED_3_CATHODE,
  LED_4_CATHODE,
  LED_5_CATHODE,
  LED_6_CATHODE
};


// ---------------------------------
// Variables
//
uint8_t curLedVal[NUM_LEDS];
uint8_t curLedMux;


// ---------------------------------
// API
//

void InitLeds()
{
  int i;
  
  pinMode(LED_R_ANODE, OUTPUT);
  digitalWrite(LED_R_ANODE, LOW);
  pinMode(LED_G_ANODE, OUTPUT);
  digitalWrite(LED_G_ANODE, LOW);

  for (i=0; i<NUM_LEDS; i++) {
    pinMode((int) ledPins[i], OUTPUT);
    digitalWrite((int) ledPins[i], HIGH);
    curLedVal[i] = LED_BLACK;
  }
  curLedMux = 0;

  // Hack taken from the PWMFrequency library to change pin 5's PWM frequency (our red channel)
  // so it works with the multiplexing.  This makes it unnecessary to have the whole library installed.
  // See: https://github.com/kiwisincebirth/Arduino/blob/master/libraries/PWMFrequency/PWMFrequency.h
  //
  // This sets the divisor to 1 (from the default 64->488 Hz PWM freq) for a pwm freq of 31250 Hz
  //
  TCCR3B = TCCR3B & 0b11111000 | 0b001;
}


void EvalLeds()
{
  // Switch off all LEDs
  _SetLedsOff();

  // Configure the color for the currently outputing LED
  _SetLedColor(curLedVal[curLedMux]);

  // Enable it
  digitalWrite((int) ledPins[curLedMux], LOW);

  // Setup for next LED
  if (++curLedMux == NUM_LEDS) curLedMux = 0;
}


void SetLed(uint8_t index, uint8_t c)
{
  index = index - 1;  // 1.. based to 0.. based for array
  
  if (index < NUM_LEDS) {
    curLedVal[index] = c;
  }
}


// ---------------------------------
// Internal Routines
//


void _SetLedColor(uint8_t c)
{
  switch (c) {
    case LED_RED:
      digitalWrite(LED_R_ANODE, HIGH);
      digitalWrite(LED_G_ANODE, LOW);
      break;
    case LED_GREEN:
      digitalWrite(LED_R_ANODE, LOW);
      digitalWrite(LED_G_ANODE, HIGH);
      break;
    case LED_YELLOW:
      // For yellow, use PWM to reduce the red contribution for a better yellow
      analogWrite(LED_R_ANODE, 192);
      //digitalWrite(LED_R_ANODE, HIGH);
      digitalWrite(LED_G_ANODE, HIGH);
      break;
    default:
      digitalWrite(LED_R_ANODE, LOW);
      digitalWrite(LED_G_ANODE, LOW);
      break;
  }
}


void _SetLedsOff()
{
  digitalWrite(LED_1_CATHODE, HIGH);
  digitalWrite(LED_2_CATHODE, HIGH);
  digitalWrite(LED_3_CATHODE, HIGH);
  digitalWrite(LED_4_CATHODE, HIGH);
  digitalWrite(LED_5_CATHODE, HIGH);
  digitalWrite(LED_6_CATHODE, HIGH);
}

