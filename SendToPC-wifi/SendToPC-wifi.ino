#include <WiFi.h>
WiFiServer server(8888);

const char* ssid     = "SK_7458_2.4G";
const char* password = "HYV21@0527";

int counter = 0;

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  server.begin();  // Start listening on port 8888
  Serial.println("Server started, waiting for client...");
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    while(client.connected()){
      Serial.print("sending");
      Serial.println(counter);
      // Once connected, send data
      client.println(counter);
      counter++;
      delay(1000);
    }
  }
}
