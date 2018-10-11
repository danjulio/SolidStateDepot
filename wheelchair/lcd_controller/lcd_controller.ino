/*
 * Wheelchair lcd control demo code
 *   - Power control
 *     - Audio
 *     - LEDs
 *     - Inverter
 *   - Display battery voltage
 *   - Display fan/temp status
 *   
 * Uses serial TTL to communicate with power controller arduino
 * 
 * LCD/Touch coordinates
 *   - Base edge is same edge as Arduino Leonardo USB/power connector
 *   - LCD Rotation mode 2 to match Touch Sensor
 *   - (0, 0) upper left
 *   - (239, 319) lower right
 *   
 * IO Assignment
 *   D0 - TXD to power controller Arduino RXD
 *   D1 - RXD to power controllre Arduino TXD
 *   D4 - Vishay TSOP2238 IR Receiver input
 *   D5 - LCD PWM Brightness
 *   
 */
#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>       // this is needed for display
#include <Adafruit_ILI9341.h>
#include <Wire.h>      // this is needed for FT6206
#include <Adafruit_FT6206.h>


// =======================
// Constants
// =======================

// Uncomment to enable debug printing of various things to USB serial
//#define EN_DEBUG_B_FLOW

#ifdef EN_DEBUG_B_FLOW
#define EN_DEBUG
#endif

// LED Backlight uses pin 5
#define LED_BL 5

// The display also uses hardware SPI, plus #9 & #10
#define TFT_CS 10
#define TFT_DC 9

// Device power control indicies
#define PWR_AUDIO_I 0
#define PWR_LED_I   1
#define PWR_FAN_I   2
#define PWR_INV_I   3

#define NUM_PWR_DEVS 4

// Status Updating
#define STATUS_UPDATE_MSEC 1000

// Response processing
#define RSP_ST_IDLE 0
#define RSP_ST_CMD  1
#define RSP_ST_ARGI 2
#define RSP_ST_ARGF 3

#define TERM_CHAR 0x0D

// Error Response indicies
#define ERR_NONE 0
#define ERR_CMD  1
#define ERR_OV   2

// Button indicies
#define BUTTON_AUDIO_I 0
#define BUTTON_LED_I   1
#define BUTTON_INV_I   2
#define BUTTON_ERR_I   3
#define BUTTON_PWM_I   4

#define NUM_BUTTONS    5

// Button position
#define BUTTON_X_INDEX  0
#define BUTTON_Y_INDEX  1
#define BUTTON_W_INDEX  2
#define BUTTON_H_INDEX  3
#define BUTTON_TX_INDEX 4
#define BUTTON_TY_INDEX 5
const int buttonInfo[NUM_BUTTONS][6] = {
  40,  50, 160, 50, 85, 66,
  40, 110, 160, 50, 85, 126,
  40, 170, 160, 50, 85, 186,
   0, 240, 240, 40, 40, 250,
   0, 280, 240, 40,  0,   0
};

// Button text
const char* buttonAmpTextP = "Sound";
const char* buttonLedTextP = "Visual";
const char* buttonInvTextP = "Power";
const char* buttonErr1P    = "Illegal Command";
const char* buttonErr2P    = "Amp Overvoltage";

const char* buttonLabelPArray[3] = {
  buttonAmpTextP,
  buttonLedTextP,
  buttonInvTextP
};

// Button colors
#define BUTTON_OUTLINE_COLOR  ILI9341_CYAN
#define BUTTON_TEXT_COLOR     ILI9341_YELLOW
#define BUTTON_FILL_ON_COLOR  ILI9341_BLUE
#define BUTTON_FILL_OFF_COLOR ILI9341_BLACK

// PWM Brightness
#define NUM_PWM_STEPS 4
const int pwmValues[NUM_PWM_STEPS] = {
  255, 100, 30, 1
};



// =======================
// Variables
// =======================

// State
boolean sawTouch = false;
boolean fingerDown = false;
int pwmIndex = 0;
boolean queryInProgress = false;

// Status items
boolean powerState[NUM_PWR_DEVS];
boolean prevPowerState[NUM_PWR_DEVS];
boolean updateOnPowerResponse[NUM_PWR_DEVS];
float battV = 0;
float prevBattV = 0;
float tempF = 0;
float prevTempF = 0;
int prevErr = ERR_NONE;
unsigned long statusPrevSampleT;

// Command processor
int RspState = RSP_ST_IDLE;
char CurRsp;
uint16_t RspIndex;
float RspArg;
float RspArgDivisor;
boolean RspHasArg;

// The FT6206 uses hardware I2C (SCL/SDA)
Adafruit_FT6206 ctp = Adafruit_FT6206();

// Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);



// =======================
// Arduino Entry Points
// =======================

void setup()
{
  int i;
  
  // Serial interfaces
#ifdef EN_DEBUG
  while (!Serial);   // Wait for USB connection for debugging output
  Serial.begin(115200);
#endif
  Serial.begin(115200);
  Serial1.begin(9600);

  // Program variables
  for (i=0; i<NUM_PWR_DEVS; i++) {
    powerState[i] = false;
    prevPowerState[i] = true;  // Force initial update
  }

  // Display
  pinMode(LED_BL, OUTPUT);
  analogWrite(LED_BL, pwmValues[pwmIndex]);
  delay(100);  // Allow LCD controller to finish reset
  InitDisplay();
  InitTouchSensor();

  statusPrevSampleT = millis();
}


void loop()
{
  char c;
  int b;
  
  // Look for user touch
  if (ctp.touched()) {
    sawTouch = !fingerDown;
    fingerDown = true;
    if (sawTouch) {
      TS_Point p = ctp.getPoint();
      if (IsButtonTouched(p.x, p.y, &b)) {
        HandleButtonPress(b);
      }
    }
  } else {
    fingerDown = false;
  }
  
  // Look for serial data from the power controller
  if (Serial1.available()) {
    c = Serial1.read();
    EvalResponse(c);
  }
  
  // Evaluate status update timer
  if (InactivityTimeout(&statusPrevSampleT, STATUS_UPDATE_MSEC)) {
    if (!queryInProgress) {
      QueryStatus();
    }
  }
}



// =======================
// Command processor Subroutines
// =======================

void EvalResponse(char c) {
    
      switch (RspState) {
        case RSP_ST_IDLE:
          if (ValidateResponse(c)) {
            CurRsp = c;
            RspIndex = 0;
            RspHasArg = false;
            RspState = RSP_ST_CMD;
          }
          break;
        
        case RSP_ST_CMD:
          if (c == '=') {
            RspArg = 0;
            RspState = RSP_ST_ARGI;
            RspHasArg = true;
          } else if (c == TERM_CHAR) {
            ProcessResponse();
            RspState = RSP_ST_IDLE;
          } else if ((c >= '0') && (c <= '9')) {
            RspIndex = RspIndex*10 + (c - '0');
          }
          break;
        
        case RSP_ST_ARGI:
          if (c == TERM_CHAR) {
            ProcessResponse();
            RspState = RSP_ST_IDLE;
          } else if (c == '.') {
            // Setup to get fractional part
            RspArgDivisor = 10.0;
            RspState = RSP_ST_ARGF;
          } else if ((c >= '0') && (c <= '9')) {
            RspArg = RspArg*10 + (c - '0');
          }
          break;

        case RSP_ST_ARGF:
          if (c == TERM_CHAR) {
            ProcessResponse();
            RspState = RSP_ST_IDLE;
          } else if ((c >= '0') && (c <= '9')) {
            RspArg = RspArg + ((c - '0') / RspArgDivisor);
            RspArgDivisor = RspArgDivisor * 10;
          }
          break;
        
        default:
          RspState = RSP_ST_IDLE;
      }
}


boolean ValidateResponse(char c) {
  switch (c) {
    case 'B':
    case 'E':
    case 'P':
    case 'T':
      return true;
      break;
    
    default:
      return false;
  }
}


void ProcessResponse() {
  int rsp;
  
  switch (CurRsp) {
    case 'B':
      if (RspHasArg) {
        battV = RspArg;
        UpdateBattV();

        // Battery is last queried
        queryInProgress = false;
      }
      break;
      
    case 'E':
      UpdateError(RspIndex);
      break;
     
    case 'P':
      if (RspHasArg) {
#ifdef EN_DEBUG_B_FLOW
        Serial.print(F("Rsp P"));
        Serial.print(RspIndex);
        Serial.print(F("="));
        Serial.println(RspArg);
#endif
        if (RspIndex < NUM_PWR_DEVS) {
          powerState[RspIndex] = RspArg != 0.0;
          if (RspIndex == PWR_FAN_I) {
            UpdateFanState();
          } else {
            rsp = PowerToButtonIndex(RspIndex);
            if ((rsp != -1) && (updateOnPowerResponse[RspIndex])) {
              UpdatePowerButton(rsp);
            }
          }
        }
      }
      break;

    case 'T':
      if (RspHasArg) {
        tempF = RspArg;
        UpdateTemp();
      }
      break;
  }
}


void SendPowerCmd(int i, boolean s)
{
  Serial1.print(F("P"));
  Serial1.print(i);
  Serial1.print(F("="));
  Serial1.println(s ? 1 : 0);
}


void QueryStatus()
{
  // Get current power state
  Serial1.print(F("P"));
  Serial1.println(PWR_AUDIO_I);
  updateOnPowerResponse[PWR_AUDIO_I] = true;
  Serial1.print(F("P"));
  Serial1.println(PWR_LED_I);
  updateOnPowerResponse[PWR_LED_I] = true;
  Serial1.print(F("P"));
  Serial1.println(PWR_FAN_I);
  updateOnPowerResponse[PWR_FAN_I] = true;
  Serial1.print(F("P"));
  Serial1.println(PWR_INV_I);
  updateOnPowerResponse[PWR_INV_I] = true;
#ifdef EN_DEBUG_B_FLOW
  Serial.println(F("Query Power loaded"));
#endif

  // LED PSU Temperature
  Serial1.println(F("T0"));

  // Battery voltage - last query item loaded, used as marker to know when we've finished a query
  Serial1.println(F("B"));

  queryInProgress = true;
}



// =======================
// Display Subroutines
// =======================

void InitDisplay()
{
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(2);

  // Draw the title area
  tft.drawRect(0, 0, 240, 30, ILI9341_GREEN);
  tft.setCursor(10, 7);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(2);
  tft.print(F("Axiom Elysium CTRL!"));

  InitStatus();
  InitButtons();
}


void InitStatus()
{
  tft.drawRect(0, 280, 240, 40, ILI9341_GREEN);
  tft.drawLine(180, 280, 180, 319, ILI9341_GREEN);
  
  UpdateBattV();
  UpdateTemp();
  UpdateFanState();
}


void InitButtons()
{
  int i;

  // Power control buttons
  for (i=0; i<3; i++) {
    tft.drawRect(buttonInfo[i][0], buttonInfo[i][1], buttonInfo[i][2], buttonInfo[i][3],
                 BUTTON_OUTLINE_COLOR);
    UpdatePowerButton(i);
  }

  // Error indicator "button"
  prevErr = ERR_NONE;
  UpdateError(ERR_NONE);
}


void UpdateBattV()
{
  if (battV != prevBattV) {
    // Erase old voltage
    tft.fillRect(10, 290, 169, 20, ILI9341_BLACK);

    // Draw new voltage
    tft.setTextSize(2);
    tft.setCursor(10, 293);
    tft.setTextColor(ILI9341_GREEN);
    tft.print(F("Batt: "));
    tft.print(battV);
    tft.print(F("V"));

    prevBattV = battV;
  }
}


void UpdateTemp()
{
  if (tempF != prevTempF) {
    // Erase old voltage
    tft.fillRect(185, 285, 50, 15, ILI9341_BLACK);

    // Draw new temp
    tft.setTextSize(1);
    tft.setCursor(185, 285);
    tft.setTextColor(ILI9341_GREEN);
    tft.print(tempF);
    tft.print(F("F"));

    prevTempF = tempF;
  }
}


void UpdateFanState()
{
  if (powerState[PWR_FAN_I] != prevPowerState[PWR_FAN_I]) {
    // Erase old fan state
    tft.fillRect(185, 302, 50, 15, ILI9341_BLACK);

    // Draw new fan state
    tft.setTextSize(1);
    tft.setCursor(185, 302);
    tft.setTextColor(ILI9341_GREEN);
    tft.print(F("Fan "));
    if (powerState[PWR_FAN_I]) {
      tft.print(F("ON"));
    } else {
      tft.print(F("OFF"));
    }

    prevPowerState[PWR_FAN_I] = powerState[PWR_FAN_I];
  }
}


void UpdatePowerButton(int b)
{
  int i;

  i = ButtonToPowerIndex(b);

  if (powerState[i] != prevPowerState[i]) {

    // Draw background
    if (powerState[i]) {
      tft.fillRect(buttonInfo[b][0]+1, buttonInfo[b][1]+1, buttonInfo[b][2]-2, buttonInfo[b][3]-2,
                   BUTTON_FILL_ON_COLOR);
    } else {
      tft.fillRect(buttonInfo[b][0]+1, buttonInfo[b][1]+1, buttonInfo[b][2]-2, buttonInfo[b][3]-2,
                   BUTTON_FILL_OFF_COLOR);
    }

    // Then redraw text again
    tft.setTextColor(BUTTON_TEXT_COLOR);
    tft.setTextSize(2);
    tft.setCursor(buttonInfo[b][4], buttonInfo[b][5]);
    tft.print(buttonLabelPArray[b]);

    prevPowerState[i] = powerState[i];
  }
}


void UpdateError(int e)
{
  if (e == ERR_NONE) {
    // Clear error display
    tft.fillRect(buttonInfo[BUTTON_ERR_I][0], buttonInfo[BUTTON_ERR_I][1],
                 buttonInfo[BUTTON_ERR_I][2], buttonInfo[BUTTON_ERR_I][3],
                 ILI9341_BLACK);
  } else {
    // Draw warning triangle
    tft.drawLine(10, 265, 32, 265, ILI9341_YELLOW);
    tft.drawLine(10, 265, 21, 245, ILI9341_YELLOW);
    tft.drawLine(21, 245, 32, 265, ILI9341_YELLOW);

    // Draw text
    tft.setTextColor(ILI9341_RED);
    tft.setTextSize(2);
    tft.setCursor(17, 250);
    tft.print(F("!"));
    
    tft.setCursor(buttonInfo[BUTTON_ERR_I][4], buttonInfo[BUTTON_ERR_I][5]);
    if (e == ERR_CMD) {
      tft.print(buttonErr1P);
    } else {
      tft.print(buttonErr2P);
    }
  }

  prevErr = e;
}



// =======================
// Button Handling Subroutines
// =======================

void InitTouchSensor()
{
  if (!ctp.begin(40)) { // pass in 'sensitivity' coefficient
    Serial.println(F("Couldn't start FT6206 touchscreen controller"));
  }
}


boolean IsButtonTouched(int x, int y, int* b)
{
  int i;

  // Scan through looking to see if we're inside of a button
  for (i=0; i<NUM_BUTTONS; i++) {
    if ((x >= buttonInfo[i][0]) && (y >= buttonInfo[i][1]) &&
        (x < (buttonInfo[i][0] + buttonInfo[i][2])) &&
        (y < (buttonInfo[i][1] + buttonInfo[i][3]))
       ) {

      *b = i;
      return (true);
    }
  }

  // Didn't find a hit
  return (false);
}


void HandleButtonPress(int b)
{
  int pwrI;
  
  if (b == BUTTON_ERR_I) {
    // See if there's an error message to clear
    if (prevErr != ERR_NONE) {
      UpdateError(ERR_NONE);
    }
  } else if (b == BUTTON_PWM_I) {
    SelectNextBrightness();
  } else {
#ifdef EN_DEBUG_B_FLOW
    Serial.print(F("But: "));
    Serial.print(b);
    Serial.print(F(" = "));
    Serial.println(!powerState[pwrI]);
#endif
    // Toggle current power state
    pwrI = ButtonToPowerIndex(b);
    powerState[pwrI] = !powerState[pwrI];
    UpdatePowerButton(b);
    
    updateOnPowerResponse[pwrI] = false;  // Cancel any previous query since we've just outdated it

    SendPowerCmd(pwrI, powerState[pwrI]);
  }
}


// =======================
// PWM Brightness Subroutines
// =======================

void SelectNextBrightness()
{
 if (++pwmIndex ==  NUM_PWM_STEPS) pwmIndex = 0;

 analogWrite(LED_BL, pwmValues[pwmIndex]);
}



// =======================
// Utility Subroutines
// =======================

boolean InactivityTimeout(unsigned long* prevT, unsigned long timeout) {
  unsigned long curT = millis();
  unsigned long deltaT;
  
  if (curT > *prevT) {
    deltaT = curT - *prevT;
  } else {
    // Handle wrap
    deltaT = ~(*prevT - curT) + 1;
  }
  
  if (deltaT >= timeout) {
    *prevT = curT;
    return (true);
  } else {
    return (false);
  }
}


int PowerToButtonIndex(int p)
{
  int i;
  
  switch (RspIndex) {
    case PWR_AUDIO_I:
       i = BUTTON_AUDIO_I;
       break;
    case PWR_LED_I:
       i = BUTTON_LED_I;
       break;
    case PWR_INV_I:
       i = BUTTON_INV_I;
       break;
    default:
       i = -1;
  }

  return i;
}


int ButtonToPowerIndex(int b)
{
  int i;
  
  switch (b) {
      case BUTTON_AUDIO_I:
        i = PWR_AUDIO_I;
        break;
      case BUTTON_LED_I:
        i = PWR_LED_I;
        break;
      case BUTTON_INV_I:
        i = PWR_INV_I;
        break;
    }

    return i;
}

