//Libraries sensor
#include <dht.h>
// Wir verwenden Software Serial
#define softserial

const byte OC1A_PIN = 9; // pin 9
const byte OC1B_PIN = 10; // pin 10
//Adjust this value to adjust the frequency (Frequency in HZ!) (Set currently to 25kHZ)
const word PWM_FREQ_HZ = 25000; 
const word TCNT1_TOP = 16000000/(2*PWM_FREQ_HZ);
// Pin FAN 2 control:
const int fan2Pin = 11;  // pin 11

// DHT22 - Sensor humidity-temperature
#define dht22Pin 7 // pin 7
dht DHT;

// HM-10 Bluetooth
#ifdef softserial
  #include <SoftwareSerial.h>
  const int BTRX = 2;  // pin 2
  const int BTTX = 3;  // pin 3
  SoftwareSerial SerialBT(BTRX, BTTX);
#else 
  HardwareSerial SerialBT = Serial1;
#endif

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
float humPrevious = 0;
float temp = 0; //Stores temperature value
int fanSpeed = 100;
int fanPercent = 0;
unsigned long time = 0;
bool hasTimeDelay = false;
bool hasExecuteCommand = true;

void setup() {
  pinMode(OC1A_PIN, OUTPUT);
  pinMode(fan2Pin, OUTPUT);

  // Clear Timer1 control and count registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  TCCR1A |= (1 << COM1A1) | (1 << WGM11);
  TCCR1B |= (1 << WGM13) | (1 << CS10);
  ICR1 = TCNT1_TOP;
  
  SerialBT.begin(9600);
  SerialBT.println("Program start!");
}

void loop() {
  int chk = DHT.read22(dht22Pin);
  //Read data and store it to variables hum and temp
  hum = DHT.humidity;
  temp= DHT.temperature;

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

  int availableBytes = SerialBT.available();

  if (availableBytes > 0) {    
    char serialRead;
    char space = ' '; //Check for space
    String strCommand = "";
    String strTime = "";
    bool hasReadStrCommand = false;    

    for(int i=0; i < availableBytes; i++){
      serialRead = SerialBT.read();      

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
      SerialBT.println("-----------PrintError-----------");
      SerialBT.print("Invalid command! - '");
      SerialBT.print(strCommand);
      SerialBT.println("'.");
      SerialBT.println("Use command 'help' or '?'");
      SerialBT.println("--------------------------------");
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
      setPwmDuty(100);
    } else if(modeDicArr[i].fan1 == 1) {
      setPwmDuty(0);
    }

    if(modeDicArr[i].fan2 == 0) { //FAN 2
      digitalWrite(fan2Pin, LOW);
    } else if(modeDicArr[i].fan2 == 1) {
      digitalWrite(fan2Pin, HIGH);
    }

    if(modeDicArr[i].fan1 != 2 || modeDicArr[i].fan2 != 2) { //AUTO
      continue;
    }

    if(hum >= 80) {
      setPwmDuty(fanPercent);
      modeDicArr[i].fan1 = 1;
      analogWrite(fan2Pin, fanSpeed);
      modeDicArr[i].fan2 = 1;
    }  else if(hum >= 70 && hum < 80) {
      setPwmDuty(fanPercent);
      modeDicArr[i].fan1 = 1;    
      digitalWrite(fan2Pin, LOW);
      modeDicArr[i].fan2 = 0;
    } else if(hum >= 60 && hum < 70) {
      setPwmDuty(fanPercent);
      modeDicArr[i].fan1 = 1;    
      digitalWrite(fan2Pin, LOW);
      modeDicArr[i].fan2 = 0;
    } else if(hum >= 0 && hum < 60) {
      setPwmDuty(100);
      modeDicArr[i].fan1 = 0;
      digitalWrite(fan2Pin, LOW); 
      modeDicArr[i].fan2 = 0;   
    }
  }
}

void executeCommandAuto() {
 if((int)humPrevious != (int)hum) {
    humPrevious = hum;
    hasExecuteCommand = true;
  }
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

    if(modeDicArr[i].cmd == "help" || modeDicArr[i].cmd == "?") {
      printHelp();
      continue;
    }
    SerialBT.println(modeDicArr[i].cmd);
    printStatusFan(modeDicArr[i].fan1, modeDicArr[i].fan2);
  }
  printDelay();
}

void printHelp() { // Run=3.1
  SerialBT.println("------------PrintHelp-----------");
  SerialBT.println("help -print help.");
  SerialBT.println("? -print help.");
  SerialBT.println("on -on all fan, space min.");
  SerialBT.println("off -off all fan, space min.");
  SerialBT.println("auto -auto all fan, space min.");
  SerialBT.println("info -print status.");
  SerialBT.println("Example: 'on 5' - on for 5 min.");
  SerialBT.println("Example: 'auto' - on auto mode.");
  SerialBT.println("Example: '?' - print help.");
  SerialBT.println("--------------------------------");
}

void printStatusHumTemp() { // Run=3.2
  if (hasExecuteCommand) {
    SerialBT.println("-------PrintStatusHumTemp-------");
    //Print temp and humidity values to serial monitor
    SerialBT.print("Humidity:");
    SerialBT.print(hum);
    SerialBT.print("%, Temp:");
    SerialBT.print(temp);
    SerialBT.println("C");
    SerialBT.println("--------------------------------");
  }  
}

void printStatusFan(byte fan1, byte fan2) { // Run=3.3
  SerialBT.println("---------PrintStatusFan---------");
  SerialBT.print("FAN 1 ");
  if(fan1 == 0){
    SerialBT.println("OFF");
  } else if(fan1 == 1) {
    SerialBT.print("ON, Speed=");
    SerialBT.print(100 - fanPercent);
    SerialBT.println("%");    
  } else if(fan1 == 2) {
    SerialBT.println("");
  }

  SerialBT.print("FAN 2 ");
  if(fan2 == 0){
    SerialBT.println("OFF");
  } else if(fan2 == 1){
    SerialBT.print("ON, Speed=");
    SerialBT.print(fanSpeed);
    SerialBT.println("");
  } else if(fan2 == 2) {
    SerialBT.println("");
  }
  SerialBT.println("--------------------------------");
}

void printDelay() { // Run=3.4  
  if(hasTimeDelay) {
    SerialBT.println("-----------PrintDelay-----------");
    SerialBT.print("Delay ");
    SerialBT.print((time / 60) / 1000); //Convert to minutes
    SerialBT.println(" min.");
    SerialBT.println("--------------------------------");
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

void setPwmDuty(byte duty) {
  OCR1A = (word) (duty*TCNT1_TOP)/100;
}
