/*
 * Button logic
 * 
 * Buttons are debounced on eval intervals.  A timer is used to create a window in which
 * single- or double-click detection occurs.  The timer is started when the first click
 * is detected.  Single-click is reported when the timer expires without a second click
 * detected (button first down).  Double-click is reported when a second click is detected
 * while the timer is still running (ends the window immediately).
 */

// ---------------------------------
// Constants
//

#define NUM_BUTTONS 6

//
// Button inputs
//
#define BUT1_2_AIN A1
#define BUT3_4_AIN A2
#define BUT5_6_AIN A3

//
// Detection window (mSec) - ideally, should be multiples of EVAL_PERIOD_MSEC
//
#define BUT_CHECK_MSEC   175

//
// ADC Count thresholds (measured because I used non-precision resistors...)
//
#define BUT_1_COUNT 528
#define BUT_2_COUNT 931
#define BUT_12_COUNT 502

#define BUT_3_COUNT 520
#define BUT_4_COUNT 931
#define BUT_34_COUNT 495

#define BUT_5_COUNT 542
#define BUT_6_COUNT 931
#define BUT_56_COUNT 515

#define BUT_GB_COUNT 10

//
// Double-click state
//
#define BUT_ST_IDLE 0
#define BUT_ST_SINGLE 1

const uint16_t butAdcCounts[3*NUM_BUTTONS/2] = {
  BUT_1_COUNT,
  BUT_2_COUNT,
  BUT_12_COUNT,
  BUT_3_COUNT,
  BUT_4_COUNT,
  BUT_34_COUNT,
  BUT_5_COUNT,
  BUT_6_COUNT,
  BUT_56_COUNT
};


// ---------------------------------
// Variables
//
uint8_t prevBut;
uint8_t butPressed;
uint8_t butReleased;
uint8_t butDown;
uint8_t butTimers[NUM_BUTTONS];
uint8_t butSeriesInProgress;
uint8_t butSingleClick;
uint8_t butDoubleClick;


// ---------------------------------
// API
//
void InitButtons()
{
  int i;
  
  prevBut = 0;
  butPressed = 0;
  butReleased = 0;
  butDown = 0;
  butSeriesInProgress = 0;
  butSingleClick = 0;
  butDoubleClick = 0;

  for (i=0; i<NUM_BUTTONS; i++) {
    butTimers[i] = BUT_CHECK_MSEC/EVAL_PERIOD_MSEC;
  }
}


void EvalButtons()
{
  uint8_t cur;
  uint8_t butMask;
  int i;
  
  // Set eval state - will be changed if necessary
  butPressed = 0;
  butReleased = 0;
  butSingleClick = 0;
  butDoubleClick = 0;

  cur = _GetCurButtonVals();

  // Compute the button state
  butPressed = cur & prevBut & ~butDown;
  butReleased = butDown & ~cur & ~prevBut;
  butDown |= cur & prevBut;      // set bits when both cur & prev set
  butDown &= ~(~cur & ~prevBut); // clear bits when both cur & prev clear
  prevBut = cur;

  // Evaluate the click detection
  butMask = 0x01;
  for (i=0; i<NUM_BUTTONS; i++) {
    if (butPressed & butMask) {
      if ((butSeriesInProgress & butMask) == 0) {
        // First click in possible series
        butSeriesInProgress |= butMask;
      } else {
        // Second click - finish series check and reset
        butDoubleClick |= butMask;
        butSeriesInProgress &= ~butMask;
        butTimers[i] = BUT_CHECK_MSEC/EVAL_PERIOD_MSEC;
      }
    } else {
      // Evaluate series timer if it is running
      if ((butSeriesInProgress & butMask) == butMask) {
        if (--butTimers[i] == 0) {
          // Never saw second click so report single click
          butSingleClick |= butMask;
          // Finish series check and reset
          butSeriesInProgress &= ~butMask;
          butTimers[i] = BUT_CHECK_MSEC/EVAL_PERIOD_MSEC;
        }
      }
    }
    butMask = butMask << 1;
  }
}


boolean ButtonPressed(uint8_t n)
{
  uint8_t mask = 1 << (n-1);

  return ((butPressed & mask) == mask);
}


boolean ButtonReleased(uint8_t n)
{
  uint8_t mask = 1 << (n-1);

  return ((butReleased & mask) == mask);
}


boolean ButtonSingleClick(uint8_t n)
{
  uint8_t mask = 1 << (n-1);

  return ((butSingleClick & mask) == mask);
}


boolean ButtonDoubleClick(uint8_t n)
{
  uint8_t mask = 1 << (n-1);

  return ((butDoubleClick & mask) == mask);
}


// ---------------------------------
// Internal Routines
//
uint8_t _GetCurButtonVals()
{
  uint16_t ainVal[3];
  int i;
  uint8_t curVal, mask;

  ainVal[0] = analogRead(BUT1_2_AIN);
  ainVal[1] = analogRead(BUT3_4_AIN);
  ainVal[2] = analogRead(BUT5_6_AIN);

  //for (i=0;i<3;i++) {Serial.print(ainVal[i]); Serial.print(" ");} Serial.println();
  
  mask = 0x03;
  curVal = 0;
  for (i=0; i<3; i++) {
    if ((ainVal[i] < butAdcCounts[i*3+2]+BUT_GB_COUNT) && (ainVal[i] > butAdcCounts[i*3+2]-BUT_GB_COUNT)) {
      // Both pressed
      curVal |= mask & 0xFF;
    } else if ((ainVal[i] < butAdcCounts[i*3+1]+BUT_GB_COUNT) && (ainVal[i] > butAdcCounts[i*3+1]-BUT_GB_COUNT)) {
      // Second button of pair pressed
      curVal |= mask & 0xAA;
    } else if ((ainVal[i] < butAdcCounts[i*3+0]+BUT_GB_COUNT) && (ainVal[i] > butAdcCounts[i*3+0]-BUT_GB_COUNT)) {
      // First button of pair pressed
      curVal |= mask & 0x55;
    }
    mask = mask << 2;
  }

  return curVal;
}


