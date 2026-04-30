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

// --- Converter Variables ---
struct ConvUnit {
  const char* label;   
  const char* unitIn;  
  const char* unitOut; 
  float factor;        
};

ConvUnit convUnits[] = {
  {"km  -> mi  ", "km",  "mi",   0.621371},
  {"mi  -> km  ", "mi",  "km",   1.60934},
  {"C   -> F   ", "C",   "F",    0},       
  {"F   -> C   ", "F",   "C",    0},       
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

int convTypeIndex = 0;   
float convInput = 0;
bool convNegative = false;
bool convDecimal = false;
int convDecPlaces = 0;
int convSlot = 0;        

// --- EEPROM ---
const int EEPROM_ADDR = 0;
const int EEPROM_LEN = 32;

void setup() {
  lcd.begin(16, 2);
  Serial.begin(9600);
  loadFromEEPROM();
  lcd.print("System Booting...");
  delay(1000);
  showCurrentMode();
}

void loop() {
  int x = analogRead(0);

  // LONG PRESS SELECT (Logic to cycle through all 3 modes)
  if (x > 600 && x < 850) {
    unsigned long startHold = millis();
    while (analogRead(0) > 600 && analogRead(0) < 850) {
      if (millis() - startHold > 1000) {
        if (currentMode == TYPING) currentMode = CALCULATOR;
        else if (currentMode == CALCULATOR) currentMode = CONVERTER;
        else currentMode = TYPING;
        
        showCurrentMode();
        while (analogRead(0) < 1000); // Wait for release
        return;
      }
    }
    // SHORT PRESS SELECT
    if (millis() - lastPressTime > debounceDelay) {
      handleSelectClick();
      lastPressTime = millis();
    }
  }

  // Handle LEFT button (Short and Long Press)
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

  // Directional Buttons
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

// --- Mode Logic ---
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
    lcd.clear(); lcd.print("Sent + Saved!");
    delay(1000);
    refreshTypingDisplay();
  } else if (currentMode == CALCULATOR) {
    calculateResult();
  } else {
    if (convSlot == 1) doConversion();
    else convSlot = 1; 
    updateConverterDisplay();
  }
}

// --- Typing logic ---
void handleTypingUp() { charIndex = (charIndex + 1) % (sizeof(charset) - 1); drawCurrentChar(); }
void handleTypingDown() { charIndex = (charIndex - 1 + (sizeof(charset) - 1)) % (sizeof(charset) - 1); drawCurrentChar(); }
void handleTypingRight() { if (cursorPosition < 31) { message += charset[charIndex]; cursorPosition++; charIndex = 0; refreshTypingDisplay(); } }
void handleTypingLeft() { if (cursorPosition > 0) { cursorPosition--; if (message.length() > 0) message.remove(message.length() - 1); refreshTypingDisplay(); } }
void clearMessage() { message = ""; cursorPosition = 0; charIndex = 0; lcd.clear(); lcd.print("Cleared!"); delay(700); refreshTypingDisplay(); }

// --- Calculator Logic ---
void handleCalcUp() {
  if (calcSlot == 0) {
    if (enteringDecimal) { decimalPlaces++; float s = pow(10,-decimalPlaces); if(num1Negative) num1-=s; else num1+=s; }
    else { if(num1Negative) num1--; else num1++; }
  } else if (calcSlot == 1) { opIndex = (opIndex + 1) % 4; }
  else {
    if (enteringDecimal) { decimalPlaces++; float s = pow(10,-decimalPlaces); if(num2Negative) num2-=s; else num2+=s; }
    else { if(num2Negative) num2--; else num2++; }
  }
  updateCalcDisplay();
}
void handleCalcDown() {
  if (calcSlot == 0) {
    if (enteringDecimal) { if(decimalPlaces>0) decimalPlaces--; }
    else { if(num1Negative) num1++; else num1--; }
  } else if (calcSlot == 1) { opIndex = (opIndex - 1 + 4) % 4; }
  else {
    if (enteringDecimal) { if(decimalPlaces>0) decimalPlaces--; }
    else { if(num2Negative) num2++; else num2--; }
  }
  updateCalcDisplay();
}
void handleCalcRight() {
  if (calcSlot == 0 || calcSlot == 2) {
    if (!enteringDecimal) { enteringDecimal = true; decimalPlaces = 0; }
    else { enteringDecimal = false; calcSlot = (calcSlot + 1) % 3; }
  } else { calcSlot = (calcSlot + 1) % 3; enteringDecimal = false; }
  updateCalcDisplay();
}
void handleCalcLeft() {
  if (calcSlot == 0) { num1Negative = !num1Negative; num1 = -num1; }
  else if (calcSlot == 2) { num2Negative = !num2Negative; num2 = -num2; }
  updateCalcDisplay();
}
void calculateResult() {
  float res = 0;
  if (ops[opIndex] == '+') res = num1 + num2;
  else if (ops[opIndex] == '-') res = num1 - num2;
  else if (ops[opIndex] == '*') res = num1 * num2;
  else if (ops[opIndex] == '/') res = (num2 != 0) ? num1 / num2 : 0;
  lcd.setCursor(0, 1); lcd.print("= "); lcd.print(res, 4); lcd.print("        ");
}

// --- Converter Logic ---
void handleConvUp() {
  if (convSlot == 0) convTypeIndex = (convTypeIndex + 1) % NUM_CONV;
  else { if (convDecimal) { convDecPlaces++; float s = pow(10, -convDecPlaces); if (convNegative) convInput -= s; else convInput += s; } else { if (convNegative) convInput--; else convInput++; } }
  updateConverterDisplay();
}
void handleConvDown() {
  if (convSlot == 0) convTypeIndex = (convTypeIndex - 1 + NUM_CONV) % NUM_CONV;
  else { if (convDecimal) { if (convDecPlaces > 0) convDecPlaces--; } else { if (convNegative) convInput++; else convInput--; } }
  updateConverterDisplay();
}
void handleConvRight() {
  if (convSlot == 0) { convSlot = 1; convInput = 0; convDecimal = false; convDecPlaces = 0; convNegative = false; }
  else { convDecimal = !convDecimal; }
  updateConverterDisplay();
}
void handleConvLeft() { if (convSlot == 1) { convNegative = !convNegative; convInput = -convInput; } updateConverterDisplay(); }
void resetConverter() { convSlot = 0; convInput = 0; convDecimal = false; convDecPlaces = 0; convNegative = false; lcd.clear(); lcd.print("Conv Reset!"); delay(700); updateConverterDisplay(); }
void doConversion() {
  float result = 0;
  if (convTypeIndex == 2) result = (convInput * 9.0 / 5.0) + 32.0;
  else if (convTypeIndex == 3) result = (convInput - 32.0) * 5.0 / 9.0;
  else result = convInput * convUnits[convTypeIndex].factor;
  lcd.clear(); lcd.setCursor(0, 0); lcd.print(convInput, 2); lcd.print(convUnits[convTypeIndex].unitIn);
  lcd.print(" = "); lcd.setCursor(0, 1); lcd.print(result, 4); lcd.print(" "); lcd.print(convUnits[convTypeIndex].unitOut);
  delay(3000); updateConverterDisplay();
}
void updateConverterDisplay() {
  lcd.noCursor(); lcd.clear();
  if (convSlot == 0) {
    lcd.setCursor(0, 0); lcd.print(convUnits[convTypeIndex].label);
    lcd.setCursor(0, 1); lcd.print("<Prev      Next>");
  } else {
    lcd.setCursor(0, 0); lcd.print(convUnits[convTypeIndex].unitIn); lcd.print(">"); lcd.print(convUnits[convTypeIndex].unitOut); if (convDecimal) lcd.print(" [.]");
    lcd.setCursor(0, 1); lcd.print(convInput, convDecPlaces); lcd.print(" "); lcd.print(convUnits[convTypeIndex].unitIn);
  }
}

// --- EEPROM Helpers ---
void saveToEEPROM(String msg) { int len = min((int)msg.length(), EEPROM_LEN - 1); for (int i = 0; i < len; i++) EEPROM.write(EEPROM_ADDR + i, msg[i]); EEPROM.write(EEPROM_ADDR + len, '\0'); }
void loadFromEEPROM() { String loaded = ""; for (int i = 0; i < EEPROM_LEN; i++) { char c = EEPROM.read(EEPROM_ADDR + i); if (c == '\0' || c == 0xFF) break; if (c >= 32 && c <= 126) loaded += c; } if (loaded.length() > 0) { message = loaded; cursorPosition = message.length(); } }

// --- Drawing Helpers ---
void drawCurrentChar() { setCursorPos(cursorPosition); lcd.print(charset[charIndex]); setCursorPos(cursorPosition); }
void setCursorPos(int pos) { if (pos < 16) lcd.setCursor(pos, 0); else lcd.setCursor(pos - 16, 1); }
void refreshTypingDisplay() {
  lcd.clear();
  int startPos = 0; if (cursorPosition >= 16) startPos = cursorPosition - 15;
  String visible = message.substring(startPos, min((int)message.length(), startPos + 32));
  lcd.setCursor(0, 0); lcd.print(visible.substring(0, min(16, (int)visible.length())));
  lcd.setCursor(0, 1); if (visible.length() > 16) lcd.print(visible.substring(16));
  drawCurrentChar(); lcd.cursor();
}
void updateCalcDisplay() {
  lcd.noCursor(); lcd.clear();
  lcd.print(num1); lcd.print(" "); lcd.print(ops[opIndex]); lcd.print(" "); lcd.print(num2);
  lcd.setCursor(0, 1);
  if (calcSlot == 0) lcd.print(enteringDecimal ? "[Num1 .]" : "[Edit Num1]");
  else if (calcSlot == 1) lcd.print("[Edit Op]  ");
  else lcd.print(enteringDecimal ? "[Num2 .]" : "[Edit Num2]");
}
