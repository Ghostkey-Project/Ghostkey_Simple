# Ghostkey Diagnostic Test Files

This document explains the purpose and usage of the special diagnostic test files included with Ghostkey. These files are designed to help troubleshoot and resolve issues with keyboard layouts, key combinations, and special keys.

## Overview

Ghostkey includes three specialized test files:

1. **`test-layout.txt`** - Tests basic keyboard layout and character typing
2. **`key-combo-test.txt`** - Tests key combinations like CTRL+key, ALT+key, etc.
3. **`shift-key-test.txt`** - Specifically tests SHIFT+key combinations

To use any of these files, simply place them in the root directory of your SD card. Ghostkey will automatically detect the presence of these files and run them instead of your regular payload script.

## test-layout.txt

### Purpose
This test file helps diagnose keyboard layout issues by typing out all possible characters and observing what appears on screen.

### What it tests:
- Alphabet characters (lowercase and uppercase)
- Numbers
- Special symbols
- Direct ASCII typing mode

### How to use:
1. Place `test-layout.txt` on your SD card
2. Power up your Ghostkey device
3. It will automatically open Notepad and type a series of test patterns
4. Compare what appears on screen with what should appear
5. Note any characters that are incorrect - these indicate layout issues

### Example output:
```
KEYBOARD LAYOUT TEST
Direct ASCII mode diagnostics:

Testing alphabet: abcdefghijklmnopqrstuvwxyz
Testing CAPS: ABCDEFGHIJKLMNOPQRSTUVWXYZ
Testing numbers: 1234567890
Testing symbols: !@#$%^&*()_+-=[]{}|\;:'",.<>/?

If you see incorrect characters above, your keyboard layout is different.

Ghostkey now running direct ASCII mode for maximum compatibility.
```

## key-combo-test.txt

### Purpose
Tests various key combinations to ensure they work correctly, especially modifier keys like CTRL, ALT, and special keys like TAB and BACKSPACE.

### What it tests:
- GUI+R combination (Run dialog)
- CTRL+A, CTRL+C, CTRL+V, etc.
- ALT+F and other menu keys
- Navigation keys (TAB, ENTER, BACKSPACE)
- Multiple modifier keys together

### How to use:
1. Place `key-combo-test.txt` on your SD card
2. Power up your Ghostkey device
3. It will run through a series of key combination tests
4. Follow the on-screen instructions and observe if each combination works correctly
5. Note any combinations that fail for further troubleshooting

### Example output:
```
=== KEYBOARD SPECIAL KEYS TEST ===

Testing TAB key:
Before tab    After tab (should be indented)

Testing BACKSPACE key:
ABCD (should be ABCD)

=== Testing CTRL+key combinations ===
Type some text and press CTRL+A to select all
This is a test line
This is another test line
```

## shift-key-test.txt

### Purpose
Specifically focuses on SHIFT+key combinations, which can be particularly problematic with different keyboard layouts.

### What it tests:
- SHIFT+letter (uppercase letters)
- SHIFT+number (symbols like !, @, #, etc.)
- SHIFT+arrow keys (text selection)
- Holding SHIFT while pressing multiple keys

### How to use:
1. Place `shift-key-test.txt` on your SD card
2. Power up your Ghostkey device
3. It will open Notepad and perform various SHIFT key tests
4. Compare the output with the expected results
5. Pay special attention to whether text selection works and if symbols appear correctly

### Example output:
```
=== SHIFT KEY TEST ===

Testing SHIFT + letter: ABC (should show ABC)

Testing SHIFT + number: !@#$% (should show !@#$%)

Testing SHIFT + arrow:
ABCDEFG (should have selected ABC)

Testing hold-down SHIFT for multiple keys:
abcdefg [Selection should have worked]
```

## Troubleshooting with Test Files

1. **If characters appear incorrect in `test-layout.txt`:**
   - This confirms you're experiencing a keyboard layout issue
   - The Direct ASCII Mode should automatically fix most layout problems
   - Check which specific characters are problematic

2. **If key combinations fail in `key-combo-test.txt`:**
   - Note which specific combinations don't work
   - Try increasing the delay values in the code for those specific combinations
   - Consider adding additional delay between pressing modifier keys and regular keys

3. **If SHIFT combinations fail in `shift-key-test.txt`:**
   - For uppercase letters, try using the direct ASCII conversion approach
   - For symbols, you may need to adjust the specific keycodes for your layout
   - For text selection, try increasing the delays between key presses

## Advanced Customization

If you're still experiencing issues after running the test files, you can modify the following files to fine-tune the behavior:

1. **`layout-utils.h`** - Contains the functions for layout-independent typing and direct ASCII mode
2. **`bypass_mode.ino`** - Contains the direct ASCII processing functions for keyboard commands
3. **`main.ino`** - Contains the main key processing functions

The most common adjustments are:

1. Increasing delay times for problematic key combinations
2. Adjusting the key mappings for special characters
3. Modifying the order of key press/release for certain combinations

## Creating Custom Test Files

You can create your own specialized test files by following the Ducky Script syntax and focusing on the specific keys or combinations you're having trouble with. Place these files on your SD card with a unique name and modify the main.ino file to recognize your custom test file.
