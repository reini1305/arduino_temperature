#include <TimerOne.h>

#include <LiquidCrystal.h>

#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 7 on the Arduino
#define ONE_WIRE_BUS 7
#define TEMPERATURE_PRECISION 12
#define MAX_SENSORS 5

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress thermometer[MAX_SENSORS];

int8_t num_sensors;

float curr_mean_temp;
float curr_std_temp;
float min_temp;
float max_temp;
bool show_current;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(9, 8, 13, 12, 11, 10);
#define PM         0
#define HEART      1
#define ARROW_UP   2
#define ARROW_DOWN 3

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
  float tempC = sensors.getTempC(deviceAddress);
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
  curr_std_temp/=num_sensors;
  curr_std_temp = sqrt(curr_std_temp);
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
    lcd.print("Temp.(");
    lcd.print(num_sensors);
    lcd.print(" sensors)");
  }
  lcd.setCursor(0, 1);
  if(show_current)
  {
    lcd.print(curr_mean_temp);
    lcd.print((char)0xDF);
    lcd.print("C ");
    lcd.write((byte)PM);
    lcd.print(" ");
    lcd.print(curr_std_temp);
  }
  else
  {
    lcd.write((byte)ARROW_DOWN);
    lcd.print(min_temp);
    lcd.print((char)0xDF);
    lcd.print(" ");
    lcd.write((byte)ARROW_UP);
    lcd.print(max_temp);
    lcd.print((char)0xDF);
  }
  show_current=!show_current;
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

  // assign address manually.  the addresses below will beed to be changed
  // to valid device addresses on your bus.  device address can be retrieved
  // by using either oneWire.search(deviceAddress) or individually via
  // sensors.getAddress(deviceAddress, index)
  //insideThermometer = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };
  //outsideThermometer   = { 0x28, 0x3F, 0x1C, 0x31, 0x2, 0x0, 0x0, 0x2 };

  // search for devices on the bus and assign based on an index.  ideally,
  // you would do this to initially discover addresses on the bus and then 
  // use those addresses and manually assign them (see above) once you know 
  // the devices on your bus (and assuming they don't change).
  // 
  // method 1: by index
  for (uint8_t i=0;i<num_sensors;i++) {
    sensors.getAddress(thermometer[i],i);
    //printAddress(thermometer[i]);
    // set the resolution to 12 bit
    sensors.setResolution(thermometer[i], TEMPERATURE_PRECISION);
  }
  show_current = true;
  max_temp = -1000;
  min_temp = 1000;
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
      for (uint8_t i = 0; i < num_sensors; i++)
      {
        printTemperature(thermometer[i]);
      }
    }
    else
    {
      calcMeanTemperature();
      printMeanTemperature();
    }
  }

}


