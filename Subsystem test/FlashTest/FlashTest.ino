#define LED 33
#define FLED 4

bool stringComplete;
String inputString;
int i = 0;

void setup(){
    pinMode(LED, OUTPUT);
    pinMode(FLED, OUTPUT);

    Serial.begin(115200);
    Serial.println("Boot");
}

void loop(){
    serialEvent();

    if(stringComplete) {
    
        if(inputString.startsWith("LED")) {
            int amount = inputString.substring(4).toInt();
            Serial.print("Ack: LED\t");
            Serial.println(amount);
            analogWrite(LED, amount);
        }

        if(inputString.startsWith("FLED")) {
            int amount = inputString.substring(5).toInt();
            Serial.print("Ack: FLED\t");
            Serial.println(amount);
            analogWrite(FLED, amount);
        }
    
        inputString = "";
        stringComplete = false;
    }
}

void serialEvent() {
    while(Serial.available()){
        char inChar = (char)Serial.read();
        inputString += inChar;
        if(inChar == '\n') {
        stringComplete = true;
        }
    }
  }