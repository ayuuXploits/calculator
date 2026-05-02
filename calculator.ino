#include <LiquidCrystal.h>
#include <EEPROM.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// --- State Management ---
enum Mode { TYPING, CALCULATOR, CONVERTER };
Mode currentMode = TYPING;
unsigned long lastPressTime = 0;
const int debounceDelay = 250;

// --- Typing Variables ---
char charset[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!?.";
int charIndex = 0;
int cursorPosition = 0;
String message = "";

// --- Calculator Variables ---
float num1 = 0, num2 = 0;
char ops[] = {'+', '-', '*', '/'};
int opIndex = 0;
int calcSlot = 0;
bool enteringDecimal = false;
int decimalPlaces = 0;
bool num1Negative = false;
bool num2Negative = false;
float lastResult = 0;
bool hasHistory = false;

// --- Converter Variables ---
struct ConvUnit {
  const char* label;   // e.g. "km -> mi"
  const char* unitIn;  // e.g. "km"
  const char* unitOut; // e.g. "mi"
  float factor;        // multiply input by this to get output
};

ConvUnit convUnits[] = {
  {"km  -> mi  ", "km",  "mi",   0.621371},
  {"mi  -> km  ", "mi",  "km",   1.60934},
  {"C   -> F   ", "C",   "F",    0},       // special case
  {"F   -> C   ", "F",   "C",    0},       // special case
  {"kg  -> lbs ", "kg",  "lbs",  2.20462},
  {"lbs -> kg  ", "lbs", "kg",   0.453592},
  {"m   -> ft  ", "m",   "ft",   3.28084},
  {"ft  -> m   ", "ft",  "m",    0.3048},
  {"L   -> gal ", "L",   "gal",  0.264172},
  {"gal -> L   ", "gal", "L",    3.78541},
  {"cm  -> in  ", "cm",  "in",   0.393701},
  {"in  -> cm  ", "in",  "cm",   2.54},
};
const int NUM_CONV = 12;

int convTypeIndex = 0;   // which conversion
float convInput = 0;
bool convNegative = false;
bool convDecimal = false;
int convDecPlaces = 0;
int convSlot = 0;        // 0=select type, 1=enter value

// --- EEPROM ---
const int EEPROM_ADDR = 0;
const int EEPROM_LEN = 32;

// =====================
// SETUP
// =====================
void setup() {
  lcd.begin(16, 2);
  Serial.begin(9600);
  loadFromEEPROM();
  lcd.print("System Booting...");
  delay(1000);
  showCurrentMode();
}

// =====================
// MAIN LOOP
// =====================
void loop() {
  int x = analogRead(0);

  // Long-press SELECT → cycle modes (TYPING → CALC → CONVERTER → TYPING)
  if (x > 600 && x < 850) {
    unsigned long startHold = millis();
    while (analogRead(0) > 600 && analogRead(0) < 850) {
      if (millis() - startHold > 1000) {
        if (currentMode == TYPING) currentMode = CALCULATOR;
        else if (currentMode == CALCULATOR) currentMode = CONVERTER;
        else currentMode = TYPING;
        showCurrentMode();
        while (analogRead(0) < 1000);
        return;
      }
    }
    if (millis() - lastPressTime > debounceDelay) {
      handleSelectClick();
      lastPressTime = millis();
    }
  }

  // Long-press LEFT in TYPING → clear all
  if (x > 400 && x < 600) {
    unsigned long startHold = millis();
    while (analogRead(0) > 400 && analogRead(0) < 600) {
      if (millis() - startHold > 1000) {
        if (currentMode == TYPING) clearMessage();
        else if (currentMode == CONVERTER) resetConverter();
        while (analogRead(0) < 1000);
        return;
      }
    }
    if (millis() - lastPressTime > debounceDelay) {
      if (currentMode == TYPING) handleTypingLeft();
      else if (currentMode == CALCULATOR) handleCalcLeft();
      else if (currentMode == CONVERTER) handleConvLeft();
      lastPressTime = millis();
    }
  }

  if (millis() - lastPressTime > debounceDelay) {
    if (x < 60) { // RIGHT
      if (currentMode == TYPING) handleTypingRight();
      else if (currentMode == CALCULATOR) handleCalcRight();
      else handleConvRight();
      lastPressTime = millis();
    }
    else if (x < 200) { // UP
      if (currentMode == TYPING) handleTypingUp();
      else if (currentMode == CALCULATOR) handleCalcUp();
      else handleConvUp();
      lastPressTime = millis();
    }
    else if (x < 400) { // DOWN
      if (currentMode == TYPING) handleTypingDown();
      else if (currentMode == CALCULATOR) handleCalcDown();
      else handleConvDown();
      lastPressTime = millis();
    }
  }
}

// =====================
// MODE SWITCHING
// =====================
void showCurrentMode() {
  lcd.clear();
  if (currentMode == TYPING) {
    lcd.print("MODE: TYPING");
    delay(800);
    refreshTypingDisplay();
  } else if (currentMode == CALCULATOR) {
    lcd.print("MODE: CALC");
    delay(800);
    updateCalcDisplay();
  } else {
    lcd.print("MODE: CONVERTER");
    delay(800);
    updateConverterDisplay();
  }
}

void handleSelectClick() {
  if (currentMode == TYPING) {
    String finalMsg = message + charset[charIndex];
    Serial.println("SENT: " + finalMsg);
    saveToEEPROM(finalMsg);
    lcd.clear();
    lcd.print("Sent + Saved!");
    delay(1000);
    refreshTypingDisplay();
  } else if (currentMode == CALCULATOR) {
    calculateResult();
  } else {
    // In converter: SELECT confirms value and shows result
    if (convSlot == 1) doConversion();
    else convSlot = 1; // move from type select to value entry
    updateConverterDisplay();
  }
}

// =====================
// TYPING FUNCTIONS
// =====================
void handleTypingUp() {
  charIndex = (charIndex + 1) % (sizeof(charset) - 1);
  drawCurrentChar();
}
void handleTypingDown() {
  charIndex = (charIndex - 1 + (sizeof(charset) - 1)) % (sizeof(charset) - 1);
  drawCurrentChar();
}
void handleTypingRight() {
  if (cursorPosition < 31) {
    message += charset[charIndex];
    cursorPosition++;
    charIndex = 0;
    refreshTypingDisplay();
  }
}
void handleTypingLeft() {
  if (cursorPosition > 0) {
    cursorPosition--;
    if (message.length() > 0) message.remove(message.length() - 1);
    refreshTypingDisplay();
  }
}
void clearMessage() {
  message = "";
  cursorPosition = 0;
  charIndex = 0;
  lcd.clear();
  lcd.print("Cleared!");
  delay(700);
  refreshTypingDisplay();
}

// =====================
// CALCULATOR FUNCTIONS
// =====================
void handleCalcUp() {
  if (calcSlot == 0) {
    if (enteringDecimal) { decimalPlaces++; float s = pow(10,-decimalPlaces); if(num1Negative) num1-=s; else num1+=s; }
    else { if(num1Negative) num1--; else num1++; }
  } else if (calcSlot == 1) {
    opIndex = (opIndex + 1) % 4;
  } else {
    if (enteringDecimal) { decimalPlaces++; float s = pow(10,-decimalPlaces); if(num2Negative) num2-=s; else num2+=s; }
    else { if(num2Negative) num2--; else num2++; }
  }
  updateCalcDisplay();
}
void handleCalcDown() {
  if (calcSlot == 0) {
    if (enteringDecimal) { if(decimalPlaces>0) decimalPlaces--; }
    else { if(num1Negative) num1++; else num1--; }
  } else if (calcSlot == 1) {
    opIndex = (opIndex - 1 + 4) % 4;
  } else {
    if (enteringDecimal) { if(decimalPlaces>0) decimalPlaces--; }
    else { if(num2Negative) num2++; else num2--; }
  }
  updateCalcDisplay();
}
void handleCalcRight() {
  if (calcSlot == 0 || calcSlot == 2) {
    if (!enteringDecimal) { enteringDecimal = true; decimalPlaces = 0; }
    else { enteringDecimal = false; calcSlot = (calcSlot + 1) % 3; }
  } else {
    calcSlot = (calcSlot + 1) % 3;
    enteringDecimal = false;
  }
  updateCalcDisplay();
}
void handleCalcLeft() {
  if (calcSlot == 0) { num1Negative = !num1Negative; num1 = -num1; }
  else if (calcSlot == 2) { num2Negative = !num2Negative; num2 = -num2; }
  updateCalcDisplay();
}
void calculateResult() {
  float res = 0;
  bool error = false;
  if (ops[opIndex] == '+') res = num1 + num2;
  else if (ops[opIndex] == '-') res = num1 - num2;
  else if (ops[opIndex] == '*') res = num1 * num2;
  else if (ops[opIndex] == '/') {
    if (num2 == 0) error = true;
    else res = num1 / num2;
  }
  if (error) {
    lcd.setCursor(0, 1); lcd.print("Error: Div/0    ");
    Serial.println("CALC ERROR: Division by zero");
    return;
  }
  lastResult = res; hasHistory = true;
  lcd.setCursor(0, 1);
  lcd.print("= "); lcd.print(res, 4); lcd.print("        ");
  Serial.print("CALC: "); Serial.print(num1); Serial.print(" ");
  Serial.print(ops[opIndex]); Serial.print(" ");
  Serial.print(num2); Serial.print(" = "); Serial.println(res, 4);
}

// =====================
// CONVERTER FUNCTIONS
// =====================
void handleConvUp() {
  if (convSlot == 0) {
    convTypeIndex = (convTypeIndex + 1) % NUM_CONV;
  } else {
    if (convDecimal) {
      convDecPlaces++;
      float s = pow(10, -convDecPlaces);
      if (convNegative) convInput -= s; else convInput += s;
    } else {
      if (convNegative) convInput--; else convInput++;
    }
  }
  updateConverterDisplay();
}

void handleConvDown() {
  if (convSlot == 0) {
    convTypeIndex = (convTypeIndex - 1 + NUM_CONV) % NUM_CONV;
  } else {
    if (convDecimal) {
      if (convDecPlaces > 0) convDecPlaces--;
    } else {
      if (convNegative) convInput++; else convInput--;
    }
  }
  updateConverterDisplay();
}

void handleConvRight() {
  if (convSlot == 0) {
    // Confirm type, move to value entry
    convSlot = 1;
    convInput = 0;
    convDecimal = false;
    convDecPlaces = 0;
    convNegative = false;
  } else {
    // Toggle decimal mode
    if (!convDecimal) { convDecimal = true; convDecPlaces = 0; }
    else { convDecimal = false; }
  }
  updateConverterDisplay();
}

void handleConvLeft() {
  if (convSlot == 1) {
    convNegative = !convNegative;
    convInput = -convInput;
  }
  updateConverterDisplay();
}

void resetConverter() {
  convSlot = 0;
  convInput = 0;
  convDecimal = false;
  convDecPlaces = 0;
  convNegative = false;
  lcd.clear();
  lcd.print("Conv Reset!");
  delay(700);
  updateConverterDisplay();
}

void doConversion() {
  float result = 0;
  String label = String(convUnits[convTypeIndex].label);

  // Special cases: temperature
  if (convTypeIndex == 2) {        // C -> F
    result = (convInput * 9.0 / 5.0) + 32.0;
  } else if (convTypeIndex == 3) { // F -> C
    result = (convInput - 32.0) * 5.0 / 9.0;
  } else {
    result = convInput * convUnits[convTypeIndex].factor;
  }

  // Show result on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(convInput, 2);
  lcd.print(convUnits[convTypeIndex].unitIn);
  lcd.print(" = ");
  lcd.setCursor(0, 1);
  lcd.print(result, 4);
  lcd.print(" ");
  lcd.print(convUnits[convTypeIndex].unitOut);

  // Log to Serial
  Serial.print("CONV: ");
  Serial.print(convInput, 4);
  Serial.print(" ");
  Serial.print(convUnits[convTypeIndex].unitIn);
  Serial.print(" = ");
  Serial.print(result, 4);
  Serial.print(" ");
  Serial.println(convUnits[convTypeIndex].unitOut);

  delay(3000); // Show result for 3 seconds, then return to input
  updateConverterDisplay();
}

void updateConverterDisplay() {
  lcd.noCursor();
  lcd.clear();

  if (convSlot == 0) {
    // Browsing conversion types
    lcd.setCursor(0, 0);
    lcd.print(convUnits[convTypeIndex].label);
    lcd.setCursor(0, 1);

    // Show prev/next hints
    int prev = (convTypeIndex - 1 + NUM_CONV) % NUM_CONV;
    int next = (convTypeIndex + 1) % NUM_CONV;
    String hint = "<";
    hint += convUnits[prev].unitIn;
    hint += " ";
    hint += convUnits[next].unitIn;
    hint += ">";
    lcd.print(hint.substring(0, 16));
  } else {
    // Value entry
    lcd.setCursor(0, 0);
    String header = String(convUnits[convTypeIndex].unitIn);
    header += ">"; header += convUnits[convTypeIndex].unitOut;
    if (convDecimal) header += " [.]";
    lcd.print(header.substring(0, 16));

    lcd.setCursor(0, 1);
    String valStr = convNegative ? "-" : "";
    valStr += String((int)abs(convInput));
    if (convDecimal) {
      valStr += ".";
      if (convDecPlaces > 0) {
        float f = abs(convInput) - (int)abs(convInput);
        valStr += String(f, convDecPlaces).substring(2);
      }
    }
    valStr += " ";
    valStr += convUnits[convTypeIndex].unitIn;
    lcd.print(valStr.substring(0, 16));
  }
}

// =====================
// EEPROM FUNCTIONS
// =====================
void saveToEEPROM(String msg) {
  int len = min((int)msg.length(), EEPROM_LEN - 1);
  for (int i = 0; i < len; i++) EEPROM.write(EEPROM_ADDR + i, msg[i]);
  EEPROM.write(EEPROM_ADDR + len, '\0');
}
void loadFromEEPROM() {
  String loaded = "";
  for (int i = 0; i < EEPROM_LEN; i++) {
    char c = EEPROM.read(EEPROM_ADDR + i);
    if (c == '\0' || c == 0xFF) break;
    if (c >= 32 && c <= 126) loaded += c;
  }
  if (loaded.length() > 0) { message = loaded; cursorPosition = message.length(); }
}

// =====================
// DISPLAY HELPERS
// =====================
void drawCurrentChar() {
  setCursorPos(cursorPosition);
  lcd.print(charset[charIndex]);
  setCursorPos(cursorPosition);
}
void setCursorPos(int pos) {
  if (pos < 16) lcd.setCursor(pos, 0);
  else lcd.setCursor(pos - 16, 1);
}
void refreshTypingDisplay() {
  lcd.clear();
  int startPos = 0;
  if (cursorPosition >= 16) startPos = cursorPosition - 15;
  String visible = message.substring(startPos, min((int)message.length(), startPos + 32));
  lcd.setCursor(0, 0); lcd.print(visible.substring(0, min(16, (int)visible.length())));
  lcd.setCursor(0, 1);
  if (visible.length() > 16) lcd.print(visible.substring(16));
  drawCurrentChar();
  lcd.cursor();
}
void updateCalcDisplay() {
  lcd.noCursor(); lcd.clear();
  String expr = "";
  expr += String((int)abs(num1));
  if (num1 < 0) expr = "-" + expr;
  if (enteringDecimal && calcSlot == 0) {
    expr += ".";
    if (decimalPlaces > 0) expr += String(abs(num1) - (int)abs(num1), decimalPlaces).substring(2);
  }
  expr += " "; expr += ops[opIndex]; expr += " ";
  String n2str = String((int)abs(num2));
  if (num2 < 0) n2str = "-" + n2str;
  if (enteringDecimal && calcSlot == 2) {
    n2str += ".";
    if (decimalPlaces > 0) n2str += String(abs(num2) - (int)abs(num2), decimalPlaces).substring(2);
  }
  expr += n2str;
  lcd.setCursor(0, 0); lcd.print(expr.substring(0, 16));
  lcd.setCursor(0, 1);
  if (calcSlot == 0) lcd.print(enteringDecimal ? "[Num1 decimal]" : "[Editing Num1]");
  else if (calcSlot == 1) lcd.print("[Editing Op]  ");
  else if (calcSlot == 2) lcd.print(enteringDecimal ? "[Num2 decimal]" : "[Editing Num2]");
}
