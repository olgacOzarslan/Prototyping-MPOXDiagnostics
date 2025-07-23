#include "Arduino.h"
#include <EEPROM.h>

#define EEPROM_START_ADDR 0

#define THERMISTOR_PIN A0
#define FAN_PIN 6
#define UV_LED_PIN 3
#define HEATER_PIN 5

#define Kp_INDEX 0
#define Ki_INDEX 1
#define Kd_INDEX 2

#define MAX_BLOCK_TEMPERATURE 100  // Maximum block temperature in °C
#define MAX_LIQUID_TEMPERATURE 90  // Maximum liquid temperature in °C

int eepromAddresses[4];

float PID_P = 0;
float PID_I = 0;
float PID_D = 0;
float previous_error = 0;
float Kp = 20;
float Ki = 0.2;
float Kd = 20;
const int numReadings = 5;  // Adjust the number of readings based on your preference
float readings[numReadings];
bool stopFlag = false;
bool debugFlag = false;
float set_point = 85;

struct heater {
  float Rt;
  float R0 = 100000;
  uint8_t pwm_pin;
  uint8_t thermistor_pin;
  uint8_t led_pin;
  uint8_t fan_pin;
  void turnOn() {
    digitalWrite(pwm_pin, HIGH);
  }
  void turnOff() {
    digitalWrite(pwm_pin, LOW);
  }
  void ledOn() {
    digitalWrite(led_pin, HIGH);
  }
  void ledOff() {
    digitalWrite(led_pin, LOW);
  }
  void fanOn() {
    digitalWrite(fan_pin, HIGH);
  }
  void fanOff() {
    digitalWrite(fan_pin, LOW);
  }

  float get_temperature() {
    float beta_value = 3950;
    float reference_temperature = 25 + 273.15;

    int numSamples = 10;
    int adc_value = 0;

    for (int i = 0; i < numSamples; i++) {
      adc_value += analogRead(thermistor_pin);
      delay(8);
    }

    adc_value /= numSamples;
    float Vout = (adc_value * 5.0) / 1024;
    float Rth = (5 * 100000 / Vout) - 100000;
    float tempKelvin = pow((1.0 / beta_value) * log(Rth / 100000.0) + 1.0 / reference_temperature, -1);
    return tempKelvin - 273.15;
  }
};

heater _heater;

void setup() {
  Serial.begin(9600);

  pinMode(HEATER_PIN, OUTPUT);
  pinMode(THERMISTOR_PIN, INPUT);
  pinMode(UV_LED_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);

  _heater.pwm_pin = HEATER_PIN;
  _heater.thermistor_pin = THERMISTOR_PIN;
  _heater.turnOff();
  _heater.led_pin = UV_LED_PIN;
  _heater.fan_pin = FAN_PIN;

  EEPROM.begin();  // You can adjust the size depending on your Nano's EEPROM size

  // Set the EEPROM addresses for each float value
  for (int i = 0; i < 4; i++) {
    eepromAddresses[i] = EEPROM_START_ADDR + i * sizeof(float);
  }

  Kp = readFloatFromEEPROM(Kp_INDEX);
  Ki = readFloatFromEEPROM(Ki_INDEX);
  Kd = readFloatFromEEPROM(Kd_INDEX);
  float tmp = _heater.get_temperature();

  Serial.print(Kp);
  Serial.print(",");
  Serial.print(Ki);
  Serial.print(",");
  Serial.print(Kd);
  Serial.print(",");
  Serial.println(tmp);
  delay(1000);
  Serial.println("ready...");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');  // Read the command from the Serial port until newline
    if (command.substring(0, 6) == "$START") {      // heater on
      command = command.substring(6);
      char *param = strtok(const_cast<char *>(command.c_str()), ",");
      float tmp = atof(param);
      Serial.println(tmp);
      for (int i = 0; i < tmp; i++) {
        param = strtok(NULL, ",");
        float set_point = atof(param);
        param = strtok(NULL, ",");
        float duration = atof(param);
        control(duration, set_point);
      }
      Serial.println("terminate");
      _heater.turnOff();
    } else if (command.substring(0, 3) == "$SP") {
      command = command.substring(4);
      char *param = strtok(const_cast<char *>(command.c_str()), ",");
      float tmp = atof(param);
      writeFloatToEEPROM(Kp_INDEX, tmp);
      Kp = tmp;
      param = strtok(NULL, ",");
      tmp = atof(param);
      writeFloatToEEPROM(Ki_INDEX, tmp);
      Ki = tmp;
      param = strtok(NULL, ",");
      tmp = atof(param);
      writeFloatToEEPROM(Kd_INDEX, tmp);
      Kd = tmp;
    }
  }
}

void control(float duration, float sp) {
  unsigned long starting_time = millis();
  unsigned long current_time = millis();
  float error = 0;

  unsigned long timeInRangeStart = 0;  // Tracks when the temperature enters the range
  unsigned long timeInRange = 0;       // Total time spent within the range
  bool inRange = false;                // Flag to track if we're in the range

  while (Serial.available() == 0 && timeInRange < duration * 60 * 1000) {  // Convert duration to milliseconds
    // Measure heating block temperature
    float blockTemperature = _heater.get_temperature();

    // Estimate liquid temperature
    float liquidTemperature = estimateLiquidTemperature(blockTemperature);

    // Calculate error based on liquid temperature
    error = sp - liquidTemperature;

    // PID control logic
    PID_P = Kp * error;
    PID_I += Ki * error;
    PID_I = constrain(PID_I, -255, 255);  // Constrain integral term
    PID_D = Kd * (error - previous_error);

    float pwm = constrain(PID_P + PID_I + PID_D, 0, 255);
    analogWrite(_heater.pwm_pin, pwm);

    // Check if liquid temperature is within ±2°C of the setpoint
    if (liquidTemperature >= sp - 2 && liquidTemperature <= sp + 2) {
      if (!inRange) {
        // Entering the range
        timeInRangeStart = millis();
        inRange = true;
      }
    } else {
      if (inRange) {
        // Exiting the range, update total time
        timeInRange += millis() - timeInRangeStart;
        inRange = false;
      }
    }

    // If still in range, update the elapsed time
    if (inRange) {
      timeInRange = millis() - timeInRangeStart;
    }

    // Send data to GUI
    Serial.print("Setpoint:");
    Serial.print(sp);
    Serial.print(",LiquidTemperature:");
    Serial.print(liquidTemperature);
    Serial.print(",BlockTemperature:");
    Serial.print(blockTemperature);
    Serial.print(",PWM:");
    Serial.print(pwm);
    Serial.print(",ElapsedTimeInRange:");
    Serial.println(timeInRange / 1000.0);  // Convert to seconds

    // Update previous error and current time
    previous_error = error;
    current_time = millis();

    // Safety check for block temperature
    if (blockTemperature > MAX_BLOCK_TEMPERATURE) {
      _heater.turnOff();
      Serial.println("Overtemperature! Heater turned off.");
      break;
    }
  }

  // If exiting the loop and still in range, update total time
  if (inRange) {
    timeInRange += millis() - timeInRangeStart;
    inRange = false;
  }

  // Final message to indicate the regulation is complete
  Serial.println("Regulation complete. Total time in range achieved.");
}

void control(float setpoints[], float durations[], int numSteps) {
  unsigned long timeInRange = 0;       // Total time spent within the range
  unsigned long timeInRangeStart = 0;  // Tracks when the temperature enters the range
  bool inRange = false;                // Flag to track if we're in the range

  for (int step = 0; step < numSteps; step++) {
    float sp = setpoints[step];
    float duration = durations[step] * 60 * 1000;  // Convert duration to milliseconds
    unsigned long stepStartTime = millis();
    unsigned long current_time = millis();

    while (Serial.available() == 0 && timeInRange < duration) {
      // Measure heating block temperature
      float blockTemperature = _heater.get_temperature();

      // Estimate liquid temperature
      float liquidTemperature = estimateLiquidTemperature(blockTemperature);

      // Calculate error based on liquid temperature
      float error = sp - liquidTemperature;

      // PID control logic
      PID_P = Kp * error;
      PID_I += Ki * error;
      PID_I = constrain(PID_I, -255, 255);  // Constrain integral term
      PID_D = Kd * (error - previous_error);

      float pwm = constrain(PID_P + PID_I + PID_D, 0, 255);
      analogWrite(_heater.pwm_pin, pwm);

      // Check if liquid temperature is within ±2°C of the setpoint
      if (liquidTemperature >= sp - 2 && liquidTemperature <= sp + 2) {
        if (!inRange) {
          // Entering the range
          timeInRangeStart = millis();
          inRange = true;
        }
      } else {
        if (inRange) {
          // Exiting the range, update total time
          timeInRange += millis() - timeInRangeStart;
          inRange = false;
        }
      }

      // If still in range, update the elapsed time
      if (inRange) {
        timeInRange = millis() - timeInRangeStart;
      }

      // LED control logic
      if (step == 0) {  // First step (70°C for 30 minutes)
        unsigned long elapsedTime = (millis() - stepStartTime) / 1000;  // Elapsed time in seconds
        if (elapsedTime % 300 == 0) {  // Every 5 minutes
          _heater.ledOn();
          delay(2000);  // Keep LED on for 2 seconds
          _heater.ledOff();
        }
      } else if (step == 1) {  // Second step (90°C for 5 minutes)
        if (timeInRange == 0) {
          _heater.ledOn();  // Turn LED on for 5 seconds at the start of the step
          delay(5000);
          _heater.ledOff();
        }

        if (timeInRange >= duration) {
          _heater.ledOn();  // Turn LED on for 30 seconds at the end of the step
          delay(30000);
          _heater.ledOff();
          break;  // Exit the loop after the final LED event
        }
      }

      // Send data to GUI
      Serial.print("Setpoint:");
      Serial.print(sp);
      Serial.print(",LiquidTemperature:");
      Serial.print(liquidTemperature);
      Serial.print(",BlockTemperature:");
      Serial.print(blockTemperature);
      Serial.print(",PWM:");
      Serial.print(pwm);
      Serial.print(",ElapsedTimeInRange:");
      Serial.println(timeInRange / 1000.0);  // Convert to seconds

      // Update previous error and current time
      previous_error = error;
      current_time = millis();

      // Safety check for block temperature
      if (blockTemperature > MAX_BLOCK_TEMPERATURE) {
        _heater.turnOff();
        Serial.println("Overtemperature! Heater turned off.");
        break;
      }
    }

    // If exiting the loop and still in range, update total time
    if (inRange) {
      timeInRange += millis() - timeInRangeStart;
      inRange = false;
    }
  }

  // Final message to indicate the regulation is complete
  Serial.println("Regulation complete. Total time in range achieved.");
}

void updateMovingAverage(float readings[], float newValue) {
  for (int i = 1; i < numReadings; i++) {
    readings[i - 1] = readings[i];
  }
  readings[numReadings - 1] = newValue;
}

float calculateAverage(float readings[]) {
  float sum = 0;
  for (int i = 0; i < numReadings; i++) {
    sum += readings[i];
  }
  return sum / numReadings;
}

void writeFloatToEEPROM(int index, float value) {
  EEPROM.put(eepromAddresses[index], value);
}

float readFloatFromEEPROM(int index) {
  float value;
  EEPROM.get(eepromAddresses[index], value);
  return value;
}

float estimateLiquidTemperature(float blockTemperature) {
  // Example linear relationship (adjust based on calibration)
  return 0.8 * blockTemperature + 5.0;  // Replace with your actual model
}
