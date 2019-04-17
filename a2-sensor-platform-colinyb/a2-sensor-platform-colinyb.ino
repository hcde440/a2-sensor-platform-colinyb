/*ICE #3.1
 * 
   DHT22 temperature and humidity sensor demo.
    
   brc 2018
*/

// Adafruit IO Temperature & Humidity Example
// Tutorial Link: https://learn.adafruit.com/adafruit-io-basics-temperature-and-humidity
//
// Adafruit invests time and resources providing this open source code.
// Please support Adafruit and open source hardware by purchasing
// products from Adafruit!
//
// Written by Todd Treece for Adafruit Industries
// Copyright (c) 2016-2017 Adafruit Industries
// Licensed under the MIT license.
//
// All text above must be included in any redistribution.

/************************** Configuration ***********************************/

// edit the config.h tab and enter your Adafruit IO credentials
// and any additional configuration needed for WiFi, cellular,
// or ethernet clients.
#include "config.h"

/************************ Example Starts Here *******************************/
//including other libraries in the build
#include <Adafruit_Sensor.h>
#include <Adafruit_Si7021.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// api keys
const char* key = "b4038b91c68deb4bdec045124c6df669";
const char* weatherkey = "73b43e4a3b95d2a36b9d9551fa564d82";

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     13 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// create an si7021 object named sensor
Adafruit_Si7021 sensor = Adafruit_Si7021();

// set up the 'temperature' and 'humidity' feeds
AdafruitIO_Feed *temperature = io.feed("temperature");
AdafruitIO_Feed *humidity = io.feed("humidity");
AdafruitIO_Feed *callapi = io.feed("callapi");

void setup() {

  // start the serial connection
  Serial.begin(115200);
  Serial.print("This board is running: ");
  Serial.println(F(__FILE__));
  Serial.print("Compiled: ");
  Serial.println(F(__DATE__ " " __TIME__));
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // wait for serial monitor to open
  while(! Serial);

  // initialize the si7021 sensor
  sensor.begin();

  // connect to io.adafruit.com
  Serial.print("Connecting to Adafruit IO");
  io.connect();

  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());
  callapi->onMessage(handleMessage); //setting up a message handler
  callapi->get(); //setting callapi to receive data

//  initialize oled display buffer
  display.display();
  delay(2000);
  display.clearDisplay();      // Clear the display
  display.setTextSize(1);      // setting text size to normal 1:1 pixel scale
  display.setTextColor(WHITE); // setting text color to draw white text
  display.cp437(true);         // setting font as the full 256 char 'Code Page 437' font
  display.setCursor(0, 0);     // setting cursor to start at top-left corner

}

void loop() {
  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io.run();

  float celsius = sensor.readTemperature(); // getting temp from sensor reading
  float fahrenheit = (celsius * 1.8) + 32;

  Serial.print("celsius: ");
  Serial.print(celsius); //printing the celsius to serial
  Serial.println("C");

  Serial.print("fahrenheit: ");
  Serial.print(fahrenheit); //printing the fahrenheit to serial
  Serial.println("F");

  // save fahrenheit (or celsius) to Adafruit IO
  temperature->save(fahrenheit); //updating adafruit io

  float humi = sensor.readHumidity();

  Serial.print("humidity: ");
  Serial.print(humi); // reading humidity from the sensor and printing it to serial
  Serial.println("%");

  // save humidity to Adafruit IO
  humidity->save(humi);

//   draw to OLED
  display.clearDisplay();  // clear the display
  display.setCursor(0, 0); // reset cursor to top left
  display.println("Temperature(C): " + (String)celsius);
  display.println("Temperature(F): " + (String)fahrenheit);
  display.println("Humidity: " + (String)humi);
  display.display();
  

  // wait 5 seconds (5000 milliseconds == 5 seconds)
  delay(5000);

}

void handleMessage(AdafruitIO_Data *data) {
  if(data->toPinLevel() == HIGH) {
    Serial.println("Received request for API call.");

    // outputting data to OLED display
    display.setCursor(0,0);
    display.println();
    display.println();
    display.println();
    display.println("Humidity (API): " + getMet());
    display.display();
    delay(5000); //delay so the user can read the output information
  }
}

String getMet() {
  HTTPClient theClient; //creates an HTTPClient object named theClient
  String apistring = "http://api.openweathermap.org/data/2.5/weather?q=" + getGeo() + "&units=imperial&appid=" + weatherkey; //concatonating the api request url
  theClient.begin(apistring); //make the request
  int httpCode = theClient.GET(); //get the HTTP code (-1 is fail)

  if (httpCode > 0) { //test if the request failed
    if (httpCode == 200) { //if successful...
      DynamicJsonBuffer jsonBuffer; //create a DynamicJsonBuffer object named jsonBuffer
      String payload = theClient.getString(); //get the string of json data from the request and assign it to payload
      Serial.println("Parsing...");
      JsonObject& root = jsonBuffer.parse(payload); //set the json data to the variable root
      
      if (!root.success()) { //check if the parsing worked correctly
        Serial.println("parseObject() failed");
        Serial.println(payload); //print what the json data is in a string form
        return("error");
      } // return the humidity
      return(root["main"]["humidity"].as<String>());
    } else { //print error if the request wasnt successful
      Serial.println("Had an error connecting to the network.");
    }
  }
}

String getIP() {
  HTTPClient theClient;
  String ipAddress;

  theClient.begin("http://api.ipify.org/?format=json"); //Make the request
  int httpCode = theClient.GET(); //get the http code for the request

  if (httpCode > 0) {
    if (httpCode == 200) { //making sure the request was successful

      DynamicJsonBuffer jsonBuffer; // create a dynamicjsonbuffer object named jsonbuffer

      String payload = theClient.getString(); //get the data from the api call and assign it to the string object called payload
      JsonObject& root = jsonBuffer.parse(payload); //create a jsonObject called root and use the jsonbuffer to parse the payload string to json accessible data
      ipAddress = root["ip"].as<String>();

    } else { //error message for unsuccessful request
      Serial.println("Something went wrong with connecting to the endpoint.");
      return "error";
    }
  }
  return ipAddress; //returning the ipAddress 
}

String getGeo() {
  HTTPClient theClient;
  Serial.println("Making HTTP request");
  theClient.begin("http://api.ipstack.com/" + getIP() + "?access_key=" + key); //return IP as .json object
  int httpCode = theClient.GET();

  if (httpCode > 0) {
    if (httpCode == 200) {
      Serial.println("Received HTTP payload.");
      DynamicJsonBuffer jsonBuffer;
      String payload = theClient.getString();
      Serial.println("Parsing...");
      JsonObject& root = jsonBuffer.parse(payload);

      // Test if parsing succeeds.
      if (!root.success()) {
        Serial.println("parseObject() failed");
        Serial.println(payload);
        return("error");
      }

      return(root["city"].as<String>()); //return the city name

    } else {
      Serial.println("Something went wrong with connecting to the endpoint.");
      return("error");
    }
  }
}
