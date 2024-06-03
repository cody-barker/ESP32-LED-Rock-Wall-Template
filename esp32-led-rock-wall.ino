#include <WiFi.h>
#include <FastLED.h>
#include <EEPROM.h>

#define LED_PIN 32
#define NUM_LEDS 132

CRGB leds[NUM_LEDS];

// Grid dimensions
const int rows = 12;
const int cols = 11;
const int numRoutes = 30;

// LED state array
bool ledStates[rows][cols] = {0};

// Color definitions
CRGB startColor = CRGB(0, 255, 0);  // Green
CRGB middleColor = CRGB(0, 0, 255);   // Blue
CRGB finishColor = CRGB(255, 0, 0);    // Red

// Replace with your network credentials
const char* ssid = //Your SSID Here;
const char* password = //Your Password Here;

WiFiServer server(80);

String header;

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// Variable to store the currently clicked button index
int currentClickedButtonIndex = -1;

void handleButtonClick(int row, int col) {
  // Calculate the correct index based on the new serpentine pattern
  int index = col * rows + (col % 2 == 0 ? (rows - row - 1) : row);
  Serial.println(row, col);
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
    leds[index] = CRGB::Black;  // Turn off
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
        leds[index] = finishColor;  // Rows 1 and 2: startColor
      } else if (row >= 3 && row <= 9) {
        leds[index] = middleColor;   // Rows 3 to 9: middleColor
      } else {
        leds[index] = startColor;    // Rows 10 to 12: finishColor
      }
    }
  }
  FastLED.show();
}

void turnOffAllButtons() {
  // Turn off all buttons/LEDs
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      ledStates[row][col] = false;
      int index = (rows - row - 1) * cols + col;
      leds[index] = CRGB::Black;
    }
  }
  FastLED.show();
}

void saveRoute(int selectedRoute) {
  // Calculate the EEPROM address based on the selected route
  int address = selectedRoute * sizeof(ledStates[0][0]) * rows * cols;

  // Debugging: Print the calculated EEPROM address
  Serial.print("Saving to EEPROM at address: ");
  Serial.println(address);

  // Debugging: Print the LED states before saving
  Serial.println("LED states before saving:");
  for (int col = 0; col < cols; col++) {
    for (int row = 0; row < rows; row++) {
      Serial.print(ledStates[row][col]);
      Serial.print(" ");
    }
    Serial.println();
  }

  // Store ledStates array to EEPROM at the calculated address
  EEPROM.put(address, ledStates);
  EEPROM.commit();

  // Debugging: Print the saved LED states for verification
  Serial.println("Saved LED states to EEPROM:");
  for (int col = 0; col < cols; col++) {
    for (int row = 0; row < rows; row++) {
      Serial.print(ledStates[row][col]);
      Serial.print(" ");
    }
    Serial.println();
  }
}


void loadRoute(int selectedRoute) {
  // Calculate the EEPROM address based on the selected route
  int address = selectedRoute * sizeof(ledStates[0][0]) * rows * cols;

  // Debugging: Print the calculated EEPROM address
  Serial.print("Loading from EEPROM at address: ");
  Serial.println(address);

  // Read saved LED states from EEPROM at the calculated address
  EEPROM.get(address, ledStates);

  // Debugging: Print the loaded LED states for verification
  Serial.println("Loaded LED states from EEPROM:");
  for (int col = 0; col < cols; col++) {
    for (int row = 0; row < rows; row++) {
      Serial.print(ledStates[row][col]);
      Serial.print(" ");
    }
    Serial.println();
  }

  // Update physical LEDs based on loaded states
  for (int col = 0; col < cols; col++) {
    for (int row = 0; row < rows; row++) {
      int index = col * rows + (col % 2 == 0 ? (rows - row - 1) : row);
      if (ledStates[row][col]) {
        // Set color based on the row
        if (row <= 2) {
          leds[index] = finishColor;  // Rows 1 and 2: startColor
        } else if (row >= 3 && row <= 9) {
          leds[index] = middleColor;  // Rows 3 to 9: middleColor
        } else {
          leds[index] = startColor;   // Rows 10 to 12: finishColor
        }
      } else {
        leds[index] = CRGB::Black;  // Turn off
      }
    }
  }
  FastLED.show();
}


void setup() {
  EEPROM.begin(4096);

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

    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Debugging: Print the received header
            Serial.println("Received Header:");
            Serial.println(header);

            // Handle HTTP request and update LED states
            if (header.startsWith("GET /")) {
              
              // Extract the button index from the URL path
              if (isdigit(header[5])) {
                int slashIndex = header.indexOf("/", 5); // Find the second slash after "GET /"
                int buttonIndex = header.substring(5, slashIndex).toInt();
                if (buttonIndex >= 0 && buttonIndex < rows * cols) {
                  // If a valid button index is extracted, update its state immediately
                  int clickedRow = buttonIndex / cols;
                  int clickedCol = buttonIndex % cols;
              
                  // Store the clicked button index
                  currentClickedButtonIndex = buttonIndex;

                  //Debugging
                  Serial.print("slashIndex: ");
                  Serial.println(slashIndex);
                  Serial.print("buttonIndex: ");
                  Serial.println(buttonIndex);
                  Serial.print("clickedRow: ");
                  Serial.println(clickedRow);
                  Serial.print("clickedCol: ");
                  Serial.println(clickedCol);
                  Serial.print("currentClickedButtonIndex ");
                  Serial.println(currentClickedButtonIndex);


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

            // Check if the "Save Route" button is clicked
            if (header.indexOf("GET /saveRoute") != -1) {
              int selectedRoute = header.substring(header.indexOf("?route=") + 7).toInt();
              saveRoute(selectedRoute);
            }

            //Check if the "Load Route" option is selected
            if (header.indexOf("GET /loadRoute") != -1) {
              int selectedRoute = header.substring(header.indexOf("?route=") + 7).toInt();
              loadRoute(selectedRoute);
            }

            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>html { font-family: Helvetica; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100%;}");
            client.println("body { background-color: #0E142B; margin: 0; }");
            client.println(".container { text-align: center; }");
            client.println("h1 { font-weight: lighter; color: #FFED8A; margin: 0; padding: 0;}");
            client.println(" a {text-decoration: none; margin: 0; padding: 0; }");
            client.println(".button-container { margin: 2em 0; }"); 
            client.println(".turn-off-all-button, .turn-on-all-button { background-color: #FF6347; color: white; border: none; padding: 1em 2em; cursor: pointer; margin: 0 1em; display: inline-block; border-radius: 2em;}");
            client.println(".button { border: none; border-radius: 50%; width: 30px; height: 30px; font-size: 10px; margin: 1px; padding: 1px; cursor: pointer;}");
            client.println(".button2 { background-color: #FFED8A; color: #0E142B; }");
            client.println(".button3 { background-color: #244FB3; color: white; }");
            client.println(".label { font-size: 10px; }");
            client.println("#saves {margin: 1em;}");
            client.println("select { padding: 0.5em; border-radius: 1em; background-color: #FF6347; color: white; }");
            client.println("option { padding: 0.5em; background-color: #FF6347; color: white; }");
            client.println(".button-orange-small {padding: 0.66em; margin-left: 1em; border-radius: 1em; background-color: #FF6347; color: white; }");
            client.println("@media (min-width: 700px) {h1 {font-size: 3rem;}.button {width: 3rem; height: 3rem; font-size: 1rem; margin: 2px;} .turn-off-all-button, .turn-on-all-button {font-size: 1rem;}}");
            client.println("</style></head>");

            client.println("<body><div class=\"container\"><h1>LED Rock Wall</h1>");

            // Display button container for Turn Off All and Turn On All
            client.println("<div class=\"button-container\">");

            // Display "Turn On All" button
            client.println("<a href=\"/turnOnAll\"><button class=\"turn-on-all-button\">All On</button></a>");

            // Display "Turn Off All" button
            client.println("<a href=\"/turnOffAll\"><button class=\"turn-off-all-button\">All Off</button></a>");

            //Display the Route select
            client.println("<div id=\"saves\" class=\"container\">");
            client.println("<select id=\"routeSelect\" onchange=\"selectRoute()\">");

           // Loop to generate options dynamically
            for (int i = 1; i <= numRoutes; i++) {
                client.print("<option value=\"" + String(i) + "\"");

                // Check if the current route matches the stored route in local storage
                client.println(">Route " + String(i) + "</option>");
            }

            client.println("</select>");
            client.println("<button class=\"button-orange-small\" onclick=\"saveRoute()\">Save Route</button>");
            client.println("</div>");


            //JavaScript functions to handle select change and save route, and manage local storage
            client.println("<script>");
            client.println("function selectRoute() {");
            client.println("  var select = document.getElementById('routeSelect');");
            client.println("  var selectedRoute = select.options[select.selectedIndex].value;");
            client.println("  localStorage.setItem('selectedRoute', selectedRoute);"); // Store selected route in local storage
            client.println("  window.location = '/loadRoute?route=' + selectedRoute;"); // Load the selected route immediately
            client.println("}");
            client.println("function saveRoute() {");
            client.println("  var select = document.getElementById('routeSelect');");
            client.println("  var selectedRoute = select.options[select.selectedIndex].value;");
            client.println("  window.location = '/saveRoute?route=' + selectedRoute;");
            client.println("}");
            client.println("document.addEventListener('DOMContentLoaded', function() {");
            client.println("  var selectedRoute = localStorage.getItem('selectedRoute');");
            client.println("  if (selectedRoute) {");
            client.println("    document.getElementById('routeSelect').value = selectedRoute;"); // Set selected route from local storage
            client.println("  }");
            client.println("});");
            client.println("</script>");


            // Display grid of buttons with labels
            for (int row = 0; row < rows; row++) {
              for (int col = 0; col < cols; col++) {
                int buttonIndex = row * cols + col;
                bool isButtonOn = ledStates[row][col]; // Check if LED is on

                client.print("<a href=\"/" + String(buttonIndex) + "/");

                // Check if the current endpoint is toggleon or toggleoff and set the button action accordingly
                if (isButtonOn) {
                  client.print("toggleoff\"><button class=\"button button2\">"); // Button is on, set to toggleoff
                } else {
                  client.print("toggleon\"><button class=\"button button3\">"); // Button is off, set to toggleon
                }

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