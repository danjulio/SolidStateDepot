/*
 * Wheelchair power controller arduino code
 *   - Power control for Amplifier, LED PSU, Fan, Inverter
 *   - Battery Voltage Monitor
 *     - Auto turn-off for Amplifier over-voltage
 *   - IR Blaster capability
 *   - Dallas 1-Wire Temperature monitor
 *     - Auto fan control for temp sensor connected to LED PSU
 * 
 * Command Interface
 *  B           -- Get Battery Voltage (Returns "B=XX.X")
 *  P<N>={0,1}  -- Control Power Enable
 *  P<N>        -- Get current Power Enable value
 *    N  Device
 *    -----------------------
 *    0  Audio
 *    1  LED PSU
 *    2  Fan
 *    3  Inverter
 *  T<N>         -- Get 1-wire Temp for sensor <N> (0..Num sensors) (Returns "T=XX.XX", temp in F)
 * 
 * Error Responses
 *  E<N>
 *    N  Error
 *    ------------------------
 *    1  Command Error
 *    2  Amplifier over-voltage shutdown
 * 
 * IO Assignment
 *   D0 - TXD to LCD Arduino RXD
 *   D1 - RXD to LCD Arduino TXD
 *   D2 - Reserved for I2C SDA
 *   D3 - Reserved for I2C SCL
 *   D4 - Active High Audio Subsystem Enable
 *   D5 - Active Low LED PSU Enable
 *   D6 - Active High Fan Enable
 *   D7 - Active High Inverter Enable
 *   D8 - 1-wire Temperature sensor input (external power, 4.7k-ohm pull-up)
 *   D9 - Active High IR LED Blaster Enable
 *   A3 - Wheelchair bus voltage (1/15)
 *   
 */
#include <OneWire.h>
#include <DallasTemperature.h>


// =======================
// Constants
// =======================

// IO Defines
#define AUDIO_EN 4
#define LED_EN_N 5
#define FAN_EN   6
#define INV_EN   7
#define TEMP_1W  8
#define IR_LED   9
#define BATT_IN  A3

#define NUM_PWR_DEVS 4

// Device power control indicies
#define PWR_AUDIO_I 0
#define PWR_LED_I   1
#define PWR_FAN_I   2
#define PWR_INV_I   3

// Battery monitoring
#define BATT_ADC_MULT    12.102
#define BATT_SAMPLE_MSEC 1000
#define NUM_BATT_SAMPLES 8

// Amplifier over-voltage
#define AMP_MAX_V        28

// Temperature monitoring
#define MAX_TEMP_SENSORS 4
#define TEMP_SAMPLE_MSEC 5000
#define LED_PSU_HOT_C    35
#define LED_PSU_TEMP_I   0
// Number of bits of resolution to use with the DS18B20 temperature sensors (9-12)
#define TEMP_RESOLUTION 12

// Command processing
#define CMD_ST_IDLE 0
#define CMD_ST_CMD  1
#define CMD_ST_ARG  2

#define TERM_CHAR 0x0D

// Error codes
#define ERR_ILL_CMD  1
#define ERR_AMP_OV   2


// =======================
// Variables
// =======================

// Main program state
boolean DevEnabled[NUM_PWR_DEVS];

int battAdcArray[NUM_BATT_SAMPLES];
int battAdcIndex;
unsigned long battPrevSampleT;
float battV;

// Command processor
int CmdState[2];
char CurCmd[2];
uint16_t CmdIndex[2];
uint16_t CmdArg[2];
boolean CmdHasArg[2];

// Temperature related
OneWire oneWire(TEMP_1W);
DallasTemperature sensors(&oneWire);
int NumTempDevices;
DeviceAddress DevAddr;
unsigned long tempPrevSampleT;


// =======================
// Arduino Entry Points
// =======================

void setup() {
  pinMode(AUDIO_EN, OUTPUT);
  digitalWrite(AUDIO_EN, LOW);
  pinMode(LED_EN_N, OUTPUT);
  digitalWrite(LED_EN_N, HIGH);
  pinMode(FAN_EN, OUTPUT);
  digitalWrite(FAN_EN, LOW);
  pinMode(INV_EN, OUTPUT);
  digitalWrite(INV_EN, LOW);

  Serial.begin(9600);
  Serial1.begin(9600);

  analogReference(INTERNAL);

  // Enumerate and initialize the temperature sensors
  sensors.begin();
  NumTempDevices = sensors.getDeviceCount();
  if (NumTempDevices > MAX_TEMP_SENSORS) NumTempDevices = MAX_TEMP_SENSORS;
  for (int i=0; i<NumTempDevices; i++) {
    (void) sensors.getAddress(DevAddr, i);
    sensors.setResolution(DevAddr, TEMP_RESOLUTION);
  }
  if (NumTempDevices > 0) {
    sensors.requestTemperatures();
  }
  tempPrevSampleT = millis();

  // Initialize program state
  for (int i=0; i<NUM_PWR_DEVS; i++) {
    DevEnabled[i] = false;
  }
  for (int i=0; i<NUM_BATT_SAMPLES; i++) {
    battAdcArray[i] = analogRead(BATT_IN);
  }
  battAdcIndex = 0;
  battPrevSampleT = millis();

  CmdState[0] = CMD_ST_IDLE;
  CmdState[1] = CMD_ST_IDLE;
}


void loop() {
  char c;

  // Evaluate the serial enable
  if (Serial.available()) {
    c = Serial.read();
    EvalCommand(c, 0);
  }
  
  if (Serial1.available()) {
    c = Serial1.read();
    EvalCommand(c, 1);
  }

  // Evaluate battery monitor
  if (InactivityTimeout(&battPrevSampleT, BATT_SAMPLE_MSEC)) {
    battAdcArray[battAdcIndex] = analogRead(BATT_IN);
    if (++battAdcIndex == NUM_BATT_SAMPLES) battAdcIndex = 0;
    battV = GetBattAvg();
  }

  // Evalute over-voltage shutdown
  if (DevEnabled[PWR_AUDIO_I]) {
    if (battV > AMP_MAX_V) {
      // Turn off amp and note
      DevEnabled[PWR_AUDIO_I] = false;
      EnableDevice(PWR_AUDIO_I, false);
      DisplayError(0, ERR_AMP_OV);
      DisplayError(1, ERR_AMP_OV);
    }
  }

  // Evaluate temperature monitor
  if (InactivityTimeout(&tempPrevSampleT, TEMP_SAMPLE_MSEC)) {
    if (NumTempDevices > 0) {
      sensors.requestTemperatures();
    }
  }

  // Evaluate auto-fan control
  if (NumTempDevices > 0) {
    sensors.getAddress(DevAddr, LED_PSU_TEMP_I);
    if ((sensors.getTempC(DevAddr) >= (LED_PSU_HOT_C + 1)) && (!DevEnabled[PWR_FAN_I])) {
      // Fan on
      DevEnabled[PWR_FAN_I] = true;
      EnableDevice(PWR_FAN_I, true);
    } else if ((sensors.getTempC(DevAddr) <= (LED_PSU_HOT_C - 1)) && (DevEnabled[PWR_FAN_I])) {
      // Fan off
      DevEnabled[PWR_FAN_I] = false;
      EnableDevice(PWR_FAN_I, false);
    }
  }
}


// =======================
// Command Subroutines
// =======================

void EvalCommand(char c, int s) {
    
      switch (CmdState[s]) {
        case CMD_ST_IDLE:
          if (ValidateCommand(c)) {
            CurCmd[s] = c;
            CmdIndex[s] = 0;
            CmdHasArg[s] = false;
            CmdState[s] = CMD_ST_CMD;
          }
          break;
        
        case CMD_ST_CMD:
          if (c == '=') {
            CmdArg[s] = 0;
            CmdState[s] = CMD_ST_ARG;
            CmdHasArg[s] = true;
          } else if (c == TERM_CHAR) {
            ProcessCommand(s);
            CmdState[s] = CMD_ST_IDLE;
          } else if ((c >= '0') && (c <= '9')) {
            CmdIndex[s] = CmdIndex[s]*10 + (c - '0');
          }
          break;
        
        case CMD_ST_ARG:
          if (c == TERM_CHAR) {
            ProcessCommand(s);
            CmdState[s] = CMD_ST_IDLE;
          } else if ((c >= '0') && (c <= '9')) {
            CmdArg[s] = CmdArg[s]*10 + (c - '0');
          }
          break;

        default:
          CmdState[s] = CMD_ST_IDLE;
      }
}


boolean ValidateCommand(char c) {
  switch (c) {
    case 'B':
    case 'P':
    case 'T':
      return true;
      break;
    
    default:
      return false;
  }
}


void ProcessCommand(int s) {
  int rsp;
  
  switch (CurCmd[s]) {
    case 'B':
      if (!CmdHasArg[s]) {
        if (s == 0) {
          Serial.print(F("B="));
          Serial.println(battV);
        } else {
          Serial1.print(F("B="));
          Serial1.println(battV);
        }
      } else {
        DisplayError(s, ERR_ILL_CMD);
      }
      break;
      
    case 'P':
      if (CmdHasArg[s]) {
        if (CmdIndex[s] < NUM_PWR_DEVS) {
          DevEnabled[CmdIndex[s]] = (CmdArg[s] == 0) ? false : true;
          EnableDevice(CmdIndex[s], DevEnabled[CmdIndex[s]]);
        } else {
          DisplayError(s, ERR_ILL_CMD);
        }
      } else {
        if (CmdIndex[s] < NUM_PWR_DEVS) {
          rsp = DevEnabled[CmdIndex[s]];

          if (s == 0) {
            Serial.print(CurCmd[s]);
            Serial.print(CmdIndex[s]);
            Serial.print(F("="));
            Serial.println(rsp);
          } else {
            Serial1.print(CurCmd[s]);
            Serial1.print(CmdIndex[s]);
            Serial1.print(F("="));
            Serial1.println(rsp);
          }
        } else {
          DisplayError(s, ERR_ILL_CMD);
        }
      }
      break;

    case 'T':
      if ((!CmdHasArg[s]) && (CmdIndex[s] >= 0) && (CmdIndex[s] < NumTempDevices)) {
        sensors.getAddress(DevAddr, uint8_t (CmdIndex[s]));
        if (s == 0) {
          Serial.print(CurCmd[s]);
          Serial.print(CmdIndex[s]);
          Serial.print(F("="));
          Serial.println(sensors.getTempF(DevAddr));
        } else {
          Serial1.print(CurCmd[s]);
          Serial1.print(CmdIndex[s]);
          Serial1.print(F("="));
          Serial1.println(sensors.getTempF(DevAddr));
        }
      } else {
        DisplayError(s, ERR_ILL_CMD);
      }
      break;
  }
}


void DisplayError(int src, int errNum)
{
  if (src == 0) {
    Serial.print(F("E"));
    Serial.println(errNum);
  } else {
    Serial1.print(F("E"));
    Serial1.println(errNum);
  }
}


// =======================
// Interface Subroutines
// =======================

void EnableDevice(int index, boolean val)
{
  switch (index) {
    case PWR_AUDIO_I:
      digitalWrite(AUDIO_EN, val ? HIGH : LOW);
      break;
    case PWR_LED_I:
      digitalWrite(LED_EN_N, val ? LOW : HIGH);
      break;
    case PWR_FAN_I:
      digitalWrite(FAN_EN, val ? HIGH : LOW);
      break;
    case PWR_INV_I:
      digitalWrite(INV_EN, val ? HIGH : LOW);
      break;
  }
}


float GetBattAvg()
{
  int i;
  int sum = 0;
  float t;

  for (i=0; i<NUM_BATT_SAMPLES; i++) {
    sum += battAdcArray[i];
  }

  t = sum / NUM_BATT_SAMPLES;  // Average ADC count
  t = 2.56 * (t / 1023.0);     // Average ADC input voltage
  return(t * BATT_ADC_MULT);   // Average system voltage
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

