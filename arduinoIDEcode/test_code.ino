// Variables for time tracking
unsigned long prevTime = 0;


// Variables for motion
float acceleration = 0.0;
float velocity = 0.0;
float displacement = 0.0;

// Constants for motion filtering
float alpha = 0.9; // Filter constant (adjust as needed)


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

Motor upLeft(A4, A5);
Motor upRight(1, 2);
Motor downLeft(A0, A1);
Motor downRight(A2, A3);

Motor motors[4] = {upLeft, downRight, upRight, downLeft};


void setup() {
  // Initialize motors
  for(int i = 0; i < 4; i++){
    pinMode(motors[i].f, OUTPUT);  // Set pins as outputs
    pinMode(motors[i].b, OUTPUT);
    digitalWrite(motors[i].f, LOW);  // Start motors off 
    digitalWrite(motors[i].b, LOW);
  }

  // Optionally, run a short test (forward) to see motor behavior
  

  d();
  w();
  w();
  a();
  w();
  d();
  d();
  s();
  s();
  d();
  s();
  a();
  d();
  d();
  w();
  s();
  a();
  w();
  w();
  w();
  

}

void loop() {

}

void w() {
  for(int i = 0; i < 4; i++){
    if(i == 2){
      digitalWrite(motors[i].f, HIGH);
      digitalWrite(motors[i].b, LOW);
    } else{
      analogWrite(motors[i].f, .87*255);
      analogWrite(motors[i].b, 0);
    }
  } 
  
  
  stopMotors(fwd);
}

void s() {
  for(int i = 0; i < 4; i++){
    digitalWrite(motors[i].f, LOW);
    digitalWrite(motors[i].b, HIGH);
  }
  stopMotors(bwd);
}

void d() {
  for(int i = 0; i < 2; i++){
    digitalWrite(motors[i].f, HIGH);
    digitalWrite(motors[i].b, LOW);
  }
  for(int i = 2; i < 4; i++){
    digitalWrite(motors[i].f, LOW);
    digitalWrite(motors[i].b, HIGH);
  }
  stopMotors(rt);
}

void a() {
  for(int i = 0; i < 2; i++){
    digitalWrite(motors[i].f, LOW);
    digitalWrite(motors[i].b, HIGH);
  }
  for(int i = 2; i < 4; i++){
    digitalWrite(motors[i].f, HIGH);
    digitalWrite(motors[i].b, LOW);
  }
  stopMotors(lt);
}

void stopMotors(int delayAmount) {
  delay(delayAmount);

  for(int i = 0; i < 4; i++){
    if(i == 2){
      digitalWrite(motors[i].f, LOW);
      digitalWrite(motors[i].b, LOW);
    } else{
      analogWrite(motors[i].f, 0);
      analogWrite(motors[i].b, 0);

    }
  }

  
}
