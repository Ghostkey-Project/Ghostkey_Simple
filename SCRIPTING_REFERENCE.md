# Ghostkey Scripting Reference

This document provides a comprehensive reference for writing scripts for the Ghostkey device. It covers both supported script formats: Ducky Script and Custom Format.

## Script Formats

Ghostkey supports two script formats:

1. **Ducky Script** - Standard format used by USB Rubber Ducky devices
2. **Custom Format** - Ghostkey's own simplified format

The script format is determined by the `SCRIPT_MODE` setting:
- `SCRIPT_MODE = 0` - Custom Format (instructions.txt)
- `SCRIPT_MODE = 1` - Ducky Script (payload.txt)

## Ducky Script Reference

### Basic Syntax

Commands are written one per line. Arguments follow the command after a space.

```
COMMAND arguments
```

### Comments

```
REM This is a comment - anything after REM is ignored
```

### Delays

```
DELAY 1000    // Delay for 1000 milliseconds (1 second)
DEFAULTDELAY 100    // Set default delay between all commands to 100ms
```

### Typing Text

```
STRING Hello World    // Types "Hello World"
STRINGLN Hello World    // Types "Hello World" and presses ENTER
```

### Special Keys

```
ENTER    // Press Enter key
SPACE    // Press Space key
BACKSPACE    // Press Backspace key
TAB    // Press Tab key
CAPSLOCK    // Press Caps Lock key
DELETE    // Press Delete key
END    // Press End key
ESC    // Press Escape key
HOME    // Press Home key
INSERT    // Press Insert key
PAGEUP    // Press Page Up key
PAGEDOWN    // Press Page Down key
PRINTSCREEN    // Press Print Screen key
PAUSE    // Press Pause/Break key
```

### Arrow Keys

```
UP or UPARROW    // Press Up Arrow key
DOWN or DOWNARROW    // Press Down Arrow key
LEFT or LEFTARROW    // Press Left Arrow key
RIGHT or RIGHTARROW    // Press Right Arrow key
```

### Function Keys

```
F1 through F12    // Press Function keys (F1, F2, etc.)
```

### Modifier Keys

```
CTRL or CONTROL    // Press Control key
SHIFT    // Press Shift key
ALT    // Press Alt key
GUI or WINDOWS    // Press GUI/Windows key
```

### Modifier Key Combinations

```
CTRL a    // Press Control+A
SHIFT 1    // Press Shift+1 (types "!")
ALT F4    // Press Alt+F4
GUI r    // Press Windows+R (Run dialog)
```

### Multiple Modifiers

```
CTRL SHIFT ESC    // Press Control+Shift+Escape (Task Manager)
```

### Advanced: Key Combinations with Plus Sign

```
CTRL+ALT+DELETE    // Press Control+Alt+Delete
```

### Script Control

```
REPEAT 3    // Repeat the entire script 3 times (set at beginning of script)
```

## Custom Format Reference

### Basic Syntax

Commands are written one per line in the format: `COMMAND:PARAMETERS`

```
COMMAND:PARAMETERS
```

### Delays

```
DELAY:1000    // Delay for 1000 milliseconds (1 second)
```

### Typing Text

```
TYPE:Hello World    // Types "Hello World"
TYPELINE:Hello World    // Types "Hello World" and presses Enter
TYPESOFT:Hello World    // Types "Hello World" with a delay between each character
```

### Special Key Commands

```
KEY:RETURN    // Press Enter key
KEY:TAB    // Press Tab key
```

### Windows Specific Commands

```
RUN    // Open the Run dialog (Windows+R)
ADMIN    // Trigger the "Run as administrator" prompt
```

### Program Commands

```
OPENNOTPAD    // Open Notepad
OPENPOWERSHELL    // Open PowerShell
OPENPOWERSHELLADMIN    // Open PowerShell as administrator
OPENCMD    // Open Command Prompt
OPENCMDADMIN    // Open Command Prompt as administrator
```

### Window Management

```
KILLAPP    // Close the current window (Alt+F4)
MINIMIZE    // Minimize all windows
KILLALL    // Close all windows using PowerShell
```

## Script Examples

### Example 1: Basic Notepad Script (Ducky Script)

```
REM Open Notepad and type a message
DELAY 1000
GUI r
DELAY 500
STRING notepad
ENTER
DELAY 1000
STRING Hello from Ghostkey!
ENTER
STRING This script was executed automatically.
ENTER
ENTER
STRING Current date: April 27, 2025
```

### Example 2: Basic Notepad Script (Custom Format)

```
DELAY:1000
RUN
TYPELINE:notepad
DELAY:1000
TYPE:Hello from Ghostkey!
KEY:RETURN
TYPE:This script was executed automatically.
KEY:RETURN
KEY:RETURN
TYPE:Current date: April 27, 2025
```

### Example 3: Text Manipulation (Ducky Script)

```
REM Text selection and manipulation example
DELAY 1000
GUI r
DELAY 500
STRING notepad
ENTER
DELAY 1000
STRING This is a test of text manipulation.
ENTER
DELAY 500
STRING We will now select some text and copy it.
ENTER
DELAY 500
UP
HOME
SHIFT END  
DELAY 500
CTRL c
DELAY 500
ENTER
ENTER
STRING Pasting copied text:
ENTER
CTRL v
```

### Example 4: PowerShell Command (Ducky Script)

```
REM Run a PowerShell command
DELAY 1000
GUI r
DELAY 500
STRING powershell
ENTER
DELAY 1500
STRING Get-Process | Sort-Object -Property CPU -Descending | Select-Object -First 5
ENTER
DELAY 2000
```

### Example 5: PowerShell Command (Custom Format)

```
DELAY:1000
OPENPOWERSHELL
DELAY:1500
TYPELINE:Get-Process | Sort-Object -Property CPU -Descending | Select-Object -First 5
DELAY:2000
```

## Best Practices

1. **Start with Delays**: Always begin scripts with a DELAY to give the system time to recognize the device.

2. **Add Delays Between Actions**: Include appropriate delays between actions, especially after opening applications.

3. **Test on Target System**: Scripts may behave differently on different systems due to performance variations.

4. **Use Direct ASCII Mode**: When experiencing keyboard layout issues, use Direct ASCII Mode.

5. **Break Long Scripts**: Split long scripts into smaller logical sections with comments.

6. **Error Handling**: Include error recovery steps where possible (like checks for specific text).

7. **Special Key Handling**: Use longer delays when working with modifier keys (SHIFT, CTRL, ALT).

8. **Comments**: Use REM commands to document your script for better readability.

## Troubleshooting

If your script doesn't work as expected:

1. **Timing Issues**: Try increasing the DELAY values in your script.

2. **Layout Problems**: Use the test-layout.txt file to check for keyboard layout issues.

3. **Key Combinations**: Test key combinations with key-combo-test.txt.

4. **SHIFT Issues**: Test SHIFT combinations with shift-key-test.txt.

5. **Serial Debugging**: Monitor the serial output to see exactly what commands are being processed.

6. **Simplify**: Break down complex scripts into smaller parts to isolate the problem.

## Advanced: Direct ASCII Mode

When using Direct ASCII Mode, which is now the default, the device sends ASCII characters directly rather than using keyboard scan codes. This helps overcome keyboard layout issues but may have limitations with non-ASCII characters.

For best results:

1. Stick to standard ASCII characters when possible
2. Use longer delays for complex key combinations
3. Test thoroughly on the target system
