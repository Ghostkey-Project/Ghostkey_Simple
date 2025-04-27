#ifndef LAYOUT_UTILS_H
#define LAYOUT_UTILS_H

#include <Keyboard.h>

// USB HID keycodes - these are standardized
#define KEY_A       4  // a and A
#define KEY_B       5  // b and B
#define KEY_C       6  // c and C
#define KEY_D       7  // d and D
#define KEY_E       8  // e and E
#define KEY_F       9  // f and F
#define KEY_G      10  // g and G
#define KEY_H      11  // h and H
#define KEY_I      12  // i and I
#define KEY_J      13  // j and J
#define KEY_K      14  // k and K
#define KEY_L      15  // l and L
#define KEY_M      16  // m and M
#define KEY_N      17  // n and N
#define KEY_O      18  // o and O
#define KEY_P      19  // p and P
#define KEY_Q      20  // q and Q
#define KEY_R      21  // r and R
#define KEY_S      22  // s and S
#define KEY_T      23  // t and T
#define KEY_U      24  // u and U
#define KEY_V      25  // v and V
#define KEY_W      26  // w and W
#define KEY_X      27  // x and X
#define KEY_Y      28  // y and Y
#define KEY_Z      29  // z and Z

#define KEY_1      30  // 1 and !
#define KEY_2      31  // 2 and @
#define KEY_3      32  // 3 and #
#define KEY_4      33  // 4 and $
#define KEY_5      34  // 5 and %
#define KEY_6      35  // 6 and ^
#define KEY_7      36  // 7 and &
#define KEY_8      37  // 8 and *
#define KEY_9      38  // 9 and (
#define KEY_0      39  // 0 and )

// Function to send raw keycode regardless of keyboard layout
void pressRawKey(uint8_t keycode, bool withShift = false) {
  // Press shift first if needed
  if (withShift) {
    Keyboard.press(KEY_LEFT_SHIFT);
    delay(250); // Much longer delay to ensure SHIFT is registered properly
  }
  
  // Press the main key
  Keyboard.press(keycode);
  
  // Hold the key(s) for a longer time to ensure they're registered
  delay(250); // Increased substantially for better reliability
  
  // Release all keys at once
  Keyboard.releaseAll();
  
  // Add a small delay after releasing to prevent key repeats/stuck keys
  delay(50); // Increased delay after key release
}

// Function to force ASCII character directly - this bypasses layout considerations
// Use this as a fallback when layout-independent mode is causing issues
void forceSendASCII(char c) {
  // The Arduino Keyboard library has a write() function that sends ASCII directly
  Keyboard.write(c);
  delay(10); // Small delay to prevent key repeats
}

// Function to type a character using scan codes
void typeLayoutIndependentChar(char c) {
  bool useShift = false;
  uint8_t keycode = 0;
  
  // Convert the character to the correct scan code
  if (c >= 'a' && c <= 'z') {
    keycode = KEY_A + (c - 'a');
    useShift = false;
  } 
  else if (c >= 'A' && c <= 'Z') {
    keycode = KEY_A + (c - 'A');
    useShift = true;
  } 
  else if (c >= '1' && c <= '9') {
    keycode = KEY_1 + (c - '1');
    useShift = false;
  } 
  else if (c == '0') {
    keycode = KEY_0;
    useShift = false;
  } 
  else {
    // Special characters - based on US keyboard layout
    // This would need to be expanded for all possible characters
    switch (c) {
      case '!': keycode = KEY_1; useShift = true; break;
      case '@': keycode = KEY_2; useShift = true; break;
      case '#': keycode = KEY_3; useShift = true; break;
      case '$': keycode = KEY_4; useShift = true; break;
      case '%': keycode = KEY_5; useShift = true; break;
      case '^': keycode = KEY_6; useShift = true; break;
      case '&': keycode = KEY_7; useShift = true; break;
      case '*': keycode = KEY_8; useShift = true; break;
      case '(': keycode = KEY_9; useShift = true; break;
      case ')': keycode = KEY_0; useShift = true; break;
      case '-': keycode = 45; useShift = false; break;  // Minus
      case '_': keycode = 45; useShift = true; break;   // Underscore
      case '=': keycode = 46; useShift = false; break;  // Equal
      case '+': keycode = 46; useShift = true; break;   // Plus
      case '[': keycode = 47; useShift = false; break;  // Left bracket
      case '{': keycode = 47; useShift = true; break;   // Left brace
      case ']': keycode = 48; useShift = false; break;  // Right bracket
      case '}': keycode = 48; useShift = true; break;   // Right brace
      case '\\': keycode = 49; useShift = false; break; // Backslash
      case '|': keycode = 49; useShift = true; break;   // Pipe
      case ';': keycode = 51; useShift = false; break;  // Semicolon
      case ':': keycode = 51; useShift = true; break;   // Colon
      case '\'': keycode = 52; useShift = false; break; // Single quote
      case '"': keycode = 52; useShift = true; break;   // Double quote
      case '`': keycode = 53; useShift = false; break;  // Backtick
      case '~': keycode = 53; useShift = true; break;   // Tilde
      case ',': keycode = 54; useShift = false; break;  // Comma
      case '<': keycode = 54; useShift = true; break;   // Less than
      case '.': keycode = 55; useShift = false; break;  // Period
      case '>': keycode = 55; useShift = true; break;   // Greater than
      case '/': keycode = 56; useShift = false; break;  // Forward slash
      case '?': keycode = 56; useShift = true; break;   // Question mark
      case ' ': keycode = 44; useShift = false; break;  // Space
      default: return; // If character isn't handled, do nothing
    }
  }
  
  // Send the key press with or without shift
  if (keycode > 0) {
    pressRawKey(keycode, useShift);
  }
}

// Type a string using layout-independent method
void typeLayoutIndependent(const String &text) {
  for (unsigned int i = 0; i < text.length(); i++) {
    typeLayoutIndependentChar(text[i]);
    delay(20); // Add a small delay between characters
  }
}

// Type a string with delay between keystrokes using layout-independent method
void typeLayoutIndependentWithDelay(const String &text, unsigned long delayMs) {
  for (unsigned int i = 0; i < text.length(); i++) {
    typeLayoutIndependentChar(text[i]);
    delay(delayMs);
  }
}

// Force type using direct ASCII bypass (for when layout-independence fails)
void typeDirectASCII(const String &text) {
  for (unsigned int i = 0; i < text.length(); i++) {
    forceSendASCII(text[i]);
    delay(20); // Add a small delay between characters
  }
}

// Force type with delay using direct ASCII bypass
void typeDirectASCIIWithDelay(const String &text, unsigned long delayMs) {
  for (unsigned int i = 0; i < text.length(); i++) {
    forceSendASCII(text[i]);
    delay(delayMs);
  }
}

#endif // LAYOUT_UTILS_H
