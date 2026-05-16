/*
  float batCap;
  float loadA;
  float loadW;
  float pvV;
  float pvW;
  int vPot;
  bool alertBat;
  bool fault;
  bool status;
  bool switch1;
  bool switch2;

  Variables which are marked as READ/WRITE in the Cloud Thing will also have functions
  which are called when their values are changed from the Dashboard.
  These functions are generated with the Thing and added at the end of this sketch.
*/
#include "thingProperties.h"
#include <Adafruit_ADS1X15.h>
#include <LCD_I2C.h>
#include <ESP32Servo.h>

#define I2C_ADDRESS_1  0x48
#define I2C_ADDRESS_2  0x49

// ADS1115 Variables

 Adafruit_ADS1115 ads1; // First ADS1115 (0x48) object
 Adafruit_ADS1115 ads2; // Second ADS1115 (0x49) object


LCD_I2C lcd(0x27, 16, 2);

  const int alertPin = 18;
  const int pvVpin = 36;
  //const int batVpin = 39;
  //const int pvApin = 34;
  //const int acApin = 35;
  //const int loadApin = 33;
  const int potpin = 32;

  const int load1pin = 15;
  const int load2pin = 2;
  const int load3pin = 19;
  const int load4pin = 23  ;

  int potValue = 0;

// setting PWM properties
const int freq = 1000;
const int pwmChannel0 = 9;
const int pwmChannel1 = 12;
const int resolution = 8;

// horizontal servo
Servo horizontal;
int servoh = 90;
int servohLimitHigh = 170;
int servohLimitLow = 10;

Servo vertical;
int servov = 90;
int servovLimitHigh = 150;
int servovLimitLow = 60;

char ssid[32]; // SSID char limit
char pass[63]; // PASS char limit

unsigned long previousMillis300 = 0, previousMillis500 = 0;

void captivePortal();

void setup() {

  pinMode (load1pin,OUTPUT);
  pinMode (load2pin,OUTPUT);
  pinMode (alertPin,INPUT);

  lcd.begin(); // If you are using more I2C devices using the Wire library use lcd.begin(false)
                 // this stop the library(LCD_I2C) from calling Wire.begin()
  lcd.backlight();
  
  lcd.setCursor(1,0);
  lcd.print("SOLAR TRACKING");
  lcd.setCursor(5,1);
  lcd.print("SYSTEM");
  delay(3000);
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("CONNECT WIFI");
  lcd.setCursor(4,1);
  lcd.print("TO START");
  
    // servo connections
  horizontal.attach(16);
  vertical.attach(17);
  // move servos
  horizontal.write(90);
  vertical.write(60);

  // Initialize serial and wait for port to open:
  Serial.begin(115200);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1000); 

  
  // Konfigurasi fungsi LED PWM
  ledcSetup(pwmChannel0, freq, resolution);
  ledcSetup(pwmChannel1, freq, resolution);
  
  // hubungkan channel ke GPIO untuk bisa di kontrol
  ledcAttachPin(load4pin, pwmChannel0);
  ledcAttachPin(load3pin, pwmChannel1);

  

   captivePortal();

  // Defined in thingProperties.h
  initProperties();

   preferredConnectionHandler(ssid, pass);
  
  Serial.println("Connecting to WiFi");
  Serial.println(SSID);
  Serial.println(PASS);

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

   /// ADS115 gain
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
    ads1.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
    ads2.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV


// Initializing both ADS1115 modules
// Starting ads1 at 0x48 I²C position
if (!ads1.begin(0x48)) {
   Serial.println("Failed to initialize ADS1.");
   while (1);
  }

// Starting ads1 at 0x49 I²C position
if (!ads2.begin(0x49)) {
   Serial.println("Failed to initialize ADS2.");
   while (1);
  }
}

void loop() {
  ArduinoCloud.update();
  // Your code here
  unsigned long currentMillis = millis();

   // Reading the first ADS1115; ads1
  int tr = ads1.readADC_SingleEnded(0); // top right
  int tl = ads1.readADC_SingleEnded(1); // top left
  int br = ads1.readADC_SingleEnded(2); // bottom right
  int bl = ads1.readADC_SingleEnded(3); // bottom left
  
  
  // Reading the second ADS1115; ads2
  int16_t adc20;
  int16_t adc21;
  int16_t adc22;
  int16_t adc23;
  adc20 = ads2.readADC_SingleEnded(0);
  adc21 = ads2.readADC_SingleEnded(1);
  adc22 = ads2.readADC_SingleEnded(2);
  adc23 = ads2.readADC_SingleEnded(3);

  int tol = 400; // Toleransi value pada sensor LDR

  int avt = (tl + tr) / 2; // average value top
  int avd = (bl + br) / 2; // average value bottom
  int avl = (tl + bl) / 2; // average value left
  int avr = (tr + br) / 2; // average value right

  int dvert = avt - avd;  // check the difference of up and down
  int dhoriz = avl - avr; // check the difference of left and right

 //doEvery 300
  if (currentMillis - previousMillis300 >= 300) {
    previousMillis300 = currentMillis; //stores last execution's timestamp
    //CODE EVERY 300  millis 
  // check if the difference is in the tolerance else change vertical angle
  if (-1 * tol > dvert || dvert > tol) {
    if (avt > avd) {
      servov = ++servov;
      if (servov > servovLimitHigh) { 
        servov = servovLimitHigh;
      }
    }
    else if (avt < avd) {
      servov = --servov;
      if (servov < servovLimitLow) {
        servov = servovLimitLow;
      }
    }
    vertical.write(servov);
  }

  // check if the difference is in the tolerance else change horizontal angle
  if (-1 * tol > dhoriz || dhoriz > tol) {
    if (avl > avr) {
      servoh = --servoh;
      if (servoh < servohLimitLow) {
        servoh = servohLimitLow;
      }
    }
    else if (avl < avr) {
      servoh = ++servoh;
      if (servoh > servohLimitHigh) {
        servoh = servohLimitHigh;
      }
    }
    else if (avl = avr) {
      // nothing
    }
    horizontal.write(servoh);
  }
  }

  
  if(digitalRead (alertPin) == HIGH){
    fault = true;
    status = false;
  }
  else {
    fault = false;
    status = true;
  }
  
  //Serial.print("V PV   = ");
  int val1 = analogRead(pvVpin);
  pvV = map(val1, 0,4095,0,2000) / 100.0;
  //Serial.println (pvV);

  
//Serial.print("V Batt = ");
  int val2 = adc20;
  double batVd = map(val2, 15310,16800,1020,1110);
  double batV = map(val2, 15310,16800,1020,1110) / 100.0;
  //Serial.println (batV);
  
//Serial.print("A Load = ");
  int val3 = adc23;
  loadA = map(val3, 12960,13970,0,200) / 100.0;
  //Serial.println (loadA);

//Serial.print("A PV = ");
  int val4 = adc22;
  double pvA = map(val4, 13190,13970,0,200) / 100.0;

//Serial.print("A AC = ");
  int val5 = adc21; 
  double acA = map(val5, 12760,13970,0,200) / 100.0;;

  pvW = pvA * pvV;
  if(pvW < 0){
      pvW = 0;
    }
  
//Serial.print("W Load = ");
  loadW = batV * loadA;
  if(loadW<0){
    loadW = 0;
  }
  
  batCap = map(batVd, 950,1260,0,100);
  if (batCap < 0){
    batCap = 0;
  }

  if (batCap < 20){
    alertBat = true;
  }
  else {
    alertBat = false;
  }
 
  potValue = analogRead(potpin);
  int pwmout1 = map(potValue, 0,4095,0,255);
  ledcWrite(pwmChannel0, pwmout1);
  //Serial.println(pwmout1);

 //doEvery 500
  if (currentMillis - previousMillis500 >= 500) {
    previousMillis500 = currentMillis; //stores last execution's timestamp
    //CODE EVERY 500  millis
  lcd.setCursor(1,0);
  lcd.print("W PV   = ");
  lcd.print(pvW);
  lcd.setCursor(1,1); 
  lcd.print("W Load = ");
  lcd.print(loadW);
  }
}
 

void captivePortal(){
  // put your setup code here, to run once:
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // it is a good practice to make sure your code sets wifi mode how you want it.

  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result
  wm.setConnectTimeout(60);
  
  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("Solar Tracker","1sampai8"); // password protected ap

  if(!res) {
      Serial.println("Failed to connect");
      ESP.restart();
  } 
  else {
      //if you get here you have connected to the WiFi    
      Serial.println("connected...yeey :)");
      String str_ssid = wm.getWiFiSSID(false);
      int len = str_ssid.length() + 1;
      str_ssid.toCharArray(ssid,len);
      Serial.println(ssid);
      String str_pass = wm.getWiFiPass(false);
      len = str_pass.length() + 1;
      str_pass.toCharArray(pass,len);
      Serial.println(pass);
  }
}

/*
  Since VPot is READ_WRITE variable, onVPotChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onVPotChange()  {
  // Add your code here to act upon VPot change
  ledcWrite(pwmChannel1, vPot);
}

/*
  Since Switch1 is READ_WRITE variable, onSwitch1Change() is
  executed every time a new value is received from IoT Cloud.
*/
void onSwitch1Change()  {
  // Add your code here to act upon Switch1 change
  if(switch1){
    digitalWrite(load1pin, HIGH);
  }
  else{
    digitalWrite(load1pin, LOW);
  }
}

/*
  Since Switch2 is READ_WRITE variable, onSwitch2Change() is
  executed every time a new value is received from IoT Cloud.
*/
void onSwitch2Change()  {
  // Add your code here to act upon Switch2 change
   if(switch2){
    digitalWrite(load2pin, HIGH);
  }
  else{
    digitalWrite(load2pin, LOW);
  }
}


