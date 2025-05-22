#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <Servo.h>

// Servo control
Servo servoRoll;
Servo servoPitch;
Servo servoYaw;

const int SERVO_ROLL_PIN = 9;
const int SERVO_PITCH_PIN = 10;
const int SERVO_YAW_PIN = 11;

// PID parameters (tune individually per axis)
float Kp[3] = {1.5, 1.5, 1.5};
float Ki[3] = {0.0, 0.0, 0.0};
float Kd[3] = {0.1, 0.1, 0.1};

// PID state
float errorSum[3] = {0, 0, 0};
float lastError[3] = {0, 0, 0};

// Angles: 0 = Roll, 1 = Pitch, 2 = Yaw
float measuredAngles[3] = {0, 0, 0};
float desiredAngles[3]  = {0, 0, 0};  // Set externally (e.g. via Serial)

unsigned long lastTime = 0;



/// defines 

#define PI  3.14159 
#define START_COMMAND_CHAR ':'
#define SEPARATE_VALUE_CHAR "|"


/////////////////// variables setting 
String receivedData;  // Buffer to store received data
SemaphoreHandle_t dataSemaphore;  // Semaphore for data synchronization
bool SpecialCharacterCapturing = false;
#define MAX_VALUES 3 




sensors_event_t event; 
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
///////////////////////////////////////
//sensor data config 
void displayRange(void)
{
  Serial.print  ("Range:         +/- "); 
  
  switch(accel.getRange())
  {
    case ADXL345_RANGE_16_G:
      Serial.print  ("16 "); 
      break;
    case ADXL345_RANGE_8_G:
      Serial.print  ("8 "); 
      break;
    case ADXL345_RANGE_4_G:
      Serial.print  ("4 "); 
      break;
    case ADXL345_RANGE_2_G:
      Serial.print  ("2 "); 
      break;
    default:
      Serial.print  ("?? "); 
      break;
  }  
  Serial.println(" g");  
}

// Task to read serial data
void readSerialTask(void *pvParameters) {
    while (1) {
      
        while (Serial.available()) {

            char c = Serial.read();

            if(c == START_COMMAND_CHAR){
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
        if (xSemaphoreTake(dataSemaphore, portMAX_DELAY) == pdTRUE) {
            Serial.print("Received: ");
            Serial.println(receivedData);

            if (receivedData.length() > 2) {
                // Trim leading and trailing delimiters
                String trimmed = receivedData.substring(0, receivedData.length() - 1);

                char command[50];
                trimmed.toCharArray(command, sizeof(command));

                char *token = strtok(command, SEPARATE_VALUE_CHAR);
                char *commandType = token;

                int valueIndex = 0;
                while ((token = strtok(NULL, SEPARATE_VALUE_CHAR)) != NULL && valueIndex < MAX_VALUES) {
                    desiredAngles[valueIndex++] = atof(token);
                }

                Serial.print("Command: ");
                Serial.println(commandType);

                for (int i = 0; i < valueIndex; i++) {
                    Serial.print("Value ");
                    Serial.print(i + 1);
                    Serial.print(": ");
                    Serial.println(desiredAngles[i]);
                }
            } else {
                Serial.println("Received data too short.");
            }

            receivedData = "";
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void control_Loop(void *pvParameters)
{
    float acc_measure[3];
    float pidOutput[3], error[3], dError[3];
    float dt;

    while (1)
    {
        // Timing
        unsigned long now = millis();
        dt = (now - lastTime) / 1000.0; // in seconds
        lastTime = now;

        // Get acceleration data
        accel.getEvent(&event);
        acc_measure[0] = event.acceleration.x;
        acc_measure[1] = event.acceleration.y;
        acc_measure[2] = event.acceleration.z;

        // Compute angles
        measuredAngles[0] = atan2(acc_measure[1], sqrt(acc_measure[0]*acc_measure[0] + acc_measure[2]*acc_measure[2])) * 180.0 / PI;  // Roll
        measuredAngles[1] = atan2(-acc_measure[0], sqrt(acc_measure[1]*acc_measure[1] + acc_measure[2]*acc_measure[2])) * 180.0 / PI;  // Pitch
        measuredAngles[2] = atan2(acc_measure[2], sqrt(acc_measure[2]*acc_measure[2] + acc_measure[0]*acc_measure[0])) * 180.0 / PI;  // Yaw 

        // PID control for Roll, Pitch, Yaw
        for (int i = 0; i < 3; i++) {
            error[i] = desiredAngles[i] - measuredAngles[i];
            errorSum[i] += error[i] * dt;
            dError[i] = (error[i] - lastError[i]) / dt;

            pidOutput[i] = Kp[i] * error[i] + Ki[i] * errorSum[i] + Kd[i] * dError[i];
            lastError[i] = error[i];
        }

        // Convert to servo angles
        int rollAngle  = constrain(90 + pidOutput[0], 0, 180);
        int pitchAngle = constrain(90 + pidOutput[1], 0, 180);
        int yawAngle   = constrain(90 + pidOutput[2], 0, 180);

        // Output to servos
        servoRoll.write(rollAngle);
        servoPitch.write(pitchAngle);
        servoYaw.write(yawAngle);

        // Optional: debug
        Serial.print("dt: "); Serial.print(dt, 4);
        Serial.print(" Roll: "); Serial.print(measuredAngles[0], 1);
        Serial.print(" Pitch: "); Serial.print(measuredAngles[1], 1);
        Serial.print(" Yaw: "); Serial.println(measuredAngles[2], 1);

        vTaskDelay(10 / portTICK_PERIOD_MS); // ~100Hz
    }
}


void displayOrientationTask(void *pvParameters) {
  while(1) {
    Serial.print("Roll: "); Serial.print(measuredAngles[0]);
    Serial.print(" | Pitch: "); Serial.print(measuredAngles[1]);
    Serial.print(" | Yaw: "); Serial.println(measuredAngles[2]);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}


void setup() {
    Serial.begin(9600); 
    pinMode(2, OUTPUT);
    servoRoll.attach(SERVO_ROLL_PIN);
    servoPitch.attach(SERVO_PITCH_PIN);
    servoYaw.attach(SERVO_YAW_PIN);
    lastTime = millis();
    
    //checking the sensor 
    //if(!accel.begin())
    //{
    // Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    //  while(1);
    //}
    //accel.setRange(ADXL345_RANGE_16_G);
    // Create the semaphore
    dataSemaphore = xSemaphoreCreateBinary();
    

    // Create the read and write tasks
    xTaskCreate(readSerialTask, "ReadSerialTask", 128, NULL, 1, NULL);
    xTaskCreate(commandChecker, "commandChecker", 128, NULL, 1, NULL);
    xTaskCreate(control_Loop , "control Loop" , 256 ,NULL , 1 , NULL);
    //xTaskCreate(displayOrientationTask, "Display", 256, NULL, 1, NULL);

}

void loop() {
    // FreeRTOS handles tasks, so no need for code in loop()
}

