#include <WiFi.h>
#include <FastLED.h>

#define LED_PIN 32
#define NUM_LEDS 132

CRGB leds[NUM_LEDS];

// Grid dimensions
const int rows = 12;
const int cols = 11;

// LED state array
bool ledStates[rows][cols] = {0};

// Color definitions
CRGB startColor = CRGB(0, 255, 0);  // Green
CRGB middleColor = CRGB(0, 0, 255);   // Blue
CRGB finishColor = CRGB(255, 0, 0);    // Red

// Replace with your network credentials
const char* ssid = <your-wifi-network-name>;
const char* password = <your-wifi-password>;

WiFiServer server(80);

String header;

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// Variables to store the current and previous clicked button indices
int currentClickedButtonIndex = -1;
int previousClickedButtonIndex = -2;

void handleButtonClick(int row, int col) {
  // Calculate the correct index based on the new serpentine pattern
  int index = col * rows + (col % 2 == 0 ? (rows - row - 1) : row);

  // Toggle LED state
  ledStates[row][col] = !ledStates[row][col];

  // Update physical LEDs and set color based on the row
  if (ledStates[row][col]) {
    if (row <= 2) {
      leds[index] = finishColor;  // Rows 1 and 2: Green color
    } else if (row >= 3 && row <= 9) {
      leds[index] = middleColor;   // Rows 3 to 9: Blue color
    } else {
      leds[index] = startColor;    // Rows 10 to 12: Red color
    }
  } else {
    leds[index] = CRGB::Black;  // Turn off: Black color
  }

  FastLED.show();
}


void turnOnAll() {
  // Turn on all LEDs with the correct colors based on their rows
  for (int col = 0; col < cols; col++) {
    for (int row = 0; row < rows; row++) {
      int index = col * rows + (col % 2 == 0 ? (rows - row - 1) : row);

      ledStates[row][col] = true;

      // Set color based on the row
      if (row <= 2) {
        leds[index] = finishColor;  // Rows 1 and 2: Green color
      } else if (row >= 3 && row <= 9) {
        leds[index] = middleColor;   // Rows 3 to 9: Blue color
      } else {
        leds[index] = startColor;    // Rows 10 to 12: Red color
      }
    }
  }

  FastLED.show();
}

void turnOffAllButtons() {
  // Turn off all buttons/LEDs to off
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      ledStates[row][col] = false;
      int index = (rows - row - 1) * cols + col;
      leds[index] = CRGB::Black;
    }
  }
  FastLED.show();
}

void setup() {
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);

  Serial.begin(115200);
  Serial.println("Serial connection established.");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");
    String currentLine = "";

    // Variable to store the button index from the current request
    int clickedButtonIndex = -1;

    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Handle HTTP request and update LED states
            if (header.startsWith("GET /")) {
              // Extract the button index from the URL path
              int slashIndex = header.indexOf("/", 5); // Find the second slash after "GET /"
              if (slashIndex != -1) {
                int buttonIndex = header.substring(5, slashIndex).toInt();
                if (buttonIndex >= 0 && buttonIndex < rows * cols) {
                  // If a valid button index is extracted, update its state immediately
                  int clickedRow = buttonIndex / cols;
                  int clickedCol = buttonIndex % cols;

                  // Store the clicked button index
                  currentClickedButtonIndex = buttonIndex;

                  handleButtonClick(clickedRow, clickedCol);
                }
              }
            }

            // Check if the Turn Off All button is clicked
            if (header.indexOf("GET /turnOffAll") != -1) {
              turnOffAllButtons();
            }

            // Check if the "Turn On All" button is clicked
            if (header.indexOf("GET /turnOnAll") != -1) {
              turnOnAll();
            }

            // Wrap the existing HTML content in a div and apply styles for centering
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>html { font-family: Helvetica; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100%;}");
            client.println("body { background-color: #0E142B; margin: 0; }");
            client.println(".container { text-align: center; }");  // Center the grid
            client.println("h1 { font-weight: lighter; color: #FFED8A; margin: 0; padding: 0;}");
            client.println(" a {text-decoration: none; margin: 0; padding: 0; }");
            client.println(".button-container { margin: 2em 0; }");  // Adjust margin as needed
            client.println(".turn-off-all-button, .turn-on-all-button { background-color: #FF6347; color: white; border: none; padding: 1em 2em; cursor: pointer; margin: 0 1em; display: inline-block; border-radius: 2em;}");
            client.println(".button { border: none; border-radius: 50%; width: 30px; height: 30px; font-size: 10px; margin: 1px; padding: 1px; cursor: pointer;}");
            client.println(".button2 { background-color: #FFED8A; color: #0E142B; }");  // On
            client.println(".button3 { background-color: #244FB3; color: white; }");  // Off
            client.println(".label { font-size: 10px; }");
            client.println("@media (min-width: 700px) {h1 {font-size: 3rem;}.button {width: 3rem; height: 3rem; font-size: 1rem; margin: 2px;} .turn-off-all-button, .turn-on-all-button {font-size: 1rem;}}");
            client.println("</style></head>");

            client.println("<body><div class=\"container\"><h1>LED Rock Wall</h1>");

            // Display button container for Turn Off All and Turn On All
            client.println("<div class=\"button-container\">");

            // Display "Turn On All" button
            client.println("<a href=\"/turnOnAll\"><button class=\"turn-on-all-button\">All On</button></a>");

            // Display "Turn Off All" button
            client.println("<a href=\"/turnOffAll\"><button class=\"turn-off-all-button\">All Off</button></a>");

            // Close button container
            client.println("</div>");

            // Display grid of buttons with labels
            for (int row = 0; row < rows; row++) {
              for (int col = 0; col < cols; col++) {
                int buttonIndex = row * cols + col;
                client.print("<a href=\"/" + String(buttonIndex) + "/toggle\"><button class=\"button");
                client.print(ledStates[row][col] ? " button2\">" : " button3\">");

                // Display labels inside buttons
                char label[4];
                snprintf(label, sizeof(label), "%c%d", 'A' + col, rows - row);
                client.print(label);

                client.println("</button></a>");
              }
              client.println("<br>");
            }

            client.println("</div></body></html>");

            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}