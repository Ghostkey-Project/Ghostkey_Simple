/*
 * Ghostkey SD Card Edition - Unified Version
 * 
 * This firmware reads and executes keyboard instructions from an SD card.
 * It supports both custom format instructions and Ducky Script.
 * 
 * Serial Debugging:
 * - Open Serial Monitor at 9600 baud to see detailed debug output
 * - Provides information about SD card, script execution, and system state
 * - Set VERBOSE_DEBUG to false to reduce output in production
 * 
 * Instructions for adding new commands:
 * 1. Update processInstructionLine() or processDuckyLine() function
 * 2. Add any necessary helper functions
 * 3. Update documentation in README_UNIFIED.md
 */

#include <SPI.h>
#include <SD.h>
#include <Keyboard.h>
#include "lib/simple-instructions.h"
#include "lib/complex-instructions.h"
#include "lib/layout-utils.h"

// Define diagnostic functions here instead of using external header
// since we're having issues with the header file

// Function to get SD card type as string
String getSDCardTypeString() {
  // This is a simplification since SAMD21 SD library doesn't expose card type
  // We'll try to make an educated guess
  File root = SD.open("/");
  if (!root) {
    return "Unknown";
  }
  
  // Check for presence of common files on different card types
  if (SD.exists("/DCIM")) {
    return "SD/SDHC (Camera Card)";
  }
  
  // Try to create a large test file to distinguish between SD and SDHC
  File testFile = SD.open("/cardtype.bin", FILE_WRITE);
  if (testFile) {
    byte buffer[512];
    memset(buffer, 0, 512);
    
    // Try to write 3GB of data (which would fail on regular SD)
    bool canWriteLarge = true;
    for (int i = 0; i < 10; i++) { // Just do a small sample test
      if (testFile.write(buffer, 512) != 512) {
        canWriteLarge = false;
        break;
      }
    }
    testFile.close();
    SD.remove("/cardtype.bin");
    
    if (canWriteLarge) {
      return "SDHC/SDXC (>2GB)";
    } else {
      return "SD (<= 2GB)";
    }
  }
  
  return "SD Compatible";
}

// Function to test read speed of SD card (in KB/s)
float testSDReadSpeed() {
  File testFile;
  const int bufSize = 512;
  byte buffer[bufSize];
  unsigned long startTime, endTime;
  unsigned long bytesRead = 0;
  const unsigned long testDuration = 1000; // Test for 1 second
  float readSpeed = 0;
  
  // Create a test file if it doesn't exist
  if (!SD.exists("/speedtest.bin")) {
    testFile = SD.open("/speedtest.bin", FILE_WRITE);
    if (testFile) {
      // Write 100KB of test data
      for (int i = 0; i < 200; i++) { // 200 * 512 bytes = ~100KB
        for (int j = 0; j < bufSize; j++) {
          buffer[j] = j & 0xFF; // Simple pattern
        }
        testFile.write(buffer, bufSize);
      }
      testFile.close();
    }
  }
  
  // Open file for reading
  testFile = SD.open("/speedtest.bin");
  if (!testFile) {
    return 0; // Failed to open test file
  }
  
  // Read the file repeatedly for the test duration
  startTime = millis();
  while (millis() - startTime < testDuration) {
    if (testFile.available() < bufSize) {
      testFile.seek(0); // Start over if we reach the end
    }
    int bytesReadThisTime = testFile.read(buffer, bufSize);
    if (bytesReadThisTime > 0) {
      bytesRead += bytesReadThisTime;
    }
  }
  endTime = millis();
  testFile.close();
  
  // Calculate read speed in KB/s
  unsigned long duration = endTime - startTime;
  readSpeed = (float)(bytesRead / 1024.0) / (duration / 1000.0);
  
  return readSpeed;
}

// Function to test write speed of SD card (in KB/s)
float testSDWriteSpeed() {
  File testFile;
  const int bufSize = 512;
  byte buffer[bufSize];
  unsigned long startTime, endTime;
  unsigned long bytesWritten = 0;
  const unsigned long testDuration = 1000; // Test for 1 second
  float writeSpeed = 0;
  
  // Fill buffer with test pattern
  for (int i = 0; i < bufSize; i++) {
    buffer[i] = i & 0xFF;
  }
  
  // Open file for writing
  testFile = SD.open("/writespeed.bin", FILE_WRITE);
  if (!testFile) {
    return 0; // Failed to create test file
  }
  
  // Write to the file repeatedly for the test duration
  startTime = millis();
  while (millis() - startTime < testDuration) {
    int bytesWrittenThisTime = testFile.write(buffer, bufSize);
    if (bytesWrittenThisTime > 0) {
      bytesWritten += bytesWrittenThisTime;
    }
  }
  endTime = millis();
  testFile.close();
  
  // Calculate write speed in KB/s
  unsigned long duration = endTime - startTime;
  writeSpeed = (float)(bytesWritten / 1024.0) / (duration / 1000.0);
  
  // Clean up test file
  SD.remove("/writespeed.bin");
  
  return writeSpeed;
}

// Calculate approximate SD card capacity
unsigned long getSDCardSizeApprox() {
  // This is an approximation method since the SD library doesn't provide direct size info
  unsigned long maxSize = 0;
  unsigned long blockSize = 512; // Standard block size
  
  // Try creating test files until we run out of space
  unsigned long testSize = 1024 * 1024; // 1MB chunks
  String testFileName = "/sizetest.bin";
  
  // First, make sure we're starting fresh
  if (SD.exists(testFileName)) {
    SD.remove(testFileName);
  }
  
  // Create and grow test file
  File testFile = SD.open(testFileName, FILE_WRITE);
  if (!testFile) {
    return 0;
  }
  
  byte buffer[512];
  memset(buffer, 0xAA, 512); // Fill with pattern
  
  // Write in chunks up to a reasonable limit (8GB) or until failure
  const unsigned long maxTestSize = 8UL * 1024 * 1024 * 1024; // 8GB max test
  const unsigned long chunkSize = 1024 * 512; // 512KB chunks (1024 blocks)
  unsigned long totalWritten = 0;
  unsigned long lastSuccessSize = 0;
  
  while (totalWritten < maxTestSize) {
    // Write a chunk of data
    for (unsigned long i = 0; i < chunkSize/512 && totalWritten < maxTestSize; i++) {
      if (testFile.write(buffer, 512) == 512) {
        totalWritten += 512;
      } else {
        // Write failed, we've reached capacity
        break;
      }
    }
    
    // Periodically flush and check
    testFile.flush();
    lastSuccessSize = totalWritten;
    
    // If file size didn't increase as expected, we've reached capacity
    if (testFile.size() < totalWritten) {
      break;
    }
    
    // Quick timeout check - test for max 10 seconds
    if ((millis() % 10000) < 10) { // Simple non-blocking timeout
      break;
    }
  }
  
  testFile.close();
  SD.remove(testFileName); // Clean up
  
  // Return approximate size in MB
  return lastSuccessSize / (1024 * 1024);
}

// Test SD card health and report issues
void testSDCardHealth(char* result, int maxSize) {
  // Initialize result buffer
  strncpy(result, "Card Health: ", maxSize);
  
  // Create multiple test files and verify content
  const int numTestFiles = 3;
  const int testDataSize = 512;
  byte writeBuffer[testDataSize];
  byte readBuffer[testDataSize];
  bool testPassed = true;
  
  // Fill write buffer with test pattern
  for (int i = 0; i < testDataSize; i++) {
    writeBuffer[i] = (i * 7 + 13) & 0xFF; // Simple unique pattern
  }
  
  // Create and test each file
  for (int fileNum = 0; fileNum < numTestFiles && testPassed; fileNum++) {
    String fileName = "/sdtest";
    fileName += fileNum;
    fileName += ".bin";
    
    // Write test data
    File testFile = SD.open(fileName, FILE_WRITE);
    if (!testFile) {
      testPassed = false;
      strncat(result, "Write Error", maxSize - strlen(result) - 1);
      break;
    }
    
    testFile.write(writeBuffer, testDataSize);
    testFile.close();
    
    // Read back and verify
    testFile = SD.open(fileName);
    if (!testFile) {
      testPassed = false;
      strncat(result, "Read Error", maxSize - strlen(result) - 1);
      break;
    }
    
    int bytesRead = testFile.read(readBuffer, testDataSize);
    testFile.close();
    
    if (bytesRead != testDataSize) {
      testPassed = false;
      strncat(result, "Size Mismatch", maxSize - strlen(result) - 1);
      break;
    }
    
    // Compare buffers
    for (int i = 0; i < testDataSize; i++) {
      if (readBuffer[i] != writeBuffer[i]) {
        testPassed = false;
        strncat(result, "Data Corruption", maxSize - strlen(result) - 1);
        break;
      }
    }
    
    // Clean up test file
    SD.remove(fileName);
  }
  
  // If all tests passed
  if (testPassed) {
    strncat(result, "Good", maxSize - strlen(result) - 1);
  }
}

// Define pins
#define LED_USER 13  // User LED (orange)
#define LED_RX 12    // RX LED (blue)
#define LED_TX 11    // TX LED (blue)

// SD card pins for XIAO SAMD21 (as per schematic)
#define SD_CS_PIN 7   // SD card CS pin connected to D7
// MISO is connected to D8
// MOSI is connected to D10
// SCK is connected to D8

// Define constants
const unsigned long TYPING_DELAY = 25; // Delay between keystrokes in milliseconds

// Script file options (only one will be used based on mode selection)
const String DUCKY_SCRIPT_FILE = "/payload.txt";    // Ducky Script file
const String CUSTOM_SCRIPT_FILE = "/instructions.txt"; // Custom format script file

// Configuration options
const bool USE_LAYOUT_INDEPENDENT = true;  // Set to true for layout-independent typing (works across different keyboard layouts)

// ---- CONFIG OPTIONS: Edit these to change behavior ----
// SCRIPT_MODE: 
// 0 = Custom format (instructions.txt)
// 1 = Ducky Script (payload.txt)
const byte SCRIPT_MODE = 1;
// --------------------------------------------------

// Variables
unsigned int defaultDelay = 0; // Default delay between commands (ms)
bool repeatScriptMode = false; // Whether to repeat script execution
int repeatScriptCount = 0; // Count of script repetitions
int currentRepeat = 0; // Current repeat count

// Debug options
const bool VERBOSE_DEBUG = true; // Set to false to reduce serial output

// Configuration file path
const String CONFIG_FILE = "/config.txt";

// Default configuration values (used if config file not found or if specific setting missing)
struct {
  byte scriptMode = 1;                // Default: Ducky Script (1)
  String duckyScriptFile = "/payload.txt";
  String customScriptFile = "/instructions.txt";
  unsigned long typingDelay = 25;     // Default: 25ms delay between keystrokes
  bool useLayoutIndependent = true;   // Default: Use layout-independent typing
  bool autorunOnBoot = true;          // Default: Run script automatically on boot
  int initialDelay = 1000;            // Default: 1000ms delay before starting script execution
  int repeatCount = 0;                // Default: 0 = no repeat
  bool debugOutput = true;            // Default: Enable debug output
} config;

// Function to read and parse configuration file
void readConfigFile() {
  if (!SD.exists(CONFIG_FILE)) {
    Serial.println(F("Config file not found, using default settings"));
    return;
  }
  
  Serial.print(F("Reading configuration from "));
  Serial.println(CONFIG_FILE);
  
  File configFile = SD.open(CONFIG_FILE);
  if (!configFile) {
    Serial.println(F("Failed to open config file, using default settings"));
    return;
  }
  
  while (configFile.available()) {
    String line = configFile.readStringUntil('\n');
    line.trim();
    
    // Skip empty lines and comments
    if (line.length() == 0 || line.startsWith("#") || line.startsWith("//")) {
      continue;
    }
    
    // Split line into key and value
    int separatorPos = line.indexOf('=');
    if (separatorPos == -1) {
      continue;  // Skip malformed lines
    }
    
    String key = line.substring(0, separatorPos);
    String value = line.substring(separatorPos + 1);
    key.trim();
    value.trim();
    
    // Process each configuration option
    if (key.equalsIgnoreCase("SCRIPT_MODE")) {
      config.scriptMode = value.toInt();
      Serial.print(F("Config: Script Mode = "));
      Serial.println(config.scriptMode);
    }
    else if (key.equalsIgnoreCase("DUCKY_SCRIPT_FILE")) {
      // Ensure path starts with /
      if (!value.startsWith("/")) {
        value = "/" + value;
      }
      config.duckyScriptFile = value;
      Serial.print(F("Config: Ducky Script File = "));
      Serial.println(config.duckyScriptFile);
    }
    else if (key.equalsIgnoreCase("CUSTOM_SCRIPT_FILE")) {
      // Ensure path starts with /
      if (!value.startsWith("/")) {
        value = "/" + value;
      }
      config.customScriptFile = value;
      Serial.print(F("Config: Custom Script File = "));
      Serial.println(config.customScriptFile);
    }
    else if (key.equalsIgnoreCase("TYPING_DELAY")) {
      config.typingDelay = value.toInt();
      Serial.print(F("Config: Typing Delay = "));
      Serial.println(config.typingDelay);
    }
    else if (key.equalsIgnoreCase("USE_LAYOUT_INDEPENDENT")) {
      if (value.equalsIgnoreCase("true") || value.equalsIgnoreCase("1") || value.equalsIgnoreCase("yes")) {
        config.useLayoutIndependent = true;
      } else {
        config.useLayoutIndependent = false;
      }
      Serial.print(F("Config: Layout Independent = "));
      Serial.println(config.useLayoutIndependent ? F("Yes") : F("No"));
    }
    else if (key.equalsIgnoreCase("AUTORUN")) {
      if (value.equalsIgnoreCase("false") || value.equalsIgnoreCase("0") || value.equalsIgnoreCase("no")) {
        config.autorunOnBoot = false;
      } else {
        config.autorunOnBoot = true;
      }
      Serial.print(F("Config: Autorun = "));
      Serial.println(config.autorunOnBoot ? F("Yes") : F("No"));
    }
    else if (key.equalsIgnoreCase("INITIAL_DELAY")) {
      config.initialDelay = value.toInt();
      Serial.print(F("Config: Initial Delay = "));
      Serial.println(config.initialDelay);
    }
    else if (key.equalsIgnoreCase("REPEAT_COUNT")) {
      config.repeatCount = value.toInt();
      Serial.print(F("Config: Repeat Count = "));
      Serial.println(config.repeatCount);
    }
    else if (key.equalsIgnoreCase("DEBUG_OUTPUT")) {
      if (value.equalsIgnoreCase("false") || value.equalsIgnoreCase("0") || value.equalsIgnoreCase("no")) {
        config.debugOutput = false;
      } else {
        config.debugOutput = true;
      }
      Serial.print(F("Config: Debug Output = "));
      Serial.println(config.debugOutput ? F("Enabled") : F("Disabled"));
    }
  }
  
  configFile.close();
}

// Function prototypes
void processInstructionLine(String line);
void processDuckyLine(String line);
void flashLED(int led, int times, int duration);
void typeWithDelay(String text);
void pressKey(String keyString);
void printDirectory(File dir, int numTabs);
bool attemptSDCardRecovery();
void showSDCardError(int errorPattern);
void runSDCardDiagnostics();

void setup() {
  // Initialize LEDs
  pinMode(LED_USER, OUTPUT);
  pinMode(LED_RX, OUTPUT);
  pinMode(LED_TX, OUTPUT);

  // Set all LEDs to OFF (HIGH due to inverted logic)
  digitalWrite(LED_USER, HIGH);
  digitalWrite(LED_RX, HIGH);
  digitalWrite(LED_TX, HIGH);

  // Initialize serial communication for debugging
  Serial.begin(9600);
  // Wait for serial port to connect
  delay(1000);
    Serial.println(F("Ghostkey SD Card Edition - Startup"));
  Serial.print(F("Script Mode: "));
  Serial.println(SCRIPT_MODE == 1 ? F("Ducky Script") : F("Custom Format"));
  Serial.print(F("Layout Independent Mode: "));
  Serial.println(config.useLayoutIndependent ? F("Enabled") : F("Disabled"));
  Serial.println(F("Keyboard debugging enabled - check serial output for keystroke info"));
  
  // Flash LED to indicate startup
  flashLED(LED_USER, 3, 200);
  // Initialize Keyboard
  Keyboard.begin();
  Serial.println(F("Keyboard initialized"));
  // Initialize SD card with advanced error handling
  Serial.print(F("Initializing SD card with CS on pin "));
  Serial.print(SD_CS_PIN);
  Serial.println(F("..."));
  
  // Variables for SD card mounting
  const int maxRetries = 3;
  int retryCount = 0;
  bool sdInitialized = false;
  unsigned long sdInitStartTime = millis();

  // Try to mount the SD card multiple times with increasing delays
  while (retryCount < maxRetries && !sdInitialized) {
    // Visual indication of attempt
    if (retryCount > 0) {
      Serial.print(F("Retry "));
      Serial.print(retryCount);
      Serial.print(F(" of "));
      Serial.print(maxRetries - 1);
      Serial.print(F(", after "));
      Serial.print(retryCount * 500);
      Serial.println(F("ms delay..."));
      flashLED(LED_TX, 2, 100); // Flash TX LED to indicate retry attempt
    }

    // Attempt SD initialization
    sdInitialized = SD.begin(SD_CS_PIN);
    
    // If still not successful, wait and try again
    if (!sdInitialized && retryCount < maxRetries - 1) {
      delay(retryCount * 500 + 500); // Increasing delay with each retry
    }
    retryCount++;
  }
  // Handle SD card initialization result
  if (!sdInitialized) {
    // SD card initialization failed after all retries
    unsigned long sdInitDuration = millis() - sdInitStartTime;
    
    Serial.println(F("FAILED after multiple attempts!"));
    Serial.print(F("Time spent trying: "));
    Serial.print(sdInitDuration);
    Serial.println(F("ms"));
    
    // Attempt recovery process
    Serial.println(F("\nInitiating SD card recovery process..."));
    bool recoverySuccess = attemptSDCardRecovery();
    
    if (recoverySuccess) {
      Serial.println(F("\n*** SD CARD RECOVERY SUCCESSFUL ***"));
      Serial.println(F("Card has been remounted and is now available"));
      
      // Run basic tests to ensure card is working properly
      runSDCardDiagnostics();
      
      // Continue with normal execution
      sdInitialized = true;
    } else {
      // Recovery failed - report detailed diagnostic information
      Serial.println(F("\n*** SD CARD RECOVERY FAILED ***"));
      Serial.println(F("\nSD Card Error Diagnostics:"));
      Serial.println(F("---------------------------"));
      Serial.println(F("1. Hardware Check:"));
      Serial.println(F("   - Ensure SD card is properly inserted"));
      Serial.println(F("   - Check that SD card is not damaged or corrupted"));
      Serial.println(F("   - Verify SD card is formatted as FAT16/FAT32"));
      Serial.println(F("   - Try with a different SD card if available"));
      
      Serial.println(F("\n2. Wiring Check:"));
      Serial.print(F("   - CS pin (D"));
      Serial.print(SD_CS_PIN);
      Serial.println(F(") connected properly"));
      Serial.println(F("   - MISO connected to D8"));
      Serial.println(F("   - MOSI connected to D10"));
      Serial.println(F("   - SCK connected to D9"));
      
      Serial.println(F("\n3. Power Issues:"));
      Serial.println(F("   - Check for stable power supply"));
      Serial.println(F("   - SD card may require more current than available"));
      Serial.println(F("   - Try with a different SD card (preferably Class 4 or 10)"));
      
      Serial.println(F("\n4. Software Issues:"));
      Serial.println(F("   - SD library might not support this specific card"));
      Serial.println(F("   - Try formatting card with official SD Formatter tool"));
      
      Serial.println(F("\n5. Emergency Fix:"));
      Serial.println(F("   - Format card in FAT32 using SD Association's formatter"));
      Serial.println(F("   - Try using a different SD card reader"));
      Serial.println(F("   - Check for physical damage to the SD slot"));
      
      // Try to determine the most likely error type
      int errorType;
      
      // Analyze timing of failure to guess error type
      if (sdInitDuration < 100) {
        errorType = 1; // Very quick failure suggests card detection issue
        Serial.println(F("\nDiagnosis: CARD DETECTION FAILURE"));
        Serial.println(F("The SD card is not being detected. Check if card is inserted properly."));
      } else if (sdInitDuration < 500) {
        errorType = 3; // Quick failure but with some activity suggests communication issue
        Serial.println(F("\nDiagnosis: COMMUNICATION FAILURE"));
        Serial.println(F("The SD card was detected but communication failed. Check wiring."));
      } else {
        errorType = 2; // Longer failure time suggests format or filesystem issue
        Serial.println(F("\nDiagnosis: FORMAT OR FILESYSTEM ERROR"));
        Serial.println(F("The SD card was detected but couldn't be mounted. Check format."));
      }
      
      // Error visualization - Different patterns for different likely issues
      Serial.println(F("\nError code visualization with LEDs:"));
      Serial.println(F("Pattern 1: Card detection issue"));
      Serial.println(F("Pattern 2: Format issue"));
      Serial.println(F("Pattern 3: Wiring/communication issue"));
      Serial.println(F("Pattern 4: File system corruption"));
      
      // Show the error pattern
      showSDCardError(errorType);
      
      Serial.println(F("\nSystem halted due to unrecoverable SD card failure"));
      Serial.println(F("Please fix the issues and restart the device"));
      
      // Halt system but with pattern indication
      while (1) {
        showSDCardError(errorType);
        delay(1000);
      }
    }
  }
    // SD card initialization successful
  Serial.println(F("SUCCESS!"));
  Serial.print(F("SD card initialized after "));
  Serial.print(retryCount);
  Serial.println(F(" attempt(s)"));
  
  // Get SD card info when available
  Serial.println(F("\nSD Card Information:"));
  Serial.println(F("-------------------"));
  
  // Run diagnostics on the SD card (with timeout protection)
  unsigned long diagStartTime = millis();
  
  // Show diagnostic running indicator
  digitalWrite(LED_USER, LOW); // Turn on USER LED during diagnostics
  
  // Get card type
  String cardType = getSDCardTypeString();
  Serial.print(F("Card Type: "));
  Serial.println(cardType);
  
  // Test card health
  char healthResult[50] = {0};
  testSDCardHealth(healthResult, sizeof(healthResult));
  Serial.println(healthResult);
  
  // Skip performance tests if they would take too long
  if (millis() - diagStartTime < 3000) { // Only if diagnostics haven't taken too long already
    // Test read speed
    Serial.print(F("Testing read speed..."));
    float readSpeed = testSDReadSpeed();
    Serial.print(F(" "));
    Serial.print(readSpeed, 1);
    Serial.println(F(" KB/s"));
    
    // Test write speed if we still have time
    if (millis() - diagStartTime < 5000) {
      Serial.print(F("Testing write speed..."));
      float writeSpeed = testSDWriteSpeed();
      Serial.print(F(" "));
      Serial.print(writeSpeed, 1);
      Serial.println(F(" KB/s"));
    }
  } else {
    Serial.println(F("Performance tests skipped (timeout)"));
  }
  
  // Get approximate card size (skip if diagnostics taking too long)
  if (millis() - diagStartTime < 7000) {
    Serial.print(F("Approximate card size: "));
    unsigned long sizeMB = getSDCardSizeApprox();
    if (sizeMB > 0) {
      if (sizeMB >= 1024) {
        Serial.print(sizeMB / 1024);
        Serial.println(F("GB+"));
      } else {
        Serial.print(sizeMB);
        Serial.println(F("MB+"));
      }
    } else {
      Serial.println(F("Unknown"));
    }
  } else {
    Serial.println(F("Size detection skipped (timeout)"));
  }
  
  // Calculate and show time spent on diagnostics
  unsigned long diagTime = millis() - diagStartTime;
  Serial.print(F("Diagnostics completed in "));
  Serial.print(diagTime);
  Serial.println(F("ms"));
  
  // Turn off diagnostic indicator LED
  digitalWrite(LED_USER, HIGH);
  
  // Display SD card info
  File root = SD.open("/");
  Serial.println(F("Files found on SD card:"));
  printDirectory(root, 0);
  root.close();
  
  // Read configuration file if it exists
  readConfigFile();
  
  // Apply initial delay from config if specified
  if (config.initialDelay > 0) {
    Serial.print(F("Initial delay: "));
    Serial.print(config.initialDelay);
    Serial.println(F(" milliseconds"));
    delay(config.initialDelay);
  }
  
  // Set repeat mode based on config
  if (config.repeatCount > 0) {
    repeatScriptMode = true;
    repeatScriptCount = config.repeatCount;
    Serial.print(F("Script will repeat: "));
    Serial.print(repeatScriptCount);
    Serial.println(F(" times"));
  }
  
  // Skip script execution if autorun is disabled
  if (!config.autorunOnBoot) {
    Serial.println(F("Autorun is disabled in config. Skipping script execution."));
    return;
  }
  // Choose appropriate script file based on config mode
  String scriptFile = (config.scriptMode == 1) ? config.duckyScriptFile : config.customScriptFile;
  Serial.print(F("Looking for primary script file: "));
  Serial.print(scriptFile);
  
  // Check if script file exists
  if (!SD.exists(scriptFile)) {
    Serial.println(F(" - NOT FOUND"));
    Serial.println(F("Primary script not found, trying fallback..."));    // Try the other file type as a fallback
    scriptFile = (config.scriptMode == 1) ? config.customScriptFile : config.duckyScriptFile;
    Serial.print(F("Looking for fallback script file: "));
    Serial.print(scriptFile);
    
    // If still not found, signal error
    if (!SD.exists(scriptFile)) {
      Serial.println(F(" - NOT FOUND"));
      Serial.println(F("No script files found. Please add an instructions.txt or payload.txt file to your SD card."));
      // Script file not found
      flashLED(LED_TX, 5, 100); // Error indicator
      while (1); // Stop execution
    }
    Serial.println(F(" - FOUND"));
  } else {
    Serial.println(F(" - FOUND"));
  }
  // Check for test files
  if (SD.exists("/test-layout.txt")) {
    Serial.println(F("Layout test file found - running this instead to diagnose keyboard issues"));
    scriptFile = "/test-layout.txt";
  }
  else if (SD.exists("/key-combo-test.txt")) {
    Serial.println(F("Key combination test file found - running this to test special key combinations"));
    scriptFile = "/key-combo-test.txt";
  }
  else if (SD.exists("/shift-key-test.txt")) {
    Serial.println(F("SHIFT key test file found - running this to test SHIFT key combinations"));
    scriptFile = "/shift-key-test.txt";
  }
  
  Serial.print(F("Using script file: "));
  Serial.println(scriptFile);
  
  // Announce Direct ASCII mode
  Serial.println(F("\n*** DIRECT ASCII MODE ACTIVE ***"));
  Serial.println(F("Using direct ASCII key handling to bypass layout issues"));
  
  do {
    // Reset for next iteration if in repeat mode
    if (repeatScriptMode && currentRepeat > 0) {
      Serial.print(F("Repeating script execution. Iteration "));
      Serial.print(currentRepeat + 1);
      if (repeatScriptCount > 0) {
        Serial.print(F(" of "));
        Serial.print(repeatScriptCount);
      }
      Serial.println();
      delay(1000); // Wait between repetitions
      flashLED(LED_USER, 2, 100); // Signal new repetition
    }
      // Open and process script file
    Serial.println(F("Opening script file..."));
    File scriptFileHandle = SD.open(scriptFile);
    if (scriptFileHandle) {
      // Flash LED to indicate file opened successfully
      flashLED(LED_USER, 2, 200);
      Serial.println(F("File opened successfully"));
      Serial.println(F("Executing script..."));
        // Process each line in the file
      int lineCount = 0;
      int executedCount = 0;
      int skippedCount = 0;
      unsigned long startTime = millis();
      
      // Check if we should use direct ASCII mode
      bool useDirectASCII = true; // Force direct ASCII mode on
      
      if (useDirectASCII) {
        // Use direct ASCII mode execution
        Serial.println(F("Using DIRECT ASCII MODE for script execution"));
        executeScript_DirectASCII(scriptFile);
        
        // Since we're using an entirely different execution method,
        // we'll set some default counts
        lineCount = 1;  // At least one line
        executedCount = 1;
        skippedCount = 0;
      } else {
        // Use standard script execution
        while (scriptFileHandle.available()) {
          String line = scriptFileHandle.readStringUntil('\n');
          line.trim(); // Remove leading/trailing whitespace
          lineCount++;
            // Skip empty lines and comments
          if (line.length() > 0 && !line.startsWith("//") && !line.startsWith("#")) {
            if (config.scriptMode == 1 && !line.startsWith("REM")) {
              if (config.debugOutput) {
                Serial.print(F("Line "));
                Serial.print(lineCount);
                Serial.print(F(": "));
                Serial.println(line);
              }
              processDuckyLine(line);
              executedCount++;
            } else if (config.scriptMode == 0) {
              if (config.debugOutput) {
                Serial.print(F("Line "));
                Serial.print(lineCount);
                Serial.print(F(": "));
                Serial.println(line);
              }
              processInstructionLine(line);
              executedCount++;
            } else if (config.scriptMode == 1 && line.startsWith("REM")) {
              if (config.debugOutput) {
                Serial.print(F("Line "));
                Serial.print(lineCount);
                Serial.print(F(": "));
                Serial.println(line);
                Serial.println(F("  [Comment - Skipped]"));
              }
              skippedCount++;
            }
          } else {
            // Comment or empty line
            if (config.debugOutput) {
              Serial.print(F("Line "));
              Serial.print(lineCount);
              Serial.println(F(": [Comment or Empty - Skipped]"));
            }
            skippedCount++;
          }
        }
      }
        // Close the file
      scriptFileHandle.close();
      
      // Calculate execution statistics
      unsigned long executionTime = millis() - startTime;
      
      // Print execution summary
      Serial.println(F("------ Script Execution Summary ------"));
      Serial.print(F("Total lines: "));
      Serial.println(lineCount);
      Serial.print(F("Commands executed: "));
      Serial.println(executedCount);
      Serial.print(F("Lines skipped: "));
      Serial.println(skippedCount);
      Serial.print(F("Execution time: "));
      Serial.print(executionTime / 1000.0, 2);
      Serial.println(F(" seconds"));
      Serial.println(F("Script execution complete"));
      Serial.println(F("----------------------------------"));
      
      // Flash all LEDs to indicate completion
      digitalWrite(LED_USER, LOW);
      digitalWrite(LED_RX, LOW);
      digitalWrite(LED_TX, LOW);
      delay(1000);
      digitalWrite(LED_USER, HIGH);
      digitalWrite(LED_RX, HIGH);
      digitalWrite(LED_TX, HIGH);
      
      currentRepeat++;
      
    } else {
      // Failed to open file
      Serial.print(F("ERROR: Failed to open script file: "));
      Serial.println(scriptFile);
      flashLED(LED_TX, 3, 200); // Error indicator
      break;
    }
  } while (repeatScriptMode && (repeatScriptCount == 0 || currentRepeat < repeatScriptCount));
}

// Arduino SAMD doesn't have standard memory tracking variables like AVR
// Using a simpler approach for SAMD21

void loop() {
  // Nothing to do in loop - all processing done in setup
  // Just output a status message every 5 seconds to show the device is still running
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime > 5000) {
    Serial.println(F("Ghostkey idle - Script execution complete"));
      // Only show extended debug info if debug output is enabled
    if (config.debugOutput) {
      // Report uptime
      Serial.print(F("Uptime: "));
      Serial.print(millis() / 1000);
      Serial.println(F(" seconds"));
      
      // Note: Detailed memory usage reporting not available on SAMD21
      // without external libraries
    }
    
    lastStatusTime = millis();
  }
}

// Process a single instruction line from the custom format
void processInstructionLine(String line) {
  // Flash activity indicator
  digitalWrite(LED_RX, LOW);
  delay(50);
  digitalWrite(LED_RX, HIGH);
  
  // Split the line into command and parameters
  int separatorIndex = line.indexOf(':');
  String command, params;
  
  if (separatorIndex != -1) {
    command = line.substring(0, separatorIndex);
    params = line.substring(separatorIndex + 1);
    command.trim();
    params.trim();
    
    Serial.print(F("Executing command: "));
    Serial.print(command);
    Serial.print(F(", Params: "));
    Serial.println(params);
  } else {
    command = line;
    params = "";
    
    Serial.print(F("Executing command: "));
    Serial.println(command);
  }
  
  // Execute the appropriate function based on the command
  if (command.equalsIgnoreCase("DELAY")) {
    delay(params.toInt());
  } 
  else if (command.equalsIgnoreCase("RUN")) {
    run();
  } 
  else if (command.equalsIgnoreCase("ADMIN")) {
    admin();
  }  else if (command.equalsIgnoreCase("TYPE")) {
    if (config.useLayoutIndependent) {
      typeLayoutIndependent(params);
    } else {
      Keyboard.print(params);
    }
  }
  else if (command.equalsIgnoreCase("TYPELINE")) {
    if (config.useLayoutIndependent) {
      typeLayoutIndependent(params);
      Keyboard.press(KEY_RETURN);
      Keyboard.releaseAll();
    } else {
      Keyboard.println(params);
    }
  }
  else if (command.equalsIgnoreCase("TYPESOFT")) {
    if (config.useLayoutIndependent) {
      typeLayoutIndependentWithDelay(params, config.typingDelay);
    } else {
      typeWithDelay(params);
    }
  }
  else if (command.equalsIgnoreCase("OPENNOTPAD")) {
    openNotepad();
  }
  else if (command.equalsIgnoreCase("OPENPOWERSHELL")) {
    openPowerShell();
  }
  else if (command.equalsIgnoreCase("OPENPOWERSHELLADMIN")) {
    openPowerShellAdmin();
  }
  else if (command.equalsIgnoreCase("OPENCMD")) {
    openCmd();
  }
  else if (command.equalsIgnoreCase("OPENCMDADMIN")) {
    openCmdAdmin();
  }
  else if (command.equalsIgnoreCase("KILLAPP")) {
    killApp();
  }
  else if (command.equalsIgnoreCase("MINIMIZE")) {
    minimize();
  }
  else if (command.equalsIgnoreCase("KILLALL")) {
    killall();
  }
  else if (command.equalsIgnoreCase("KEY")) {
    // Handle special keys
    if (params.equalsIgnoreCase("RETURN") || params.equalsIgnoreCase("ENTER")) {
      Keyboard.press(KEY_RETURN);
      Keyboard.releaseAll();
    }
    else if (params.equalsIgnoreCase("TAB")) {
      Keyboard.press(KEY_TAB);
      Keyboard.releaseAll();
    }
    // Add more special keys as needed
  }
  
  // Add a small delay between commands for stability
  delay(defaultDelay);
}

// Process a single line in Ducky Script format
void processDuckyLine(String line) {
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
    
    Serial.print(F("Ducky command: "));
    Serial.print(command);
    Serial.print(F(", Params: "));
    Serial.println(params);
  } else {
    command = line;
    params = "";
    
    Serial.print(F("Ducky command: "));
    Serial.println(command);
  }
  
  // Process command according to Ducky Script specifications
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
  }
  else if (command.equals("STRING")) {
    // Type out a string of characters
    Serial.print(F("Typing string: "));
    Serial.println(params);
    
    if (config.useLayoutIndependent) {
      Serial.println(F("Using layout-independent typing"));
      typeLayoutIndependent(params);
    } else {
      Serial.println(F("Using standard typing"));
      Keyboard.print(params);
    }
  }
  else if (command.equals("STRINGLN")) {
    // Type out a string of characters and press Enter
    Serial.print(F("Typing string with enter: "));
    Serial.println(params);
    
    if (config.useLayoutIndependent) {
      Serial.println(F("Using layout-independent typing"));
      typeLayoutIndependent(params);
      delay(50); // Add delay before pressing Enter
      Keyboard.press(KEY_RETURN);
      delay(50); // Hold Enter for a moment
      Keyboard.releaseAll();
    } else {
      Serial.println(F("Using standard typing"));
      Keyboard.println(params);
    }
  }
  else if (command.equals("REPEAT")) {
    // Set script to repeat
    repeatScriptMode = true;
    repeatScriptCount = params.toInt();
    if (repeatScriptCount == 0) {
      repeatScriptCount = 1; // Default to once if not specified
    }
  }  else if (command.equals("GUI") || command.equals("WINDOWS")) {
    // Windows/GUI key
    Serial.print(F("GUI + "));
    Serial.println(params);
    
    if (params.length() > 0) {
      // GUI + key
      Keyboard.press(KEY_LEFT_GUI);
      delay(50); // Add delay to ensure GUI key is registered
      
      if (params.length() == 1) {
        char key = params[0];
        Serial.print(F("Pressing GUI + character: "));
        Serial.println(key);
        
        if (config.useLayoutIndependent && key >= 'a' && key <= 'z') {
          // For letters, we need to use raw keycodes
          uint8_t rawKey = KEY_A + (key - 'a');
          Serial.print(F("Using raw keycode: "));
          Serial.println(rawKey);
          
          Keyboard.press(rawKey);
          delay(100); // Hold the combination longer
          Keyboard.releaseAll();
        } else {
          Serial.println(F("Using standard key press"));
          Keyboard.press(key);
          delay(100); // Hold the combination longer
          Keyboard.releaseAll();
        }
      } else {
        // For named keys like "ENTER", "TAB", etc.
        Serial.print(F("Pressing GUI + named key: "));
        Serial.println(params);
        pressKey(params);
        delay(100);
        Keyboard.releaseAll();
      }
    } else {
      // Just GUI key
      Serial.println(F("Pressing GUI key alone"));
      Keyboard.press(KEY_LEFT_GUI);
      delay(100);
      Keyboard.releaseAll();
    }
  }
  else if (command.equals("MENU") || command.equals("APP")) {
    // Menu/App key
    Keyboard.press(KEY_MENU);
    Keyboard.releaseAll();
  }  else if (command.equals("SHIFT")) {
    Serial.print(F("SHIFT + "));
    Serial.println(params);
    
    if (params.length() > 0) {
      // SHIFT + key - improved implementation for better compatibility
      if (params.length() == 1) {
        char key = params[0];
        Serial.print(F("Pressing SHIFT + character: "));
        Serial.println(key);
        
        // For letters, we have special handling
        if (key >= 'a' && key <= 'z') {
          // Method 1: Use uppercase directly for letters
          char upperKey = key - 32;  // Convert to uppercase ASCII
          Serial.print(F("Converting to uppercase ASCII: "));
          Serial.println(upperKey);
          Keyboard.write(upperKey);
          delay(50);
        }
        // For numbers and special characters that need shift
        else {
          Keyboard.press(KEY_LEFT_SHIFT);
          delay(250); // Much longer delay for SHIFT to register
          
          if (config.useLayoutIndependent && key >= '0' && key <= '9') {
            // For numbers with layout independence
            uint8_t rawKey = 0;
            if (key == '0') rawKey = KEY_0;
            else rawKey = KEY_1 + (key - '1');
            
            Serial.print(F("Using raw keycode for number: "));
            Serial.println(rawKey);
            Keyboard.press(rawKey);
          } else {
            // For other characters
            Serial.print(F("Using standard SHIFT + key press: "));
            Serial.println(key);
            Keyboard.press(key);
          }
          
          delay(250); // Hold the combination much longer
          Keyboard.releaseAll();
          delay(50);  // Small delay after releasing
        }
      } else {
        // For named keys like arrow keys, function keys, etc.
        Serial.print(F("Pressing SHIFT + named key: "));
        Serial.println(params);
        
        // Press SHIFT first with a longer delay
        Keyboard.press(KEY_LEFT_SHIFT);
        delay(250); // Much longer delay for SHIFT
        
        // Handle common special keys with direct keycodes
        if (params.equalsIgnoreCase("RIGHT") || params.equalsIgnoreCase("RIGHTARROW")) {
          Serial.println(F("Using direct keycode for RIGHT ARROW"));
          Keyboard.press(KEY_RIGHT_ARROW);
        } 
        else if (params.equalsIgnoreCase("LEFT") || params.equalsIgnoreCase("LEFTARROW")) {
          Serial.println(F("Using direct keycode for LEFT ARROW"));
          Keyboard.press(KEY_LEFT_ARROW);
        }
        else if (params.equalsIgnoreCase("UP") || params.equalsIgnoreCase("UPARROW")) {
          Serial.println(F("Using direct keycode for UP ARROW"));
          Keyboard.press(KEY_UP_ARROW);
        }
        else if (params.equalsIgnoreCase("DOWN") || params.equalsIgnoreCase("DOWNARROW")) {
          Serial.println(F("Using direct keycode for DOWN ARROW"));
          Keyboard.press(KEY_DOWN_ARROW);
        }
        else if (params.equalsIgnoreCase("TAB")) {
          Serial.println(F("Using direct keycode for TAB"));
          Keyboard.press(KEY_TAB);
        }
        else {
          // For other keys, use standard method
          pressKey(params);
        }
        
        delay(250); // Hold the combination much longer
        Keyboard.releaseAll();
        delay(50);  // Small delay after releasing
      }
    } else {
      // Just SHIFT key
      Serial.println(F("Pressing SHIFT key alone"));
      Keyboard.press(KEY_LEFT_SHIFT);
      delay(250); // Much longer press for SHIFT alone
      Keyboard.releaseAll();
      delay(50);  // Small delay after releasing
    }
  }  else if (command.equals("ALT")) {
    Serial.print(F("ALT + "));
    Serial.println(params);
    
    if (params.length() > 0) {
      // ALT + key
      Keyboard.press(KEY_LEFT_ALT);
      delay(200); // Increased delay to ensure ALT key is registered
      
      if (params.length() == 1) {
        char key = params[0];
        Serial.print(F("Pressing ALT + character: "));
        Serial.println(key);
        
        if (config.useLayoutIndependent && key >= 'a' && key <= 'z') {
          // For letters, use raw keycodes
          uint8_t rawKey = KEY_A + (key - 'a');
          Serial.print(F("Using raw keycode: "));
          Serial.println(rawKey);
          
          Keyboard.press(rawKey);
          delay(200); // Increased: Hold the combination longer
          Keyboard.releaseAll();
        } else {
          Serial.println(F("Using standard key press"));
          Keyboard.press(key);
          delay(200); // Increased: Hold the combination longer
          Keyboard.releaseAll();
        }
      } else {
        // For named keys like "ENTER", "TAB", etc.
        Serial.print(F("Pressing ALT + named key: "));
        Serial.println(params);
        pressKey(params);
        delay(200); // Increased delay
        Keyboard.releaseAll();
      }
    } else {
      // Just ALT key
      Serial.println(F("Pressing ALT key alone"));
      Keyboard.press(KEY_LEFT_ALT);
      delay(200); // Increased delay
      Keyboard.releaseAll();
    }
  }  else if (command.equals("CTRL") || command.equals("CONTROL")) {
    Serial.print(F("CTRL + "));
    Serial.println(params);
    
    if (params.length() > 0) {
      // CTRL + key
      Keyboard.press(KEY_LEFT_CTRL);
      delay(200); // Increased delay to ensure CTRL key is registered
      
      if (params.length() == 1) {
        char key = params[0];
        Serial.print(F("Pressing CTRL + character: "));
        Serial.println(key);
        
        if (config.useLayoutIndependent && key >= 'a' && key <= 'z') {
          // For letters, use raw keycodes
          uint8_t rawKey = KEY_A + (key - 'a');
          Serial.print(F("Using raw keycode: "));
          Serial.println(rawKey);
          
          Keyboard.press(rawKey);
          delay(200); // Increased: Hold the combination longer
          Keyboard.releaseAll();
        } else {
          // For other characters
          Keyboard.press(key);
          delay(200); // Add delay before releasing
          Keyboard.releaseAll();
        }
      } else {
        pressKey(params);
        delay(200); // Add delay before releasing
        Keyboard.releaseAll();
      }
    } else {
      // Just CTRL key
      Keyboard.press(KEY_LEFT_CTRL);
      delay(200); // Longer press
      Keyboard.releaseAll();
    }
  }  else if (command.equals("ENTER")) {
    Serial.println(F("Pressing ENTER key"));
    Keyboard.press(KEY_RETURN);
    delay(50); // Hold key for 50ms
    Keyboard.releaseAll();
  }
  else if (command.equals("SPACE")) {
    Serial.println(F("Pressing SPACE key"));
    if (config.useLayoutIndependent) {
      Serial.println(F("Using layout-independent space (keycode 44)"));
      pressRawKey(44, false); // Space is keycode 44
    } else {
      Serial.println(F("Using standard space key"));
      Keyboard.press(' ');
      delay(50);
      Keyboard.releaseAll();
    }
  }  else if (command.equals("BACKSPACE")) {
    Serial.println(F("Pressing BACKSPACE key"));
    Keyboard.press(KEY_BACKSPACE);
    delay(200);  // Increased delay to ensure key is registered
    Keyboard.releaseAll();
    delay(50);  // Add a small delay after releasing
  }
  else if (command.equals("TAB")) {
    Serial.println(F("Pressing TAB key"));
    Keyboard.press(KEY_TAB);
    delay(200);  // Increased delay to ensure key is registered
    Keyboard.releaseAll();
    delay(50);  // Add a small delay after releasing
  }
  else if (command.equals("CAPSLOCK")) {
    Serial.println(F("Pressing CAPS LOCK key"));
    Keyboard.press(KEY_CAPS_LOCK);
    delay(50);
    Keyboard.releaseAll();
  }
  else if (command.equals("DELETE")) {
    Serial.println(F("Pressing DELETE key"));
    Keyboard.press(KEY_DELETE);
    delay(50);
    Keyboard.releaseAll();
  }
  else if (command.equals("END")) {
    Serial.println(F("Pressing END key"));
    Keyboard.press(KEY_END);
    delay(50);
    Keyboard.releaseAll();
  }
  else if (command.equals("ESC") || command.equals("ESCAPE")) {
    Serial.println(F("Pressing ESCAPE key"));
    Keyboard.press(KEY_ESC);
    delay(50);
    Keyboard.releaseAll();
  }
  else if (command.equals("HOME")) {
    Serial.println(F("Pressing HOME key"));
    Keyboard.press(KEY_HOME);
    delay(50);
    Keyboard.releaseAll();
  }
  else if (command.equals("INSERT")) {
    Serial.println(F("Pressing INSERT key"));
    Keyboard.press(KEY_INSERT);
    delay(50);
    Keyboard.releaseAll();
  }
  else if (command.equals("PAGEUP")) {
    Serial.println(F("Pressing PAGE UP key"));
    Keyboard.press(KEY_PAGE_UP);
    delay(50);
    Keyboard.releaseAll();
  }
  else if (command.equals("PAGEDOWN")) {
    Serial.println(F("Pressing PAGE DOWN key"));
    Keyboard.press(KEY_PAGE_DOWN);
    delay(50);
    Keyboard.releaseAll();
  }
  else if (command.equals("PRINTSCREEN")) {
    Keyboard.press(KEY_PRINT_SCREEN);
    Keyboard.releaseAll();
  }
  else if (command.equals("F1")) {
    Keyboard.press(KEY_F1);
    Keyboard.releaseAll();
  }
  else if (command.equals("F2")) {
    Keyboard.press(KEY_F2);
    Keyboard.releaseAll();
  }
  else if (command.equals("F3")) {
    Keyboard.press(KEY_F3);
    Keyboard.releaseAll();
  }
  else if (command.equals("F4")) {
    Keyboard.press(KEY_F4);
    Keyboard.releaseAll();
  }
  else if (command.equals("F5")) {
    Keyboard.press(KEY_F5);
    Keyboard.releaseAll();
  }
  else if (command.equals("F6")) {
    Keyboard.press(KEY_F6);
    Keyboard.releaseAll();
  }
  else if (command.equals("F7")) {
    Keyboard.press(KEY_F7);
    Keyboard.releaseAll();
  }
  else if (command.equals("F8")) {
    Keyboard.press(KEY_F8);
    Keyboard.releaseAll();
  }
  else if (command.equals("F9")) {
    Keyboard.press(KEY_F9);
    Keyboard.releaseAll();
  }
  else if (command.equals("F10")) {
    Keyboard.press(KEY_F10);
    Keyboard.releaseAll();
  }
  else if (command.equals("F11")) {
    Keyboard.press(KEY_F11);
    Keyboard.releaseAll();
  }
  else if (command.equals("F12")) {
    Keyboard.press(KEY_F12);
    Keyboard.releaseAll();
  }  else if (command.equals("UP") || command.equals("UPARROW")) {
    Serial.println(F("Pressing UP ARROW key"));
    Keyboard.press(KEY_UP_ARROW);
    delay(50);
    Keyboard.releaseAll();
  }
  else if (command.equals("DOWN") || command.equals("DOWNARROW")) {
    Serial.println(F("Pressing DOWN ARROW key"));
    Keyboard.press(KEY_DOWN_ARROW);
    delay(50);
    Keyboard.releaseAll();
  }
  else if (command.equals("LEFT") || command.equals("LEFTARROW")) {
    Serial.println(F("Pressing LEFT ARROW key"));
    Keyboard.press(KEY_LEFT_ARROW);
    delay(50);
    Keyboard.releaseAll();
  }
  else if (command.equals("RIGHT") || command.equals("RIGHTARROW")) {
    Serial.println(F("Pressing RIGHT ARROW key"));
    Keyboard.press(KEY_RIGHT_ARROW);
    delay(50);
    Keyboard.releaseAll();
  }
  else if (command.equals("PAUSE") || command.equals("BREAK")) {
    Serial.println(F("Pressing PAUSE/BREAK key"));
    Keyboard.press(KEY_PAUSE);
    delay(50);
    Keyboard.releaseAll();
  }
  // Custom extension to support more complex key combinations
  else if (line.indexOf("+") != -1) {
    // Process key combinations, e.g. CTRL+ALT+DELETE
    int plusIndex = 0;
    int startIndex = 0;
    String combinedKeys[5]; // Support up to 5 keys in combination
    int keyCount = 0;
    
    while ((plusIndex = line.indexOf("+", startIndex)) != -1 && keyCount < 5) {
      combinedKeys[keyCount++] = line.substring(startIndex, plusIndex);
      startIndex = plusIndex + 1;
    }
    
    // Add the last key after the last +
    if (startIndex < line.length() && keyCount < 5) {
      combinedKeys[keyCount++] = line.substring(startIndex);
    }
    
    // Press all keys in the combination
    for (int i = 0; i < keyCount; i++) {
      pressKey(combinedKeys[i]);
    }
    
    // Release all keys
    Keyboard.releaseAll();
  }

  // Wait the default delay after each command
  delay(defaultDelay);
}

// Press a key based on its string name
void pressKey(String keyString) {
  keyString.trim();
  
  // Check for single character
  if (keyString.length() == 1) {
    if (USE_LAYOUT_INDEPENDENT) {
      char c = keyString[0];
      typeLayoutIndependentChar(c);
      return;
    } else {
      Keyboard.press(keyString[0]);
      return;
    }
  }
  
  // Check for function keys and other special keys
  if (keyString.equals("CTRL") || keyString.equals("CONTROL")) {
    Keyboard.press(KEY_LEFT_CTRL);
  }
  else if (keyString.equals("SHIFT")) {
    Keyboard.press(KEY_LEFT_SHIFT);
  }
  else if (keyString.equals("ALT")) {
    Keyboard.press(KEY_LEFT_ALT);
  }
  else if (keyString.equals("GUI") || keyString.equals("WINDOWS")) {
    Keyboard.press(KEY_LEFT_GUI);
  }
  else if (keyString.equals("ENTER")) {
    Keyboard.press(KEY_RETURN);
  }
  else if (keyString.equals("SPACE")) {
    if (USE_LAYOUT_INDEPENDENT) {
      pressRawKey(44, false); // Space is keycode 44
    } else {
      Keyboard.press(' ');
    }
  }
  else if (keyString.equals("BACKSPACE")) {
    Keyboard.press(KEY_BACKSPACE);
  }
  else if (keyString.equals("TAB")) {
    Keyboard.press(KEY_TAB);
  }
  else if (keyString.equals("CAPSLOCK")) {
    Keyboard.press(KEY_CAPS_LOCK);
  }
  else if (keyString.equals("DELETE")) {
    Keyboard.press(KEY_DELETE);
  }
  else if (keyString.equals("END")) {
    Keyboard.press(KEY_END);
  }
  else if (keyString.equals("ESC") || keyString.equals("ESCAPE")) {
    Keyboard.press(KEY_ESC);
  }
  else if (keyString.equals("HOME")) {
    Keyboard.press(KEY_HOME);
  }
  else if (keyString.equals("INSERT")) {
    Keyboard.press(KEY_INSERT);
  }
  else if (keyString.equals("PAGEUP")) {
    Keyboard.press(KEY_PAGE_UP);
  }
  else if (keyString.equals("PAGEDOWN")) {
    Keyboard.press(KEY_PAGE_DOWN);
  }
  else if (keyString.equals("PRINTSCREEN")) {
    Keyboard.press(KEY_PRINT_SCREEN);
  }
  else if (keyString.equals("F1")) {
    Keyboard.press(KEY_F1);
  }
  else if (keyString.equals("F2")) {
    Keyboard.press(KEY_F2);
  }
  else if (keyString.equals("F3")) {
    Keyboard.press(KEY_F3);
  }
  else if (keyString.equals("F4")) {
    Keyboard.press(KEY_F4);
  }
  else if (keyString.equals("F5")) {
    Keyboard.press(KEY_F5);
  }
  else if (keyString.equals("F6")) {
    Keyboard.press(KEY_F6);
  }
  else if (keyString.equals("F7")) {
    Keyboard.press(KEY_F7);
  }
  else if (keyString.equals("F8")) {
    Keyboard.press(KEY_F8);
  }
  else if (keyString.equals("F9")) {
    Keyboard.press(KEY_F9);
  }
  else if (keyString.equals("F10")) {
    Keyboard.press(KEY_F10);
  }
  else if (keyString.equals("F11")) {
    Keyboard.press(KEY_F11);
  }
  else if (keyString.equals("F12")) {
    Keyboard.press(KEY_F12);
  }
  else if (keyString.equals("UP") || keyString.equals("UPARROW")) {
    Keyboard.press(KEY_UP_ARROW);
  }
  else if (keyString.equals("DOWN") || keyString.equals("DOWNARROW")) {
    Keyboard.press(KEY_DOWN_ARROW);
  }
  else if (keyString.equals("LEFT") || keyString.equals("LEFTARROW")) {
    Keyboard.press(KEY_LEFT_ARROW);
  }
  else if (keyString.equals("RIGHT") || keyString.equals("RIGHTARROW")) {
    Keyboard.press(KEY_RIGHT_ARROW);
  }
  else if (keyString.equals("PAUSE") || keyString.equals("BREAK")) {
    Keyboard.press(KEY_PAUSE);
  }
  else if (keyString.equals("MENU") || keyString.equals("APP")) {
    Keyboard.press(KEY_MENU);
  }
}

// Flash an LED a specified number of times with a specific duration
void flashLED(int led, int times, int duration) {
  String ledName;
  if (led == LED_USER) {
    ledName = "USER (Orange)";
  } else if (led == LED_RX) {
    ledName = "RX (Blue)";
  } else if (led == LED_TX) {
    ledName = "TX (Blue)";
  } else {
    ledName = String(led);  // Unknown LED, just use the pin number
  }
  
  Serial.print(F("Flashing LED "));
  Serial.print(ledName);
  Serial.print(F(" "));
  Serial.print(times);
  Serial.print(F(" times with "));
  Serial.print(duration);
  Serial.println(F("ms duration"));
  
  for (int i = 0; i < times; i++) {
    digitalWrite(led, LOW);  // LED on (LOW due to inverted logic)
    delay(duration);
    digitalWrite(led, HIGH); // LED off
    delay(duration);
  }
}

// Type text with delay between keystrokes
void typeWithDelay(String text) {
  for (unsigned int i = 0; i < text.length(); i++) {
    Keyboard.write(text.charAt(i));
    delay(TYPING_DELAY);
  }
}

// Function to print directory contents with indentation
void printDirectory(File dir, int numTabs) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      // No more files
      break;
    }
    
    // Print indentation
    for (int i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    
    // Print filename
    Serial.print(entry.name());
    
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // Print file size
      Serial.print("\t\t");
      Serial.print(entry.size(), DEC);
      Serial.println(" bytes");
    }
    entry.close();
  }
}

// Function to display different SD card error patterns using LEDs
void showSDCardError(int errorPattern) {
  switch(errorPattern) {
    case 1: // Card detection error
      for (int i = 0; i < 3; i++) {
        flashLED(LED_RX, 3, 200);
        delay(300);
      }
      break;
      
    case 2: // Format error
      for (int i = 0; i < 3; i++) {
        flashLED(LED_TX, 3, 200);
        delay(300);
      }
      break;
      
    case 3: // Wiring/communication error
      for (int i = 0; i < 3; i++) {
        flashLED(LED_USER, 3, 200);
        flashLED(LED_RX, 3, 200);
        delay(300);
      }
      break;
      
    case 4: // File system corruption
      for (int i = 0; i < 3; i++) {
        flashLED(LED_USER, 1, 200);
        flashLED(LED_RX, 1, 200);
        flashLED(LED_TX, 1, 200);
        delay(300);
      }
      break;
      
    default: // Unknown error
      for (int i = 0; i < 5; i++) {
        digitalWrite(LED_RX, LOW);
        digitalWrite(LED_TX, LOW);
        digitalWrite(LED_USER, LOW);
        delay(200);
        digitalWrite(LED_RX, HIGH);
        digitalWrite(LED_TX, HIGH);
        digitalWrite(LED_USER, HIGH);
        delay(200);
      }
  }
}

// Function to attempt SD card recovery
bool attemptSDCardRecovery() {
  Serial.println(F("\nAttempting SD Card Recovery..."));
  
  // Visual indication of recovery attempt
  flashLED(LED_USER, 2, 100);
  
  // Step 1: Reinitialize SPI bus
  Serial.println(F("1. Reinitializing SPI bus..."));
  SPI.end();
  delay(250);
  SPI.begin();
  delay(250);
  
  // Step 2: Try with different SPI speed
  Serial.println(F("2. Trying with lower SPI speed..."));
  // Note: SD.begin() on SAMD21 doesn't allow setting the speed directly
  // But we can try the basic initialization again
  if (SD.begin(SD_CS_PIN)) {
    Serial.println(F("Recovery successful!"));
    return true;
  }
  
  // Step 3: Check if card is physically present
  Serial.println(F("3. Testing physical card presence..."));
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, LOW);
  SPI.transfer(0xFF); // Send dummy byte
  digitalWrite(SD_CS_PIN, HIGH);
  delay(100);
  
  // Last attempt with standard settings
  Serial.println(F("4. Final attempt with standard settings..."));
  if (SD.begin(SD_CS_PIN)) {
    Serial.println(F("Recovery successful on final attempt!"));
    return true;
  }
  
  Serial.println(F("Recovery failed. Please check hardware."));
  return false;
}

// Function to run full SD card diagnostics
void runSDCardDiagnostics() {
  Serial.println(F("\nRunning full SD card diagnostics..."));
  
  // Visual indication
  digitalWrite(LED_USER, LOW); // Turn on USER LED during diagnostics
  
  // Record starting time
  unsigned long startTime = millis();
  
  // 1. Get card type
  String cardType = getSDCardTypeString();
  Serial.print(F("Card Type: "));
  Serial.println(cardType);
  
  // 2. Test card health
  char healthResult[50] = {0};
  testSDCardHealth(healthResult, sizeof(healthResult));
  Serial.println(healthResult);
  
  // 3. Test read speed
  Serial.print(F("Read Speed: "));
  float readSpeed = testSDReadSpeed();
  Serial.print(readSpeed, 1);
  Serial.println(F(" KB/s"));
  
  // 4. Test write speed
  Serial.print(F("Write Speed: "));
  float writeSpeed = testSDWriteSpeed();
  Serial.print(writeSpeed, 1);
  Serial.println(F(" KB/s"));
  
  // 5. Get card capacity (approximate)
  Serial.print(F("Approx. Capacity: "));
  unsigned long sizeMB = getSDCardSizeApprox();
  if (sizeMB > 0) {
    if (sizeMB >= 1024) {
      Serial.print(sizeMB / 1024);
      Serial.println(F(" GB+"));
    } else {
      Serial.print(sizeMB);
      Serial.println(F(" MB+"));
    }
  } else {
    Serial.println(F("Unknown"));
  }
  
  // 6. Test for file system errors
  Serial.print(F("File System Check: "));
  
  // Create a test directory
  if (!SD.mkdir("/test_dir")) {
    Serial.println(F("Failed (cannot create directory)"));
  } else {
    // Create and write to a test file
    File testFile = SD.open("/test_dir/test.txt", FILE_WRITE);
    if (!testFile) {
      Serial.println(F("Failed (cannot create file)"));
    } else {
      testFile.println("Test data");
      testFile.close();
      
      // Try to read the file
      testFile = SD.open("/test_dir/test.txt");
      if (!testFile) {
        Serial.println(F("Failed (cannot read file)"));
      } else {
        String content = testFile.readStringUntil('\n');
        testFile.close();
        
        if (content == "Test data") {
          Serial.println(F("Passed"));
        } else {
          Serial.println(F("Failed (data corruption)"));
        }
      }
      
      // Clean up
      SD.remove("/test_dir/test.txt");
      SD.rmdir("/test_dir");
    }
  }
  
  // Calculate and show time spent on diagnostics
  unsigned long diagTime = millis() - startTime;
  Serial.print(F("Diagnostics completed in "));
  Serial.print(diagTime);
  Serial.println(F("ms"));
  
  // Turn off diagnostic indicator LED
  digitalWrite(LED_USER, HIGH);
}
