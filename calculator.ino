#include <Keypad.h>

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

// Changed to float for decimal support
float number1 = 0;
float number2 = 0;
char operation = ' ';
bool newNumber = true; // True when entering first number, False for second

void setup() {
  Serial.begin(9600);
  Serial.println("--- Arduino Calculator Ready ---");
  Serial.println("Enter number:");
}

void loop() {
  char customKey = customKeypad.getKey();

  if (customKey) {
    // 1. Check for digits
    if (customKey >= '0' && customKey <= '9') {
      if (newNumber) {
        number1 = (number1 * 10) + (customKey - '0');
        Serial.print(customKey);
      } else {
        number2 = (number2 * 10) + (customKey - '0');
        Serial.print(customKey);
      }
    } 
    // 2. Check for Clear (Fixed case sensitivity from 'C' to 'c')
    else if (customKey == 'c') {
      number1 = 0;
      number2 = 0;
      operation = ' ';
      newNumber = true;
      Serial.println("\n--- Cleared ---");
      Serial.println("Enter first number:");
    } 
    // 3. Check for Operation
    else if (customKey == '+' || customKey == '-' || customKey == '*' || customKey == '/') {
      if (newNumber) {
        operation = customKey;
        newNumber = false; // Switch to entering second number
        Serial.print(" ");
        Serial.print(operation);
        Serial.print(" ");
      }
    } 
    // 4. Check for Equals
    else if (customKey == '=') {
      if (operation != ' ') {
        calculate();
      }
    }
  }
}

void calculate() {
  float result = 0;
  
  switch (operation) {
    case '+': result = number1 + number2; break;
    case '-': result = number1 - number2; break;
    case '*': result = number1 * number2; break;
    case '/':
      if (number2 != 0) {
        result = number1 / number2;
      } else {
        Serial.println("\nError: Div by 0");
        number1 = 0; number2 = 0; operation = ' '; newNumber = true;
        return;
      }
      break;
  }
  
  Serial.print(" = ");
  Serial.println(result);
  
  // Prepare for chained calculation
  number1 = result;
  number2 = 0;
  operation = ' ';
  newNumber = true;
  Serial.println("Result saved. Enter next operation or 'c' to clear.");
}
