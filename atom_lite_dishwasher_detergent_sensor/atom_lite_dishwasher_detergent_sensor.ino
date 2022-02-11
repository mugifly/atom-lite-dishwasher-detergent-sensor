// atom-lite-dishwasher-detergent-sensor
// https://github.com/mugifly/atom-lite-dishwasher-detergent-sensor

// M5Atom v0.0.2 - https://github.com/m5stack/M5Atom
#include "M5Atom.h"

// HX711 Arduino Library v0.7.5 - https://github.com/bogde/HX711
#include "HX711.h"

// Configurations
#define LOADCELL_DOUT_PIN 32
#define LOADCELL_SCK_PIN 26
#define LOADCELL_DIVIDER 5895655
#define LOADCELL_OFFSET 50682624

#define LOADCELL_VALID_MINIMUM_THRESHOLD_WEIGHT_GRAM 10

#define DETERGENT_ONLY_FEW_LEFT_THRESHOLD_WEIGHT_GRAM 100
#define DETERGENT_EXTRA_USAGE_THRESHOLD_WEIGHT_GRAM 8
#define DETERGENT_MIN_USAGE_THRESHOLD_WEIGHT_GRAM 5
#define DETERGENT_USAGE_RESETTING_INTERVAL_MILISEC 5400000 // 1.5 hours

// Loadcell sensor (scale)
HX711 loadcell;
float detergentBottleWeight = 0.0;

// Status
enum Status {
  // LED is turn off
  DETERGENT_NOT_USED,
  // LED is blinking white
  LOADCELL_RESETTED, // blinking 5 times
  // LED is blinking green
  DETERGENT_USUALLY_USED, // Once blinking
  DETERGENT_EXTRA_USED, // Twice blinking
  // LED is blinking yellow
  DETERGENT_USUALLY_USED_AND_ONLY_FEW_LEFT, // Once blinking
  DETERGENT_EXTRA_USED_AND_ONLY_FEW_LEFT, // Twice blinking
  // LED is blinking red
  MEASURED_WEIGHT_INVALID, // Fast blinking
};
Status status = DETERGENT_NOT_USED;
unsigned long statusUpdatedAt = 0L;

void setup() {
  M5.begin(true, false, true);

  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  loadcell.set_scale(LOADCELL_DIVIDER);
  loadcell.set_offset(LOADCELL_OFFSET);
}

void resetLoadcell() {
  loadcell.tare(10);
  status = LOADCELL_RESETTED;
  detergentBottleWeight = loadcell.get_units(10);
}

void measureRemainingAmount() {
  unsigned long now = millis();
  float weight = loadcell.get_units(10);

  if (abs(detergentBottleWeight - weight) <= 1) { // weight is almost not changed
    switch (status) {
      case DETERGENT_USUALLY_USED:
      case DETERGENT_EXTRA_USED:
      case DETERGENT_USUALLY_USED_AND_ONLY_FEW_LEFT:
      case DETERGENT_EXTRA_USED_AND_ONLY_FEW_LEFT:
        if (DETERGENT_USAGE_RESETTING_INTERVAL_MILISEC <= now - statusUpdatedAt) {
          // Reset status to "detergent is not used"
          status = DETERGENT_NOT_USED;
          statusUpdatedAt = now;
        }
        break;
    };
    return;
  }

  if (weight <= LOADCELL_VALID_MINIMUM_THRESHOLD_WEIGHT_GRAM) {
    status = MEASURED_WEIGHT_INVALID;
  } else if (weight <= detergentBottleWeight - DETERGENT_EXTRA_USAGE_THRESHOLD_WEIGHT_GRAM) { // a lot of used
    if (weight <= DETERGENT_ONLY_FEW_LEFT_THRESHOLD_WEIGHT_GRAM) {
      status = DETERGENT_EXTRA_USED_AND_ONLY_FEW_LEFT;
    } else {
      status = DETERGENT_EXTRA_USED;
    }
  } else if (weight <= detergentBottleWeight - DETERGENT_MIN_USAGE_THRESHOLD_WEIGHT_GRAM) { // usually used
    if (weight <= DETERGENT_ONLY_FEW_LEFT_THRESHOLD_WEIGHT_GRAM) {
      status = DETERGENT_USUALLY_USED_AND_ONLY_FEW_LEFT;
    } else {
      status = DETERGENT_USUALLY_USED;
    }
  } else { // Not used or replenished
    status = DETERGENT_NOT_USED;
  }
    
  statusUpdatedAt = now;
  if (status != MEASURED_WEIGHT_INVALID) {
    detergentBottleWeight = weight;
  }

}

void showStatus() {

  int color = 0x000000;
  int numOfBlinks = 0;

  switch (status) {
    case LOADCELL_RESETTED:
      color = 0xf0f0f0; // white
      numOfBlinks = 5;
    case DETERGENT_NOT_USED:
      // LED is turn off
      break;
    case DETERGENT_USUALLY_USED:
      color = 0xf00000; // green
      numOfBlinks = 1;
      break;
    case DETERGENT_EXTRA_USED:
      color = 0xf00000; // green
      numOfBlinks = 2;
      break;
    case DETERGENT_USUALLY_USED_AND_ONLY_FEW_LEFT:
      color = 0xf0f000; // yellow
      numOfBlinks = 1;
      break;
    case DETERGENT_EXTRA_USED_AND_ONLY_FEW_LEFT:
      color = 0xf0f000; // yellow
      numOfBlinks = 2;
      break;
    case MEASURED_WEIGHT_INVALID:
      color = 0, 0x00f000; // red
      numOfBlinks = -1; // fast blinking
      break;
  };

  if (color == 0x000000) {
    // LED is turn off
    M5.dis.drawpix(0, color);
    return;
  }

  if (numOfBlinks == -1) {
    // LED is fast blinking
    M5.dis.drawpix(0, color);
    delay(50);
    M5.dis.drawpix(0, 0x000000);
    delay(50);
    return;
  }

  for (int count = 0; count < numOfBlinks; count++) {
    // LED is fast blinking
    M5.dis.drawpix(0, color);
    delay(400);
    M5.dis.drawpix(0, 0x000000);
    delay(100);
  }
  delay(500);

}

void loop() {
  M5.update();

  // Set status
  if (M5.Btn.wasPressed()) {
    resetLoadcell();
  } else {
    measureRemainingAmount();
  }

  // Show status using LED
  showStatus();
}
