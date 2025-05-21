void setup() {
  Serial.begin(115200);
  // prepare Serial2 on GP21→TX, GP20→RX
  Serial2.setTX(21);
  Serial2.setRX(20);
  Serial2.begin( 9600 );    // config-mode baud

  pinMode(18, OUTPUT);      // CFG0
  digitalWrite(18, LOW);    // enter config mode
  delay(50);

  // — DHCP ON (0x33,0x01,0xAA)
  Serial2.write((uint8_t)0x33);
  Serial2.write((uint8_t)0x01);
  Serial2.write((uint8_t)0xAA);
  delay(20);

  // — TCP-Server (0x10,0x00,0xAA)
  Serial2.write((uint8_t)0x10);
  Serial2.write((uint8_t)0x00);
  Serial2.write((uint8_t)0xAA);
  delay(20);

  digitalWrite(18, HIGH);   // exit config mode
  delay(100);

  Serial2.begin(115200);    // data-mode baud
  Serial.println("Ready!");
}

void loop() {
  while (Serial2.available()) Serial.write(Serial2.read());
  while (Serial.available())  Serial2.write(Serial.read());
}
