// NewTone is used to avoid the timer incompatibiliy of Tone with NewPing. Download https://bitbucket.org/teckel12/arduino-new-tone/downloads/ and import .zip
#include <NewTone.h>
#include <NewPing.h>
#include <SPI.h>

const int MIN_DISTANCE = 70; 

NewPing sonar(5, 6, MIN_DISTANCE); // the first and the second number are the pins of the sensor of volume, the third is the maximum distance

// digital potentiometer values
byte address = 0x00;
int CS= 10;
int potentiometer_value = 0;
int distance = 0;

// ultrasonic sensor values (controls frequency)
int us_echo = 3;                            
int us_trigger = 4;

unsigned long TimeResponse;
float distance2;
float tone1;

int digitalPotWrite(int value) {
  digitalWrite(CS, LOW); // this uses SPI protocol to communicate with the potentiometer and sets the resistance
  SPI.transfer(address);
  SPI.transfer(value);
  digitalWrite(CS, HIGH);
}


// task struct definition
typedef struct task {
  int state;
  unsigned long period;
  unsigned long elapsedTime;
  int(*TickFct)(int);

} task;

int delay_gcd;
const unsigned short tasksNum = 3;
task tasks[tasksNum];


// Frequency Task State Machine Definition
enum SM1_States { SM1_INIT };
int SM1_Tick(int state) {
  switch (state)
  {
  case SM1_INIT:
    state = SM1_INIT;
    break;
  default:
    state = SM1_INIT;
    break;
  }

  switch (state)
  {
  case SM1_INIT:
    digitalWrite(us_trigger, HIGH);
    delayMicroseconds(10);
    digitalWrite(us_trigger, LOW);
    TimeResponse = pulseIn(us_echo, HIGH);
    distance2 = TimeResponse / 58;
    break;
  default:
    break;
  }

  return state;
}

// Volume Task State Machine Definition
enum SM3_States { SM3_INIT };
int SM3_Tick(int state) {
  switch (state)
  {
  case SM3_INIT:
    state = SM3_INIT;
    break;
  default:
    state = SM3_INIT;
    break;
  }

  switch (state)
  {
  case SM3_INIT:
    // calculate hand's distance from the sensor
    distance = sonar.ping_cm();
    // distance converted to appropriate values to be sent to digital potentiometer 
    potentiometer_value = map(distance, 0, 70, 240, 255);
    // no hand is detected; max volume played
    if (distance == 0) { potentiometer_value = 255; }
    digitalPotWrite(potentiometer_value);
    break;
  default:
    break;
  }

  return state;
}

// Sound Output Task State Machine Definition
enum SM2_States { SM2_INIT, S0, S1 };
int SM2_Tick(int state) {
  switch (state)
  {
  case SM2_INIT:
    if (distance2 < MIN_DISTANCE) {
      state = S0;
    }
    else {
      state = S1;
    }
    break;
  case S0:
    if (distance2 > MIN_DISTANCE) {
      state = S1;
    }
    else {
      state = S0;
    }
    break;
  case S1:
    if (distance2 < MIN_DISTANCE) {
      state = S0;
    }
    else {
      state = S1;
    }
    break;
  default:
    state = SM2_INIT;
    break;
  }

  switch (state)
  {
  case SM2_INIT:
    break;
  case S0:
    // determine what tone to output based on hand's distance from the sensor
    tone1 = 50.0 * pow(2, (distance2 / 12.0));
    pinMode(9, OUTPUT);
    NewTone(9, tone1);
    break;
  case S1:
    pinMode(9, OUTPUT);
    NewTone(9, 0);
  default:
    break;
  }

  return state;
}



void setup() {
  // set all the pins 
  Serial.begin(9600); 
  pinMode(us_trigger, OUTPUT); // tone sensor                    
  pinMode(us_echo, INPUT); // tone sensor    
  pinMode(CS, OUTPUT);
  SPI.begin();


  unsigned char i = 0;
  tasks[i].state = SM1_INIT;
  tasks[i].period = 10;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &SM1_Tick;
  i++;
  tasks[i].state = SM2_INIT;
  tasks[i].period = 10;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &SM2_Tick;
  i++;
  tasks[i].state = SM3_INIT;
  tasks[i].period = 10;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &SM3_Tick;

  //delay_gcd = 30; // GCD
}

void loop() {   
  for (unsigned char i = 0; i < tasksNum; ++i) {
    if ((millis() - tasks[i].elapsedTime) >= tasks[i].period) {
      tasks[i].state = tasks[i].TickFct(tasks[i].state);
      tasks[i].elapsedTime = millis();
    }
  }
}