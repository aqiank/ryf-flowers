// Playback program

#include <EEPROM.h>

#define MOSS_START_PIN (44)
#define MOSS_END_PIN (46)

#define FLOWERS_START_PIN (3)
#define FLOWERS_END_PIN (12)

#define EEPROM_HEADER_LENGTH (10)
#define BUF_LEN (4096 - EEPROM_HEADER_LENGTH) // change 4096 to suit EEPROM size

static const uint8_t MAGIC[] = { 0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF };

static uint8_t buf[BUF_LEN];
static int record_idx;
static int play_idx;
static bool can_play;

void setup() {
  Serial.begin(9600);
  
  for (int i = MOSS_START_PIN; i <= MOSS_END_PIN; i++) {
    pinMode(i, OUTPUT);
  }
  
  for (int i = FLOWERS_START_PIN; i <= FLOWERS_END_PIN; i++) {
    pinMode(i, OUTPUT);
  }
  
  can_play = has_eeprom_data();
  if (can_play) {
    record_idx = EEPROM.read(8) | EEPROM.read(9) << 8;
    for (int i = 0; i < record_idx; i++) {
      buf[i] = EEPROM.read(EEPROM_HEADER_LENGTH + i);
    }
  }
}

void loop() {
  if (record_idx > 0 && record_idx < BUF_LEN) {
    int b = buf[play_idx];
    moss(b);
    flowers(b);
    play_idx = (play_idx + 1) % (record_idx + 1);
  }
  delay(20);
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
