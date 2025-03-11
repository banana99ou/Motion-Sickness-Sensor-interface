typedef union{
  float number;
  uint8_t bytes[4];
} FLOATUNION_t;

FLOATUNION_t myValue;

#include <WiFi.h>
WiFiServer server(8888);

const char* SSID     = "FMCL";
const char* PASSWORD = "66955144";

int counter1 = 0;
float counter2 = 0;

void setup() {
  Serial.begin(115200);

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");
  server.begin();
  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/stream");
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    while(client.connected()){
      Serial.write('A'); 
      Serial.print("sending");
      Serial.print(counter1);
      Serial.print(", ");
      Serial.println(counter2);

      myValue.number = counter1;
      for (int i=0; i<4; i++){
        client.write(myValue.bytes[i]); 
      }
      
      myValue.number = counter2;
      for (int i=0; i<4; i++){
        client.write(myValue.bytes[i]); 
      }
      
      // Use the same delay in the Serial Receive block
      counter1++;
      counter2 = 4 + counter2;
      delay(50);
    }
  }
}
