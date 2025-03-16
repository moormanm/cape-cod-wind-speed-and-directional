#include "arduino_secrets.h"

char apiKey[] = SECRET_apiKey; // openweather.org api key

double latitude = SECRET_latitude;
double longitude = SECRET_longitude;

// Dependencies:
// ArduinoJson

#include <WiFi.h>
#include <ArduinoJson.h>


// Map each pin to the directional indicator
// Needs a 220K ohm resistor for each, with mains power
#define N_D 5
#define NE_D 2
#define E_D 3
#define SE_D 9
#define S_D 8
#define SW_D 6
#define W_D 7
#define NW_D 4

// Map one pin to the wind speed indicator
// needs a 41K ohm resistor
#define WIND_SPEED_PIN 10


char server[] = "api.openweathermap.org";    // name address for Google (using DNS)
WiFiClient client;
int status = WL_IDLE_STATUS;


struct WeatherData {
  double windSpeed;
  int direction;
};

void debug(const char* str) {
  Serial.print(str);
}

void debugln(const char* str) {
  Serial.println(str);
}

int degToDir(double degrees) {
    if(degrees < 0 || degrees > 360) {
      debugln("cant parse degrees");
      return -1;
    }

    // Determine the direction
    if (degrees >= 337.5 || degrees < 22.5) return N_D;
    if (degrees >= 22.5 && degrees < 67.5) return NE_D;
    if (degrees >= 67.5 && degrees < 112.5) return E_D;
    if (degrees >= 112.5 && degrees < 157.5) return SE_D;
    if (degrees >= 157.5 && degrees < 202.5) return S_D;
    if (degrees >= 202.5 && degrees < 247.5) return SW_D;
    if (degrees >= 247.5 && degrees < 292.5) return W_D;
    if (degrees >= 292.5 && degrees < 337.5) return NW_D;

    return -1; // Should never reach here
}

WeatherData fetchWeatherData() {
  WeatherData ret = WeatherData();
  if (!client.connect(server, 80)) {
    ret.windSpeed = -1;
    return ret;
  }

  
  
  // Make a HTTP request:
  char buf[8192];
  snprintf(buf, 512, "GET /data/2.5/weather?lat=%f&lon=%f&appid=%s HTTP/1.1", latitude, longitude, apiKey);
  debug(buf);

  client.println(buf);
  client.println("Host: api.openweathermap.org");
  client.println("Connection: close");
  client.println();

  uint32_t received_data_num = 0;

  memset(buf, 0, 8192);


  int i =0;
  while(client.connected()) {
    while (client.available()) {
      char c = client.read();
      buf[i++] = c;
      if(i == 8191) {
        debugln("response too big");
        ret.windSpeed = -1;
        return ret;
      }
    }   
  }
  
  if(!strstr(buf, "HTTP/1.1 200 OK")) {
        debugln("Not a 200 response");
        ret.windSpeed = -1;
        return ret;
  }

  const char* body = strstr(buf, "\r\n\r\n");
  if(!body) {
        debugln("could not find body");
        ret.windSpeed = -1;
        return ret;
  }

  const char* json = body + 4;
  debugln("Body:");
  debugln(json);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if(error) {
    debugln(error.c_str());
    ret.windSpeed = -1;
    return ret;
  }
  char db[255];

  double windSpeed =  doc["wind"]["gust"];
  double windDeg = doc["wind"]["deg"];
  

  snprintf(db, 255, "got speed(gust): %f - deg: %f", windSpeed, windDeg);

  debug(db);
  
  
  ret.windSpeed = windSpeed;
  ret.direction = degToDir(windDeg);  
  return ret;
}



bool connectToWifi(char* ssid, char* pass) {
  
  // attempt to connect to WiFi network:
  int try_number = 0;
  while (status != WL_CONNECTED) {
    try_number++;
    debug("Attempting to connect to SSID: ");
    debug(ssid);
    debugln("");
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
     
    // wait 10 seconds for connection:
    delay(10000);
    
    if(try_number == 2) {
     toggleOne(N_D);
     toggleOne(S_D);
     toggleOne(N_D);
     toggleOne(S_D);
     delay(1000);
      return false;
    }
  }
  
  
  if(fetchWeatherData().windSpeed == -1) {
     toggleOne(W_D);
     toggleOne(E_D);
     toggleOne(W_D);
     toggleOne(E_D);
     delay(1000);
     return false;
  }
  return true;
  
}

void writeWindSpeed(double mph) {
  analogWrite(WIND_SPEED_PIN, int(max(mph,0) * 2.55 * 1.125));
}
void writeWindDir(int dir) {
  if(dir >= 0) {
    digitalWrite(dir, HIGH);
  } 
}

void writeAllDirections(PinStatus val) {
  digitalWrite(N_D, val);
  digitalWrite(NE_D, val);
  digitalWrite(E_D, val);
  digitalWrite(SE_D, val);
  digitalWrite(S_D, val);
  digitalWrite(SW_D, val);
  digitalWrite(W_D, val);
  digitalWrite(NW_D, val);
}

void oneStartupIteration() {
  
  writeWindSpeed(10);
  toggleOne(N_D);
  toggleOne(NE_D);
  writeWindSpeed(20);
  toggleOne(E_D);
  toggleOne(SE_D);
  writeWindSpeed(30);
  toggleOne(S_D);
  toggleOne(SW_D);
  writeWindSpeed(40);
  toggleOne(W_D);
  toggleOne(NW_D);
  toggleOne(N_D);
  writeWindSpeed(50);
  delay(4000);
  writeWindSpeed(0);
}


void toggleOne(int pin) {
  digitalWrite(pin, HIGH);
  delay(100);
  digitalWrite(pin, LOW);
  delay(100);
}


void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  randomSeed(analogRead(0));
  pinMode(N_D, OUTPUT);
  pinMode(NE_D, OUTPUT);
  pinMode(E_D, OUTPUT);
  pinMode(SE_D, OUTPUT);
  pinMode(S_D, OUTPUT);
  pinMode(SW_D, OUTPUT);
  pinMode(W_D, OUTPUT);
  pinMode(NW_D, OUTPUT);
  pinMode(WIND_SPEED_PIN, OUTPUT);

  writeAllDirections(LOW);
  oneStartupIteration();


  while(true) {
    if(connectToWifi(SECRET_ssid_1, SECRET_pass_1)) {
      break;
    }
    if(connectToWifi(SECRET_ssid_2, SECRET_pass_2)) {
      break;
    }
    delay(5000);
  }
  

  
  
}

void loop() {
  
   WeatherData wd = fetchWeatherData();
   writeAllDirections(LOW);
   writeWindSpeed(0);
   if(!(wd.windSpeed == -1 || wd.direction == -1)) {
     writeWindSpeed(wd.windSpeed);
     writeWindDir(wd.direction);
   }
   else {
      //Failure condition -- try again later
      toggleOne(N_D);
      toggleOne(E_D);
      toggleOne(S_D);
      toggleOne(W_D);
      delay(10000); 
   }
   delay(10000);
   
}
