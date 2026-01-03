/*
 * Hardware Module - External RGB LED Control
 *
 * Manages external common-cathode RGB LED via PWM
 * Pins: G8 (Red), G5 (Green), G6 (Blue), G7 (Ground)
 */

// ============================================================================
// LED Initialization
// ============================================================================

void setupLED() {
  pinMode(LED_PIN_GND, OUTPUT);
  digitalWrite(LED_PIN_GND, LOW);

  Serial.println("[LED] Initialising PWM pins with ledcAttach...");

  if (ledcAttach(LED_PIN_RED, pwmFreq, pwmResolution)) {
    Serial.println("[LED] PWM attached to RED pin");
  } else {
    Serial.println("[LED] ERROR attaching PWM to RED pin");
  }

  if (ledcAttach(LED_PIN_GREEN, pwmFreq, pwmResolution)) {
    Serial.println("[LED] PWM attached to GREEN pin");
  } else {
    Serial.println("[LED] ERROR attaching PWM to GREEN pin");
  }

  if (ledcAttach(LED_PIN_BLUE, pwmFreq, pwmResolution)) {
    Serial.println("[LED] PWM attached to BLUE pin");
  } else {
    Serial.println("[LED] ERROR attaching PWM to BLUE pin");
  }

  setExternalLedColor(0, 0, 0);

  // LED power-on test
  Serial.println("[LED] Running power-on test...");
  setExternalLedColor(255, 255, 255);
}

// ============================================================================
// LED Color Control
// ============================================================================

void setExternalLedColor(uint8_t r, uint8_t g, uint8_t b) {
  lastColorR = r;
  lastColorG = g;
  lastColorB = b;

  // Scale by brightness (min 15% to keep LED visible)
  uint8_t scaledR = r * max(brightness, 15) / 100;
  uint8_t scaledG = g * max(brightness, 15) / 100;
  uint8_t scaledB = b * max(brightness, 15) / 100;

  // For common anode LED, uncomment these lines:
  // scaledR = 255 - scaledR;
  // scaledG = 255 - scaledG;
  // scaledB = 255 - scaledB;

  Serial.print("[LED] setExternalLedColor raw r/g/b = ");
  Serial.print(r); Serial.print("/");
  Serial.print(g); Serial.print("/");
  Serial.print(b);
  Serial.print("  scaled = ");
  Serial.print(scaledR); Serial.print("/");
  Serial.print(scaledG); Serial.print("/");
  Serial.println(scaledB);

  ledcWrite(LED_PIN_RED,   scaledR);
  ledcWrite(LED_PIN_GREEN, scaledG);
  ledcWrite(LED_PIN_BLUE,  scaledB);
}

// ============================================================================
// Connection Status LED Blink
// ============================================================================

void updateReconnectingLED() {
  unsigned long now = millis();

  if (now - lastBlinkTime >= blinkIntervalMs) {
    blinkState = !blinkState;
    lastBlinkTime = now;

    if (blinkState) {
      setExternalLedColor(255, 0, 0);  // Red ON
    } else {
      setExternalLedColor(0, 0, 0);    // OFF
    }
  }
}
