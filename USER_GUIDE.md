# Ghostkey User Guide

## Introduction

Ghostkey is a programmable USB keyboard device based on the XIAO SAMD21 that automatically types keyboard commands and text from scripts stored on an SD card. This guide will walk you through the basic usage and troubleshooting steps.

## Quick Start

1. **Insert SD card** - Make sure your script file (payload.txt or instructions.txt) is on the SD card
2. **Connect Ghostkey** - Plug the device into a USB port on your computer
3. **Watch it work** - The device will automatically execute the script after a brief initialization

## Using Scripts

Ghostkey supports two script formats:

### Ducky Script (payload.txt)

```
REM This opens Notepad and types "Hello World"
DELAY 1000
GUI r
DELAY 500
STRING notepad
ENTER
DELAY 1000
STRING Hello World
```

### Custom Format (instructions.txt)

```
DELAY:1000
RUN
TYPELINE:notepad
DELAY:500
TYPE:Hello World
```

## Special Test Files

If you're having issues with keyboard layout or key combinations, Ghostkey includes special diagnostic files:

1. **test-layout.txt** - Tests basic character typing
2. **key-combo-test.txt** - Tests key combinations like CTRL+C, ALT+F, etc.
3. **shift-key-test.txt** - Specifically tests SHIFT key combinations

To use these, just place one on your SD card, and Ghostkey will run it automatically.

## Configuration

You can customize Ghostkey's behavior by creating a `config.txt` file on your SD card:

```ini
# Script settings
SCRIPT_MODE = 1
DUCKY_SCRIPT_FILE = payload.txt

# Typing settings
TYPING_DELAY = 150
USE_LAYOUT_INDEPENDENT = true

# Execution settings
AUTORUN = true
INITIAL_DELAY = 1000
```

## Common Issues & Solutions

### Wrong Characters Being Typed

**Symptoms:** You see characters like `ñ` when you expect `/`, or symbols in the wrong places.

**Solution:** Your computer is using a different keyboard layout than what Ghostkey expects.

1. Place `test-layout.txt` on your SD card
2. Run the test to see which characters are wrong
3. Ghostkey will automatically switch to Direct ASCII mode which should fix the problem

### Key Combinations Not Working

**Symptoms:** CTRL+C, SHIFT+arrow keys, or other combinations don't have the expected effect.

**Solution:**

1. Place `key-combo-test.txt` on your SD card
2. Run the test to see which specific combinations fail
3. If SHIFT combinations specifically are the problem, use `shift-key-test.txt`

### Device Not Running Script

**Symptoms:** The device lights up but doesn't execute commands.

**Solution:**

1. Check that your script file is in the root directory of the SD card
2. Make sure the SD card is properly formatted (FAT16/FAT32)
3. Watch the LED patterns - rapid flashing indicates an SD card error
4. Try a different SD card if issues persist

### Script Running Too Fast or Too Slow

**Symptoms:** Commands are missed or there's too much delay between actions.

**Solution:** Create a `config.txt` file with these settings:

```ini
# Adjust this value - lower is faster, higher is slower
TYPING_DELAY = 150

# Add delays in your script using DELAY command:
# DELAY 1000  (for 1 second pause)
```

## LED Indicators

- **Orange LED** - Flashes at startup and when script execution completes
- **Blue LEDs** - Flash during command execution and when errors occur

Multiple rapid flashes generally indicate an error condition.

## Advanced Features

- **Direct ASCII Mode** - Bypasses keyboard layout issues
- **Enhanced Key Combinations** - More reliable modifier key combinations
- **Layout-Independent Typing** - Works across different keyboard layouts

## Need Help?

If you're still having issues after trying the solutions in this guide:

1. Connect Ghostkey to your computer
2. Open the Arduino IDE
3. Select the correct COM port
4. Open Serial Monitor (Tools > Serial Monitor)
5. Set the baud rate to 9600
6. Observe the detailed debug output which will help diagnose the problem
