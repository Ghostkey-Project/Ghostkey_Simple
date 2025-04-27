/*
 * Bypass Mode - Direct ASCII Typing
 * 
 * This is a special mode for Ghostkey that bypasses all layout-independent typing
 * and directly sends ASCII values to work around keyboard layout issues.
 */

// Define this macro at the top of your main.ino file
#define USE_DIRECT_ASCII true

// We're using the typeDirectASCII function defined in layout-utils.h
// No need to redefine it here

// Modified process function for ducky script that uses direct ASCII mode
void processDuckyLine_DirectASCII(String line) {
  // Flash activity indicator
  digitalWrite(LED_RX, LOW);
  delay(25);
  digitalWrite(LED_RX, HIGH);
  
  // Remove extra whitespace
  line.trim();
  
  // Skip REM (comments)
  if (line.startsWith("REM")) {
    Serial.print(F("Skipping comment: "));
    Serial.println(line);
    return;
  }
  
  // Split the line into command and parameters (space-delimited)
  int spaceIndex = line.indexOf(' ');
  String command, params;
  
  if (spaceIndex != -1) {
    command = line.substring(0, spaceIndex);
    params = line.substring(spaceIndex + 1);
    command.trim();
    params.trim();
    
    Serial.print(F("Ducky command (Direct ASCII): "));
    Serial.print(command);
    Serial.print(F(", Params: "));
    Serial.println(params);
  } else {
    command = line;
    params = "";
    
    Serial.print(F("Ducky command (Direct ASCII): "));
    Serial.println(command);
  }
  
  // Process command according to Ducky Script specifications but using Direct ASCII mode
  if (command.equals("DEFAULT_DELAY") || command.equals("DEFAULTDELAY")) {
    // Set the default delay between commands
    defaultDelay = params.toInt();
    Serial.print(F("Setting default delay to "));
    Serial.print(defaultDelay);
    Serial.println(F("ms"));
  }
  else if (command.equals("DELAY")) {
    // Delay for a specific amount of time
    Serial.print(F("Delaying for "));
    Serial.print(params.toInt());
    Serial.println(F("ms"));
    delay(params.toInt());
  }  else if (command.equals("STRING")) {
    // Type out a string of characters using direct ASCII mode
    Serial.print(F("Typing string (Direct ASCII): "));
    Serial.println(params);
    
    // Using typeDirectASCII from layout-utils.h
    typeDirectASCII(params);
  }
  else if (command.equals("STRINGLN")) {
    // Type out a string of characters and press Enter
    Serial.print(F("Typing string with enter (Direct ASCII): "));
    Serial.println(params);
    
    // Using typeDirectASCII from layout-utils.h
    typeDirectASCII(params);
    delay(50); 
    Keyboard.write(KEY_RETURN);  // Use write instead of press/release
    delay(50); 
  }  else if (command.equals("ENTER")) {
    Serial.println(F("Direct ASCII - Pressing ENTER key"));
    Keyboard.press(KEY_RETURN);
    delay(200);
    Keyboard.releaseAll();
    delay(50);
  }else if (command.equals("GUI") || command.equals("WINDOWS")) {
    // Windows/GUI key
    Serial.print(F("GUI + "));
    Serial.println(params);
    
    if (params.length() > 0) {
      // GUI + key with longer delays and more reliable approach
      Keyboard.press(KEY_LEFT_GUI);
      delay(200);  // Increased to 200ms to make sure GUI is registered
      
      // Use write() for the key
      if (params.length() == 1) {
        Keyboard.write(params[0]);
      } else {
        // Try to handle special keys
        if (params.equalsIgnoreCase("r")) {
          Keyboard.write('r');
        } else {
          // For other keys, fall back to standard approach
          pressKey(params);
        }
      }
      
      delay(200);  // Longer delay to ensure key combination is registered
      Keyboard.releaseAll();
    } else {
      // Just GUI key
      Keyboard.press(KEY_LEFT_GUI);
      delay(200);
      Keyboard.releaseAll();
    }
  }
  else if (command.equals("CTRL") || command.equals("CONTROL")) {
    // CTRL key handling
    Serial.print(F("CTRL + "));
    Serial.println(params);
    
    if (params.length() > 0) {
      // CTRL + key with longer delays for more reliable operation
      Keyboard.press(KEY_LEFT_CTRL);
      delay(200);  // Increased to 200ms
      
      if (params.length() == 1) {
        char key = params[0];
        Serial.print(F("Direct ASCII - Pressing CTRL + character: "));
        Serial.println(key);
        
        // For letters, use the letter directly
        Keyboard.press(key);
        delay(200);  // Longer delay
        Keyboard.releaseAll();
      } else {
        // For named keys, use proper key code
        Serial.print(F("Direct ASCII - Pressing CTRL + named key: "));
        Serial.println(params);
        
        if (params.equalsIgnoreCase("a")) {
          Keyboard.press('a');
        } else if (params.equalsIgnoreCase("c")) {
          Keyboard.press('c');
        } else if (params.equalsIgnoreCase("v")) {
          Keyboard.press('v');
        } else if (params.equalsIgnoreCase("x")) {
          Keyboard.press('x');
        } else if (params.equalsIgnoreCase("z")) {
          Keyboard.press('z');
        } else if (params.equalsIgnoreCase("y")) {
          Keyboard.press('y');
        } else if (params.equalsIgnoreCase("s")) {
          Keyboard.press('s');
        } else {
          // Try a more generic approach for other keys
          pressKey(params);
        }
        
        delay(200);
        Keyboard.releaseAll();
      }
    } else {
      // Just CTRL key
      Keyboard.press(KEY_LEFT_CTRL);
      delay(200);
      Keyboard.releaseAll();
    }
  }
  else if (command.equals("ALT")) {
    // ALT key handling
    Serial.print(F("ALT + "));
    Serial.println(params);
    
    if (params.length() > 0) {
      // ALT + key
      Keyboard.press(KEY_LEFT_ALT);
      delay(200);  // Increased delay
      
      if (params.length() == 1) {
        char key = params[0];
        Serial.print(F("Direct ASCII - Pressing ALT + character: "));
        Serial.println(key);
        
        // For single characters
        Keyboard.press(key);
        delay(200);  // Longer delay
        Keyboard.releaseAll();
      } else {
        // For named keys
        Serial.print(F("Direct ASCII - Pressing ALT + named key: "));
        Serial.println(params);
        pressKey(params);
        delay(200);
        Keyboard.releaseAll();
      }
    } else {
      // Just ALT key
      Keyboard.press(KEY_LEFT_ALT);
      delay(200);
      Keyboard.releaseAll();
    }
  }  else if (command.equals("SHIFT")) {
    // SHIFT key handling - completely revised for better compatibility
    Serial.print(F("SHIFT + "));
    Serial.println(params);
    
    if (params.length() > 0) {
      // Special handling for SHIFT + key combinations
      if (params.length() == 1) {
        char key = params[0];
        Serial.print(F("Direct ASCII - Pressing SHIFT + character: "));
        Serial.println(key);
        
        // For letters, generate uppercase directly using ASCII values
        if (key >= 'a' && key <= 'z') {
          char upperKey = key - 32; // Convert to uppercase
          Serial.print(F("Converting to uppercase: "));
          Serial.println(upperKey);
          Keyboard.write(upperKey); // Write directly as uppercase
        } 
        // Handle special keys with SHIFT
        else if (key >= '0' && key <= '9') {
          // For numbers, use SHIFT + number
          Keyboard.press(KEY_LEFT_SHIFT);
          delay(250);  // Even longer delay for shift
          Keyboard.press(key);
          delay(250);  // Longer hold time
          Keyboard.releaseAll();
          delay(50);   // Delay after releasing
        }
        else {
          // For other characters, use SHIFT + key with extra delay
          Keyboard.press(KEY_LEFT_SHIFT);
          delay(250);  // Even longer delay for shift
          Keyboard.press(key);
          delay(250);  // Longer delay
          Keyboard.releaseAll();
          delay(50);   // Delay after releasing
        }
      } 
      // For named keys like SHIFT+RIGHT, etc.
      else {
        Serial.print(F("Direct ASCII - Pressing SHIFT + named key: "));
        Serial.println(params);
        
        // Handle arrow keys and other special keys
        Keyboard.press(KEY_LEFT_SHIFT);
        delay(250); // Extended delay
        
        // Special handling for common arrow keys
        if (params.equalsIgnoreCase("RIGHT") || params.equalsIgnoreCase("RIGHTARROW")) {
          Keyboard.press(KEY_RIGHT_ARROW);
        } 
        else if (params.equalsIgnoreCase("LEFT") || params.equalsIgnoreCase("LEFTARROW")) {
          Keyboard.press(KEY_LEFT_ARROW);
        }
        else if (params.equalsIgnoreCase("UP") || params.equalsIgnoreCase("UPARROW")) {
          Keyboard.press(KEY_UP_ARROW);
        }
        else if (params.equalsIgnoreCase("DOWN") || params.equalsIgnoreCase("DOWNARROW")) {
          Keyboard.press(KEY_DOWN_ARROW);
        }
        else if (params.equalsIgnoreCase("TAB")) {
          Keyboard.press(KEY_TAB);
        }
        else {
          // For other named keys
          pressKey(params);
        }
        
        delay(250); // Extended hold time
        Keyboard.releaseAll();
        delay(50);  // Delay after releasing
      }
    } else {
      // Just SHIFT key
      Serial.println(F("Pressing SHIFT key alone"));
      Keyboard.press(KEY_LEFT_SHIFT);
      delay(250); // Longer press for SHIFT alone
      Keyboard.releaseAll();
      delay(50);  // Delay after releasing
    }
  }
  else if (command.equals("TAB")) {
    Serial.println(F("Direct ASCII - Pressing TAB key"));
    // More reliable method for TAB - using direct keycode
    Keyboard.press(KEY_TAB);
    delay(200);
    Keyboard.releaseAll();
    delay(50);
  }
  else if (command.equals("BACKSPACE")) {
    Serial.println(F("Direct ASCII - Pressing BACKSPACE key"));
    // More reliable method for BACKSPACE - using direct keycode
    Keyboard.press(KEY_BACKSPACE);
    delay(200);
    Keyboard.releaseAll();
    delay(50);
  }
  
  // Continue with other key handling, but prefer press/releaseAll with longer delays
  
  // Wait the default delay after each command
  delay(defaultDelay);
}

// Modified main script execution for direct ASCII mode
// This function uses the typeDirectASCII function defined in layout-utils.h
void executeScript_DirectASCII(String scriptFile) {
  File scriptFileHandle = SD.open(scriptFile);
  
  if (scriptFileHandle) {
    Serial.println(F("DIRECT ASCII MODE: File opened successfully"));
    Serial.println(F("Executing script..."));
    
    // Process each line in the file
    int lineCount = 0;
    
    while (scriptFileHandle.available()) {
      String line = scriptFileHandle.readStringUntil('\n');
      line.trim(); // Remove leading/trailing whitespace
      lineCount++;
      
      // Skip empty lines and comments
      if (line.length() > 0 && !line.startsWith("//") && !line.startsWith("#")) {
        if (!line.startsWith("REM")) {
          Serial.print(F("Line "));
          Serial.print(lineCount);
          Serial.print(F(": "));
          Serial.println(line);
          
          // Use the direct ASCII processor
          processDuckyLine_DirectASCII(line);
        } else {
          Serial.print(F("Line "));
          Serial.print(lineCount);
          Serial.print(F(": "));
          Serial.println(line);
          Serial.println(F("  [Comment - Skipped]"));
        }
      }
    }
    
    scriptFileHandle.close();
    Serial.println(F("DIRECT ASCII MODE: Script execution complete"));
  }
}
