#include <LiquidCrystal.h>

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

void setup() {
  lcd.begin(16, 2);
  Serial.begin(9600);
  
  lcd.print("System Booting...");
  delay(1000);
  showCurrentMode();
}

void loop() {
  int x = analogRead(0);
  
  // Check for Long Press on SELECT to switch modes
  if (x > 600 && x < 850) {
    unsigned long startHold = millis();
    while (analogRead(0) > 600 && analogRead(0) < 850) {
      if (millis() - startHold > 1000) { // 1 second hold
        currentMode = (currentMode == TYPING) ? CALCULATOR : TYPING;
        showCurrentMode();
        while(analogRead(0) < 1000); // Wait for release
        return;
      }
    }
    
    // If it wasn't a long press, treat as a normal SELECT click
    if (millis() - lastPressTime > debounceDelay) {
      handleSelectClick();
      lastPressTime = millis();
    }
  }

  // Handle other buttons
  if (millis() - lastPressTime > debounceDelay) {
    if (x < 60) { // RIGHT
      if (currentMode == TYPING) handleTypingRight(); else handleCalcRight();
      lastPressTime = millis();
    } 
    else if (x < 200) { // UP
      if (currentMode == TYPING) handleTypingUp(); else handleCalcUp();
      lastPressTime = millis();
    } 
    else if (x < 400) { // DOWN
      if (currentMode == TYPING) handleTypingDown(); else handleCalcDown();
      lastPressTime = millis();
    } 
    else if (x < 600) { // LEFT
      if (currentMode == TYPING) handleTypingLeft();
      lastPressTime = millis();
    }
  }
}

// --- Mode Switching Logic ---
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
    // Send full message to Serial Monitor
    String finalMsg = message + charset[charIndex];
    Serial.println("SENT MESSAGE: " + finalMsg);
    
    lcd.clear();
    lcd.print("Sent to Serial!");
    delay(1000);
    refreshTypingDisplay();
  } else {
    // Calculate result in Calc Mode
    calculateResult();
  }
}

// --- Typing Functions ---
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
    charIndex = 0; // Reset to space
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

// --- Calculator Functions ---
void handleCalcUp() {
  if (calcSlot == 0) num1++; 
  else if (calcSlot == 1) opIndex = (opIndex + 1) % 4; 
  else num2++;
  updateCalcDisplay();
}

void handleCalcDown() {
  if (calcSlot == 0) num1--; 
  else if (calcSlot == 1) opIndex = (opIndex - 1 + 4) % 4; 
  else num2--;
  updateCalcDisplay();
}

void handleCalcRight() {
  calcSlot = (calcSlot + 1) % 3;
  updateCalcDisplay();
}

void calculateResult() {
  float res = 0;
  if (ops[opIndex] == '+') res = num1 + num2;
  else if (ops[opIndex] == '-') res = num1 - num2;
  else if (ops[opIndex] == '*') res = num1 * num2;
  else if (ops[opIndex] == '/') res = (num2 != 0) ? num1 / num2 : 0;
  
  lcd.setCursor(0, 1);
  lcd.print("= "); lcd.print(res);
  lcd.print("            "); 
}

// --- Display Helpers ---
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
  lcd.setCursor(0,0); lcd.print(message.substring(0, 16));
  lcd.setCursor(0,1); lcd.print(message.substring(16));
  drawCurrentChar();
  lcd.cursor();
}

void updateCalcDisplay() {
  lcd.noCursor();
  lcd.clear();
  lcd.print((int)num1); lcd.print(" "); lcd.print(ops[opIndex]); lcd.print(" "); lcd.print((int)num2);
  lcd.setCursor(0, 1);
  if (calcSlot == 0) lcd.print("[Editing Num1]");
  else if (calcSlot == 1) lcd.print("[Editing Op]");
  else if (calcSlot == 2) lcd.print("[Editing Num2]");
}
