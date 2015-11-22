// Recording program

#include <EEPROM.h>

#define BUTTON_PIN (2) // record button pin

#define MOSS_START_PIN (44)
#define MOSS_END_PIN (46)

#define FLOWERS_START_PIN (3)
#define FLOWERS_END_PIN (12)

#define SAVE_TO_EEPROM (true)
#define PLAY_FROM_EEPROM (false)

#define EEPROM_HEADER_LENGTH (10)
#define BUF_LEN (4096 - EEPROM_HEADER_LENGTH) // change 4096 to suit EEPROM size

static const uint8_t MAGIC[] = { 0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF };

static bool recording;

static uint8_t buf[BUF_LEN];
static int record_idx;
static int play_idx;

void setup() {
  Serial.begin(9600);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  for (int i = MOSS_START_PIN; i <= MOSS_END_PIN; i++) {
    pinMode(i, OUTPUT);
  }
  
  for (int i = FLOWERS_START_PIN; i <= FLOWERS_END_PIN; i++) {
    pinMode(i, OUTPUT);
  }
  
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  
  if (SAVE_TO_EEPROM) {
    for (int i = 0; i < sizeof(MAGIC); i++) {
      EEPROM.write(i, MAGIC[i]);
    }
  }
  
  if (PLAY_FROM_EEPROM && has_eeprom_data()) {
    record_idx = EEPROM.read(8) | EEPROM.read(9) << 8;
    for (int i = 0; i < record_idx; i++) {
      buf[i] = EEPROM.read(EEPROM_HEADER_LENGTH + i);
    }
  }
}

void loop() {
  if (!PLAY_FROM_EEPROM) {
    bool tmp = recording;
    recording = !digitalRead(BUTTON_PIN);
    if (tmp != recording) {
      if (recording) {
        digitalWrite(13, HIGH);
        record_idx = play_idx = 0;
      } else {
        digitalWrite(13, LOW);
        if (SAVE_TO_EEPROM) {
          EEPROM.write(8, record_idx & 0xff);
          EEPROM.write(9, (record_idx >> 8) & 0xff);
        }
      }
      moss(0);
      flowers(0);
    }
  
    if (Serial.available()) {
      int b = Serial.read();
      if (recording) {
        buf[record_idx++] = b;
        moss(b);
        if (SAVE_TO_EEPROM) {
          EEPROM.write(EEPROM_HEADER_LENGTH + record_idx, b);
        }
      }
    }
  }
    
  bool playing = !recording && record_idx > 0;
  if (playing) {
    int b = buf[play_idx];
    moss(b);
    flowers(b);
    play_idx = (play_idx + 1) % record_idx;
    delay(21);
  }
}

void moss(int brightness) {
  for (int i = MOSS_START_PIN; i <= MOSS_END_PIN; i++) {
    analogWrite(i, brightness);
  }
}

void flowers(int brightness) {
  for (int i = FLOWERS_START_PIN + 1; i <= FLOWERS_END_PIN; i++) {
    analogWrite(i, brightness);
  }
}

bool has_eeprom_data() {
  for (int i = 0; i < sizeof(MAGIC); i++) {
    if (EEPROM.read(i) != MAGIC[i]) {
      return false;
    }
  }
  return true;
}
