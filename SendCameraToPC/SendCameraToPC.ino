// Create a union to easily convert float to byte
typedef union{
  float number;
  uint8_t bytes[4];
} FLOATUNION_t;

// Create the variable you want to send
FLOATUNION_t myValue;

// Hard-coded 14x14 MJPEG image data (dummy example)
// A valid JPEG file always starts with 0xFF,0xD8 (SOI) and ends with 0xFF,0xD9 (EOI).
// Replace the following array with your actual MJPEG frame data.
static uint8_t imageData[14 * 14] = {
  0xFF, 0xD8,             // SOI marker
  // --- JFIF header (example) ---
  0xFF, 0xE0, 0x00, 0x10, 
  'J', 'F', 'I', 'F', 0x00,
  0x01, 0x02, 0x01, 0x00, 0x48, 0x00, 0x48, 0x00, 0x00,
  // --- Quantization Table (dummy) ---
  0xFF, 0xDB, 0x00, 0x43,
  /* (Quantization table data would go here) */
  // --- Start Of Frame (SOF) marker ---
  0xFF, 0xC0, 0x00, 0x11, 
  0x08,                   // Sample precision
  0x00, 0x0E,             // Image height = 14
  0x00, 0x0E,             // Image width = 14
  0x03,                   // Number of components
  0x01, 0x11, 0x00,       // Component 1
  0x02, 0x11, 0x01,       // Component 2
  0x03, 0x11, 0x01,       // Component 3
  // --- Huffman Table (dummy) ---
  0xFF, 0xC4, 0x00, 0x14,
  /* (Huffman table data would go here) */
  // --- Start Of Scan (SOS) ---
  0xFF, 0xDA, 0x00, 0x0C, 
  0x03, 0x01, 0x00, 0x02, 0x11, 0x03, 0x11, 
  0x00, 0x3F, 0x00,
  // --- Compressed image data (dummy minimal data) ---
  0x00, 0x11, 0x22, 0x33, 0x44,
  // --- End Of Image (EOI) ---
  0xFF, 0xD9
};

void setup() {
  Serial.begin(115200);
  // Optional: Wait for the Serial port to be available.
  while (!Serial) { /* wait */ }
  
  // Debug message (can be removed if not needed)
  Serial.println("Sending hard-coded 14x14 MJPEG image frame...");
}

void loop() {
  // Send a header character for synchronization
  Serial.write('A');

  // Send the image data over Serial
  for (int row = 0; row < 14; row++) {
    for (int col = 0; col < 14; col++) {
      myValue.number = imageData[row * 14 + col];
      for (int i=0; i<4; i++){
        Serial.write(myValue.bytes[i]); 
      }
    }
  }
  
  // Send a terminator (newline) to mark the end of the frame
  Serial.print('\n');
  delay(100);
}
