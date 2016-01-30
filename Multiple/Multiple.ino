#include <TimerOne.h>

#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 7 on the Arduino
#define ONE_WIRE_BUS 7
#define TEMPERATURE_PRECISION 12
#define MAX_SENSORS 5
#define EEPROM_RESET 450

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress thermometer[MAX_SENSORS];

int8_t num_sensors;

float cal_temp = -1.4f;
float curr_mean_temp;
float curr_std_temp;
float min_temp;
float max_temp;

int eeprom_counter=EEPROM_RESET; // one update per hour

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(9, 8, 13, 12, 11, 10);
#define PM         0
#define HEART      1
#define ARROW_UP   2
#define ARROW_DOWN 3
#define CIRCLE     4

byte pm[8] = {
  B00100,
  B00100,
  B11111,
  B00100,
  B00100,
  B00000,
  B11111,
};

byte heart[8] = {
  B00000,
  B01010,
  B11111,
  B11111,
  B11111,
  B01110,
  B00100,
};

byte arrow_up[8] = {
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00100,
  B00100,
};

byte arrow_down[8] = {
  B00100,
  B00100,
  B00100,
  B00100,
  B10101,
  B01110,
  B00100,
};

byte circle[8] = {
  B00000,
  B01110,
  B11111,
  B11111,
  B11111,
  B01110,
  B00000,
};

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress)+cal_temp;
  Serial.print("Temp C: ");
  Serial.println(tempC);
}

void calcMeanTemperature(void)
{
  curr_mean_temp = 0.0f;
  curr_std_temp = 0.0f;
  for (uint8_t i = 0; i < num_sensors; i++)
  {
    curr_mean_temp += sensors.getTempC(thermometer[i]);
  }
  curr_mean_temp/=num_sensors;
  for (uint8_t i = 0; i < num_sensors; i++)
  {
    curr_std_temp += (sensors.getTempC(thermometer[i]) - curr_mean_temp) * (sensors.getTempC(thermometer[i]) - curr_mean_temp);
  }
  curr_mean_temp+=cal_temp;
  curr_std_temp/=num_sensors;
  curr_std_temp = sqrt(curr_std_temp);
  if(curr_std_temp<0.2)
  {
    if(min_temp>curr_mean_temp)
      min_temp = curr_mean_temp;
    if(max_temp<curr_mean_temp)
      max_temp = curr_mean_temp;
  }
}

void printMeanTemperature(void)
{
  Serial.print("Temp C: ");
  Serial.print(curr_mean_temp);
  Serial.print(" +- ");
  Serial.println(curr_std_temp);
}

void refreshDisplay(void)
{
  // handle EEPROM first
  if(--eeprom_counter<=0) {
    EEPROM.put(0,min_temp);
    EEPROM.put(sizeof(float),max_temp);
    eeprom_counter=EEPROM_RESET;
  }
  lcd.setCursor(15, 1);
  lcd.write((byte)HEART);
  sensors.requestTemperatures();
  calcMeanTemperature();
  lcd.setCursor(15, 1);
  lcd.print(" ");
  lcd.setCursor(0,0);
  if(curr_mean_temp>35.0f)
  {
    lcd.print("Time to go home!");
  }
  else
  {
    lcd.write((byte)ARROW_DOWN);
    lcd.print(min_temp);
    lcd.print((char)0xDF);
    lcd.print("  ");
    lcd.print(max_temp);
    lcd.print((char)0xDF);
    lcd.write((byte)ARROW_UP);
  }
  lcd.setCursor(0, 1);
  lcd.write((byte)CIRCLE);
  lcd.print(curr_mean_temp);
  lcd.print((char)0xDF);
  lcd.print(" ");
  lcd.write((byte)PM);
  lcd.print(" ");
  lcd.print(curr_std_temp);
  lcd.print((char)0xDF);
}

void setup(void)
{
  // start serial port
  Serial.begin(9600);
  // Start up the library
  sensors.begin();

  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  num_sensors = sensors.getDeviceCount();
  Serial.print(num_sensors, DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  // method 1: by index
  for (uint8_t i=0;i<num_sensors;i++) {
    sensors.getAddress(thermometer[i],i);
    //printAddress(thermometer[i]);
    // set the resolution to 12 bit
    sensors.setResolution(thermometer[i], TEMPERATURE_PRECISION);
  }
  //max_temp = -1000;
  //min_temp = 1000;
  EEPROM.get(0,min_temp);
  EEPROM.get(sizeof(float),max_temp);
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Temp.(");
  lcd.print(num_sensors);
  lcd.print(" sensors)");
  lcd.createChar(PM,pm);
  lcd.createChar(HEART,heart);
  lcd.createChar(ARROW_UP,arrow_up);
  lcd.createChar(ARROW_DOWN, arrow_down);
  lcd.createChar(CIRCLE,circle);
  Timer1.initialize(8800000);
  Timer1.attachInterrupt(refreshDisplay);
}

void loop(void)
{ 
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  //Serial.print("Requesting temperatures...");

  if (Serial.available() > 0)
  {
    // read the incoming byte:
    uint8_t incomingByte = Serial.read();
    sensors.requestTemperatures();
    if(incomingByte == 'r')
    {
      // reset min, max temperature
      max_temp = -1000;
      min_temp = 1000;
      calcMeanTemperature();
      EEPROM.put(0,min_temp);
      EEPROM.put(sizeof(float),max_temp);
      eeprom_counter=EEPROM_RESET;
    }
    else
    {
      calcMeanTemperature();
      printMeanTemperature();
    }
  }

}


