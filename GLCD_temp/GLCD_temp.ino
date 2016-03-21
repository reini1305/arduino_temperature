#include <openGLCD.h>

#include <TimerOne.h>

#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 8 on the Arduino
#define ONE_WIRE_BUS 10

#define TEMPERATURE_PRECISION 12
#define MAX_SENSORS 5
#define EEPROM_RESET 450
#define HISTORY_RESET 1
#define SIZE_HISTORY 100

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
float temp_history[SIZE_HISTORY];
unsigned int curr_idx=0;
unsigned int history_counter=HISTORY_RESET;

int eeprom_counter=EEPROM_RESET; // one update per hour
gText textTemp = gText(0,0,GLCD.Right-10,GLCD.Bottom);
gText textMax = gText(0,0,GLCD.Right-10,18);
gText textMin = gText(0,GLCD.Bottom-10,GLCD.Right-10,GLCD.Bottom);
char temp_buffer[10],max_buffer[10],min_buffer[10];
bool draw_temp = true;

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

void refreshEEPROM(void)
{
  if(--eeprom_counter<=0) {
    EEPROM.put(0,min_temp);
    EEPROM.put(sizeof(float),max_temp);
    eeprom_counter=EEPROM_RESET;
  }
}

void updateTemperatureHistory(void)
{
  if(--history_counter==0) {
    temp_history[curr_idx] = curr_mean_temp;
    curr_idx = (curr_idx + 1) % SIZE_HISTORY;
    history_counter=HISTORY_RESET;
  }
}

void refreshDisplay(void)
{
  refreshEEPROM();
  sensors.requestTemperatures();
  //calcMeanTemperature();
  updateTemperatureHistory();

  GLCD.ClearScreen();
  // Draw thermometer
  GLCD.DrawCircle(128-6,64-6,5);
  GLCD.DrawCircle(128-6,4,3);
  GLCD.FillRect(128-8,6,5,64-10,PIXEL_OFF);
  GLCD.DrawLine(128-9,6,128-9,64-10);
  GLCD.DrawLine(128-3,6,128-3,64-10);

  // Fake temperatures for now
  min_temp = 18.0;
  max_temp = 27.0;
  curr_mean_temp = random(18, 27);
  GLCD.DrawVBarGraph(GLCD.Right-6, GLCD.Bottom-6, 3, -(GLCD.Height-10), 0, min_temp*10, max_temp*10, curr_mean_temp*10);

 if(draw_temp) {
    // Update min/max values
    sprintf(temp_buffer,"%d.%d C",(int)curr_mean_temp,(int)(curr_mean_temp*10.f)-((int)curr_mean_temp)*10);
    textTemp.DrawString(temp_buffer,gTextfmt_center, gTextfmt_center);
    sprintf(max_buffer,"%d.%d C",(int)max_temp,(int)(max_temp*10.f)-((int)max_temp)*10);
    textMax.DrawString(max_buffer,gTextfmt_center, gTextfmt_center);
    sprintf(min_buffer,"%d.%d C",(int)min_temp,(int)(min_temp*10.f)-((int)min_temp)*10);
    textMin.DrawString(min_buffer,gTextfmt_center, gTextfmt_center);
 } else {
   // draw the history
   for (int i=1;i<SIZE_HISTORY;i++) {
    if(temp_history[(curr_idx+i)%SIZE_HISTORY]>0) {
      int y=(temp_history[(curr_idx+i)%SIZE_HISTORY]-min_temp)/((max_temp-min_temp)/64);
      GLCD.DrawLine(i,GLCD.Bottom,i,GLCD.Bottom-y);
    }
   }
 }

  draw_temp=!draw_temp;
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
  curr_idx = SIZE_HISTORY-1;
  for (int i=0;i<SIZE_HISTORY;i++) {
    temp_history[i] = 0;
  }

  GLCD.Init();
  textTemp.SelectFont(Cooper26);
  textMin.SelectFont(Cooper19);
  textMax.SelectFont(Cooper19);
  randomSeed(analogRead(5));
  
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


