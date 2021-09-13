extern "C" {
#include "user_interface.h"
}

#define SLEEP_TIME  50

unsigned long last_print_time;
// 10 seconds, since we're working with microsecond values
const unsigned long print_interval = 10e6;
unsigned long startTime;

// system total time in microseconds
unsigned long activeTotalTime = 0;
int analogReadingPos = 0; 
int recentAnalogReadings[10];
unsigned long readingsCount = 0;
int sleepCount = 0;

int ldrPin = A0;

// 5400 [mA] * 3600 [s] * 3,3 [V] = 64152 [Wh] = 64152 [J]
unsigned int batteryTotalCharge = 64152;
int batteryCurrentCharge = batteryTotalCharge;
// [mA]
int activeModeCurrent = 170;
// [mA]
float sleepModeCurrent = 0.9; 
// [V]
float voltage = 3.3;

void setup() {
  Serial.begin(115200);
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  last_print_time = micros();
  startTime = micros();
  activeTotalTime = micros();
  pinMode(ldrPin, INPUT);
} 

void loop() {
  unsigned long time_now = micros();
  
//  Serial.println(time_now);
//  Serial.println(last_print_time);
//  Serial.println(print_interval); 
  storeSensorReadings();
//  activeTotalTime = micros() - (1000 * SLEEP_TIME);
  if(((time_now - last_print_time) > print_interval)) {
    Serial.print("tempo total: ");
    Serial.print(time_now/1e6);
    Serial.print(" [s]");
    last_print_time = time_now;
    if (batteryCurrentCharge > 0) {
  //    Serial.print("--- sleep count: ");
  //    Serial.println(sleepCount);
      Serial.print(" | ");
      long totalTimeDiffSec = (time_now - startTime)/1e6;
      double samplingRate = readingsCount/totalTimeDiffSec;
      Serial.print("taxa de amostragem: ");
      Serial.print(samplingRate);
      Serial.print(" [Hz]");
      Serial.print(" | ");
      long idleTimeMicro = sleepCount * SLEEP_TIME * 1000;
  //    Serial.print("--- tempo idle: ");
  //    Serial.println(idleTimeMicro);
      activeTotalTime = micros();
      long activeTotalTimeSec = (activeTotalTime - idleTimeMicro)/1e6;
  //    Serial.print("--- tempo ativo: ");
  //    Serial.println(activeTotalTimeSec);
  //    Serial.print("--- tempo total: ");
  //    Serial.println(totalTimeDiffSec);
      Serial.print("duty cycle: ");
      float dutyCycle = (float) activeTotalTimeSec/totalTimeDiffSec;
      Serial.print(dutyCycle);
      Serial.print(" | ");
  //    float sleepCycle = 1 - dutyCycle;
  //    float dutyPower = voltage * activeModeCurrent;
  //    float sleepPower = voltage * sleepModeCurrent;
  //    float averagePower = ((dutyPower * dutyCycle) + (sleepPower * sleepCycle))/1e3;
  //    // [Wh]
  //    float averageEnergyConsumption = averagePower * 3600;
      float charge = calculateBatteryCurrentCharge(dutyCycle);
  //    Serial.print("--- Average Power: ");
  //    Serial.println(averagePower);
      Serial.print("bateria: ");
      Serial.print((float) (charge/batteryTotalCharge)*100);
      Serial.print("%");
      Serial.print(" | ");
      double average = getSamplingAverage();
      Serial.print("média de amostras: ");
      Serial.print(average); 
      Serial.println(" [Ohms]");   
  //    Serial.print("--- Diff Start Time: ");
  //    Serial.println(totalTimeDiffSec);
  //    Serial.println("--------------------------------------------------------------------");
      sleepCount = 0;
    } else {
      Serial.print(" | ");
      Serial.println("Bateria descarregada totalmente");
    }
  }
  delay(SLEEP_TIME);
  sleepCount++;
}

void storeSensorReadings() {
  double reading = analogRead(ldrPin);
  recentAnalogReadings[analogReadingPos] = reading;
  if (analogReadingPos > 9) {
    analogReadingPos = 0;
  } else {    
    analogReadingPos++;    
  }  
  readingsCount++;
}

float calculateBatteryCurrentCharge(float dutyCycle) {
//  Serial.println("");
//  Serial.print("--- batteryCurrentCharge: ");
//  Serial.println(batteryCurrentCharge);
  float sleepCycle = 1 - dutyCycle;
  // [mW]
  float dutyPower = voltage * activeModeCurrent;
  // [mW]
  float sleepPower = voltage * sleepModeCurrent;
  // [W]
  float averagePower = ((dutyPower * dutyCycle) + (sleepPower * sleepCycle))/1e3;
  // [W / 10[s]]
  int averageEnergyConsumption = averagePower * 10;
//  Serial.print("--- averageEnergyConsumption: ");
//  Serial.println(averageEnergyConsumption);
  batteryCurrentCharge = batteryCurrentCharge - averageEnergyConsumption;
  if (batteryCurrentCharge > 0) {
    return batteryCurrentCharge;
  } else {
    return 0;
  }
}

double getSamplingAverage() {
  int samplingCount = 10;
  int valuesSum = 0;
  for (int index = 0; index < samplingCount; index++) {
    double value = recentAnalogReadings[index];
//    Serial.print("A0 reading: ");
//    Serial.print(value);
//    Serial.println("");
    valuesSum += value;
  }
  return valuesSum / samplingCount;
}
