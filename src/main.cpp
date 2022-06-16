/*

To Giang Tuan Anh
EEACIU18133

Complete Senior Project source code from github:
Project Name: Autonomous Rotary Drum Filter in Aquaculture

    A SENIOR PROJECT SUBMITTED TO THE SCHOOL OF ELECTRICAL ENGINEERING
    IN PARTIAL FULFILLMENT OF THE REQUIREMENTS FOR THE DEGREE OF
    BACHELOR OF ELECTRICAL ENGINEERING

*/

// pin out so far
/*

- DS18B20    - D3
- LCD 16X2   - D1 & D2
- Relay      - D4

*/

/////// include library section //////////////////////////////////
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "addons/TokenHelper.h" // Provide the token generation process info.
#include "addons/RTDBHelper.h"  // Provide the RTDB payload printing info and other helper functions.
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C_Hangul.h>
//#include <LiquidCrystal_I2C.h>

#define WIFI_SSID "Tamxinhdep2-5G" // Insert prj network credentials
#define WIFI_PASSWORD "25081989"
#define API_KEY "AIzaSyC6l3fjZIqmF_p1VulxnHyZnL0XuVq2Rys" // Insert Firebase project API Key
#define USER_EMAIL "tuananhto132@gmail.com"               // Insert Authorized Email and Corresponding Password
#define USER_PASSWORD "Godlight132"
#define DATABASE_URL "https://auto-rotary-drum-filter-default-rtdb.asia-southeast1.firebasedatabase.app/" // Insert RTDB URLefine the RTDB URL
#define ONE_WIRE_BUS 0                                                                                    // D3
#define relayInput 2                                                                                      // D4

OneWire oneWire(ONE_WIRE_BUS);
// lcd(0x27, 16, 2);                       // SCL - D1
LiquidCrystal_I2C_Hangul lcd(0x27, 16, 2); // SDA - D2
DallasTemperature sensors(&oneWire);

FirebaseData fbdo; // Define Firebase objects
FirebaseAuth auth;
FirebaseConfig config;

String uid;          // Variable to save USER UID
String databasePath; // Variables to save database paths
String tempPath;
// String humPath;
// String presPath;
String rl1Path;
String waterlPath;

Adafruit_BME280 bme; // I2C

/////// global variable /////////////////////////////////////////////////////////////////////
// int relayInput = 2;
int relayState;
float temperature;
// float humidity;
// float pressure;
int relay1;
int waterlevel;
//----- setup Led and button ---
const int ledPin1 = 12; // led attached to this pin
const int buttonPin1 = 13;
const int ledPin2 = 11;    // led attached to this pin
const int buttonPin2 = 14; // push button attached to this pin
int buttonState1 = LOW;    // this variable tracks the state of the button, low if not pressed, high if pressed
int ledState1 = -1;        // this variable tracks the state of the LED, negative if off, positive if on

long lastDebounceTime1 = 0; // the last time the output pin was toggled
long debounceDelay1 = 500;  // the debounce time; increase if the output flikersc

int buttonState2 = LOW; // this variable tracks the state of the button, low if not pressed, high if pressed
int ledState2 = -1;     // this variable tracks the state of the LED, negative if off, positive if on

long lastDebounceTime2 = 0; // the last time the output pin was toggled
long debounceDelay2 = 500;

unsigned long sendDataPrevMillis = 0; // Timer variables (send new readings every three minutes)
unsigned long timerDelay = 180000;

int temp_up = 0;
int temp_down = 0;
//----- setup Sensor -----------
int phao_up = 7;   // This is the input of the upper sensor
int phao_down = 8; // This is the input of the lower sensor
int val_up = 0;    // This is the variable reading the status of the upper sensor
int val_down = 0;  // This is the variable reading the status of the lower sensor
int phao_state = 0;

/////// function define section //////////////////////////////////////////////////////////////
void initBME();
void initWiFi();
void sendFloat(String path, float value);
void sendInt(String path, int value);
void loading();
void na_display(int pos);
void temp_display(int pos);
void relay_check(int relayState);
int relay_on();
int relay_off();
int waterLevelTest(int value);
// void CheckLed1();
// void CheckLed2();
// int checkPhaoState(int value_waterlevel, int value_up, int value_down, int temp_up, int temp_down);

/////// setup code section ///////////////////////////////////////////////////////////////////
void setup()
{

  Serial.begin(9600);
  // Serial.begin(115200);

  // Initialize BME280 sensor
  // initBME();
  initWiFi();
  /* initialize the LCD */
  Wire.begin(D2, D1);
  sensors.begin();
  lcd.init();

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "")
  {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid;

  // Update database path for sensor readings
  tempPath = databasePath + "/temperature";  // --> UsersData/<user_uid>/temperature
  rl1Path = databasePath + "/relay1";        // --> UsersData/<user_uid>/relay1
  waterlPath = databasePath + "/waterlevel"; // --> UsersData/<user_uid>/waterlevel
  // presPath = databasePath + "/pressure";    // --> UsersData/<user_uid>/pressure

  // lcd.begin();

  /* Turn on the blacklight and print a message. */
  loading();

  //--------set the mode of the pins...--------------
  pinMode(relayInput, OUTPUT);
  pinMode(ledPin1, OUTPUT);
  pinMode(buttonPin1, INPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(buttonPin2, INPUT);
  pinMode(phao_up, INPUT);   // sets the digital pin 7 as input
  pinMode(phao_down, INPUT); // sets the digital pin 7 as input
  temp_up = val_up;
  temp_down = val_down;
}

/////// main code section //////////////////////////////////////////////////////////////
void loop()
{
  //---- Send new readings to database --------------------------------------------------------------------------
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();

    // Get latest sensor readings
    sensors.requestTemperatures();
    temperature = sensors.getTempCByIndex(0);
    // Send readings to database:
    sendFloat(tempPath, temperature);
    relay_check(relayState);

    waterLevelTest(waterlevel);
    sendInt(waterlPath, waterlevel);
    sendInt(rl1Path, relayState);

    na_display(4);
    na_display(8);
    temp_display(12);

    // relay_on();
    // delay(2000);
    // relay_check(relayState);

    // relay_off();
    // delay(2000);
    // relay_check(relayState);

    // sendInt(rl1Path, relayState);

    //------- check Water Level ----------------
    // CheckLed1();
    // CheckLed2();
    val_up = digitalRead(phao_up);     // read the input pin
    val_down = digitalRead(phao_down); // read the input pin
  }
  //------ Run in background -------------------------------------------------------------------------------------
  // na_display(0);
}

/////// function code section /////////////////////////////////////////////////////////

// Initialize BME280
void initBME()
{
  if (!bme.begin(0x76))
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1)
      ;
  }
}

// Initialize WiFi
void initWiFi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Write float values to the database
void sendFloat(String path, float value)
{
  if (Firebase.RTDB.setFloat(&fbdo, path.c_str(), value))
  {
    Serial.print("Writing value: ");
    Serial.print(value);
    Serial.print(" on the following path: ");
    Serial.println(path);
    Serial.println("PASSED");
    Serial.println("PATH: " + fbdo.dataPath());
    Serial.println("TYPE: " + fbdo.dataType());
  }
  else
  {
    Serial.println("FAILED");
    Serial.println("REASON: " + fbdo.errorReason());
  }
}
void sendInt(String path, int value)
{
  if (Firebase.RTDB.setInt(&fbdo, path.c_str(), value))
  {
    Serial.print("Writing value: ");
    Serial.print(value);
    Serial.print(" on the following path: ");
    Serial.println(path);
    Serial.println("PASSED");
    Serial.println("PATH: " + fbdo.dataPath());
    Serial.println("TYPE: " + fbdo.dataType());
  }
  else
  {
    Serial.println("FAILED");
    Serial.println("REASON: " + fbdo.errorReason());
  }
}

int waterLevelTest(int value)
{
  return value = 100;
}

void loading()
{
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Starting Prj ");
  for (int i = 0; i < 15; i++)
  {
    lcd.setCursor(i, 1);
    lcd.print("-");
    delay(500);
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Welcome");
}

void na_display(int pos)
{
  lcd.setCursor(pos, 0);
  lcd.print("N/A ");
  lcd.setCursor(pos, 1);
  lcd.print("--- ");
}
void temp_display(int pos)
{
  lcd.setCursor(pos, 0);
  lcd.print("Temp");
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  lcd.setCursor(pos, 1);
  lcd.print(temp);
}
int relay_on()
{
  digitalWrite(relayInput, HIGH);
  return relayState = 1;
}

int relay_off()
{
  digitalWrite(relayInput, LOW);
  return relayState = 0;
}
void relay_check(int relayState)
{
  lcd.setCursor(0, 0);
  lcd.print("Drum");
  if (relayState == 1)
  {
    lcd.setCursor(0, 1);
    lcd.print("ON  ");
  }
  else if (relayState == 0)
  {
    lcd.setCursor(0, 1);
    lcd.print("OFF ");
  }
}

/*
void CheckLed1()
{
  // sample the state of the button - is it pressed or not?
  buttonState1 = digitalRead(buttonPin1);

  // filter out any noise by setting a time buffer
  if ((millis() - lastDebounceTime1) > debounceDelay1)
  {

    // if the button has been pressed, lets toggle the LED from "off to on" or "on to off"
    if ((buttonState1 == HIGH) && (ledState1 < 0))
    {

      digitalWrite(ledPin1, HIGH); // turn LED on
      temp_up = 1;
      ledState1 = -ledState1;       // now the LED is on, we need to change the state
      lastDebounceTime1 = millis(); // set the current time
    }
    else if ((buttonState1 == HIGH) && (ledState1 > 0))
    {

      digitalWrite(ledPin1, LOW); // turn LED off
      temp_up = 0;
      ledState1 = -ledState1;       // now the LED is off, we need to change the state
      lastDebounceTime1 = millis(); // set the current time
    }
  }
}
*/

/*
void CheckLed2()
{
  // sample the state of the button - is it pressed or not?
  buttonState2 = digitalRead(buttonPin2);
  // filter out any noise by setting a time buffer
  if ((millis() - lastDebounceTime2) > debounceDelay2)
  {

    // if the button has been pressed, lets toggle the LED from "off to on" or "on to off"
    if ((buttonState2 == HIGH) && (ledState2 < 0))
    {

      digitalWrite(ledPin2, HIGH); // turn LED on
      temp_down = 1;
      ledState2 = -ledState2;       // now the LED is on, we need to change the state
      lastDebounceTime2 = millis(); // set the current time
    }
    else if ((buttonState2 == HIGH) && (ledState2 > 0))
    {

      digitalWrite(ledPin2, LOW); // turn LED off
      temp_down = 0;
      ledState2 = -ledState2;       // now the LED is off, we need to change the state
      lastDebounceTime2 = millis(); // set the current time
    }
  }
}

*/
//////

/*

int checkPhaoState(int value_waterlevel, int value_up, int value_down, int temp_up, int temp_down)
{
  if (val_up == 0 && val_down == 1 && temp_up == 0)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tank is FULL");
    lcd.setCursor(0, 1);
    lcd.print("press 1: ?");
  }
  else if (val_up == 1 && val_down == 0 && temp_down == 0)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tank is EMPTY");
    ;
    lcd.setCursor(0, 1);
    lcd.print("press 2: ?");
  }
  else if (val_up == 0 && val_down == 1 && temp_up == 1 && temp_down == 0)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PUMP OUT");
  }
  else if (val_up == 1 && val_down == 1 && temp_up == 1 && temp_down == 0)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PUMP OUT");
  }
  else if (val_down == 0 && val_up == 1 && temp_down == 1 && temp_up == 0)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PUMP IN");
  }
  else if (val_down == 1 && val_up == 1 && temp_down == 1 && temp_up == 0)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PUMP IN");
  }
  else if (temp_down == 1 && temp_up == 1)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CHOOOSE 1 ONLY");
  }
  else if (val_down == 1 && val_up == 1 && temp_down == 0 && temp_up == 0)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("FILL MORE WATER");
  }
  else
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ERROR");
  }
}

*/
//////