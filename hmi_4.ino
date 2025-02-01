#include <Adafruit_GFX.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <MCUFRIEND_kbv.h>
#include <TimeLib.h>
#include <EEPROM.h>

// Create TFT LCD object
MCUFRIEND_kbv tft;
SemaphoreHandle_t displayMutex;

// Color definitions
#define BLACK   0x0000
#define BLUE    0x07FF  // Neon blue
#define RED     0xF81F  // Neon red
#define GREEN   0x07FF  // Neon green
#define CYAN    0x0FFF  // Neon cyan
#define MAGENTA 0xF81F  // Neon magenta
#define YELLOW  0xFFE0  // Neon yellow
#define WHITE   0xFFFF


// Global variables
float totalDistance = 0.0;
float trip1 = 0.0;
float trip2 = 0.0;
int speed = 0;
int temp = 0;
int batteryLevel = 0; // Initial battery level percentage
int Battery = 0;
int t =0;
int spd = 0;
int prevHour = -1;
int prevMinute = -1;
int prevSecond = -1;


// EEPROM read/write functions
 void writeIntIntoEEPROM(int address, float value)
{ 
  byte *bytePointer = (byte *)(void *)&value; // Convert the float to a byte pointer
    for (int i = 0; i< sizeof(float); i++) {
        EEPROM.write(address + i, bytePointer[i]); // Write each byte to EEPROM
    }
}

float readIntFromEEPROM(int address)
{
   float value;
    byte *bytePointer = (byte *)(void *)&value; // Convert the float to a byte pointer
    for (int i = 0; i < sizeof(float); i++) {
        bytePointer[i] = EEPROM.read(address + i); // Read each byte from EEPROM
    }
    return value;
  }

void setup() {
    Serial.begin(9600);
    uint16_t ID = tft.readID();
    if (ID == 0xD3D3) ID = 0x9486;
    tft.begin(ID);
    tft.setRotation(1);
    tft.fillScreen(BLACK);

    // Draw static parts of the display
    drawStaticElements();

    displayMutex = xSemaphoreCreateMutex();

    // Create tasks
    xTaskCreate(displayTask, "Display", 1000, NULL, 1, NULL);
    xTaskCreate(speedTask, "Speed", 1000, NULL, 1, NULL);
    xTaskCreate(temptask, "Data Update", 1000, NULL, 1, NULL);
    xTaskCreate(serialTask, "Serial", 1000, NULL, 1, NULL);
    xTaskCreate(updateDataTask, "Data Update", 1000, NULL, 1, NULL);
    xTaskCreate(TaskPrintTime, "PrintTime", 256, NULL, 1, NULL);



    // Start FreeRTOS scheduler
    vTaskStartScheduler();
}

void drawStaticElements() {
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    
    // Top indicators
    tft.fillRect(10, 10, 50, 50, GREEN); // Neutral indicator
    tft.setCursor(20, 20);
    tft.setTextColor(BLACK);
    tft.print("N");

    tft.setTextColor(WHITE);
    tft.drawRect(70, 10, 50, 50, WHITE); // Oil indicator
    tft.drawRect(130, 10, 50, 50, WHITE); // Turn indicator
    tft.drawRect(190, 10, 50, 50, WHITE); // High beam indicator

    // Left info boxes
    tft.drawRect(10, 70, 150, 60, WHITE); // MPG box
    tft.setCursor(20, 80);
    tft.print("MPG");
    tft.setCursor(90, 80);
    tft.print("0");

    tft.drawRect(10, 140, 150, 60, WHITE); // Range box
    tft.setCursor(20, 150);
    tft.print("RANGE");
    tft.setCursor(90, 150);
    tft.print("0");

    tft.drawRect(10, 210, 150, 60, WHITE); // Time box
    tft.setCursor(20, 220);
    tft.print("TIME");
    tft.setCursor(90, 220);
  //  tft.print("0:0");

    tft.drawRect(10, 280, 150, 60, WHITE); // Max speed box
    tft.setCursor(20, 290);
    tft.print("KC");
    tft.setCursor(90, 290);
    tft.print("0");

    // Bottom info boxes
    tft.setCursor(10, 350);
    tft.print("TRIP 1:");
    tft.setCursor(10, 380);
    tft.print("TRIP 2:");
    tft.setCursor(10, 410);
    tft.print("ODO:");
    tft.setCursor(10, 440);
    tft.print("TIME:");
    
    // Draw labels for dynamic data
    tft.setCursor(80, 350);
    tft.print("0.0");
    tft.setCursor(80, 380);
    tft.print("0.0");
    tft.setCursor(80, 410);
    tft.print("0.0");
    tft.setCursor(80, 440);
    tft.print("00:00");
    
    // Right speedometer area
    tft.drawRect(180, 90, 160, 160, WHITE); // Speed box
    tft.setTextSize(7);
    tft.setTextColor(BLUE);
    tft.setCursor(195, 110); // Adjust cursor to center text
  //  tft.print("0");

    tft.setTextSize(2);
    tft.setCursor(240, 210); // Position MPH label outside the box
    tft.setTextColor(WHITE);
    tft.print("KPH");
    
    // Draw temperature box
    tft.drawRect(180, 270, 100, 40, WHITE); // Temperature box
    tft.setCursor(190, 280);
    tft.print("77 C");

    // Draw fuel gauge box
    tft.drawRect(180, 320, 100, 20, WHITE); // Fuel gauge box
    tft.fillRect(180, 320, 60, 20, WHITE); // Fuel gauge level

    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(240, 10); // Adjusted position for the label
    tft.print("Battery");

    // Draw battery box outer layer and cap
    int batteryX = tft.width() - 120; // Adjusted position to be slightly to the left
    int batteryY = 20; // Adjusted position to be slightly lower
    tft.drawRect(batteryX, batteryY, 104, 40, WHITE); // Outer rectangle for battery box
    tft.fillRect(batteryX + 104, batteryY + 10, 10, 20, WHITE); // Battery cap
}

void displayTask(void *pvParameters) {
    (void) pvParameters;
   
    char buffer[10];

    while (1) {
        if (xSemaphoreTake(displayMutex, portMAX_DELAY) == pdTRUE) {
            // Update speed
         

            // Update trip1
            tft.fillRect(80, 350, 100, 20, BLACK); // Clear previous trip1
            tft.setTextSize(2);
            tft.setTextColor(WHITE, BLACK);
            tft.setCursor(80, 350);
            tft.print(trip1, 1);

            // Update trip2
            tft.fillRect(80, 380, 100, 20, BLACK); // Clear previous trip2
            tft.setTextSize(2);
            tft.setCursor(80, 380);
            tft.print(trip2, 1);

            // Update total distance
            tft.fillRect(80, 410, 100, 20, BLACK); // Clear previous total distance
            tft.setTextSize(2);
            tft.setCursor(80, 410);
            tft.print(totalDistance, 1);

            // Update temperature
            tft.fillRect(190, 280, 80, 20, BLACK); // Clear previous temperature
            tft.setTextSize(2);
            tft.setTextColor(WHITE, BLACK);
            tft.setCursor(190, 280);
            tft.print(t);
            tft.print(" C");

            int batteryX = tft.width() - 120; // Adjusted position to be slightly to the left
            int batteryY = 20; // Adjusted position to be slightly lower
            int segmentWidth = 20; // Width of each segment

            // Clear previous battery bar
            tft.fillRect(batteryX + 2, batteryY + 2, 100, 36, BLACK);

            // Draw battery segments
            int numSegments = map(batteryLevel, 0, 100, 0, 5);
            for (int i = 0; i < numSegments; i++) {
                int color = (batteryLevel < 15 && i == 0) ? RED : GREEN;
                tft.fillRect(batteryX + 2 + (i * segmentWidth), batteryY + 2, segmentWidth - 2, 36, color);
            }

            // Display battery percentage
            int textX = batteryX + 52 - 18; // Center the text horizontally
            int textY = batteryY + (36 - 12) / 2; // Center the text vertically
            tft.setTextSize(2);
            tft.setCursor(textX, textY);
            tft.setTextColor(WHITE);
            sprintf(buffer, "%d%%", batteryLevel);
            tft.print(buffer);

            xSemaphoreGive(displayMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // Update every 500 ms
    }
}

void speedTask(void *pvParameters) {
    (void) pvParameters;

    int prevSpeed = -1; // Initialize to an invalid speed value to ensure the first update occurs
    int spdDiff = 0;
    float totalDistance = 0;   // Variable to store the total distance traveled
    totalDistance = readIntFromEEPROM(45);


    while (1) {
        int spd = speed; // Read the current speed
         spdDiff += spd;

    // Calculate distance traveled since the last speed measurement
        float distance = (float)spd * 0.1 / 36000.0; // Assuming spd is in km/h and delay is 100ms (0.1s)

    // Add the distance to the total distance traveled
        totalDistance += distance;

    // Add the current speed difference to the accumulated difference

        Serial.println(spdDiff);


        if (spd != prevSpeed) { // Only update if the speed has changed
            if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                // Clear previous speed
                tft.fillRect(200, 130, 140, 70, BLACK);
                tft.setTextSize(7);
                tft.setTextColor(BLUE, BLACK);
                tft.setCursor(230, 130); // Adjust cursor to center text
                tft.print(spd);

                writeIntIntoEEPROM(45, totalDistance); // Write the updated value to EEPROM
                 tft.setCursor(100, 290);
                 tft.setTextSize(2);
                 tft.setTextColor(CYAN, BLACK);
                 tft.print(totalDistance,2);

                // Release the mutex
                xSemaphoreGive(displayMutex);
            }

            // Update the previous speed value
            prevSpeed = spd;
        }

        // Delay for 500 ms
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void temptask(void *pvParameters) {
    (void) pvParameters;

    while (1) {
        t = temp; // Generate random speed for example
        vTaskDelay(pdMS_TO_TICKS(500)); // Generate new values every second
    }
}


void updateDataTask(void *pvParameters) {
    (void) pvParameters;

    while (1) {
        batteryLevel = Battery; // Generate random battery level percentage
        vTaskDelay(pdMS_TO_TICKS(2000)); // Generate new values every 2 seconds
    }
}


void serialTask(void *pvParameters) {
    (void) pvParameters;

    while (1) {
        if (Serial.available() > 0) {
            String data = Serial.readStringUntil('\n');
            int s1, s2, s3;
            sscanf(data.c_str(), "%d,%d,%d", &s1, &s2, &s3);

            if (xSemaphoreTake(displayMutex, portMAX_DELAY) == pdTRUE) {
                speed = s1;
                batteryLevel = s2;
                temp = s3;
                xSemaphoreGive(displayMutex);
            }

            Serial.println("Data received: " + data);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay to avoid overwhelming the serial buffer
    }
}
void TaskPrintTime(void *pvParameters) {
  (void)pvParameters;

  while (1) {
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
      int currentHour = hour();
      int currentMinute = minute();
      int currentSecond = second();

      // Clear and update hour if it has changed
      if (currentHour != prevHour) {
        tft.fillRect(20, 245, 40, 20, BLACK); // Clear previous hour
        tft.setCursor(20, 245);
        tft.setTextSize(2);
        tft.setTextColor(WHITE);
        tft.print(currentHour <10 ? "0" : "");
        tft.print(currentHour);
        prevHour = currentHour;
      }

      // Clear and update minute if it has changed
      if (currentMinute != prevMinute) {
        tft.fillRect(50, 245, 40, 20, BLACK); // Clear previous minute
        tft.setCursor(50, 245);
        tft.setTextSize(2);
        tft.setTextColor(WHITE);
        tft.print(currentMinute <10 ? "0" : "");
        tft.print(currentMinute);
        prevMinute = currentMinute;
      }

      // Clear and update second if it has changed
      if (currentSecond != prevSecond) {
        tft.fillRect(80, 245, 40, 20, BLACK); // Clear previous second
        tft.setCursor(80, 245);
        tft.setTextSize(2);
        tft.setTextColor(WHITE);
        tft.print(currentSecond <10 ? "0" : "");
        tft.print(currentSecond);
        prevSecond = currentSecond;
      }

      xSemaphoreGive(displayMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay outside of the mutex-protected section
  }
}

void printDigits(int digits) {
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


void loop() {
    // Empty loop as FreeRTOS handles tasks
}
