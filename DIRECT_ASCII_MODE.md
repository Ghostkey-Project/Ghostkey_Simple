# Direct ASCII Mode for Ghostkey

## Overview

Direct ASCII Mode is a specialized typing method in Ghostkey that helps overcome keyboard layout compatibility issues. This document explains how it works, when to use it, and how to troubleshoot any remaining layout problems.

## The Problem: Keyboard Layout Differences

Different countries and languages use different keyboard layouts. For example:

- US layout: Typing `'2'` produces `2`, and SHIFT+`'2'` produces `@`
- Spanish layout: Typing `'2'` produces `2`, and SHIFT+`'2'` produces `"`
- German layout: Typing `'z'` produces `y`, and `'y'` produces `z`

When Ghostkey sends raw keycodes, the receiving computer interprets them according to its own keyboard layout, which may be different from what the script intended. This leads to wrong characters being typed.

## The Solution: Direct ASCII Mode

Direct ASCII Mode bypasses layout issues by using the `Keyboard.write()` function which sends the actual ASCII value of characters instead of scan codes. This ensures that regardless of keyboard layout, the correct character appears.

### How It Works

1. Instead of sending "press key with scan code X", it sends "type ASCII character with value Y"
2. The operating system receives the specific character value rather than a key position
3. This works regardless of the active keyboard layout on the target system

### When It's Activated

Direct ASCII Mode is now activated automatically in the latest version of Ghostkey. It's used for all STRING commands and other text input.

## Components of Direct ASCII Mode

### In layout-utils.h:

```cpp
// Force type using direct ASCII bypass
void typeDirectASCII(const String &text) {
  for (unsigned int i = 0; i < text.length(); i++) {
    forceSendASCII(text[i]);
    delay(20); // Add a small delay between characters
  }
}

// Function to force ASCII character directly
void forceSendASCII(char c) {
  // The Arduino Keyboard library has a write() function that sends ASCII directly
  Keyboard.write(c);
  delay(10); // Small delay to prevent key repeats
}
```

### In bypass_mode.ino:

```cpp
// Modified process function for ducky script that uses direct ASCII mode
void processDuckyLine_DirectASCII(String line) {
  // Process commands using Direct ASCII mode
  // ...
}

// Modified main script execution for direct ASCII mode
void executeScript_DirectASCII(String scriptFile) {
  // Execute script with Direct ASCII mode
  // ...
}
```

## Enhanced Key Combination Handling

In addition to direct ASCII character typing, Ghostkey now has improved handling for key combinations:

### SHIFT Key Handling

```cpp
if (params.length() == 1) {
  char key = params[0];
  
  // For letters, generate uppercase directly using ASCII values
  if (key >= 'a' && key <= 'z') {
    char upperKey = key - 32; // Convert to uppercase
    Keyboard.write(upperKey); // Write directly as uppercase
  } 
  // For numbers, use SHIFT + number with extended delays
  else if (key >= '0' && key <= '9') {
    Keyboard.press(KEY_LEFT_SHIFT);
    delay(250);  // Even longer delay for SHIFT
    Keyboard.press(key);
    delay(250);  // Longer hold time
    Keyboard.releaseAll();
  }
}
```

### CTRL Key Handling

```cpp
Keyboard.press(KEY_LEFT_CTRL);
delay(200);  // Increased delay
  
if (params.length() == 1) {
  char key = params[0];
  Keyboard.press(key);
  delay(200);  // Longer delay
  Keyboard.releaseAll();
}
```

### Special Key Handling

```cpp
// For TAB key
Keyboard.press(KEY_TAB);
delay(200);  // Increased delay
Keyboard.releaseAll();
delay(50);   // Small delay after releasing

// For BACKSPACE key
Keyboard.press(KEY_BACKSPACE);
delay(200);  // Increased delay
Keyboard.releaseAll();
delay(50);   // Small delay after releasing
```

## Testing & Diagnosis

To help diagnose keyboard layout and key combination issues, Ghostkey includes three specialized test files:

1. **test-layout.txt** - Tests basic character output and detects layout differences
2. **key-combo-test.txt** - Tests all key combinations (CTRL, ALT, GUI, etc.)
3. **shift-key-test.txt** - Focused testing on SHIFT key combinations which can be problematic

## Remaining Limitations

While Direct ASCII Mode solves most keyboard layout issues, there are some limitations:

1. **Non-ASCII Characters**: Characters outside the ASCII range (like é, ñ, ß, etc.) may still have issues
2. **Complex Key Combinations**: Some combinations might still behave differently on certain systems
3. **ALT Codes**: Direct ASCII can't handle Windows ALT+numpad combinations for special characters

## Advanced Configuration

If you're still experiencing issues with specific characters, you can modify the `layout-utils.h` file to adjust:

1. **Timing**: Increase delay values for problematic keys
2. **Special Key Mapping**: Update the character-to-keycode mappings in `typeLayoutIndependentChar()`
3. **Direct ASCII Conversion**: Add special handling for problematic characters

## Best Practices

1. **Test First**: Always test your scripts with the diagnostic files first
2. **Simple Scripts**: Start with simple scripts and build up complexity
3. **Consistent Delays**: Add appropriate delays between key combinations
4. **Debug Output**: Use the serial monitor to see detailed information about key presses

## Technical Details

Direct ASCII Mode works at a lower level than the regular keyboard emulation:

- Regular Mode: `Keyboard.press()` and `Keyboard.release()` send HID scan codes
- Direct ASCII Mode: `Keyboard.write()` sends ASCII values directly

This is why it's more reliable across different keyboard layouts, but also why it has limitations with non-ASCII characters and complex key combinations.
