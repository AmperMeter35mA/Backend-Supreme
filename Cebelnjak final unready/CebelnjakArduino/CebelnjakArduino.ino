
#include "DHT.h"

DHT dht(4, DHT11);

//https://www.youtube.com/watch?v=pSFmwoTtVJM
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 3
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

const int arrSize = 26;
char arr[arrSize];
unsigned long timeNow = 0;
unsigned long timeBefore = 0;

void setup()
{
  //temperature1
  sensors.begin();

  //temperature2
  dht.begin();

  Serial.begin(9600);
  arr[21] = '\0';
  timeNow = millis();

  /*char a = 'd';
  while(a != 'a')
  {
    if(Serial.available())
    {
      digitalWrite(2, HIGH);
      a = (char)Serial.read();
    }
  }
  Serial.write('b');// + (char)3 + (char)3//*/
}

void getInputs(char* arr);
void loop()
{
  
  if(timeNow - timeBefore >= 1000)
  {
    timeBefore = timeNow;

    getInputs(arr);

    for(int i = 0; i < arrSize - 1; i++)
    {
      Serial.write(arr[i]);//write//print
    }
  }

  timeNow = millis();
}

char getTemp();
char getTemp2();
int getLight();
int getRain();
int getMoist();
int getSound();
char getWeight();

int soundAvg = 0;
int counter = 1;
void getInputs(char* arr)
{
  arr[0] = (char)1;
  arr[1] = (char)2;
  arr[2] = (char)3;

  arr[3] = getTemp();
  arr[4] = getTemp2();//getTemp2();//implement

  int l = getLight();
  arr[5] = (char)l;
  arr[6] = (char)(l >> 8);
  arr[7] = (char)(l >> 16);
  arr[8] = (char)(l >> 24);

  int n = getRain();
  arr[9] = (char)n;
  arr[10] = (char)(n >> 8);
  arr[11] = (char)(n >> 16);
  arr[12] = (char)(n >> 24);

  int m = getMoist();
  arr[13] = (char)m;
  arr[14] = (char)(m >> 8);
  arr[15] = (char)(m >> 16);
  arr[16] = (char)(m >> 24);

  int s = getSound();

  soundAvg += s;
  s = soundAvg / counter++;

  arr[17] = (char)s;
  arr[18] = (char)(s >> 8);
  arr[19] = (char)(s >> 16);
  arr[20] = (char)(s >> 24);

  if(counter >= 60)
  {
    counter = 1;
    soundAvg = 0;
  }

  arr[21] = getWeight();

  arr[22] = (char)4;
  arr[23] = (char)5;
  arr[24] = (char)6;
  arr[25] = '\0';
}

char getTemp()
{
  sensors.requestTemperatures();
  int temp = sensors.getTempCByIndex(0);

  return (char)temp;
}

char getTemp2()
{
  return (char)dht.readTemperature();
}

int getLight()
{
  return analogRead(A3);//random(20, 30)//counter//2//digitalRead(4)
}

int getRain()
{
  return analogRead(A2);//random(0, 100)//counter//3//analogRead(A2)
}

int getMoist()
{
  return (int)dht.readHumidity();
}

int getSound()
{
  return analogRead(A4);
}

char getWeight()
{
  return (char)29;//random(20, 30);
}


/*
void getInputs(char* arr)
{
  arr[0] = (char)1;
  arr[1] = (char)2;
  arr[2] = (char)3;

  arr[3] = getTemp();
  arr[4] = arr[3] + 2;//getTemp2();

  
  arr[5] = getLight();

  int n = getRain();
  arr[6] = (char)n;
  arr[7] = (char)(n >> 8);
  arr[8] = (char)(n >> 16);
  arr[9] = (char)(n >> 24);

  arr[10] = getWeight();

  arr[11] = (char)4;
  arr[12] = (char)5;
  arr[13] = (char)6;
  arr[14] = '\0';
}
*/




