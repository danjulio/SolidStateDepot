//
// Contains routines to handle dj2 packets
//
//  Handles the following packets
//    Command 0x31 - FadeHSVUnit - passed with modified address (possibly converted to FadeHSVGroup)
//    Command 0x33 - SetHSVUnit - passed with modified address (possibly converted to SetHSVGroup)
//    Command 0x3B - SetRandomSeed - thrown away (allow groups of lights to fade to different colors when program enabled)
//    Command 0x22 - Set Mode 
//       Mode 0x21 - passed with modified address (converted to multiple units for group addresses)
//       Mode 0xFF - trigger effect
//         Mode Value 0x01 - "Classroom" lighting
//         Mode Value 0x02 - Bay White Lighting ON
//         Mode Value 0x03 - Bay White Lighting OFF
//
//     All other commands ignored (we shouldn't receive anything from the modified remote other than those listed above)
//

//
// dj2 RX States
//
#define DJ2_RX_IDLE     0
#define DJ2_RX_DATA_HI  1
#define DJ2_RX_DATA_LO  2

//
// dj2 packet byte masks
//
#define DJ2_MARKB      0x20
#define DJ2_MARKE_MASK 0x30
#define DJ2_DATA_MASK  0x40

//
// dj2 Packet Command Bytes
//
#define DJ2_SETMODE      0x22
#define DJ2_FADEHSV_GRP  0x30
#define DJ2_FADEHSV_UNIT 0x31
#define DJ2_SETHSV_GRP   0x32
#define DJ2_SETHSV_UNIT  0x33
#define DJ2_SETRND       0x3B

//
// dj2 SetMode Mode Types
//
#define DJ2_MODE_OP      0x21
#define DJ2_MODE_SPECIAL 0xFF


//
// Variables
//
uint8_t dj2rxState;
uint8_t dj2rxPushIndex;
uint8_t dj2rxCurByte;
uint8_t dj2rxParity;
uint8_t dj2rxCount;
uint8_t dj2rxBuffer[16];
boolean dj2rxValid;

uint8_t dj2txCount;
uint8_t dj2txBuffer[16];


//
// API routines
//
void InitDj2() {
  Serial1.begin(57600);
  
  dj2rxState = DJ2_RX_IDLE;
  dj2rxValid = false;
}


void EvalDj2() {
  uint16_t z;
  uint16_t a2;
  boolean isUnit;
  
  // Note we will consume this packet
  dj2rxValid = false;
  
  // Get the current address
  GetCurrentAddress(&z, &a2, &isUnit);

  // Process the packet based on its command byte
  switch (dj2rxBuffer[0]) {
    case DJ2_FADEHSV_UNIT:
    case DJ2_SETHSV_UNIT:
      // Forward this packet with a modified address
      if (isUnit) {
        _Dj2CopyPacketForUnit(z, a2);
      } else {
        _Dj2CopyPacketForGroup(z, a2 & 0xFF);
      }
      _Dj2SendPkt();
      break;
      
    case DJ2_SETMODE:
      // Determine which SetMode command it is
      if (dj2rxBuffer[5] == DJ2_MODE_SPECIAL) {
        // Trigger one of the sequences
        TriggerEffect(dj2rxBuffer[6]);
      } else {
        // Set the operating light mode of the selected fixture(s)
        if (isUnit) {
          // Send to a single unit address
          _Dj2CopyPacketForUnit(z, a2);
          _Dj2SendPkt();
        } else {
          // Send to all units in the selected group
          _Dj2SendSetMode2Group(z, a2 & 0xFF);
        }
      }
      break;
      
      // All other packets are ignored (including the SetRandomSeed packet, which is the only other one we expect)
  }
}


void Dj2ProcessRxChar(char c) {
  uint8_t d;

  switch (dj2rxState) {
    case DJ2_RX_IDLE:
      if (c == DJ2_MARKB) {
        dj2rxPushIndex = 0;
        dj2rxCount = 0;
        dj2rxParity = 0;
        dj2rxState = DJ2_RX_DATA_HI;
      }
      break;
      
    case DJ2_RX_DATA_HI:
      if ((c & 0xF0) == DJ2_DATA_MASK) {
        dj2rxCurByte = (c & 0x0F) << 4;
        dj2rxParity ^= c & 0x0F;
        dj2rxState = DJ2_RX_DATA_LO;
      } else if ((c & 0xF0) == DJ2_MARKE_MASK) {
        if ((c & 0x0F) == dj2rxParity) {
           dj2rxValid = true;
        }
        dj2rxState = DJ2_RX_IDLE;
      } else {
        dj2rxState = DJ2_RX_IDLE;
      }
      break;
      
    case DJ2_RX_DATA_LO:
      if ((c & 0xF0) == DJ2_DATA_MASK) {
        dj2rxCurByte |= c & 0x0F;
        dj2rxParity ^= c & 0x0F;
        if (dj2rxCount < 16) {
          dj2rxBuffer[dj2rxCount++] = dj2rxCurByte;
          dj2rxState = DJ2_RX_DATA_HI;
        } else {
          dj2rxState = DJ2_RX_IDLE;
        }
      } else {
        dj2rxState = DJ2_RX_IDLE;
      }
      break;
    
    default:
      dj2rxState = DJ2_RX_IDLE;
  }
}


boolean Dj2ValidRxPkt() {
  return dj2rxValid;
}


// h is 0 - 0x5FFF
// s is 0 - 0x0100
// v is 0 - 0x0100
// fT is units of 100 mSec
void Dj2LoadFadeHsv(uint16_t z, uint16_t a2, boolean isUnit, uint16_t h, uint16_t s, uint16_t v, uint16_t fT) {
  uint8_t i = 0;
  
  // Command
  if (isUnit) {
    dj2txBuffer[i++] = DJ2_FADEHSV_UNIT;
  } else {
    dj2txBuffer[i++] = DJ2_FADEHSV_GRP;
  }
  
  // Address
  dj2txBuffer[i++] = z >> 8;
  dj2txBuffer[i++] = z & 0xFF;
  if (isUnit) {
    dj2txBuffer[i++] = a2 >> 8;
  }
  dj2txBuffer[i++] = a2 & 0xFF;
  
  // HSV Flags
  dj2txBuffer[i++] = 0x0E;
  
  // HSV
  dj2txBuffer[i++] = h >> 8;
  dj2txBuffer[i++] = h & 0xFF;
  dj2txBuffer[i++] = s >> 8;
  dj2txBuffer[i++] = s & 0xFF;
  dj2txBuffer[i++] = v >> 8;
  dj2txBuffer[i++] = v & 0xFF;
  
  // Fade Time
  dj2txBuffer[i++] = fT >> 8;
  dj2txBuffer[i++] = fT & 0xFF;
  
  dj2txCount = i;
  _Dj2SendPkt();
}


// h is 0 - 0x5FFF
// s is 0 - 0x0100
// v is 0 - 0x0100
void Dj2LoadSetHsv(uint16_t z, uint16_t a2, boolean isUnit, uint16_t h, uint16_t s, uint16_t v) {
  uint8_t i = 0;
  
  // Command
  if (isUnit) {
    dj2txBuffer[i++] = DJ2_SETHSV_UNIT;
  } else {
    dj2txBuffer[i++] = DJ2_SETHSV_GRP;
  }
  
  // Address
  dj2txBuffer[i++] = z >> 8;
  dj2txBuffer[i++] = z & 0xFF;
  if (isUnit) {
    dj2txBuffer[i++] = a2 >> 8;
  }
  dj2txBuffer[i++] = a2 & 0xFF;
  
  // HSV Flags
  dj2txBuffer[i++] = 0x0E;
  
  // HSV
  dj2txBuffer[i++] = h >> 8;
  dj2txBuffer[i++] = h & 0xFF;
  dj2txBuffer[i++] = s >> 8;
  dj2txBuffer[i++] = s & 0xFF;
  dj2txBuffer[i++] = v >> 8;
  dj2txBuffer[i++] = v & 0xFF;
  
  dj2txCount = i;
  _Dj2SendPkt();
}
  


//
// Internal routines
//
void _Dj2SendPkt() {
  uint8_t parity = 0;
  uint8_t c;
  
  Serial1.write(DJ2_MARKB);
  for (int i=0; i<dj2txCount; i++) {
    c = dj2txBuffer[i];
    Serial1.write(DJ2_DATA_MASK | (c >> 4));
    parity ^= c >> 4;
    Serial1.write(DJ2_DATA_MASK | (c & 0x0F));
    parity ^= c & 0x0F;
  }
  Serial1.write(DJ2_MARKE_MASK | (parity & 0x0F));
}


void _Dj2CopyPacketForGroup(uint16_t z, uint8_t g) {
  int i;
  
  // Treat byte[3] as group address and skip byte[5] (Unit[7:0])
  dj2txCount = dj2rxCount - 1;
  
  for (int i=0; i<4; i++) {
    dj2txBuffer[i] = dj2rxBuffer[i];
  }
  for (int i=5; i<dj2rxCount; i++) {
    dj2txBuffer[i-1] = dj2rxBuffer[i];
  }
  
  // Change the command to the group version (this is a hack - light commands have bit[0] clear for group version)
  dj2txBuffer[0] &= 0xFE;
  
  // Update address information
  dj2txBuffer[1] = z >> 8;
  dj2txBuffer[2] = z & 0xFF;
  dj2txBuffer[3] = g;
}


void _Dj2CopyPacketForUnit(uint16_t z, uint16_t u) {
  // Copy all bytes from RX to TX
  dj2txCount = dj2rxCount;
  for (int i=0; i<dj2rxCount; i++) {
    dj2txBuffer[i] = dj2rxBuffer[i];
  }
  
  // Update address information
  dj2txBuffer[1] = z >> 8;
  dj2txBuffer[2] = z & 0xFF;
  dj2txBuffer[3] = u >> 8;
  dj2txBuffer[4] = u & 0xFF;
}


void _Dj2SendSetMode2Group(uint16_t z, uint8_t g) {
  uint8_t n;
  uint16_t u;
  
  if ((n = SetupGroupUnitAddressList(g)) != 0) {
    for (uint8_t i=0; i<n; i++) {
      u = GetGroupUnit(i);
      _Dj2CopyPacketForUnit(z, u);
      _Dj2SendPkt();
    }
  }
}

