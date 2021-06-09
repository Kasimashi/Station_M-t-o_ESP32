    /////////////////////////////////////////////////////////////////
   //         ESP32 Weather Station Project     v1.00             //
  //       Get the latest version of the code here:              //
 //         http://educ8s.tv/esp32-weather-station              //
/////////////////////////////////////////////////////////////////


#define ARDUINOJSON_ENABLE_PROGMEM 0

#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>    //https://github.com/bblanchon/ArduinoJson
#include <WiFi.h>

const char* ssid     = "aaaaaaaa";
const char* password = "aaaaaa";
unsigned int CityIDs[4] = {2984701,2984701,3022242,2997465}; //Quimper,Quimper,Crozon,Loudéac
String APIKEY = "aa";
#define ALTITUDE 30.0 // Altitude in Sparta, Greece

#define I2C_SDA 21
#define I2C_SCL 22

#define SEALEVELPRESSURE_HPA (1013.25)

#define LED_PIN 2
#define BME280_ADDRESS 0x76  //If the sensor does not work, try the 0x77 address as well

char city[10];
float temperature = 0;
float humidity = 0;
float pressure = 0;
int winddirection = 0;
char stringwinddirection[2];
int windspeed = 0;
int weatherID = 0;

TwoWire I2CBME = TwoWire(0);
Adafruit_BME280 bme;

WiFiClient client;
char* servername ="api.openweathermap.org";  // remote server we will connect to
String result;

int  iterations = 1800;
unsigned int  current_city = 0;
String weatherDescription ="";
String weatherLocation = "";

void setup() 
{
  pinMode(LED_PIN, OUTPUT);
  
  Serial.begin(9600);

  initSensor();

  connectToWifi();
}

void loop() {
 
 delay(2000);
 
 if(iterations > 30)//We check for updated weather forecast once every min
 {
   current_city++;
   if (current_city == 4){
    current_city =0;
   }
   getWeatherData();
   printWeatherIcon(weatherID);
   iterations = 0;
   
 }

 getCity();
 sendCityToNextion();

 getWind();
 sendWindToNextion();
 
 getTemperature();
 sendTemperatureToNextion();
 
 getHumidity();
 sendHumidityToNextion();
 
 getPressure();
 sendPressureToNextion();

 iterations++;
 
 blinkLED();
}

void connectToWifi()
{
  WiFi.enableSTA(true);
  
  delay(2000);

  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
}

void initSensor()
{
  bool status = bme.begin(BME280_ADDRESS, &I2CBME);
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
}

void blinkLED()
{
  digitalWrite(LED_PIN,HIGH);
  delay(100);
  digitalWrite(LED_PIN,LOW);
}

void getCity()
{
  switch(CityIDs[current_city]){
    case 2984701:
      if (current_city == 0){
        strncpy(city,"Home",sizeof(city));
      }else{
        strncpy(city,"Quimper",sizeof(city));
      }
      break;
    case 3022242:
      strncpy(city,"Crozon",sizeof(city));
      break;
    case 2997465:
      strncpy(city,"Loudeac",sizeof(city));
      break;
    default:
      strncpy(city,"Home",sizeof(city));
      break;
  }
  
}

void getWindDirection(int value){
  if (value >= 337 || value <= 22)   strncpy(stringwinddirection,"N",2);
  else if (value >= 22 && value <= 68)   strncpy(stringwinddirection,"NE",2);
  else if (value >= 68 && value <= 112)   strncpy(stringwinddirection,"E ",2);
  else if (value >= 112 && value <= 157)   strncpy(stringwinddirection,"SE ",2);
  else if (value >= 157 && value <= 202)   strncpy(stringwinddirection,"S ",2);
  else if (value >= 202 && value <= 247) strncpy(stringwinddirection,"SW ",2);
  else if (value >= 247 && value <= 292) strncpy(stringwinddirection,"W ",2);
  else if (value >= 292 && value <= 337) strncpy(stringwinddirection,"NW ",2);
}

float getWind()
{
  if (current_city == 0)
  { //If Home or Quimper
    windspeed = 0;
    winddirection = 0;
    stringwinddirection[0] = 0;
    stringwinddirection[1] = 1;
  }
}

float getTemperature()
{
  if (current_city == 0)
  { //If Home or Quimper
    temperature = bme.readTemperature();
  }
}

float getHumidity()
{
  if (current_city == 0)
  { //If Home or Quimper
    humidity = bme.readHumidity();
  }
}

float getPressure()
{
  if (current_city == 0)
  { //If Home or Quimper
    pressure = bme.readPressure();
    pressure = bme.seaLevelForAltitude(ALTITUDE,pressure);
    pressure = pressure/100.0F;
  }
}

void getWeatherData() //client function to send/receive GET request data.
{
  String result ="";
  WiFiClient client;
  const int httpPort = 80;
  String CityID = String(CityIDs[current_city]);
  if (!client.connect(servername, httpPort)) {
        return;
    }
      // We now create a URI for the request
    String url = "/data/2.5/forecast?id="+CityID+"&units=metric&cnt=1&APPID="+APIKEY;

       // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + servername + "\r\n" +
                 "Connection: close\r\n\r\n");
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            client.stop();
            return;
        }
    }

    // Read all the lines of the reply from server
    while(client.available()) {
        result = client.readStringUntil('\r');
    }

result.replace('[', ' ');
result.replace(']', ' ');

char jsonArray [result.length()+1];
result.toCharArray(jsonArray,sizeof(jsonArray));
jsonArray[result.length() + 1] = '\0';

StaticJsonBuffer<1024> json_buf;
JsonObject &root = json_buf.parseObject(jsonArray);
if (!root.success())
{
  Serial.println("parseObject() failed");
}

String location = root["city"]["name"];
String weather = root["list"]["weather"]["main"];
String description = root["list"]["weather"]["description"];
String idString = root["list"]["weather"]["id"];
String timeS = root["list"]["dt_txt"];

temperature = root["list"]["main"]["temp"];
humidity = root["list"]["main"]["humidity"];
pressure = root["list"]["main"]["pressure"];
getWindDirection((int)(root["list"]["wind"]["deg"]));
windspeed = (int)(((float)(root["list"]["wind"]["speed"])) * 1.94);//Convert m/s to kt
//http://api.openweathermap.org/data/2.5/forecast?id=2984701&units=metric&cnt=1&APPID=fd816d77a7e7462a295a3abf2a22a03a
weatherID = idString.toInt();
Serial.print("\nWeatherID: ");
Serial.print(weatherID);
endNextionCommand(); //We need that in order the nextion to recognise the first command after the serial print

}

void showConnectingIcon()
{
  Serial.println();
  String command = "weatherIcon.pic=3";
  Serial.print(command);
  endNextionCommand();
}

void sendCityToNextion()
{
  String command = "city.txt=\""+String(city)+"\"";
  Serial.print(command);
  endNextionCommand();
}

void sendWindToNextion()
{
  String command ;
  command = "winddirection.txt=\""+String(stringwinddirection)+"\"";
  Serial.print(command);
  endNextionCommand();

  command = "windspeed.txt=\""+String(windspeed)+"\"";
  Serial.print(command);
  endNextionCommand();
}

void sendHumidityToNextion()
{
  String command = "humidity.txt=\""+String(humidity,1)+"\"";
  Serial.print(command);
  endNextionCommand();
}

void sendTemperatureToNextion()
{
  String command = "temperature.txt=\""+String(temperature,1)+"\"";
  Serial.print(command);
  endNextionCommand();
}

void sendPressureToNextion()
{
  String command = "pressure.txt=\""+String(pressure,1)+"\"";
  Serial.print(command);
  endNextionCommand();
}

void endNextionCommand()
{
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}

void printWeatherIcon(int id)
{
 switch(id)
 {
  case 800: drawClearWeather(); break;
  case 801: drawFewClouds(); break;
  case 802: drawFewClouds(); break;
  case 803: drawCloud(); break;
  case 804: drawCloud(); break;
  
  case 200: drawThunderstorm(); break;
  case 201: drawThunderstorm(); break;
  case 202: drawThunderstorm(); break;
  case 210: drawThunderstorm(); break;
  case 211: drawThunderstorm(); break;
  case 212: drawThunderstorm(); break;
  case 221: drawThunderstorm(); break;
  case 230: drawThunderstorm(); break;
  case 231: drawThunderstorm(); break;
  case 232: drawThunderstorm(); break;

  case 300: drawLightRain(); break;
  case 301: drawLightRain(); break;
  case 302: drawLightRain(); break;
  case 310: drawLightRain(); break;
  case 311: drawLightRain(); break;
  case 312: drawLightRain(); break;
  case 313: drawLightRain(); break;
  case 314: drawLightRain(); break;
  case 321: drawLightRain(); break;

  case 500: drawLightRainWithSunOrMoon(); break;
  case 501: drawLightRainWithSunOrMoon(); break;
  case 502: drawLightRainWithSunOrMoon(); break;
  case 503: drawLightRainWithSunOrMoon(); break;
  case 504: drawLightRainWithSunOrMoon(); break;
  case 511: drawLightRain(); break;
  case 520: drawModerateRain(); break;
  case 521: drawModerateRain(); break;
  case 522: drawHeavyRain(); break;
  case 531: drawHeavyRain(); break;

  case 600: drawLightSnowfall(); break;
  case 601: drawModerateSnowfall(); break;
  case 602: drawHeavySnowfall(); break;
  case 611: drawLightSnowfall(); break;
  case 612: drawLightSnowfall(); break;
  case 615: drawLightSnowfall(); break;
  case 616: drawLightSnowfall(); break;
  case 620: drawLightSnowfall(); break;
  case 621: drawModerateSnowfall(); break;
  case 622: drawHeavySnowfall(); break;

  case 701: drawFog(); break;
  case 711: drawFog(); break;
  case 721: drawFog(); break;
  case 731: drawFog(); break;
  case 741: drawFog(); break;
  case 751: drawFog(); break;
  case 761: drawFog(); break;
  case 762: drawFog(); break;
  case 771: drawFog(); break;
  case 781: drawFog(); break;

  default:break; 
 }
}

void drawFog()
{
  String command = "weatherIcon.pic=13";
  Serial.print(command);
  endNextionCommand();
}

void drawHeavySnowfall()
{
  String command = "weatherIcon.pic=8";
  Serial.print(command);
  endNextionCommand();
}

void drawModerateSnowfall()
{
  String command = "weatherIcon.pic=8";
  Serial.print(command);
  endNextionCommand();
}

void drawLightSnowfall()
{
  String command = "weatherIcon.pic=11";
  Serial.print(command);
  endNextionCommand();
}

void drawHeavyRain()
{
  String command = "weatherIcon.pic=10";
  Serial.print(command);
  endNextionCommand();
}

void drawModerateRain()
{
  String command = "weatherIcon.pic=6";
  Serial.print(command);
  endNextionCommand();
}

void drawLightRain()
{
  String command = "weatherIcon.pic=6";
  Serial.print(command);
  endNextionCommand();
}

void drawLightRainWithSunOrMoon()
{
  String command = "weatherIcon.pic=7";
  Serial.print(command);
  endNextionCommand(); 
}
void drawThunderstorm()
{
  String command = "weatherIcon.pic=3";
  Serial.print(command);
  endNextionCommand();
}

void drawClearWeather()
{
  String command = "weatherIcon.pic=4";
  Serial.print(command);
  endNextionCommand();
}

void drawCloud()
{
  String command = "weatherIcon.pic=9";
  Serial.print(command);
  endNextionCommand();
}

void drawFewClouds()
{
  String command = "weatherIcon.pic=5";
  Serial.print(command);
  endNextionCommand(); 
}
