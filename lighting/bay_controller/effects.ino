//
// Sequence generator for on/off/classroom button presses
//
// Sequences comprised of SetHSV{G,U} or FadeHSV{G,U}, delays and triggered sound effects
//
// Sound effects
//   Trigger 1: Smooth Power-up sound
//   Trigger 2: Buzzy Power-up sound
//   Trigger 3: Small relay
//   Trigger 4: Large relay
//   Trigger 5: Short-circuity sound
//   Trigger 6: Power-down
//

//
// dj2 SetMode Special Mode values
//
#define DJ2_MODE_VAL_CLASSROOM  0x01
#define DJ2_MODE_VAL_ON         0x02
#define DJ2_MODE_VAL_OFF        0x03


//
// Sequence Indexes (must match strings displayed on LCD from special_S)
//
#define EFFECT_RANDOM   0
#define EFFECT_ALL_ON   1
#define EFFECT_SMOOTH   2
#define EFFECT_RELAY1   3
#define EFFECT_RELAY2   4
#define EFFECT_FLUOR    5
#define EFFECT_CLASS    254
#define EFFECT_OFF      255


//
// Percentage odds that a specific "ON" effect will be selected.  Should always add up to 100.
//
#define NUM_SPECIFIC_EFFECTS 5
#define PERCENT_ALL_ON       50
#define PERCENT_SMOOTH       30
#define PERCENT_RELAY1       7
#define PERCENT_RELAY2       8
#define PERCENT_FLUOR        5


//
// Percentages for sound effects
#define PERCENT_RND_ON       5
#define PERCENT_RND_OFF      5


//
// Module Variables
//
uint16_t effectTimer;      // Set in units of 10 mSec to delay steps in a sequence
uint8_t curEffect;
uint8_t effectStep;

uint8_t allowedEffect;
boolean doSound;


//
// API routines
//
void InitEffects() {
  allowedEffect = EFFECT_RANDOM;
  effectStep = 0;
}


// Designed to be called on 10 mSec intervals
void EvalEffects() {
  if (effectStep != 0) {
    switch (curEffect) {
      case EFFECT_ALL_ON:
        _EvalEffectAllOn();
        break;

      case EFFECT_SMOOTH:
        _EvalEffectSmooth();
        break;

      case EFFECT_RELAY1:
        _EvalEffectRelay1();
        break;

      case EFFECT_RELAY2:
        _EvalEffectRelay2();
        break;

      case EFFECT_FLUOR:
        _EvalEffectFluorescent();
        break;

      case EFFECT_CLASS:
        _EvalEffectClassroom();
        break;

      case EFFECT_OFF:
        _EvalEffectOff();
        break;

      default:
        effectStep = 0;
    }
  }
}


void SetAllowedEffect(uint8_t e) {
  allowedEffect = e;
}


void TriggerEffect(uint8_t t) {
  // Only allow a new effect if we are idle
  if (effectStep == 0) {
    effectStep = 1;
    switch (t) {
      case DJ2_MODE_VAL_CLASSROOM:
        curEffect = EFFECT_CLASS;
        break;

      case DJ2_MODE_VAL_ON:
        // Pick one of the possible "ON" sequences
        if (allowedEffect == EFFECT_RANDOM) {
          curEffect = _PickRandomEffect();
        } else {
          curEffect = allowedEffect;
        }
        _EffectSoundOn();
        break;

      case DJ2_MODE_VAL_OFF:
        curEffect = EFFECT_OFF;
        _EffectSoundOff();
        break;
    }
  }
}



//
// Internal routines
//
boolean _EffectTimerExpired() {
  if (effectTimer != 0) {
    if (--effectTimer == 0) {
      return true;
    } else {
      return false;
    }
  } else {
    return true;
  }
}


uint8_t _PickRandomEffect() {
  uint8_t selArray[NUM_SPECIFIC_EFFECTS];
  int rnd;
  int i = 0;
  
  selArray[0] = PERCENT_ALL_ON;
  selArray[1] = selArray[0] + PERCENT_SMOOTH;
  selArray[2] = selArray[1] + PERCENT_RELAY1;
  selArray[3] = selArray[2] + PERCENT_RELAY2;
  selArray[4] = selArray[3] + PERCENT_FLUOR;
  
  // Select one of the known items
  rnd = random(101);
  
  while (i < NUM_SPECIFIC_EFFECTS-1) {
    if (rnd < selArray[i]) {
      break;
    }
    i++;
  }
  
  return i+1;
}


void _EffectSoundOn() {
  int rndN;
  
  doSound = true;  // Will be set false if necessary
  
  if ((allowedEffect == EFFECT_RANDOM) && (curEffect == EFFECT_ALL_ON)) {
    // For randomly selected effects, we only generate a sound for the usually selected
    // smooth power on occasionally
    rndN = random(101);
    if (rndN >= PERCENT_RND_ON) {
      doSound = false;
    }
  }
}


void _EffectSoundOff() {
  int rndN = random(101);
  
  // We generate a sound for power down only occasionally
  if (rndN < PERCENT_RND_OFF) {
    SoundEnable(6);
  }
}


void _EvalEffectAllOn() {
  // Simple effect that fades all white lights in Group 2 (Bay Area) on in one step
  Dj2LoadFadeHsv(200, 0, true, 0, 0, 0x0100, 10);
  if (doSound) {
    SoundEnable(1);
  }
  effectStep = 0;
}


void _EvalEffectSmooth() {
  // Load multiple packets at 200 mSec intervals turning on rows 10 - 13
  
  switch (effectStep) {
    case 1: // Row 10
      Dj2LoadFadeHsv(200, 10, false, 0x0000, 0x0000, 0x0100, 20);
      if (doSound) {
        SoundEnable(2);
      }
      effectTimer = 20;
      effectStep = 2;
      break;

    case 2: // Row 11
      if (_EffectTimerExpired()) {
        Dj2LoadFadeHsv(200, 11, false, 0x0000, 0x0000, 0x0100, 20);
        effectTimer = 20;
        effectStep = 3;
      }
      break;
      
    case 3: // Row 12
      if (_EffectTimerExpired()) {
        Dj2LoadFadeHsv(200, 12, false, 0x0000, 0x0000, 0x0100, 20);
        effectTimer = 20;
        effectStep = 4;
      }
      break;
    
    case 4: // Row 13
      if (_EffectTimerExpired()) {
        Dj2LoadFadeHsv(200, 13, false, 0x0000, 0x0000, 0x0100, 20);
        effectStep = 0;
      }
      break;

    default:
      effectStep = 0;
  }
}


void _EvalEffectRelay1() {
  // Sequence through units
  if (effectStep == 1) {
    // First fixture turned on immediately
    Dj2LoadSetHsv(200, 200+effectStep, true, 0x0000, 0x0000, 0x0100);
    if (doSound) {
      SoundEnable(3);
    }
    effectTimer = 20;
    effectStep = 2;
  } else {
    // Subsequent fixtures at 200 mSec intervals
    if (_EffectTimerExpired()) {
      Dj2LoadSetHsv(200, 200+effectStep, true, 0x0000, 0x0000, 0x0100);
      if (doSound) {
        SoundEnable(3);
      }
      effectTimer = 20;
      if (++effectStep == 13) {
        // Done
        effectStep = 0;
      }
    }
  }
}


void _EvalEffectRelay2 () {
  // Sequence through rows
  if (effectStep == 1) {
    // First row turned on immediately
    if (doSound) {
      SoundEnable(4);
    }
    Dj2LoadSetHsv(200, 9+effectStep, false, 0x0000, 0x0000, 0x0100);
    effectTimer = 25;
    effectStep = 2;
  } else {
    // Subsequent fixtures at 250 mSec intervals
    if (_EffectTimerExpired()) {
      if (doSound) {
        SoundEnable(4);
      }
      Dj2LoadSetHsv(200, 9+effectStep, false, 0x0000, 0x0000, 0x0100);
      effectTimer = 25;
      if (++effectStep == 5) {
        // Done
        effectStep = 0;
      }
    }
  }
}


void _EvalEffectFluorescent() {
  // Some units fade up quickly and a few units flicker to life
  static uint8_t numFlickerUnits;
  static uint16_t flickerUnit[5];      // max of 5 units flickering
  static uint32_t flickerPattern[5];  // max of 5 units flickering
  uint8_t i, rndUnit;

  if (effectStep == 1) {
    // Setup effect and start initial units
    //
    // Select which units to flicker and their flicker patterns
    numFlickerUnits = random(1, 6);   // 1 - 5 units selected to flicker
    for (i=0; i<4; i++) flickerUnit[i] = 0;
    i = 0;
    //Serial.println(numFlickerUnits);
    while (i < numFlickerUnits) {
      rndUnit = random(1, 13) + 200;        // Select a unit 1 - 12 and convert to an actual unit
      //Serial.print("rndUnit = "); Serial.println(rndUnit);
      if ((flickerUnit[0] != rndUnit) && (flickerUnit[1] != rndUnit) && (flickerUnit[2] != rndUnit) && (flickerUnit[3] != rndUnit)) {
        flickerUnit[i] = rndUnit;
        flickerPattern[i] = random(0, 0x10000) | (random(0, 0x10000) << 16) | 0x80000001;   // Make sure flicker units start and end on
        //Serial.print("  flickerUnit = "); Serial.println(flickerUnit[i], HEX);
        //Serial.print("  flickerPattern = "); Serial.println(flickerPattern[i], HEX);
        i++;
      }
    }
    
    // Fade on all lights initially
    Dj2LoadFadeHsv(200, 0, true, 0, 0, 0x0100, 3);
    if (doSound) {
      SoundEnable(5);
    }
    
    effectTimer = 3;  // Evaluate every 30 mSec
    effectStep = 2;
  } else {
    if (_EffectTimerExpired()) {
      // Look at the transition between the least signficiant 2 bits of each pattern to see if we should turn on or off the unit
      for (i=0; i<numFlickerUnits; i++) {
        switch (flickerPattern[i] & 0x03) {
          case 1: // '01' ---> OFF
            Dj2LoadSetHsv(200, flickerUnit[i], true, 0x0000, 0x0000, 0x0000);
            //Serial.print(flickerUnit[i], HEX); Serial.println(" off");
            break;
            
          case 2: // '10' ---> ON
            Dj2LoadSetHsv(200, flickerUnit[i], true, 0x0000, 0x0000, 0x0100);
            //Serial.print(flickerUnit[i], HEX); Serial.println(" on");
            break;
        }
        flickerPattern[i] >>= 1;   // Shift down 1 bit for next transition
      }
      
      // Setup next step
      effectTimer = 3;
      if (++effectStep == 33) {
        // Done after all 15 transitions of the flickerPatterns have been processed
        effectStep = 0;
      }
    }
  }
}


void _EvalEffectClassroom() {
  // Load multiple packets at 50-200 mSec intervals to implement the following "scene"
  //   1. Column 1 (White: 50%, Color: Blue @ 100%)
  //   2. Column 2 (White: 25%, Color: Blue @100%)
  //   3. Unit 203 (Color: Blue @100%)
  //   4. Unit 204 (Color: Blue @100%)
  //   5. Unit 209 (White: 25%, Color: Blue @ 100%)
  //   6. Unit 210 (White: 50%, Color: Blue @ 100%)
  //
  // Take advantage of the fact that we can stuff 2 packets into the Arduino's Serial TX FIFO

  switch (effectStep) {
    case 1: // Column 1
      Dj2LoadFadeHsv(200, 20, false, 0x0400, 0x00C0, 0x0080, 20);
      Dj2LoadFadeHsv(201, 20, false, 0x0400, 0x00C0, 0x0100, 20);
      effectTimer = 20;
      effectStep = 2;
      break;

    case 2: // Column 2
      if (_EffectTimerExpired()) {
        Dj2LoadFadeHsv(200, 21, false, 0x0400, 0x00E0, 0x0040, 20);
        Dj2LoadFadeHsv(201, 21, false, 0x0400, 0x00E0, 0x0100, 20);
        effectTimer = 20;
        effectStep = 3;
      }
      break;

    case 3: // White Units 203 & 204
      if (_EffectTimerExpired()) {
        Dj2LoadFadeHsv(200, 203, true, 0x0400, 0x0100, 0x0000, 20);
        Dj2LoadFadeHsv(200, 204, true, 0x0400, 0x0100, 0x0000, 20);
        effectTimer = 2;
        effectStep = 4;
      }
      break;

    case 4: // Color Units 203 & 204
      if (_EffectTimerExpired()) {
        Dj2LoadFadeHsv(201, 203, true, 0x0400, 0x0100, 0x0100, 20);
        Dj2LoadFadeHsv(201, 204, true, 0x0400, 0x0100, 0x0100, 20);
        effectTimer = 5;
        effectStep = 5;
      }
      break;
      
      case 5: // White Units 209 & 210
      if (_EffectTimerExpired()) {
        Dj2LoadFadeHsv(200, 209, true, 0x0400, 0x0100, 0x0040, 20);
        Dj2LoadFadeHsv(200, 210, true, 0x0400, 0x0100, 0x0080, 20);
        effectTimer = 2;
        effectStep = 6;
      }
      break;

    case 6: // Color Units 209 & 210
      if (_EffectTimerExpired()) {
        Dj2LoadFadeHsv(201, 209, true, 0x0400, 0x0100, 0x0100, 20);
        Dj2LoadFadeHsv(201, 210, true, 0x0400, 0x0100, 0x0100, 20);
        effectTimer = 2;
        effectStep = 7;
      }
      break;
      
    case 7: // Bay DMX controllers
      if (_EffectTimerExpired()) {
        Dj2LoadFadeHsv(202, 213, true, 0x0400, 0x0100, 0x0100, 20);  // Lobby Cave
        Dj2LoadFadeHsv(202, 214, true, 0x0400, 0x0100, 0x0000, 20);  // Spot
        effectStep = 0;
      }
      break;
      
    default:
      effectStep = 0;
  }
}


void _EvalEffectOff() {
  // Simple effect that fades all lights in Group 2 (Bay Area) off in one step
  Dj2LoadFadeHsv(0, 2, false, 0, 0, 0, 20);
  effectStep = 0;
}
