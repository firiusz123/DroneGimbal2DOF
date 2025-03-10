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
void commandChecker(void *pvParameters) {
    while (1) {
        //checking if data is prepared 
        if (xSemaphoreTake(dataSemaphore, portMAX_DELAY) == pdTRUE) {
            Serial.print("Received: ");
            Serial.println(receivedData);
              // array for command 
                char command[50];
                //copying data 
                strncpy(command, receivedData + 1, strlen(receivedData) - 2);
                command[strlen(receivedData) - 2] = '\0';
                
                //getting the tokens by splting by # 
                char *token = strtok(command, "#");
                char *commandType = token;
                float values[2];
                int valueIndex = 0;
                //extracting values                 
                while ((token = strtok(NULL, "#")) != NULL && valueIndex < 2) {
                    values[valueIndex++] = atof(token);
                }
            
                Serial.print("Command: ");
                Serial.println(commandType);
                
                for (int i = 0; i < valueIndex; i++) {
                    Serial.print("Value ");
                    Serial.print(i + 1);
                    Serial.print(": ");
                    Serial.println(values[i]);
                }
            

            receivedData = "";
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(9600); 

    // Create the semaphore
    dataSemaphore = xSemaphoreCreateBinary();

    // Create the read and write tasks
    xTaskCreate(readSerialTask, "ReadSerialTask", 128, NULL, 1, NULL);
    xTaskCreate(commandChecker, "commandChecker", 128, NULL, 1, NULL);
}

void loop() {
    // FreeRTOS handles tasks, so no need for code in loop()
}

