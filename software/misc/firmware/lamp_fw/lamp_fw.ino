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
  void ledOn(){
    digitalWrite(led_pin, HIGH);
  }
  void ledOff(){
    digitalWrite(led_pin, LOW);
  }
  void fanOn(){
    digitalWrite(fan_pin, HIGH);
  }
  void fanOff(){
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
  _heater.ledOn();
}


void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');  // Read the command from the Serial port until newline format : START,1,72,300000 or START,2,72,95,10000,20000
    if (command.substring(0, 6) == "$START") {      // heater on
      command = command.substring(6);
      char *param = strtok(const_cast<char *>(command.c_str()), ",");
      float tmp = atof(param);
      Serial.println(tmp);
      for(int i=0;i<tmp;i++){
        param = strtok(NULL, ",");
        float set_point = atof(param);
        param = strtok(NULL, ",");
        float duration = atof(param);
        control(duration, set_point);
      }
      char* dump;
      Serial.readBytes(dump, Serial.available());
      Serial.println("terminate");
      _heater.turnOff();
      free(dump);
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
  Serial.print("Kp : ");
  Serial.print(Kp);
  Serial.print("\t");
  Serial.print("Ki : ");
  Serial.print(Ki);
  Serial.print("\t");
  Serial.print("Kd : ");
  Serial.println(Kd);
  float current_time = millis();
  float starting_time = millis();
  bool initial = true;
  while (Serial.available() == 0 && current_time - starting_time < duration) {

    if(initial){
      for(int i=0;i<5;i++){
        float temperature = _heater.get_temperature();
        updateMovingAverage(readings, temperature);
        temperature = calculateAverage(readings);
      }
      initial=false;
    }
    float temperature = _heater.get_temperature();
    updateMovingAverage(readings, temperature);
    temperature = calculateAverage(readings);
    float error = sp - temperature;
    float PID_P = Kp * error;
    float PID_I = Ki * error;
    PID_I = constrain(PID_I, -200, 200);
    float PID_D = Kd * (error - previous_error);
    float pwm = PID_P + PID_I + PID_D;
    pwm = constrain(pwm, 0, 255);
    analogWrite(_heater.pwm_pin, static_cast<int>(pwm));
    Serial.print("Setpoint : ");
    Serial.print(sp);
    Serial.print(",");
    Serial.print("Temperature : ");
    Serial.print(temperature);
    Serial.print(",");
    Serial.print("PWM : ");
    Serial.print(static_cast<int>(pwm));
    Serial.print(",");
    Serial.print("dt : ");
    Serial.println(millis() - current_time);
    previous_error = error;
    current_time = millis();
  }
}

// Function to update moving average
void updateMovingAverage(float readings[], float newValue) {
  for (int i = 1; i < numReadings; i++) {
    readings[i - 1] = readings[i];
  }
  readings[numReadings - 1] = newValue;
}

// Function to calculate average of an array
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
