/*
 * SD Card Diagnostics Library for Ghostkey
 * 
 * This provides functions for testing SD card functionality
 * and collecting diagnostic information.
 */

#ifndef SD_DIAGNOSTICS_H
#define SD_DIAGNOSTICS_H

#include <SPI.h>
#include <SD.h>

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

#endif // SD_DIAGNOSTICS_H
