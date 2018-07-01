//
// Sound control
//


//
// SOUND_TRIGGER TIME is mSec
//
#define SOUND_TRIGGER_TIME 50


uint16_t soundTimer;    // Set in units of 10mSec to pulse the input to the sound board
int soundPin;


void InitSound() {
  // All trigger signals are left as input until used to let the sound board's pull-ups
  // pull them to 3.3V (since we're a 5V device, we don't want to drive high-levels to the
  // sound board)
  pinMode(SOUND_1_PIN, INPUT);
  pinMode(SOUND_2_PIN, INPUT);
  pinMode(SOUND_3_PIN, INPUT);
  pinMode(SOUND_4_PIN, INPUT);
  pinMode(SOUND_5_PIN, INPUT);
  pinMode(SOUND_6_PIN, INPUT);
}


void EvalSound() {
  if (soundTimer != 0) {
    if (--soundTimer == 0) {
      _SoundDisable();
    }
  }
}


void SoundEnable(int n) {
  _SoundNtoPin(n);
  digitalWrite(soundPin, LOW);
  pinMode(soundPin, OUTPUT);
  soundTimer = SOUND_TRIGGER_TIME/10;
}


void _SoundNtoPin(int n) {
  switch (n) {
    case 1: soundPin = SOUND_1_PIN; break;
    case 2: soundPin = SOUND_2_PIN; break;
    case 3: soundPin = SOUND_3_PIN; break;
    case 4: soundPin = SOUND_4_PIN; break;
    case 5: soundPin = SOUND_5_PIN; break;
    case 6: soundPin = SOUND_6_PIN; break;
  }
}

void _SoundDisable() {
  digitalWrite(soundPin, HIGH);
  pinMode(soundPin, INPUT);
}
