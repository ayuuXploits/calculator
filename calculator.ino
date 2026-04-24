#include <Keypad.h>

// Keypad Configuration
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'/', 'c', '0', '='},
  {'*', '9', '8', '7'},
  {'-', '6', '5', '4'},
  {'+', '3', '2', '1'}
};
byte rowPins[ROWS] = {5, 4, 3, 2};
byte colPins[COLS] = {9, 8, 7, 6};

Keypad customKeypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// State Machine Variables
enum State { INPUT_VAL1, INPUT_VAL2 };
State currentState = INPUT_VAL1;

float val1 = 0;
float val2 = 0;
char operation = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("--- Arduino Calculator Ready ---");
}

void loop() {
  char key = customKeypad.getKey();
  if (!key) return;

  if (key == 'c') {
    resetCalculator();
    return;
  }

  if (key >= '0' && key <= '9') {
    handleDigit(key - '0');
  } 
  else if (key == '+' || key == '-' || key == '*' || key == '/') {
    handleOperator(key);
  } 
  else if (key == '=') {
    performCalculation();
  }
}

void handleDigit(int digit) {
  if (currentState == INPUT_VAL1) {
    val1 = (val1 * 10) + digit;
    Serial.print(digit);
  } else {
    val2 = (val2 * 10) + digit;
    Serial.print(digit);
  }
}

void handleOperator(char op) {
  // If we already have an operator, calculate the intermediate result first
  if (currentState == INPUT_VAL2) {
    performCalculation();
  }
  
  operation = op;
  currentState = INPUT_VAL2;
  Serial.print(" ");
  Serial.print(operation);
  Serial.print(" ");
}

void performCalculation() {
  if (currentState != INPUT_VAL2) return;

  float result = 0;
  switch (operation) {
    case '+': result = val1 + val2; break;
    case '-': result = val1 - val2; break;
    case '*': result = val1 * val2; break;
    case '/':
      if (val2 != 0) result = val1 / val2;
      else {
        Serial.println("\nError: Div by 0");
        resetCalculator();
        return;
      }
      break;
  }

  Serial.print(" = ");
  Serial.println(result);
  
  // Set up for next operation (Chainable)
  val1 = result;
  val2 = 0;
  operation = 0;
  currentState = INPUT_VAL1; 
  Serial.println("Result stored. Enter next op or new number.");
}

void resetCalculator() {
  val1 = 0;
  val2 = 0;
  operation = 0;
  currentState = INPUT_VAL1;
  Serial.println("\n--- Cleared ---");
}
