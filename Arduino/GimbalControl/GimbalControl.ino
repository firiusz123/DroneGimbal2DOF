#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>




/// defines 

#define PI  3.14159 ;


/////////////////// variables setting 
String receivedData;  // Buffer to store received data
SemaphoreHandle_t dataSemaphore;  // Semaphore for data synchronization
bool SpecialCharacterCapturing = false;
float desiredAngles[2] = {0 , 0} ;
float measuredAngles[2] = {0 , 0} ;
///////////////////////////////////////

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
                
                int valueIndex = 0;
                //extracting values                 
                while ((token = strtok(NULL, "#")) != NULL && valueIndex < 2) {
                    desiredAngles[valueIndex++] = atof(token);
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

void control_Loop(void *pvParameters)
{
    sensors_event_t event; 
    float acc_measure[3] = {0,0,0};
    float pitch , roll ;
    
    while(1)
    {
        accel.getEvent(&event);

        acc_measure[0] = event.acceleration.x ;
        acc_measure[1] = event.acceleration.y ;
        acc_measure[2] = event.acceleration.z ;
        

        // for now calculating just 2 because it is 2dof gimbal , might add 3rd degree later on
        measuredAngles[0] = atan2(acc_measure[1], sqrt(acc_measure[0] * acc_measure[0] + acc_measure[2] * acc_measure[2])) * 180.0 / PI;  // Roll
        measuredAngles[1] = atan2(-acc_measure[0], sqrt(acc_measure[1] * acc_measure[1] + acc_measure[2] * acc_measure[2])) * 180.0 / PI;  // Pitch

        

    }
}

void setup() {
    Serial.begin(9600); 
    
    Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
    //checking the sensor 
    if(!accel.begin())
    {
      Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
      while(1);
    }
    accel.setRange(ADXL345_RANGE_16_G);
    // Create the semaphore
    dataSemaphore = xSemaphoreCreateBinary();
    

    // Create the read and write tasks
    xTaskCreate(readSerialTask, "ReadSerialTask", 128, NULL, 1, NULL);
    xTaskCreate(commandChecker, "commandChecker", 128, NULL, 1, NULL);
    xTaskCreate(control_Loop , "control Loop" , 256 ,NULL , 1 , NULL);
}

void loop() {
    // FreeRTOS handles tasks, so no need for code in loop()
}

