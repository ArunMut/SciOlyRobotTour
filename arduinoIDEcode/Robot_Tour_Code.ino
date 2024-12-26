#include <Arduino_LSM6DS3.h> // Library for the IMU (adjust based on your IMU)

// Variables for time tracking
unsigned long prevTime = 0;

// Variables for motion
float acceleration = 0.0;
float velocity = 0.0;
float displacement = 0.0;

// Constants for motion filtering
float alpha = 0.85; // Filter constant (adjust as needed)

struct Motor {
  int f, b; // For the pins

  // Constructor to initialize pins
  Motor(int forwardPin, int backwardPin) {
    f = forwardPin;
    b = backwardPin;
  }
};

// Time for movement
int fwd = 1000;
int bwd = 1000;
int rt = 1000;
int lt = 1000;
int def = 1000;

Motor upLeft(5, 4);
Motor upRight(6, 7);
Motor downLeft(11, 10);
Motor downRight(9, 8);

Motor motors[4] = {upLeft, downRight, upRight, downLeft};


void setup() {
  Serial.begin(9600); //9600 bits per sec


  // Initialize the IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1); //pause evth
  }
  Serial.println("IMU initialized");

  // Initialize time with amnt of time passed
  prevTime = millis();

  // Initialize motors
  for(int i = 0; i < 4; i++){
    pinMode(motors[i].f, OUTPUT);  // Set pins as outputs
    pinMode(motors[i].b, OUTPUT);
    digitalWrite(motors[i].f, LOW);  // Start motors off 
    digitalWrite(motors[i].b, LOW);
    Serial.println("Initialized a motor");
  }
}

void loop() {
  // Here you can add your movement control logic or conditions
}

int distanceNeeded = 1.0;

void forward() {
  for(int i = 0; i < 4; i++){
    digitalWrite(motors[i].f, HIGH);
    digitalWrite(motors[i].b, LOW);
  }

  settingIMU('y');

  stopMotors();
}

void backward() {
  for(int i = 0; i < 4; i++){
    digitalWrite(motors[i].f, LOW);
    digitalWrite(motors[i].b, HIGH);
  }

  settingIMU('y');

  stopMotors();
}

void right() {
  for(int i = 0; i < 2; i++){
    digitalWrite(motors[i].f, HIGH);
    digitalWrite(motors[i].b, LOW);
  }
  for(int i = 2; i < 4; i++){
    digitalWrite(motors[i].f, LOW);
    digitalWrite(motors[i].b, HIGH);
  }
  
  settingIMU('x');

  stopMotors();
}

void left() {
  for(int i = 0; i < 2; i++){
    digitalWrite(motors[i].f, LOW);
    digitalWrite(motors[i].b, HIGH);
  }
  for(int i = 2; i < 4; i++){
    digitalWrite(motors[i].f, HIGH);
    digitalWrite(motors[i].b, LOW);
  }
  
  settingIMU('x');

  stopMotors();
}

void stopMotors() {
  for(int i = 0; i < 4; i++){
    digitalWrite(motors[i].f, LOW);
    digitalWrite(motors[i].b, LOW);
  }

  delay(def);
}

void settingIMU(char axis){
  acceleration = displacement = velocity = 0;
  
  unsigned long timeout = millis() + 10000; // Timeout after 10 seconds

  while(abs(displacement) < distanceNeeded && millis() < timeout){
    float xAcc, yAcc, zAcc;
    // Read acceleration from the IMU
    if (IMU.accelerationAvailable()) {
      IMU.readAcceleration(xAcc, yAcc, zAcc);
    }
    
    // Filter acceleration based on axis
    acceleration = (axis == 'y') ? alpha * acceleration + (1 - alpha) * yAcc : alpha * acceleration + (1 - alpha) * xAcc;

    // Calculate time delta (in seconds)
    unsigned long currentTime = millis();
    float deltaTime = (currentTime - prevTime) / 1000.0;
    prevTime = currentTime;

    // Integrate to find velocity
    velocity += acceleration * deltaTime;

    // Integrate velocity to find displacement
    displacement += velocity * deltaTime;

    delay(50); // Delay for stability
  }
}
