#include <Arduino_FreeRTOS.h>
#include <semphr.h>

String receivedData;  // Buffer to store received data
SemaphoreHandle_t dataSemaphore;  // Semaphore for data synchronization
bool SpecialCharacterCapturing = false;

// Task to read serial data
void readSerialTask(void *pvParameters) {
    while (1) {
      
        while (Serial.available()) {

            char c = Serial.read();

            if(c == '$'){
              if(SpecialCharacterCapturing){
                //if already triggered it means it finished
                SpecialCharacterCapturing = false;
                xSemaphoreGive(dataSemaphore);
              }
              else{     
                //clearing data buffer and setting the character saving to true                           
                receivedData = "";
                SpecialCharacterCapturing = true;              
              }
            }
            else if(SpecialCharacterCapturing){
              receivedData += c;  
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);  // Small delay to avoid CPU overload
    }
}

// Task to write received serial data
void writeSerialTask(void *pvParameters) {
    while (1) {
        // waiting for semaphore to let us proceed with data 
        if (xSemaphoreTake(dataSemaphore, portMAX_DELAY) == pdTRUE) {
            
            Serial.print("Received: ");
            Serial.println(receivedData);
            receivedData = "";  // Clear the buffer after printing
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);  // Small delay
    }
}

void setup() {
    Serial.begin(9600); 

    // Create the semaphore
    dataSemaphore = xSemaphoreCreateBinary();

    // Create the read and write tasks
    xTaskCreate(readSerialTask, "ReadSerialTask", 128, NULL, 1, NULL);
    xTaskCreate(writeSerialTask, "WriteSerialTask", 128, NULL, 1, NULL);
}

void loop() {
    // FreeRTOS handles tasks, so no need for code in loop()
}

