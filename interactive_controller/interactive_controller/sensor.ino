/*
 * ST VL6180X Distance sensor for beat detection
 * 
 * Assume Wire.begin and VL6180X instanced as "sensor".
 * 
 */

// ---------------------------------
// Constants
//

//
// Beat detection states
//
#define SNS_ST_IDLE 0
#define SNS_ST_HIGH 1
#define SNS_ST_LOW  2

//
// Detection levels (measured mm)
//
#define SNS_LOW_THRESH  100


// ---------------------------------
// Variables
//
boolean sensorValid = false;
uint8_t sensorValue = 255;
uint8_t sensorState = SNS_ST_IDLE;
boolean sensorBeat = false;


// ---------------------------------
// API
//

void InitSensor()
{
  sensor.init();
  sensor.configureDefault();

  // Reduce range max convergence time and ALS integration
  // time to 30 ms and 50 ms, respectively, to allow 10 Hz
  // operation (as suggested by Table 6 ("Interleaved mode
  // limits (10 Hz operation)") in the datasheet).
  sensor.writeReg(VL6180X::SYSRANGE__MAX_CONVERGENCE_TIME, 30);
  sensor.writeReg16Bit(VL6180X::SYSALS__INTEGRATION_PERIOD, 50);
  
   // Stop continuous mode if already active
  sensor.stopContinuous();
  
  // In case stopContinuous() triggered a single-shot
  // measurement, wait for it to complete
  delay(200);
  
  // Start interleaved continuous mode with period of 100 ms
  sensor.startRangeContinuous(100);
}


void EvalSensor()
{
  boolean localSensorValid = false;
  uint8_t d;

  // Look for valid sensor data - we only expect data about every 100 mSec
  if (_RangeMeasurementReady()) {
    d = _ReadRange();
    if (d < 255) {
      sensorValid = true;
      sensorValue = d;
    }
    localSensorValid = true;
  }

  // Evaluate beat detection logic
  switch (sensorState) {
    case SNS_ST_IDLE:
      if (localSensorValid) {
        if (d <= SNS_LOW_THRESH) {
          sensorState = SNS_ST_LOW;
          sensorBeat = true;
        } else if (d < 255) {
          sensorState = SNS_ST_HIGH;
        }
      }
      break;
    case SNS_ST_HIGH:
      if (localSensorValid) {
        if (d <= SNS_LOW_THRESH) {
          sensorState = SNS_ST_LOW;
          sensorBeat = true;
        } else if (d >= 255) {
          sensorState = SNS_ST_IDLE;
        }
      }
      break;
    case SNS_ST_LOW:
      if (localSensorValid) {
        if (d == 255) {
          sensorState = SNS_ST_IDLE;
        } else if (d > SNS_LOW_THRESH) {
          sensorState = SNS_ST_HIGH;
        }
      }
      break;
    default:
      sensorState = SNS_ST_IDLE;
  }
}


boolean SensorValid(uint8_t* v)
{
  *v = sensorValue;
  if (sensorValid) {
    sensorValid = false;
    return true;
  } else {
    return false;
  }
}


boolean SensorBeat()
{
  if (sensorBeat) {
    sensorBeat = false; // consumed
    return true;
  } else {
    return false;
  }
}


// ---------------------------------
// Internal Routines
//

//
// These routines augment the VL6180X driver to allow us to poll it instead of getting
// hung inside its routines waiting for data
//

boolean _RangeMeasurementReady()
{
  return ((sensor.readReg(VL6180X::RESULT__INTERRUPT_STATUS_GPIO) & 0x04) != 0);
}


uint8_t _ReadRange()
{
  uint8_t range = sensor.readReg(VL6180X::RESULT__RANGE_VAL);
  sensor.writeReg(VL6180X::SYSTEM__INTERRUPT_CLEAR, 0x01);

  return range;
}

