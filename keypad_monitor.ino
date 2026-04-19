#include <Keypad.h>

const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns

// Define the keypad layout
char keys[ROWS][COLS] = {
  {'/', 'c', '0', '='},
  {'*', '9', '8', '7'},
  {'-', '6', '5', '4'},
  {'+', '3', '2', '1'}
};

byte rowPins[ROWS] = {5, 4, 3, 2}; // Connect to the row pinouts of the keypad
byte colPins[COLS] = {9, 8, 7, 6}; // Connect to the column pinouts of the keypad

// Initialize the Keypad library
Keypad customKeypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Variables for the calculator logic
long number1 = 0;
long number2 = 0;
char operation = ' ';
bool newNumber = true;

void setup() {
  Serial.begin(9600);
  Serial.println("Arduino Calculator Ready");
  Serial.println("Enter first number:");
}

void loop() {
  char customKey = customKeypad.getKey();

  if (customKey) {
    if (isDigit(customKey)) {
      if (newNumber) {
        // Start a new number entry
        number1 = (number1 * 10) + (customKey - '0');
        Serial.print(customKey);
      } else {
        // Start entering the second number
        number2 = (number2 * 10) + (customKey - '0');
        Serial.print(customKey);
      }
    } else if (customKey == 'C') {
      // Clear function
      number1 = 0;
      number2 = 0;
      operation = ' ';
      newNumber = true;
      Serial.println("\nCleared. Enter first number:");
    } else if (customKey == '=') {
      // Calculation
      calculate();
    } else if (customKey == '+' || customKey == '-' || customKey == '*' || customKey == '/') {
      // Operation selection
      if (newNumber) {
        operation = customKey;
        newNumber = false;
        Serial.print(" ");
        Serial.print(operation);
        Serial.print(" ");
        Serial.println("\nEnter second number:");
      }
    }
  }
}

void calculate() {
  long result = 0;
  switch (operation) {
    case '+':
      result = number1 + number2;
      break;
    case '-':
      result = number1 - number2;
      break;
    case '*':
      result = number1 * number2;
      break;
    case '/':
      // Check for division by zero
      if (number2 != 0) {
        result = number1 / number2;
      } else {
        Serial.println("\nError: Division by zero!");
        return; // Exit the function to avoid displaying incorrect result
      }
      break;
  }
  
  Serial.print(" = ");
  Serial.println(result);
  
  // Prepare for the next calculation
  number1 = result;
  number2 = 0;
  operation = ' ';
  newNumber = true; // Wait for the next operation
  Serial.println("\nResult stored. Enter new operation or 'C' to clear:");
}
