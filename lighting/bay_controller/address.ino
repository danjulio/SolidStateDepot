//
// Contains code that manages the current address and updates the LCD
//

//
// Address Modes
//
#define ADDR_MODE_BAY_W       0
#define ADDR_MODE_BAY_C       1
#define ADDR_MODE_BAY_A       2
#define ADDR_MODE_LOBBY       3
#define ADDR_MODE_SPC_NEXT_ON 4

//
// Rotary counts per change - sensitivity
//
#define ROT_COUNTS_PER_INC    8


//
// Addresses - this is a gross hack!  We want to use PROGMEM so this stuff isn't stored in RAM.  I originally defined a struct
// but couldn't figure out how to successfully store it all in PROGMEM so in a fit of pique, gave up and moved everything into
// individual arrays.  Sorry.  (of course, later I figured out, at least in my head, how to do it but by then, the motivation to
// move everything back was gone).
//
#define NUM_BAY_ADDRESSES 22
const char BAY_S1[] PROGMEM = "    All Units   ";
const char BAY_S2[] PROGMEM = "     Unit 1     ";
const char BAY_S3[] PROGMEM = "     Unit 2     ";
const char BAY_S4[] PROGMEM = "     Unit 3     ";
const char BAY_S5[] PROGMEM = "     Unit 4     ";
const char BAY_S6[] PROGMEM = "     Unit 5     ";
const char BAY_S7[] PROGMEM = "     Unit 6     ";
const char BAY_S8[] PROGMEM = "     Unit 7     ";
const char BAY_S9[] PROGMEM = "     Unit 8     ";
const char BAY_S10[] PROGMEM = "     Unit 9     ";
const char BAY_S11[] PROGMEM = "     Unit 10    ";
const char BAY_S12[] PROGMEM = "     Unit 11    ";
const char BAY_S13[] PROGMEM = "     Unit 12    ";
const char BAY_S14[] PROGMEM = "     Row 1      ";
const char BAY_S15[] PROGMEM = "     Row 2      ";
const char BAY_S16[] PROGMEM = "     Row 3      ";
const char BAY_S17[] PROGMEM = "     Row 4      ";
const char BAY_S18[] PROGMEM = "    Column 1    ";
const char BAY_S19[] PROGMEM = "    Column 2    ";
const char BAY_S20[] PROGMEM = "    Column 3    ";
const char BAY_S21[] PROGMEM = "      Even      ";
const char BAY_S22[] PROGMEM = "      Odd       ";
const uint16_t bayAddrsWC_Z[NUM_BAY_ADDRESSES] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const uint16_t bayAddrsWC_A2[NUM_BAY_ADDRESSES] PROGMEM = {2, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212,
                                                           10, 11, 12, 13, 20, 21, 22, 30, 31};
const boolean bayAddrsWC_isU[NUM_BAY_ADDRESSES] PROGMEM = {false, true, true, true, true, true, true, true, true, true, true, true, true,
                                                           false, false, false, false, false, false, false, false, false};
const char* const bayAddrsWC_S[NUM_BAY_ADDRESSES] PROGMEM = {BAY_S1, BAY_S2, BAY_S3, BAY_S4, BAY_S5, BAY_S6, BAY_S7, BAY_S8, BAY_S9, BAY_S10, BAY_S11,
                                                             BAY_S12, BAY_S13, BAY_S14, BAY_S15, BAY_S16, BAY_S17, BAY_S18, BAY_S19, BAY_S20, BAY_S21, BAY_S22};


const uint16_t bayAddrsW_Z[NUM_BAY_ADDRESSES] PROGMEM = {200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200,
                                                         200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200};
const uint16_t bayAddrsW_A2[NUM_BAY_ADDRESSES] PROGMEM = {0, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212,
                                                          10, 11, 12, 13, 20, 21, 22, 30, 31};
const boolean bayAddrsW_isU[NUM_BAY_ADDRESSES] PROGMEM = {true, true, true, true, true, true, true, true, true, true, true, true, true,
                                                          false, false, false, false, false, false, false, false, false};
const char* const bayAddrsW_S[NUM_BAY_ADDRESSES] PROGMEM = {BAY_S1, BAY_S2, BAY_S3, BAY_S4, BAY_S5, BAY_S6, BAY_S7, BAY_S8, BAY_S9, BAY_S10, BAY_S11,
                                                            BAY_S12, BAY_S13, BAY_S14, BAY_S15, BAY_S16, BAY_S17, BAY_S18, BAY_S19, BAY_S20, BAY_S21, BAY_S22};


const uint16_t bayAddrsC_Z[NUM_BAY_ADDRESSES] PROGMEM = {201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201,
                                                         201, 201, 201, 201, 201, 201, 201, 201, 201, 201, 201};
const uint16_t bayAddrsC_A2[NUM_BAY_ADDRESSES] PROGMEM = {0, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212,
                                                          10, 11, 12, 13, 20, 21, 22, 30, 31};
const boolean bayAddrsC_isU[NUM_BAY_ADDRESSES] PROGMEM = {true, true, true, true, true, true, true, true, true, true, true, true, true,
                                                          false, false, false, false, false, false, false, false, false};
const char* const bayAddrsC_S[NUM_BAY_ADDRESSES] PROGMEM = {BAY_S1, BAY_S2, BAY_S3, BAY_S4, BAY_S5, BAY_S6, BAY_S7, BAY_S8, BAY_S9, BAY_S10, BAY_S11,
                                                            BAY_S12, BAY_S13, BAY_S14, BAY_S15, BAY_S16, BAY_S17, BAY_S18, BAY_S19, BAY_S20, BAY_S21, BAY_S22};



#define NUM_BAY_DMX 5
const char BAY_ALL[] PROGMEM = "    All Units   ";
const char BAY_DMX1[] PROGMEM = "      Cave      ";
const char BAY_DMX2[] PROGMEM = "      Spot      ";
const char BAY_DMX3[] PROGMEM = "    Geometry    ";
const char BAY_DMX4[] PROGMEM = "    Cylinder    ";
const uint16_t bayAddrsDmx_Z[NUM_BAY_DMX] PROGMEM  = {202, 202, 202, 202, 202};
const uint16_t bayAddrsDmx_A2[NUM_BAY_DMX] PROGMEM = {0, 213, 214, 215, 216};
const boolean bayAddrsDmx_isU[NUM_BAY_DMX] PROGMEM = {true, true, true, true, true};
const char* const bayAddrsDmx_S[NUM_BAY_DMX] PROGMEM = {BAY_ALL, BAY_DMX1, BAY_DMX2, BAY_DMX3, BAY_DMX4};


#define NUM_LOBBY 17
const char LOBBY_S1[] PROGMEM = "    All Units   ";
const char LOBBY_S2[] PROGMEM = "    Overhead    ";
const char LOBBY_S3[] PROGMEM = "    All Color   ";
const char LOBBY_S4[] PROGMEM = "    SSD Sign    ";
const char LOBBY_S5[] PROGMEM = "   Wall Unit 1  ";
const char LOBBY_S6[] PROGMEM = "   Wall Unit 2  ";
const char LOBBY_S7[] PROGMEM = "   Wall Unit 3  ";
const char LOBBY_S8[] PROGMEM = "   Wall Unit 4  ";
const char LOBBY_S9[] PROGMEM = "   Wall Unit 5  ";
const char LOBBY_S10[] PROGMEM = "   Wall Unit 6  ";
const char LOBBY_S11[] PROGMEM = "  All Lanterns  ";
const char LOBBY_S12[] PROGMEM = " Small Lantern  ";
const char LOBBY_S13[] PROGMEM = " Medium Lantern ";
const char LOBBY_S14[] PROGMEM = " Large Lantern  ";
const char LOBBY_S15[] PROGMEM = "   South Wall   ";
const char LOBBY_S16[] PROGMEM = "   North Wall   ";
const char LOBBY_S17[] PROGMEM = "    End Walls   ";
const uint16_t lobbyAddrs_Z[NUM_LOBBY] PROGMEM  = {0, 100, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101};
const uint16_t lobbyAddrs_A2[NUM_LOBBY] PROGMEM = {1, 1, 0, 2, 101, 102, 103, 104, 105, 106, 40, 107, 108, 109, 14, 16, 15};
const boolean lobbyAddrs_isU[NUM_LOBBY] PROGMEM = {false, true, true, true, true, true, true, true, true, true, 
                                                   false, true, true, true, false, false, false};
const char* const lobbyAddrs_S[NUM_LOBBY] PROGMEM = {LOBBY_S1, LOBBY_S2, LOBBY_S3, LOBBY_S4, LOBBY_S5, LOBBY_S6, LOBBY_S7,
                                                     LOBBY_S8, LOBBY_S9, LOBBY_S10, LOBBY_S11, LOBBY_S12, LOBBY_S13, LOBBY_S14,
                                                     LOBBY_S15, LOBBY_S16, LOBBY_S17};
                                                     

// specialAddrs are treated "specially".  We only use the text string.
#define NUM_SPECIAL 6
const char SPECIAL_S1[] PROGMEM = "     Random     ";
const char SPECIAL_S2[] PROGMEM = "  All At Once   ";
const char SPECIAL_S3[] PROGMEM = "Smooth Sequence ";
const char SPECIAL_S4[] PROGMEM = " Relays Clatter ";
const char SPECIAL_S5[] PROGMEM = "  Relays Clunk  ";
const char SPECIAL_S6[] PROGMEM = "Old Fluorescent ";
const char* const special_S[NUM_SPECIAL] PROGMEM = {SPECIAL_S1, SPECIAL_S2, SPECIAL_S3, SPECIAL_S4, SPECIAL_S5, SPECIAL_S6};


//
// Units in each group (necessary because the SetMode that is issued when the "AUTO" touch button on the dj2_sc touchpad
// is pressed command can only be sent to unit addresses).  Sadly these need to be updated each time a new group is added
// to any of the above address collections.  There is code to display a missing group on the LCD.  Ideally this all should
// in a data structure that associated all units with their groups and could be queried to automatically compute this info.
//
#define NUM_GROUP1_UNITS 8
const uint16_t group1units[NUM_GROUP1_UNITS] PROGMEM = {1, 2, 101, 102, 103, 104, 105, 106};
#define NUM_GROUP2_UNITS 13
const uint16_t group2units[NUM_GROUP2_UNITS] PROGMEM = {201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213};
#define NUM_GROUP10_UNITS 3
const uint16_t group10units[NUM_GROUP10_UNITS] PROGMEM = {201, 202, 203};
#define NUM_GROUP11_UNITS 3
const uint16_t group11units[NUM_GROUP11_UNITS] PROGMEM = {204, 205, 206};
#define NUM_GROUP12_UNITS 3
const uint16_t group12units[NUM_GROUP12_UNITS] PROGMEM = {207, 208, 209};
#define NUM_GROUP13_UNITS 3
const uint16_t group13units[NUM_GROUP13_UNITS] PROGMEM = {210, 211, 212};
#define NUM_GROUP14_UNITS 3
const uint16_t group14units[NUM_GROUP14_UNITS] PROGMEM = {102, 103, 109};
#define NUM_GROUP15_UNITS 2
const uint16_t group15units[NUM_GROUP15_UNITS] PROGMEM = {101, 104};
#define NUM_GROUP16_UNITS 4
const uint16_t group16units[NUM_GROUP16_UNITS] PROGMEM = {105, 106, 107, 108};
#define NUM_GROUP20_UNITS 4
const uint16_t group20units[NUM_GROUP20_UNITS] PROGMEM = {201, 206, 207, 212};
#define NUM_GROUP21_UNITS 4
const uint16_t group21units[NUM_GROUP21_UNITS] PROGMEM = {202, 205, 208, 211};
#define NUM_GROUP22_UNITS 4
const uint16_t group22units[NUM_GROUP22_UNITS] PROGMEM = {203, 204, 209, 210};
#define NUM_GROUP23_UNITS 3
const uint16_t group23units[NUM_GROUP23_UNITS] PROGMEM = {104, 108, 109};
#define NUM_GROUP24_UNITS 2
const uint16_t group24units[NUM_GROUP24_UNITS] PROGMEM = {103, 105};
#define NUM_GROUP25_UNITS 2
const uint16_t group25units[NUM_GROUP24_UNITS] PROGMEM = {102, 106};
#define NUM_GROUP26_UNITS 2
const uint16_t group26units[NUM_GROUP26_UNITS] PROGMEM = {101, 107};
#define NUM_GROUP30_UNITS 6
const uint16_t group30units[NUM_GROUP30_UNITS] PROGMEM = {202, 204, 206, 208, 210, 212};
#define NUM_GROUP31_UNITS 6
const uint16_t group31units[NUM_GROUP31_UNITS] PROGMEM = {201, 203, 205, 207, 209, 211};
#define NUM_GROUP32_UNITS 3
const uint16_t group32units[NUM_GROUP32_UNITS] PROGMEM = {102, 104, 106};
#define NUM_GROUP33_UNITS 3
const uint16_t group33units[NUM_GROUP33_UNITS] PROGMEM = {101, 103, 105};
#define NUM_GROUP40_UNITS 3
const uint16_t group40units[NUM_GROUP40_UNITS] PROGMEM = {107, 108, 109};




//
// Module variables
//
uint8_t addrMode;
uint8_t addrIndex;

uint8_t prevAddrMode;
uint8_t prevAddrIndex;

uint8_t prevSpecMode;
uint8_t prevSpecIndex;

int remainingRotAcc;

uint16_t curZoneAddr;
uint16_t curSecondAddr;
boolean curIsUnit;  // True for Unit address, False for Group Address

uint8_t maxGroupUnits;
uint16_t* curGroupArrayP;


//
// API routines
//
void InitAddress() {
  lcd.begin(DOG_LCD_M162);
  lcd.noCursor();
  
  prevSpecMode = ADDR_MODE_SPC_NEXT_ON;
  prevSpecIndex = 0;
  SetDefaultAddress();
}


void EvalAddress() {
  int8_t c;
  uint8_t b;
  boolean sawChange = false;
  
  // Look for rotary encoder change
  c = GetRotaryCount();
  if (c != 0) {
    sawChange = _UpdateIndex(c);
  }
  
  // Look for rotary button press
  b = GetRotaryButton();
  if (b == ROT_BUT_SHORT) {
    // Increment between selecting addresses
    if (addrMode != ADDR_MODE_SPC_NEXT_ON) {
      if (++addrMode > ADDR_MODE_LOBBY) {
       addrMode = ADDR_MODE_BAY_W;
      }
      remainingRotAcc = 0;
      addrIndex = 0;
    }
    sawChange = true;
  } else if (b == ROT_BUT_LONG) {
    // Swap between selecting addresses and power on-sequences.
    if (addrMode == ADDR_MODE_SPC_NEXT_ON) {
      prevSpecMode = addrMode;
      prevSpecIndex = addrIndex;
      addrMode = prevAddrMode;
      addrIndex = prevAddrIndex;
    } else {
      prevAddrMode = addrMode;
      prevAddrIndex = addrIndex;
      addrMode = prevSpecMode;
      addrIndex = prevSpecIndex;
    }
    remainingRotAcc = 0;
    sawChange = true;
  }
  
  // Update our state if something changed
  if (sawChange) {
    NoteUserActivity();
    _UpdateAddrState();
  }
}


void SetDefaultAddress() {
  addrMode = ADDR_MODE_BAY_W;
  addrIndex = 0;
  prevAddrMode = ADDR_MODE_BAY_W;
  prevAddrIndex = 0;
  
  remainingRotAcc = 0;
  
  _UpdateAddrState();
}


void GetCurrentAddress(uint16_t* z, uint16_t* a2, boolean* isUnit) {
  *z = curZoneAddr;
  *a2 = curSecondAddr;
  *isUnit = curIsUnit;
}


// Returns number of unit addresses in the group, 0 if the group wasn't found
uint8_t SetupGroupUnitAddressList(uint8_t g) {
  maxGroupUnits = 0;
  
  switch (g) {
    case 1:
      maxGroupUnits = NUM_GROUP1_UNITS;
      curGroupArrayP = (uint16_t *) group1units;
      break;
    
    case 2:
      maxGroupUnits = NUM_GROUP2_UNITS;
      curGroupArrayP = (uint16_t *) group2units;
      break;
    
    case 10:
      maxGroupUnits = NUM_GROUP10_UNITS;
      curGroupArrayP = (uint16_t *) group10units;
      break;
    
    case 11:
      maxGroupUnits = NUM_GROUP11_UNITS;
      curGroupArrayP = (uint16_t *) group11units;
      break;
    
    case 12:
      maxGroupUnits = NUM_GROUP12_UNITS;
      curGroupArrayP = (uint16_t *) group12units;
      break;
    
    case 13:
      maxGroupUnits = NUM_GROUP13_UNITS;
      curGroupArrayP = (uint16_t *) group13units;
      break;
    
    case 14:
      maxGroupUnits = NUM_GROUP14_UNITS;
      curGroupArrayP = (uint16_t *) group14units;
      break;
    
    case 15:
      maxGroupUnits = NUM_GROUP15_UNITS;
      curGroupArrayP = (uint16_t *) group15units;
      break;
    
    case 16:
      maxGroupUnits = NUM_GROUP16_UNITS;
      curGroupArrayP = (uint16_t *) group16units;
      break;
      break;
    
    case 20:
      maxGroupUnits = NUM_GROUP20_UNITS;
      curGroupArrayP = (uint16_t *) group20units;
      break;
    
    case 21:
      maxGroupUnits = NUM_GROUP21_UNITS;
      curGroupArrayP = (uint16_t *) group21units;
      break;
    
    case 22:
      maxGroupUnits = NUM_GROUP22_UNITS;
      curGroupArrayP = (uint16_t *) group22units;
      break;
    
    case 23:
      maxGroupUnits = NUM_GROUP23_UNITS;
      curGroupArrayP = (uint16_t *) group23units;
      break;
    
    case 24:
      maxGroupUnits = NUM_GROUP24_UNITS;
      curGroupArrayP = (uint16_t *) group24units;
      break;
    
    case 25:
      maxGroupUnits = NUM_GROUP25_UNITS;
      curGroupArrayP = (uint16_t *) group25units;
      break;
    
    case 26:
      maxGroupUnits = NUM_GROUP26_UNITS;
      curGroupArrayP = (uint16_t *) group26units;
      break;
    
    case 30:
      maxGroupUnits = NUM_GROUP30_UNITS;
      curGroupArrayP = (uint16_t *) group30units;
      break;
    
    case 31:
      maxGroupUnits = NUM_GROUP31_UNITS;
      curGroupArrayP = (uint16_t *) group31units;
      break;
    
    case 32:
      maxGroupUnits = NUM_GROUP32_UNITS;
      curGroupArrayP = (uint16_t *) group32units;
      break;
    
    case 33:
      maxGroupUnits = NUM_GROUP33_UNITS;
      curGroupArrayP = (uint16_t *) group33units;
      break;
      
    case 40:
      maxGroupUnits = NUM_GROUP40_UNITS;
      curGroupArrayP = (uint16_t *) group40units;
      break;
    
    default:
     // Group not found, display an error message on the LCD
     lcd.setCursor(0, 0);
     lcd.print("  Group Error!  ");
     lcd.setCursor(0, 1);
     lcd.print("Grp ");
     lcd.print(g);
     lcd.print(" not found");
  }
  
  return maxGroupUnits;
}


// Don't call this with an index that is out of bounds... (you get that from SetupGroupUnitAddressList)
uint16_t GetGroupUnit(uint8_t i) {
  uint16_t u = 0;
  
  if (i < maxGroupUnits) {
    u = (uint16_t) pgm_read_word(curGroupArrayP + i);
  }
  
  return u;
}


//
// Internal routines
//
void _UpdateAddrState() {
  _SetRotaryColor();
  _SetValues();
  _SetLCD();
};



void _SetRotaryColor() {
  switch (addrMode) {
    case ADDR_MODE_BAY_W:
    case ADDR_MODE_BAY_C:
    case ADDR_MODE_BAY_A:
      SetRotaryLed(ROT_LED_GREEN);
      break;
    
    case ADDR_MODE_LOBBY:
      SetRotaryLed(ROT_LED_YELLOW);
      break;
    
    default:
      SetRotaryLed(ROT_LED_RED);
      break;
  }
}


void _SetLCD() {
  char* pstrPtr;
  char lcdBuff[17];
  
  lcd.setCursor(0, 0);
  switch (addrMode) {
    case ADDR_MODE_BAY_W:
      lcd.print(F(" Overhead White "));
      pstrPtr = (char *) pgm_read_word(&(bayAddrsW_S[addrIndex]));
      break;
      
    case ADDR_MODE_BAY_C:
      lcd.print(F(" Overhead Color "));
      pstrPtr = (char *) pgm_read_word(&(bayAddrsC_S[addrIndex]));
      break;
    
    case ADDR_MODE_BAY_A:
      lcd.print(F("   Bay Accent   "));
      pstrPtr = (char *) pgm_read_word(&(bayAddrsDmx_S[addrIndex]));
      break;
    
    case ADDR_MODE_LOBBY:
      lcd.print(F("     Lobby      "));
      pstrPtr = (char *) pgm_read_word(&(lobbyAddrs_S[addrIndex]));
      break;
      
    default:
      lcd.print(F("  On Sequence   "));
      pstrPtr = (char *) pgm_read_word(&(special_S[addrIndex]));
  }
  
  // Get and update the second line from our program memory array
  strcpy_P(lcdBuff, pstrPtr);
  lcd.setCursor(0, 1);
  lcd.print(lcdBuff);
}


uint8_t _GetMaxIndex() {
  switch (addrMode) {
    case ADDR_MODE_BAY_W:
    case ADDR_MODE_BAY_C:
      return(NUM_BAY_ADDRESSES-1);

    case ADDR_MODE_BAY_A:
      return(NUM_BAY_DMX-1);
      
    case ADDR_MODE_LOBBY:
      return(NUM_LOBBY-1);
      
    default:
    return(NUM_SPECIAL-1);
  }
}


boolean _UpdateIndex(int8_t c) {
  uint8_t deltaChange;
  
  // Accumulate current rotary change
  remainingRotAcc += c;
  
  // Look for enough change
  if (abs(remainingRotAcc) > ROT_COUNTS_PER_INC) {
    deltaChange = abs(remainingRotAcc) / ROT_COUNTS_PER_INC;
    if (remainingRotAcc > 0) {
      // Positive
      _IncIndex(deltaChange);
      remainingRotAcc -= deltaChange * ROT_COUNTS_PER_INC;
    } else {
      // Negative
      _DecIndex(deltaChange);
      remainingRotAcc += deltaChange * ROT_COUNTS_PER_INC;
    }
    return(true);
  } else {
    return(false);
  }
}


void _IncIndex(uint8_t d) {
  uint8_t maxIndex= _GetMaxIndex();
  
  while (d--) {
    if (++addrIndex > maxIndex) addrIndex = 0;
  }
}


void _DecIndex(uint8_t d) {
  uint8_t maxIndex= _GetMaxIndex();
  while (d--) {
    if (addrIndex == 0) {
      addrIndex = maxIndex;
    } else {
      --addrIndex;
    }
  }
}


void _SetValues() {
  if (addrMode != ADDR_MODE_SPC_NEXT_ON) {
    switch (addrMode) {
      case ADDR_MODE_BAY_W:
        curZoneAddr = (uint16_t) pgm_read_word(&bayAddrsW_Z[addrIndex]);
        curSecondAddr = (uint16_t) pgm_read_word(&bayAddrsW_A2[addrIndex]);
        curIsUnit = pgm_read_byte(&bayAddrsW_isU[addrIndex]);
        break;
      
      case ADDR_MODE_BAY_C:
        curZoneAddr = (uint16_t) pgm_read_word(&bayAddrsC_Z[addrIndex]);
        curSecondAddr = (uint16_t) pgm_read_word(&bayAddrsC_A2[addrIndex]);
        curIsUnit = pgm_read_byte(&bayAddrsC_isU[addrIndex]);
        break;
    
      case ADDR_MODE_BAY_A:
        curZoneAddr = (uint16_t) pgm_read_word(&bayAddrsDmx_Z[addrIndex]);
        curSecondAddr = (uint16_t) pgm_read_word(&bayAddrsDmx_A2[addrIndex]);
        curIsUnit = pgm_read_byte(&bayAddrsDmx_isU[addrIndex]);
        break;
    
      case ADDR_MODE_LOBBY:
        curZoneAddr = (uint16_t) pgm_read_word(&lobbyAddrs_Z[addrIndex]);
        curSecondAddr = (uint16_t) pgm_read_word(&lobbyAddrs_A2[addrIndex]);
        curIsUnit = pgm_read_byte(&lobbyAddrs_isU[addrIndex]);
        break;
    }
  } else {
    SetAllowedEffect(addrIndex);
  }
}
