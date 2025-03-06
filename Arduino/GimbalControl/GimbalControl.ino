#include <Arduino_FreeRTOS.h>
#include <semphr.h>

String receivedData = "";  // Buffer to store received data
SemaphoreHandle_t dataSemaphore;  // Semaphore for data synchronization
unsigned long timeoutStartTime = 0;  // Timer start time for timeout
bool timeoutFlag = false;  // Timeout flag
const unsigned long timeoutDuration = 2000;  // Timeout duration (2 seconds)

void readSerialTask(void *pvParameters) {
    while (1) {
        timeoutFlag = false;
        timeoutStartTime = millis();  // Start the timeout timer

        while (Serial.available()) {
            char c = Serial.read();
            receivedData += c;  // Append received char to buffer

            // Check if the timeout duration has passed
            if (millis() - timeoutStartTime > timeoutDuration) {
                timeoutFlag = true;  // Timeout occurred
                break;
            }
        }

        // If timeout occurred, print timeout message
        if (timeoutFlag) {
            Serial.println("Timeout got it!");
            receivedData = "";  // Clear received data
        }

        // If data has been received, give the semaphore to Task 2
        if (receivedData.length() > 0) {
            xSemaphoreGive(dataSemaphore);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);  // Small delay to avoid CPU overload
    }
}

// Task to write received serial data
void writeSerialTask(void *pvParameters) {
    while (1) {
        // Wait for the semaphore, indicating that data is ready
        if (xSemaphoreTake(dataSemaphore, portMAX_DELAY) == pdTRUE) {
            // Print the received data
            Serial.print("Received: ");
            Serial.println(receivedData);
            receivedData = "";  // Clear the buffer after printing
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);  // Small delay
    }
}

void setup() {
    Serial.begin(9600);  // Initialize serial communication

    // Create the semaphore
    dataSemaphore = xSemaphoreCreateBinary();

    // Create the read and write tasks
    xTaskCreate(readSerialTask, "ReadSerialTask", 128, NULL, 1, NULL);
    xTaskCreate(writeSerialTask, "WriteSerialTask", 128, NULL, 1, NULL);
}

void loop() {
    // FreeRTOS handles tasks, so no need for code in loop()
}


