/*
 * Display Module - Bitmap and Text Rendering
 *
 * Handles both display modes:
 * - BITMAP: 72x72 RGB/RGBA upscaling to 128x128
 * - TEXT: Dynamic font sizing, word-wrapping, color parsing
 */

// ============================================================================
// Base64 Decoding
// ============================================================================

const char* B64_TABLE = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int b64Index(char c) {
  const char* p = strchr(B64_TABLE, c);
  if (!p) return -1;
  return (int)(p - B64_TABLE);
}

String decodeBase64(const String& input) {
  int len = input.length();
  int val = 0;
  int valb = -8;
  String out;

  for (int i = 0; i < len; i++) {
    char c = input[i];
    if (c == '=') break;
    int idx = b64Index(c);
    if (idx < 0) break;

    val = (val << 6) + idx;
    valb += 6;
    if (valb >= 0) {
      char outChar = (char)((val >> valb) & 0xFF);
      out += outChar;
      valb -= 8;
    }
  }
  return out;
}

String decodeCompanionText(const String& encoded) {
  if (encoded.length() == 0) return encoded;

  // Check if it looks like base64
  bool looksBase64 = true;
  for (size_t i = 0; i < encoded.length(); i++) {
    char c = encoded[i];
    if (!((c >= 'A' && c <= 'Z') ||
          (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9') ||
          c == '+' || c == '/' || c == '=')) {
      looksBase64 = false;
      break;
    }
  }

  if (!looksBase64) return encoded;

  String decoded = decodeBase64(encoded);
  if (decoded.length() == 0) return encoded;
  return decoded;
}

// ============================================================================
// Color Parsing
// ============================================================================

bool parseColorToken(const String& line, const String& key, int &r, int &g, int &b) {
  int pos = line.indexOf(key);
  if (pos < 0) return false;

  pos += key.length();
  if (pos < (int)line.length() && line[pos] == '=') pos++;

  int end = line.indexOf(' ', pos);
  if (end < 0) end = line.length();

  String val = line.substring(pos, end);
  val.trim();
  if (val.length() == 0) return false;

  // Strip quotes
  if (val.length() >= 2 && val[0] == '"' && val[val.length() - 1] == '"') {
    val = val.substring(1, val.length() - 1);
  }

  // rgb()/rgba()
  if (val.startsWith("rgb(") || val.startsWith("rgba(")) {
    String c = val;
    c.replace("rgba(", "");
    c.replace("rgb(", "");
    c.replace(")", "");
    c.replace(" ", "");
    int p1 = c.indexOf(',');
    int p2 = c.indexOf(',', p1+1);
    if (p1 < 0 || p2 < 0) return false;
    int p3 = c.indexOf(',', p2+1);

    r = c.substring(0, p1).toInt();
    g = c.substring(p1+1, p2).toInt();
    if (p3 >= 0) {
      b = c.substring(p2+1, p3).toInt();
    } else {
      b = c.substring(p2+1).toInt();
    }
    return true;
  }

  // Hex: #RRGGBB
  if (val[0] == '#') {
    if (val.length() < 7) return false;
    String rs = val.substring(1, 3);
    String gs = val.substring(3, 5);
    String bs = val.substring(5, 7);
    r = (int) strtol(rs.c_str(), nullptr, 16);
    g = (int) strtol(gs.c_str(), nullptr, 16);
    b = (int) strtol(bs.c_str(), nullptr, 16);
    return true;
  }

  // CSV: R,G,B
  int c1 = val.indexOf(',');
  int c2 = val.indexOf(',', c1 + 1);
  if (c1 < 0 || c2 < 0) return false;

  r = val.substring(0, c1).toInt();
  g = val.substring(c1 + 1, c2).toInt();
  b = val.substring(c2 + 1).toInt();
  return true;
}

// ============================================================================
// Display Helper Functions
// ============================================================================

void clearScreen(uint16_t color) {
  M5.Display.fillScreen(color);
}

void drawCenterText(const String& txt, uint16_t color, uint16_t bg) {
  M5.Display.fillScreen(bg);
  M5.Display.setFont(&fonts::Font0);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(color, bg);
  M5.Display.setTextDatum(middle_center);

  // Split into lines
  std::vector<String> lines;
  int start = 0;
  while (true) {
    int idx = txt.indexOf('\n', start);
    if (idx < 0) {
      lines.push_back(txt.substring(start));
      break;
    }
    lines.push_back(txt.substring(start, idx));
    start = idx + 1;
  }

  int lineHeight  = M5.Display.fontHeight();
  int totalHeight = lineHeight * lines.size();
  int topY        = (M5.Display.height() - totalHeight) / 2 + (lineHeight / 2);

  for (int i = 0; i < (int)lines.size(); i++) {
    int y = topY + i * lineHeight;
    M5.Display.drawString(lines[i], M5.Display.width() / 2, y);
  }
}

void applyDisplayBrightness() {
  int p = brightness;
  if (p < 0)   p = 0;
  if (p > 100) p = 100;
  uint8_t level = map(p, 0, 100, 0, 255);
  M5.Display.setBrightness(level);
}

void drawBitmapRGB888FullScreen(uint8_t* rgb, int size) {
  int sw = M5.Display.width();
  int sh = M5.Display.height();

  for (int y = 0; y < sh; y++) {
    int srcY = (y * size) / sh;
    for (int x = 0; x < sw; x++) {
      int srcX = (x * size) / sw;
      int idx = (srcY * size + srcX) * 3;
      M5.Display.drawPixel(x, y, M5.Display.color565(rgb[idx], rgb[idx+1], rgb[idx+2]));
    }
  }
}

// ============================================================================
// Text Mode Font Helpers
// ============================================================================

void setExtraBigFont() {
  M5.Display.setFont(&fonts::Font8);
  M5.Display.setTextSize(1);
}

void setUltraFont() {
  M5.Display.setFont(&fonts::Font4);
  M5.Display.setTextSize(2);
}

void setLargeFont() {
  M5.Display.setFont(&fonts::Font4);
  M5.Display.setTextSize(2);
}

void setNormalFont() {
  M5.Display.setFont(&fonts::Font4);
  M5.Display.setTextSize(1);
}

// ============================================================================
// Text Mode Display Helpers
// ============================================================================

bool wrapToLines(const String& src, String& l1, String& l2, String& l3, int& outLines) {
  l1 = "";
  l2 = "";
  l3 = "";
  outLines = 0;

  if (src.length() == 0) return true;

  M5.Display.setTextWrap(false);

  int screenW = M5.Display.width();

  // Split into words
  std::vector<String> words;
  int start = 0;
  while (start < (int)src.length()) {
    int space = src.indexOf(' ', start);
    if (space < 0) space = src.length();
    String w = src.substring(start, space);
    if (w.length() > 0) words.push_back(w);
    start = space + 1;
  }

  String linesBuf[MAX_AUTO_LINES];
  String* lines[MAX_AUTO_LINES];
  for (int i = 0; i < MAX_AUTO_LINES; i++) {
    linesBuf[i] = "";
    lines[i] = &linesBuf[i];
  }

  int currentLine = 0;

  for (size_t i = 0; i < words.size(); i++) {
    if (currentLine >= MAX_AUTO_LINES) {
      return false;
    }

    String candidate;
    if (lines[currentLine]->length() == 0) {
      candidate = words[i];
    } else {
      candidate = *lines[currentLine] + " " + words[i];
    }

    int w = M5.Display.textWidth(candidate);

    if (w <= screenW) {
      *lines[currentLine] = candidate;
    } else {
      currentLine++;
      if (currentLine >= MAX_AUTO_LINES) {
        return false;
      }
      *lines[currentLine] = words[i];
    }
  }

  outLines = currentLine + 1;

  if (outLines >= 1) l1 = linesBuf[0];
  if (outLines >= 2) l2 = linesBuf[1];
  if (outLines >= 3) l3 = linesBuf[2];

  return true;
}

void analyseLayout() {
  line1 = "";
  line2 = "";
  line3 = "";
  numLines = 0;
  useManualLines = false;
  manualLines.clear();

  int len = currentText.length();
  if (len == 0) return;

  // Manual \n lines
  if (currentText.indexOf('\n') >= 0) {
    setNormalFont();
    M5.Display.setTextWrap(false);

    int start = 0;
    while (true) {
      int idx = currentText.indexOf('\n', start);
      if (idx < 0) {
        manualLines.push_back(currentText.substring(start));
        break;
      }
      manualLines.push_back(currentText.substring(start, idx));
      start = idx + 1;
    }

    // Limit to 5 visible lines
    if ((int)manualLines.size() > 5) {
      manualLines.resize(5);
    }

    useManualLines = true;
    return;
  }

  // Extra-big for ≤2 chars
  if (len <= 2) {
    setExtraBigFont();
    numLines = 1;
    line1    = currentText;
    return;
  }

  // Ultra for 3 chars
  if (len == 3) {
    setUltraFont();
    numLines = 1;
    line1    = currentText;
    return;
  }

  // Large for 4-6 chars
  if (len <= 6) {
    setLargeFont();
    numLines = 1;
    line1    = currentText;
    return;
  }

  // Normal with auto-wrap
  setNormalFont();
  M5.Display.setTextWrap(false);

  String w1, w2, w3;
  int    lines = 0;
  bool   fits  = wrapToLines(currentText, w1, w2, w3, lines);

  if (fits && lines > 0 && lines <= MAX_AUTO_LINES) {
    numLines  = lines;
    line1     = w1;
    line2     = (lines >= 2) ? w2 : "";
    line3     = (lines >= 3) ? w3 : "";
    return;
  }

  // Fallback: single line
  numLines = 1;
  line1    = currentText;
}

void drawTextPressedBorderIfNeeded() {
  if (!textPressedBorder) return;

  int w = M5.Display.width();
  int h = M5.Display.height();
  uint16_t borderColor = M5.Display.color565(255, 255, 0);

  for (int i = 0; i < 4; i++) {
    M5.Display.drawRect(i, i, w - i * 2, h - i * 2, borderColor);
  }
}

void refreshTextDisplay() {
  M5.Display.fillScreen(bgColor);
  M5.Display.setTextColor(txtColor, bgColor);
  M5.Display.setTextWrap(false);

  if (currentText.length() == 0) {
    drawTextPressedBorderIfNeeded();
    return;
  }

  int screenW = M5.Display.width();
  int screenH = M5.Display.height();

  // Manual multi-line mode
  if (useManualLines) {
    setNormalFont();
    M5.Display.setTextDatum(middle_center);

    int lineHeight  = M5.Display.fontHeight();
    int lines       = manualLines.size();
    int totalHeight = lineHeight * lines;
    int topY        = (screenH - totalHeight) / 2 + (lineHeight / 2);

    for (int i = 0; i < lines; i++) {
      int y = topY + i * lineHeight;
      M5.Display.drawString(manualLines[i], screenW / 2, y);
    }

    drawTextPressedBorderIfNeeded();
    return;
  }

  M5.Display.setTextDatum(middle_center);

  int len = currentText.length();

  // Extra-big (≤2 chars)
  if (len <= 2) {
    setExtraBigFont();
    M5.Display.drawString(currentText, screenW / 2, screenH / 2);
    drawTextPressedBorderIfNeeded();
    return;
  }

  // Ultra (3 chars)
  if (len == 3) {
    setUltraFont();
    M5.Display.drawString(currentText, screenW / 2, screenH / 2);
    drawTextPressedBorderIfNeeded();
    return;
  }

  // Large (4-6 chars)
  if (len <= 6) {
    setLargeFont();
    M5.Display.drawString(currentText, screenW / 2, screenH / 2);
    drawTextPressedBorderIfNeeded();
    return;
  }

  // Multi-line centered
  if (numLines >= 1 && numLines <= MAX_AUTO_LINES) {
    setNormalFont();
    String lines[3] = { line1, line2, line3 };

    int lineHeight  = M5.Display.fontHeight();
    int totalHeight = lineHeight * numLines;
    int topY        = (screenH - totalHeight) / 2 + (lineHeight / 2);

    for (int i = 0; i < numLines && i < 3; i++) {
      int y = topY + i * lineHeight;
      M5.Display.drawString(lines[i], screenW / 2, y);
    }
  }

  drawTextPressedBorderIfNeeded();
}

// ============================================================================
// Text Mode Update Functions
// ============================================================================

void setTextNow(const String& txt) {
  currentText = txt;
  analyseLayout();
  refreshTextDisplay();
}

void setText(const String& txt) {
  setTextNow(txt);
}

// ============================================================================
// Text Mode API Handlers
// ============================================================================

void handleKeyStateTextField(const String& line) {
  int tPos = line.indexOf("TEXT=");
  if (tPos < 0) return;

  int firstQuote = line.indexOf('"', tPos);
  if (firstQuote < 0) return;

  int secondQuote = line.indexOf('"', firstQuote + 1);
  if (secondQuote < 0) return;

  String textField = line.substring(firstQuote + 1, secondQuote);
  String decoded   = decodeCompanionText(textField);
  decoded.replace("\\n", "\n");

  Serial.print("[API] TEXT encoded=\"");
  Serial.print(textField);
  Serial.print("\" decoded=\"");
  Serial.print(decoded);
  Serial.println("\"");

  setText(decoded);
}

void handleTextModeColors(const String& line) {
  int r, g, b;
  bool bgOk  = false;
  bool txtOk = false;

  if (parseColorToken(line, "COLOR", r, g, b)) {
    bgColor = M5.Display.color565(r, g, b);
    bgOk = true;
    Serial.printf("[API] TEXT BG COLOR r=%d g=%d b=%d\n", r, g, b);
  }

  if (parseColorToken(line, "TEXTCOLOR", r, g, b)) {
    txtColor = M5.Display.color565(r, g, b);
    txtOk = true;
    Serial.printf("[API] TEXT FG COLOR r=%d g=%d b=%d\n", r, g, b);
  }

  if (bgOk || txtOk) {
    refreshTextDisplay();
  }
}
