/*
const byte OC1A_PIN = 9; // pin 9
const byte OC1B_PIN = 10; // pin 10
//Adjust this value to adjust the frequency (Frequency in HZ!) (Set currently to 25kHZ)
const word PWM_FREQ_HZ = 25000; 
const word TCNT1_TOP = 16000000/(2*PWM_FREQ_HZ);
*/

// DHT22 - Sensor humidity-temperature
//#define dht22Pin 7 // pin 7
//dht DHT;

// Pin FAN 2 control:
const int fan2Pin = 11;  // pin 11
//Test
#define temperatureSensorPin A0

typedef struct { 
  int index;
  String cmd;  //command name;
  char *msg;    //description command
  byte event; //0 - off, 1 - on;
  byte value; //active fans;
  byte fan1;  //0 - off, 1 - on, 2- auto;
  byte fan2;  //0 - off, 1 - on, 2- auto;
} modeDictionary;

modeDictionary modeDicArr[9] {
  {0, "auto", "FAN ALL AUTO", 1, 1, 2, 2},
  {1, "on", "FAN ALL ON", 0, 1, 1, 1},
  {2, "off", "FAN ALL OFF", 0, 1, 0, 0},
  {3, "help", "Print Help", 1, 0, 0, 0},
  {4, "?", "Print Help", 0, 0, 0, 0},
  {5, "info", "Print Info", 0, 0, 0, 0},
  {6, "i", "Print Info", 0, 0, 0, 0},
  {7, "status", "Print Status", 0, 0, 0, 0},
  {8, "s", "Print Status", 0, 0, 0, 0}
};
int modeDicSize = sizeof(modeDicArr)/sizeof(modeDictionary);

float hum = 0;  //Stores humidity value
float temp = 0; //Stores temperature value
int fanSpeed = 100;
int fanPercent = 0;
unsigned long time = 0;
bool hasTimeDelay = false;
bool hasExecuteCommand = true;

void setup() {
  //pinMode(OC1A_PIN, OUTPUT);
  pinMode(fan2Pin, OUTPUT);
  /*
  // Clear Timer1 control and count registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  TCCR1A |= (1 << COM1A1) | (1 << WGM11);
  TCCR1B |= (1 << WGM13) | (1 << CS10);
  ICR1 = TCNT1_TOP;
  */
  Serial.begin(9600);
  Serial.println("Program start!");
}

void loop() {
  //int chk = DHT.read22(dht22Pin);
  //Read data and store it to variables hum and temp
  //hum = DHT.humidity;
  //temp= DHT.temperature;

  fanPercent = (((100-hum)/100) * 100);
  //fanSpeed = 1.5 * fanPercent;
  fanSpeed = 1.5 * hum;

  serialEvent();
  executeCommandAuto();
  executeCommand();
  print();
  timeDelay();
  resetVariables();
  delay(1000);  //Delay 1 sec.
}

void serialEvent() { // Run=1

  int availableBytes = Serial.available();

  if (availableBytes > 0) {    
    char serialRead;
    char space = ' '; //Check for space
    String strCommand = "";
    String strTime = "";
    bool hasReadStrCommand = false;    

    for(int i=0; i < availableBytes; i++){
      serialRead = Serial.read();      

      int strCmdLength = strCommand.length();

      if (serialRead == space && strCmdLength == 0) {
        continue;
      }        

      if (serialRead == space && strCmdLength > 0) {
        hasReadStrCommand = true;      
        continue;
      }

      if (hasReadStrCommand) {
        if (serialRead == space && strTime.length() > 0) {      
          continue;
        }          
        strTime += serialRead;          
        continue;
      }

      strCommand += serialRead;
    }

    for(int i = 0; i < modeDicSize; ++i) {

      if(modeDicArr[i].cmd != strCommand){
        continue;
      }

      for(int j = 0; j < modeDicSize; ++j){
        modeDicArr[j].event = 0;
      }

      modeDicArr[i].event = 1;
      hasExecuteCommand = true;
      hasTimeDelay = strTime.length() > 0 && modeDicArr[i].value == 1;
      if(hasTimeDelay){
        time = strTime.toInt(); //minutes
        time *= 1000; //seconds 
        time *= 60; //milliseconds
        continue;
      }

      time = 0;
      hasTimeDelay = false;
    }

    if(!hasExecuteCommand){
      Serial.println("-----------PrintError-----------");
      Serial.print("Invalid command! - '");
      Serial.print(strCommand);
      Serial.println("'.");
      Serial.println("Use command 'help' or '?'");
      Serial.println("--------------------------------");
    }  
  }
}

void executeCommand() { // Run=2

  if(!hasExecuteCommand){
    return;
  }

  for(int i = 0; i < modeDicSize; ++i){
    if(modeDicArr[i].event != 1 || modeDicArr[i].value != 1){
      continue;
    }

    if(modeDicArr[i].fan1 == 0) { //FAN 1
      //setPwmDuty(100);
    } else if(modeDicArr[i].fan1 == 1) {
      //setPwmDuty(0);
    }

    if(modeDicArr[i].fan2 == 0) { //FAN 2
      digitalWrite(fan2Pin, LOW);
    } else if(modeDicArr[i].fan2 == 1) {
      digitalWrite(fan2Pin, HIGH);
      //Serial.println(modeDicArr[i].fan2);
    }

    if(modeDicArr[i].fan1 != 2 || modeDicArr[i].fan2 != 2) { //AUTO
      continue;
    }

    if(hum >= 80) {
      //setPwmDuty(fanPercent);
      modeDicArr[i].fan1 = 1;
      analogWrite(fan2Pin, fanSpeed);
      modeDicArr[i].fan2 = 1;
    }  else if(hum >= 70 && hum < 80) {
      //setPwmDuty(fanPercent);
      modeDicArr[i].fan1 = 1;    
      digitalWrite(fan2Pin, LOW);
      modeDicArr[i].fan2 = 0;
    } else if(hum >= 65 && hum < 70) {
      //setPwmDuty(fanPercent);
      modeDicArr[i].fan1 = 1;    
      digitalWrite(fan2Pin, LOW);
      modeDicArr[i].fan2 = 0;
    } else if(hum >= 0 && hum < 65) {
      //setPwmDuty(100);
      modeDicArr[i].fan1 = 0;
      digitalWrite(fan2Pin, LOW); 
      modeDicArr[i].fan2 = 0;   
    }
  }
}

void executeCommandAuto() {
  tempEvent(); //TEST
}

void print(){ // Run=3
  if(!hasExecuteCommand){
    return;
  }
  printStatusHumTemp();

  for(int i = 0; i < modeDicSize; ++i){
    if(modeDicArr[i].event != 1){
      continue;
    }   

    Serial.println(modeDicArr[i].msg);
    if(modeDicArr[i].cmd == "help" || modeDicArr[i].cmd == "?") {
      printHelp();
      continue;
    }

    printStatusFan(modeDicArr[i].fan1, modeDicArr[i].fan2);
  }
  printDelay();
}

void printHelp() { // Run=3.1
  Serial.println("------------PrintHelp-----------");
  Serial.println("help -print help.");
  Serial.println("? -print help.");
  Serial.println("on -on all fan, space min.");
  Serial.println("off -off all fan, space min.");
  Serial.println("auto -auto all fan, space min.");
  Serial.println("info -print status.");
  Serial.println("Example: 'on 5' - on for 5 min.");
  Serial.println("Example: 'auto' - on auto mode.");
  Serial.println("Example: '?' - print help.");
  Serial.println("--------------------------------");
}

void printStatusHumTemp() { // Run=3.2
  if (hasExecuteCommand) {
    Serial.println("-------PrintStatusHumTemp-------");
    //Print temp and humidity values to serial monitor
    Serial.print("Humidity:");
    Serial.print(hum);
    Serial.print("%, Temp:");
    Serial.print(temp);
    Serial.println("C");
    Serial.println("--------------------------------");
  }  
}

void printStatusFan(byte fan1, byte fan2) { // Run=3.3
  Serial.println("---------PrintStatusFan---------");
  Serial.print("FAN 1 ");
  if(fan1 == 0){
    Serial.println("OFF");
  } else if(fan1 == 1) {
    Serial.print("ON, Speed=");
    Serial.print(100-fanPercent);
    Serial.println("%");    
  } else if(fan1 == 2) {
    Serial.println("");
  }

  Serial.print("FAN 2 ");
  if(fan2 == 0){
    Serial.println("OFF");
  } else if(fan2 == 1){
    Serial.print("ON, Speed=");
    Serial.print(fanSpeed);
    Serial.println("");
  } else if(fan2 == 2) {
    Serial.println("");
  }
  Serial.println("--------------------------------");
}

void printDelay() { // Run=3.4  
  if(hasTimeDelay) {
    Serial.println("-----------PrintDelay-----------");
    Serial.print("Delay ");
    Serial.print((time / 60) / 1000); //Convert to minutes
    Serial.println(" min.");
    Serial.println("--------------------------------");
  }  
}

void timeDelay() { // Run=4
  if(hasTimeDelay) {
    delay(time);
    return;
  }
  hasExecuteCommand = false;
}

void resetVariables() { // Run=5  
  //Reset auto mode fans
  for(int i = 0; i < modeDicSize; ++i) {
    modeDicArr[i].event = 0;
  }  
  modeDicArr[0].fan1 = 2;
  modeDicArr[0].fan2 = 2;
  modeDicArr[0].event = 1;

  //Reset time
  time = 0;
  hasTimeDelay = false;
}

void tempEvent(){
  // Get a reading from the temperature sensor:
  int readingTemp = analogRead(temperatureSensorPin);
  // Converting that reading to voltage, for 3.3v arduino use 3.3
  float voltageTemp = (readingTemp * 5.0) / 1024.0;
  // Convert the voltage into the temperature in degree Celsius:
  float temperatureC = (voltageTemp - 0.5) * 100;

  if(hum != temperatureC){
    hum = temperatureC;
    hasExecuteCommand = true;
  }
}
