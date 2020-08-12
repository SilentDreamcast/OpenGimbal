//================================
//=          OPEN GIMBAL         =
//=  By CheckEm#9384 on Discord  =
//================================
//=         Version 1.0          =
//================================

//NOTES
//LIMITS
//Xspeed fast = 450
//Xspeed slow = 3000
//X Usable Steps = 4400
//DELAY SPEEDS 40 IS STABLE
//Pspeed fast = 30
//Pspeed dependent on delay fixed uSecond = speedLimit = 450
//Pan Usable Steps = 200 (full circle)
//Tspeed fast = 35
//Tspeed dependent on delay
//Tilt Usable Steps = 80 NEW SHORTER TILT ARM ALLOWS FOR NEARLY FULL CIRCLE
//POSITION TRACKING NOT IMPLEMENTED DUE TO NO ENDSTOPS AND LACK OF CLOSED LOOP FEEDBACK LIKE ENCODERS
//POSITION MAY NOT BE ACCURATE DUE TO LOW NEMA 17 TORQUE

//Enable verbose
boolean DEBUG = false;

//Define physical step limits
//#define Xmax 4400
//#define Pmax 200
//#define Tmax 80 //this will be upgraded to 200 is can get smaller Yaw motor
//#define Ymax 400

#define focusButton A1
#define shutterButton A2

//Define stepper driver pins
#define Xstep 12
#define Xdir 11
#define Pstep 10
#define Pdir 9
#define Tstep 8
#define Tdir 7
#define Ystep 6
#define Ydir 5
#define EN 4

//Define endstop interrupt pins
//#define Xstop 2
//#define Pstop 3

//Define additional sensor pins
#define ENdebug A0 //active LOW
//A4, A5 i2c
//A6, A7 pot

//some values to keep track of shit
//int syncDuration = 0;
//const int homeSpeed = 1000;
const int speedLimit = 450; //higher = slower
String input = "";
bool inputDone = false;

int Xpos = 0;
int Ppos = 0;
int Tpos = 0;
int Ypos = 0;
int shutter = 0;


void setup() {
  // put your setup code here, to run once:
  pinMode(ENdebug,INPUT);
  if(!digitalRead(ENdebug)){
    DEBUG = true;
  }

  pinMode(EN, OUTPUT);
  digitalWrite(EN,HIGH);
  
  Serial.begin(9600); //reduced speed to 9600 from 115200 to reduce host CPU usage
  input.reserve(100); //reserve 200 bytes just in case
  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(Xstep, OUTPUT);
  pinMode(Xdir, OUTPUT);
  pinMode(Pstep, OUTPUT);
  pinMode(Pdir, OUTPUT);
  pinMode(Tstep, OUTPUT);
  pinMode(Tdir, OUTPUT);
  pinMode(Ystep, OUTPUT);
  pinMode(Ydir, OUTPUT);

  digitalWrite(Xdir,LOW);
  digitalWrite(Xstep, LOW);
  digitalWrite(Pdir,LOW);
  digitalWrite(Pstep, LOW);
  digitalWrite(Tdir,LOW);
  digitalWrite(Tstep, LOW);
  digitalWrite(Ydir,LOW);
  digitalWrite(Ystep, LOW);
  

  Serial.println("READY!");
  Serial.println("OPTIONS: m (manual), s (sync), h(home), e (motorEN)");
  Serial.println("WARNING: valid input is not checked. run at your own risk");
}

byte mode = 0;
boolean printMenu = true;
boolean ENtoggle;
int control[] = {0,0,0,0,0,0,0,0};
int sync[] = {0,0,0,0,0,0};

void loop() {
  // put your main code here, to run repeatedly:
  if(printMenu){
      Serial.println("MODES: manual, sync, home");
    if(mode == 0){
      Serial.println("CURRENT MODE: manual");
      Serial.println("input: Xstep,Xdelay,Pstep,Pdelay,Tstep,Tdelay,Ystep,Ydelay");
    }
    else if(mode == 1){
      Serial.println("CURRENT MODE: sync");
      Serial.println("input: pause,numFrames,Xstep,Pstep,Tstep,Ystep");
    }
    printMenu = false;
  }

  //input mode switching handler
  if(inputDone){
    Serial.println("SYSTEM BUSY");
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(EN,LOW); //enable drivers to lock motor
    if(DEBUG){
      Serial.print("MODE: ");
      if(mode == 0){
        Serial.println("manual");
      }
      else if(mode == 1){
        Serial.println("sync");
      }
      Serial.print("INPUT: ");
      Serial.println(input);
    }

    //synchronized motor mode
    if(mode == 1){
      parseSCSV();//get info from input
      //if(sync[1] > 0){ //check that sync array has something// REMOVED TO MAKE IT POSSIBLE TO TEST THE CAMERA PATH WITHOUT TAKING A PICTURE
        goSync();
        for(byte i = 0; i < 6; i++){ //reset the array
          sync[i] = 0;
        }
      //}
    }

    //manual motor mode
    if(mode == 0){
      parseMCSV();
      manualControl();
      for(byte i = 0; i < 8; i++){ //reset the array
        control[i] = 0;
      }
    }

    input = ""; //clear serial buffer
    inputDone = false; //reset 
    //printMenu();
    printMenu = true;
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("SYSTEM READY");
  }
}

//void printMenu(){
//  Serial.println("asdf");
//}

//serial event interrupt built into Arduino
//modes: m=manual h=home 
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    //look for char modes
    //manual mode
    if(inChar == 'm'){
      mode = 0;
    }
    if(inChar == 's'){
      mode = 1;
    }
    // add it to the inputString:
    input += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      inputDone = true;
    }
  }
}

//FOR MANUAL MODE this breaks up a CSV like serial input and puts the data into the control array
void parseMCSV(){
  byte preLocation = 0;
  byte curLocation = 0;
  curLocation = input.indexOf(',');
  control[0] = input.substring(0,curLocation).toInt();
  preLocation = curLocation;
  for(byte i = 1; i < 8; i++){
    curLocation = input.indexOf(',',preLocation+1);
    control[i] = input.substring(preLocation+1,curLocation+1).toInt();
    preLocation = curLocation;
  }

  if(DEBUG){
    Serial.print("Parsed: ");
    for(byte i = 0; i < 8; i++){
      Serial.print(control[i]);
      Serial.print(", ");
    }
    Serial.println();
  }
}

//FOR SYNC MODE this breaks up a CSV like serial input and puts the data into the control array
void parseSCSV(){
  byte preLocation = 0;
  byte curLocation = 0;
  curLocation = input.indexOf(',');
  sync[0] = input.substring(0,curLocation).toInt();
  preLocation = curLocation;
  for(byte i = 1; i < 6; i++){
    curLocation = input.indexOf(',',preLocation+1);
    sync[i] = input.substring(preLocation+1,curLocation+1).toInt();
    preLocation = curLocation;
  }

  if(DEBUG){
    Serial.print("Parsed: ");
    for(byte i = 0; i < 6; i++){
      Serial.print(sync[i]);
      Serial.print(", ");
    }
    Serial.println();
  }
}

//sequential motor control in order slide > pan > tilt > yaw
void manualControl(){
  //control[] = {Xstep,Xdelay,Pstep,Pdelay,Tstep,Tdelay,Ystep,Ydelay};

  //slide motor control
  if(control[0] < 0){digitalWrite(Xdir,HIGH);}
  else{digitalWrite(Xdir,LOW);}
  if(control[1] < 0){
    for(int i = 0; i < abs(control[0]); i++){
      digitalWrite(Xstep, HIGH);
      delayMicroseconds(control[1]);
      digitalWrite(Xstep, LOW);
      //for(byte j = 0; j < 100; j++){
        delay(abs(control[1]));
      //}
      
      //delay(35); 
    }
  }
  else{
    for(int i = 0; i < abs(control[0]); i++){
      digitalWrite(Xstep, HIGH);
      delayMicroseconds(control[1]);
      digitalWrite(Xstep, LOW);
      delayMicroseconds(control[1]);
      //delay(35);
    }
  }

  //pan motor control
  if(control[2] < 0){digitalWrite(Pdir,HIGH);}
  else{digitalWrite(Pdir,LOW);}
  for(int i = 0; i < abs(control[2]); i++){
    digitalWrite(Pstep, HIGH);
    delayMicroseconds(speedLimit);
    digitalWrite(Pstep, LOW);
    //delayMicroseconds(control[3]);
    delay(control[3]);
  }

  //tilt motor control
  if(control[4] < 0){digitalWrite(Tdir,HIGH);}
  else{digitalWrite(Tdir,LOW);}
  for(int i = 0; i < abs(control[4]); i++){
    digitalWrite(Tstep, HIGH);
    delayMicroseconds(speedLimit);
    digitalWrite(Tstep, LOW);
    //delayMicroseconds(control[5]);
    delay(control[5]);
  }

  //yaw motor control
  if(control[6] < 0){digitalWrite(Ydir,HIGH);}
  else{digitalWrite(Ydir,LOW);}
  for(int i = 0; i < abs(control[6]); i++){
    digitalWrite(Ystep, HIGH);
    delayMicroseconds(control[7]);
    digitalWrite(Ystep, LOW);
    delayMicroseconds(control[7]);
  }
}

//syncronized motor control where steps start and end at the same time. Step delays are scaled by the motor with the most steps
int frameNumber = 0;
void goSync(){
  //sync[] = {pause,numFrames,Xstep,Pstep,Tstep,Ystep}
  
  //set direction of Xmotor
  if(sync[2] < 0){digitalWrite(Xdir,HIGH);}
  else{digitalWrite(Xdir,LOW);}
  //set direction of Pmotor
  if(sync[3] < 0){digitalWrite(Pdir,HIGH);}
  else{digitalWrite(Pdir,LOW);}
  //set direction of Tmotor
  if(sync[4] < 0){digitalWrite(Tdir,HIGH);}
  else{digitalWrite(Tdir,LOW);}
  //set direction of Ymotor
  if(sync[5] < 0){digitalWrite(Ydir,HIGH);}
  else{digitalWrite(Ydir,LOW);}

  //find the max step
  byte maxStepIndex = 0;
  if(abs(sync[2]) > abs(sync[3])){maxStepIndex = 2;}
  else if(abs(sync[3]) > abs(sync[4])){maxStepIndex = 3;}
  else if(abs(sync[4]) > abs(sync[5])){maxStepIndex = 4;}
  else{maxStepIndex = 5;}
  if(DEBUG){
    Serial.print("max steps: ");
    Serial.println(maxStepIndex);
  }

  //scale pulse delay. NOT NEEDED BVECAUSE WE ARE DOING PAUSE DURATION.
  //int XstepDelay = map(abs(control[0]),0,abs(control[0]),0,syncDuration);
  //int PstepDelay = map(abs(control[2]),0,abs(control[2]),0,syncDuration);
  //int TstepDelay = map(abs(control[4]),0,abs(control[4]),0,syncDuration);
  //int YstepDelay = map(abs(control[6]),0,abs(control[6]),0,syncDuration);

  //pulse speeds for steps needed between points are 1500uS and 40ms dependign on the motor

  //calculate motor step divisor
  int synced[4];
  for(byte i = 0; i < 4; i++){
    if(sync[i+2] != 0){ //honestly arduino wont care if we divide by 0 but we gonna put a check anyway
      synced[i] = sync[maxStepIndex]/sync[i+2];
    }
  }

  //run it
  frameNumber = 0;
  for(int i = 0; i < abs(sync[maxStepIndex]); i++){
    //step Xmotor if needed
    if(i%synced[0] == 0 && abs(sync[2]) > 0){
      //Serial.println("Xstepped");
      digitalWrite(Xstep,HIGH);
    }
    //step Pmotor if needed
    if(i%synced[1] == 0 && abs(sync[3]) > 0){
      //Serial.println("Pstepped");
      digitalWrite(Pstep,HIGH);
    }
    //step Tmotor if needed
    if(i%synced[2] == 0 && abs(sync[4]) > 0){
      //Serial.println("Tstepped");
      digitalWrite(Tstep,HIGH);
    }
    if(i%synced[3] == 0 && abs(sync[5]) > 0){
      //Serial.println("Ystepped");
      digitalWrite(Ystep,HIGH);
    }
    
    if(sync[1] == -1){ //used for video
      delayMicroseconds(sync[0]);
      digitalWrite(Xstep,LOW);
      digitalWrite(Pstep,LOW);
      digitalWrite(Tstep,LOW);
      digitalWrite(Ystep,LOW);
    }
    else{ //used for shutter control
      delayMicroseconds(1000);
      digitalWrite(Xstep,LOW);
      digitalWrite(Pstep,LOW);
      digitalWrite(Tstep,LOW);
      digitalWrite(Ystep,LOW);
      shutterTrig(sync[0], sync[1]);//this will handle our motor off time delay
      //delay(sync[0]*sync[1]); //gonna multiply the time because it's time (camera exposure time) * (number of shots at position)
    }
  }
}

//unfinished since endstops are not yet installed and we currently dont keep track of position
void goHome(){
  Serial.println("going home");
  /*int timeout = 5000;
  while(timeout > 0){
    if(DEBUG){
      Serial.print(".");
    }
    timeout--;
    if(digitalRead(Xstop)){
      Xpos = 0;
      timeout = -1;
      Serial.
    }
    else{
      digitalWrite(Xstep, HIGH);
      delayMicroseconds(homeSpeed);
      digitalWrite(Xstep, LOW);
      delayMicroseconds(homeSpeed);
    }
  }*/
  Serial.println("home set");
}

//trigger the focus and shutter buttons
void shutterTrig(int shutterspeed, int numFrames){
  //at full track run with 4400 steps it will take 4.48 hours to complete excluding shutter
  //delay(4000); //wait for the shitty plastic and motors to stop vibrating
  delay(2000); //should be sufficient enough for vibration to stop
  for(byte i = 0; i < numFrames; i++){
    digitalWrite(focusButton,HIGH);
    delay(500); //should be in manual mode, focus not necessary but needs to be pressed
    digitalWrite(shutterButton,HIGH);
    delay(250);
    digitalWrite(shutterButton,LOW);
    digitalWrite(focusButton,LOW);
    frameNumber++;
    Serial.print("Frame: ");
    Serial.println(frameNumber);
    delay(shutterspeed);
  }
}
