# Ghostkey SD Card Edition - Config File Version

This is an enhanced version of Ghostkey that reads and executes instructions from an SD card, supporting both custom format instructions and Ducky Script. This version adds support for a configuration file, making it easy to change settings without re-flashing the firmware.

## Hardware Requirements

1. XIAO SAMD21 or compatible ESP board
2. Micro SD card adapter compatible with your board
3. Micro SD card (formatted as FAT16/FAT32)

## Features

- **Dual Script Mode:** Supports both custom instruction format and standard Ducky Script
- **Layout Independent Typing:** Works correctly regardless of keyboard language settings
- **Direct ASCII Mode:** Bypass layout issues with direct ASCII character typing
- **Enhanced Key Combinations:** Improved reliability for CTRL, ALT, SHIFT and special key combos
- **Configuration File:** Easily change settings without re-flashing the firmware
- **Visual Feedback:** LED indicators for status and errors
- **Debug Output:** Comprehensive serial monitoring for troubleshooting

## Setup Instructions

1. **Wire the SD card module to your XIAO SAMD21 board:**
   - Connect VCC to 3.3V
   - Connect GND to GND
   - Connect MISO to D9 (pin 10)
   - Connect MOSI to D10 (pin 10)
   - Connect SCK to D8 (pin 8)
   - Connect CS to D7 (pin 7)

2. **Format your SD card:**
   - Format the SD card as FAT16 or FAT32

3. **Create configuration file:**
   - Create a file named `config.txt` in the root directory of the SD card
   - Use the provided template to customize your settings
   - Adjust script type, keyboard settings, and other options

4. **Create script file:**
   - For custom format: Create a file named `instructions.txt` (or the name specified in your config file)
   - For Ducky Script: Create a file named `payload.txt` (or the name specified in your config file)
   - Add commands following the appropriate format
   - Save the file to the root directory of the SD card

5. **Upload the code:**
   - Open `main.ino` in the Arduino IDE
   - Select your board type
   - Upload the code to your device

## Configuration File Options

The `config.txt` file allows you to customize various aspects of Ghostkey's operation:

### Script Settings

```ini
# Script mode: 0 = Custom Format, 1 = Ducky Script
SCRIPT_MODE = 1

# Script file paths
DUCKY_SCRIPT_FILE = payload.txt
CUSTOM_SCRIPT_FILE = instructions.txt
```

### Typing Settings

```ini
# Delay between keystrokes in milliseconds
TYPING_DELAY = 150

# Use layout-independent typing mode
USE_LAYOUT_INDEPENDENT = true
```

### Execution Settings

```ini
# Run script automatically on boot
AUTORUN = true

# Initial delay before starting script execution (milliseconds)
INITIAL_DELAY = 1000

# Number of times to repeat script execution (0 = no repeat)
REPEAT_COUNT = 0
```

### Debug Settings

```ini
# Enable debug output to serial monitor
DEBUG_OUTPUT = true
```

## Special Test Files

Ghostkey now supports special test files for diagnosing and fixing keyboard-related issues:

1. **`test-layout.txt`**: Tests basic keyboard layout and character typing
2. **`key-combo-test.txt`**: Tests key combinations like CTRL+key, ALT+key, etc.
3. **`shift-key-test.txt`**: Specifically tests SHIFT+key combinations which can be tricky

Place any of these files on your SD card, and Ghostkey will automatically detect and run them instead of the regular payload file.

## Script Formats

### Custom Format
Commands in `instructions.txt` should be written one per line in the format: `COMMAND:PARAMETERS`

Example:

```
DELAY:1000
RUN
TYPELINE:notepad
DELAY:500
TYPE:Hello, this is custom format!
```

### Ducky Script Format
Commands in `payload.txt` should follow standard Ducky Script syntax:

Example:

```
REM This is a comment
DELAY 1000
GUI r
STRING notepad
ENTER
STRING Hello, this is Ducky Script!
```

## Direct ASCII Mode

Ghostkey now includes a Direct ASCII Mode that completely bypasses layout-independent typing and uses Keyboard.write() to send ASCII values directly. This helps solve issues with different keyboard layouts (like Spanish, German, French, etc.).

Direct ASCII Mode is activated automatically and provides more reliable typing across different keyboard layouts, especially for special characters and key combinations.

## Enhanced Key Combinations

The latest version includes improvements for key combinations:

- **SHIFT+key**: Fixed issues with uppercase letters and symbol typing
- **CTRL+key**: More reliable key combinations for copy/paste/etc.
- **ALT+key**: Better handling of ALT combinations
- **Special keys**: Improved TAB, BACKSPACE, and other special keys

These improvements use longer delays and more sophisticated key handling to ensure compatibility across different systems.

## LED Indicators

- **LED_USER (Orange)** - Flashes at startup and when processing is complete
- **LED_RX (Blue)** - Flashes briefly when processing each instruction
- **LED_TX (Blue)** - Flashes in case of errors
  - 5 rapid flashes: SD card initialization failed
  - 3 slower flashes: Failed to open script file

## Debugging with Serial Monitor

The Ghostkey provides detailed debug information through its serial port. To use this feature:

1. Connect your device to your computer via USB
2. Open the Arduino IDE
3. Select the correct COM port for your device
4. Open the Serial Monitor (Tools > Serial Monitor)
5. Set the baud rate to 9600

The serial output provides information about:

- Configuration settings loaded from config file
- Initialization process and SD card details
- Script file detection and validation
- Line-by-line command execution in real-time
- Error messages and troubleshooting hints
- Execution statistics and completion status

Example serial output:

```
Ghostkey SD Card Edition - Config File Version
Keyboard initialized
Initializing SD card with CS on pin 7...SUCCESS!
SD card initialized.
Files found on SD card:
config.txt       342 bytes
payload.txt      125 bytes
instructions.txt 213 bytes
Reading configuration from /config.txt
Config: Script Mode = 1
Config: Ducky Script File = /payload.txt
Config: Typing Delay = 150
Config: Layout Independent = Yes
Config: Autorun = Yes
Config: Initial Delay = 1000
Config: Debug Output = Enabled
Initial delay: 1000 milliseconds
Looking for script file: /payload.txt - FOUND
Using script file: /payload.txt
Opening script file...
File opened successfully
Executing script...
```

## Troubleshooting

1. **Configuration file issues:**
   - Ensure `config.txt` is in the root directory of the SD card
   - Check for syntax errors in your configuration file
   - Verify that all keys and values are properly formatted
   - Use the serial monitor to see which settings were loaded

2. **SD card not recognized:**
   - Check wiring connections
   - Try a different SD card
   - Verify the SD card is formatted correctly
   - Check Serial Monitor for detailed error messages

3. **Scripts not executing:**
   - Check that your script file is in the root directory of the SD card
   - Verify the file format matches the specifications
   - Check for syntax errors in your script file
   - Make sure SCRIPT_MODE is set correctly for your file type
   - View Serial Monitor for command execution logs

4. **Keyboard layout issues:**
   - Use Direct ASCII Mode by placing `test-layout.txt` on the SD card first to diagnose the issue
   - Try using `shift-key-test.txt` if you're having issues specifically with SHIFT combinations
   - Consider updating `layout-utils.h` if certain characters still don't work properly

5. **Key combination problems:**
   - Place `key-combo-test.txt` on your SD card to diagnose key combination issues
   - Increase the delay values in the code if keys are still not being recognized
   - Check the serial output to see if the combinations are being properly processed

## Extending the Code

To add new commands:

1. Update the `processInstructionLine()` or `processDuckyLine()` function
2. Add any necessary helper functions
3. Update the documentation with the new commands

## Security Considerations

Remember that anyone with access to the SD card can modify the instructions and configuration. Consider physical security measures if your device will be used in sensitive environments.
