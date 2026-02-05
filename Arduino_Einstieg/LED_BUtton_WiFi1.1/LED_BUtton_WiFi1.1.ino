#include <SPI.h>
#include <WiFiNINA.h>

#define LED_PIN 11
#define BUTTON_PIN 12

char ssid[] = "@BayernWLAN"; // Your WiFi SSID
int status = WL_IDLE_STATUS;
WiFiServer server(80);
String readString;

bool ledState = LOW; // Variable to keep track of the LED state

void setup() {
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT); // Use a simple input (external pull-down resistor)
    Serial.begin(9600);

    // Connect to WiFi
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to Network named: ");
        Serial.println(ssid);
        status = WiFi.begin(ssid);
        delay(10000);
    }
    server.begin();

    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void loop() {
    // Check the button state
    if (digitalRead(BUTTON_PIN) == HIGH) { // Button pressed
        delay(50); // debounce delay
        if (digitalRead(BUTTON_PIN) == HIGH) { // Check again after debounce
            ledState = !ledState; // Toggle LED state
            digitalWrite(LED_PIN, ledState); // Set LED
            while (digitalRead(BUTTON_PIN) == HIGH); // Wait for button to be released
        }
    }

    // Handle incoming clients
    WiFiClient client = server.available();
    if (client) {
        Serial.println("new client");
        readString = ""; // Clear previous readString

        // Read data from the client until it gets disconnected
        while (client.connected()) {
            while (client.available()) {
                char c = client.read();
                readString += c; // Append the character to the request string

                // Look for the end of the request line
                if (c == '\n') {
                    // Check if we received a full request and process it
                    if (readString.indexOf("GET /?lighton") >= 0) {
                        ledState = true;
                    } else if (readString.indexOf("GET /?lightoff") >= 0) {
                        ledState = false;
                    }
                    digitalWrite(LED_PIN, ledState); // Update LED state

                    // Generate HTTP response
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-type:text/html");
                    client.println();
                    client.println("<html><body>");
                    client.println("<h1>LED Control</h1>");
                    client.println("<p>LED is now: " + String(ledState ? "On" : "Off") + "</p>");
                    client.println("<a href=\"/?lighton\">Turn On Light</a><br />");
                    client.println("<a href=\"/?lightoff\">Turn Off Light</a><br />");
                    client.println("</body></html>");

                    // After sending the response, clear readString for the next request
                    readString = ""; // Clear the read string
                    break; // Exit the inner while and continue checking the client
                }
            }
            // Add a small delay to avoid overwhelming the device
            delay(1);
        }

        // Once the client disconnects, log it
        client.stop();
        Serial.println("client disconnected");
    }
}