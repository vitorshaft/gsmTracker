// Created by: Vitor Domingues
// Date of creation: 09 Jan 2025
// Last modified on: 09 Jan 2025

// COMMUNICATION VIA SMS AND PHONE CALL USING SIM800L MODULE WITH I2C OLED DISPLAY
// REMEMBER TO CONFIGURE THE DESTINATION PHONE NUMBER IN LINES 94 AND 119

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static const int RXPin = 4, TXPin = 5; // Pins for GPS
static const int rxGSM = 2, txGSM = 3;
static const uint32_t GPSBaud = 9600;  // GPS baud rate
double lat = 0;
double lng = 0;

TinyGPSPlus gps;            // TinyGPS++ object
SoftwareSerial ss(RXPin, TXPin); // SoftwareSerial object for GPS communication
SoftwareSerial sim800L(rxGSM, txGSM);

String buff;

void sendGPSSMS(double lat, double lon);
void waitForBasicResponse();

void setup() {
  Serial.begin(9600);
  ss.begin(GPSBaud);
  sim800L.begin(9600);

  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Failed to initialize the OLED display!"));
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Starting..."));
  display.display();

  // SIM800L configuration
  sim800L.println("AT");
  waitForBasicResponse();
  sim800L.println("ATE1");
  waitForBasicResponse();
  sim800L.println("AT+CMGF=1");
  waitForBasicResponse();
  sim800L.println("AT+CNMI=1,2,0,0,0"); // Configuration to automatically receive SMS
  waitForBasicResponse();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("Ready to use!"));
  display.display();
  delay(1000);
}

void loop() {
  // Check if there are messages received in SIM800L
  if (sim800L.available()) {
    buff = sim800L.readString();
    Serial.println(buff);

    // Check for the "gps1234" text in the received SMS
    if (buff.indexOf("gps1234") != -1) {
      // Retrieve GPS coordinates
      if (gps.location.isUpdated()) {
        lat = gps.location.lat();
        lng = gps.location.lng();

        // Send the coordinates as an SMS response
        sendGPSSMS(lat, lng);
      } else {
        Serial.println("GPS not updated. Coordinates not sent.");
      }
    }
  }

  // Update GPS data
  if (ss.available() > 0) {
    gps.encode(ss.read());
  }
}

// Function to send GPS coordinates as an SMS response
void sendGPSSMS(double lat, double lon) {
  String number = "+5581996040596"; // Destination number
  String message;

  if (lat == 0.0 && lon == 0.0) {
    message = "GPS not available at the moment.";
  } else {
    // Format coordinates in degrees and minutes
    int latDegrees = abs((int)lat);
    double latMinutes = (abs(lat) - latDegrees) * 60.0;
    char latDir = (lat >= 0) ? 'N' : 'S';

    int lonDegrees = abs((int)lon);
    double lonMinutes = (abs(lon) - lonDegrees) * 60.0;
    char lonDir = (lon >= 0) ? 'E' : 'W';

    message = "Coordinates:\nLat: " + String(latDegrees) + "° " + String(latMinutes, 2) + "' " + latDir +
              "\nLon: " + String(lonDegrees) + "° " + String(lonMinutes, 2) + "' " + lonDir;
  }

  sim800L.print("AT+CMGS=\"");
  sim800L.print(number);
  sim800L.println("\"");
  waitForBasicResponse();

  sim800L.print(message);
  sim800L.write(0x1A); // Send Ctrl+Z to finish the SMS
  waitForBasicResponse();

  Serial.println("SMS sent: ");
  Serial.println(message);

  // Update OLED display
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("SMS Sent:"));
  display.setCursor(0, 10);
  display.print("Lat: ");
  display.println(String(lat, 6));
  display.setCursor(0, 20);
  display.print("Lon: ");
  display.println(String(lon, 6));
  display.display();
}

// Function to wait for a basic response from SIM800L
void waitForBasicResponse() {
  delay(1000);
  while (sim800L.available()) {
    String response = sim800L.readString();
    Serial.println(response);
  }
  sim800L.read();
}
