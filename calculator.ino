#include <LiquidCrystal.h>
#include <EEPROM.h>

// Pins for standard LCD Keypad Shield
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// --- State Management ---
enum Mode { TYPING, CALCULATOR };
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
int calcSlot = 0; // 0:Num1, 1:Op, 2:Num2
bool enteringDecimal = false;
int decimalPlaces = 0;
bool num1Negative = false;
bool num2Negative = false;

// --- History Variables ---
float lastResult = 0;
bool hasHistory = false;

// --- EEPROM ---
const int EEPROM_ADDR = 0;
const int EEPROM_LEN = 32;

// =====================
// SETUP
// =====================
void setup() {
  lcd.begin(16, 2);
  Serial.begin(9600);

  // Load saved message from EEPROM
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

  // Long-press SELECT to switch modes
  if (x > 600 && x < 850) {
    unsigned long startHold = millis();
    while (analogRead(0) > 600 && analogRead(0) < 850) {
      if (millis() - startHold > 1000) {
        currentMode = (currentMode == TYPING) ? CALCULATOR : TYPING;
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

  // Long-press LEFT in TYPING to clear all
  if (x > 400 && x < 600) {
    unsigned long startHold = millis();
    while (analogRead(0) > 400 && analogRead(0) < 600) {
      if (millis() - startHold > 1000) {
        if (currentMode == TYPING) clearMessage();
        while (analogRead(0) < 1000);
        return;
      }
    }
    if (millis() - lastPressTime > debounceDelay) {
      if (currentMode == TYPING) handleTypingLeft();
      lastPressTime = millis();
    }
  }

  if (millis() - lastPressTime > debounceDelay) {
    if (x < 60) { // RIGHT
      if (currentMode == TYPING) handleTypingRight();
      else handleCalcRight();
      lastPressTime = millis();
    }
    else if (x < 200) { // UP
      if (currentMode == TYPING) handleTypingUp();
      else handleCalcUp();
      lastPressTime = millis();
    }
    else if (x < 400) { // DOWN
      if (currentMode == TYPING) handleTypingDown();
      else handleCalcDown();
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
  } else {
    lcd.print("MODE: CALC");
    delay(800);
    updateCalcDisplay();
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
  } else {
    calculateResult();
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
    if (enteringDecimal) {
      decimalPlaces++;
      float step = pow(10, -decimalPlaces);
      if (num1Negative) num1 -= step; else num1 += step;
    } else {
      if (num1Negative) num1--; else num1++;
    }
  }
  else if (calcSlot == 1) {
    opIndex = (opIndex + 1) % 4;
  }
  else {
    if (enteringDecimal) {
      decimalPlaces++;
      float step = pow(10, -decimalPlaces);
      if (num2Negative) num2 -= step; else num2 += step;
    } else {
      if (num2Negative) num2--; else num2++;
    }
  }
  updateCalcDisplay();
}

void handleCalcDown() {
  if (calcSlot == 0) {
    if (enteringDecimal) {
      if (decimalPlaces > 0) decimalPlaces--;
    } else {
      if (num1Negative) num1++; else num1--;
    }
  }
  else if (calcSlot == 1) {
    opIndex = (opIndex - 1 + 4) % 4;
  }
  else {
    if (enteringDecimal) {
      if (decimalPlaces > 0) decimalPlaces--;
    } else {
      if (num2Negative) num2++; else num2--;
    }
  }
  updateCalcDisplay();
}

void handleCalcRight() {
  // RIGHT cycles slot; within num slots, RIGHT toggles decimal mode
  if (calcSlot == 0 || calcSlot == 2) {
    if (!enteringDecimal) {
      enteringDecimal = true;
      decimalPlaces = 0;
    } else {
      enteringDecimal = false;
      calcSlot = (calcSlot + 1) % 3;
    }
  } else {
    calcSlot = (calcSlot + 1) % 3;
    enteringDecimal = false;
  }
  updateCalcDisplay();
}

// Toggle negative via LEFT in calc mode
void handleCalcLeft() {
  if (calcSlot == 0) num1Negative = !num1Negative, num1 = -num1;
  else if (calcSlot == 2) num2Negative = !num2Negative, num2 = -num2;
  updateCalcDisplay();
}

void calculateResult() {
  float res = 0;
  bool error = false;

  if (ops[opIndex] == '+') res = num1 + num2;
  else if (ops[opIndex] == '-') res = num1 - num2;
  else if (ops[opIndex] == '*') res = num1 * num2;
  else if (ops[opIndex] == '/') {
    if (num2 == 0) { error = true; }
    else res = num1 / num2;
  }

  if (error) {
    lcd.setCursor(0, 1);
    lcd.print("Error: Div/0    ");
    Serial.println("CALC ERROR: Division by zero");
    return;
  }

  // Save to history
  lastResult = res;
  hasHistory = true;

  lcd.setCursor(0, 1);
  lcd.print("= ");
  lcd.print(res, 4); // 4 decimal places
  lcd.print("        ");

  Serial.print("CALC: ");
  Serial.print(num1); Serial.print(" ");
  Serial.print(ops[opIndex]); Serial.print(" ");
  Serial.print(num2); Serial.print(" = ");
  Serial.println(res, 4);
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
    if (c >= 32 && c <= 126) loaded += c; // printable ASCII only
  }
  if (loaded.length() > 0) {
    message = loaded;
    cursorPosition = message.length();
  }
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
  // Handle scrolling: show 16 chars around the cursor
  int startPos = 0;
  if (cursorPosition >= 16) startPos = cursorPosition - 15;

  String visible = message.substring(startPos, min((int)message.length(), startPos + 32));
  lcd.setCursor(0, 0);
  lcd.print(visible.substring(0, min(16, (int)visible.length())));
  lcd.setCursor(0, 1);
  if (visible.length() > 16) lcd.print(visible.substring(16));

  drawCurrentChar();
  lcd.cursor();
}

void updateCalcDisplay() {
  lcd.noCursor();
  lcd.clear();

  // Row 1: expression
  String expr = "";
  expr += String((int)abs(num1));
  if (num1 < 0) expr = "-" + expr;
  if (enteringDecimal && (calcSlot == 0)) {
    expr += ".";
    if (decimalPlaces > 0) expr += String(frac(abs(num1)), decimalPlaces);
  }
  expr += " "; expr += ops[opIndex]; expr += " ";
  String n2str = String((int)abs(num2));
  if (num2 < 0) n2str = "-" + n2str;
  if (enteringDecimal && (calcSlot == 2)) {
    n2str += ".";
    if (decimalPlaces > 0) n2str += String(frac(abs(num2)), decimalPlaces);
  }
  expr += n2str;
  lcd.setCursor(0, 0);
  lcd.print(expr.substring(0, 16));

  // Row 2: status and history
  lcd.setCursor(0, 1);
  if (calcSlot == 0) lcd.print(enteringDecimal ? "[Num1 decimal]" : "[Editing Num1]");
  else if (calcSlot == 1) lcd.print("[Editing Op]  ");
  else if (calcSlot == 2) lcd.print(enteringDecimal ? "[Num2 decimal]" : "[Editing Num2]");

  // Show last result hint on Serial if available
  if (hasHistory) {
    Serial.print("Last result: ");
    Serial.println(lastResult, 4);
  }
}

// Helper: fractional part
float frac(float x) {
  return x - (int)x;
}
