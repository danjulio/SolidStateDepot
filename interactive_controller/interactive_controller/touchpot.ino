/*
 * djd Touchpot access routine
 * 
 * Assumes Wire.begin executed
 * 
 */

// ---------------------------------
// Constants
//

// I2C Address
#define TP_I2C_ADDR 0x08


// ---------------------------------
// Variables
//
uint8_t prevTpVal;
boolean tpChanged;


// ---------------------------------
// API
//

void InitTouchPot()
{
  prevTpVal = _ReadTouchPot();
  tpChanged = false;
}

void EvalTouchPot()
{
  uint8_t curTpVal;

  curTpVal = _ReadTouchPot();
  if (curTpVal != prevTpVal) {
    prevTpVal = curTpVal;
    tpChanged = true;
  }
}


boolean GetTpChanged()
{
  return tpChanged;
}


uint8_t GetTpValue()
{
  tpChanged = false;   // Consumed value
  return prevTpVal;
}


// ---------------------------------
// Internal Routines
//

uint8_t _ReadTouchPot()
{
  Wire.requestFrom(TP_I2C_ADDR, 1);
  if (Wire.available()) {
    return Wire.read();
  } else {
    Serial.println(F("Error: Touchpot read failed"));
    return 0;
  }
}

