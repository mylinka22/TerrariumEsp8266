#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const char* ssid = "MI";
const char* password = "1285012850";

ESP8266WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ru.pool.ntp.org", 10800, 60000); // Смещение 3 часа (10800 секунд) для Москвы

const int numRelays = 4;
const int relayPins[numRelays] = {14, 12, 13, 15};
bool relayStates[numRelays] = {false, false, false, false};
int currentHour;
int currentMinute;

struct RelaySchedule {
  int id;
  int Hour;
  int Minute;
  boolean onof;
};

RelaySchedule *alarms = new RelaySchedule[10];
int idalarm = 0;


void handleRoot() {
  String html = "<html><head><meta charset='UTF-8'>";
  html += "<link rel='stylesheet' href='style.css'>";
  html += "</head><body>";
  html += "<h1>Управление реле</h1>";
  for (int i = 0; i < numRelays; i++) {
    html += "<p>Реле " + String(i) + ": ";
    if (relayStates[i]) {
      html += "<a href='/of" + String(i) + "'><button>Выключить</button></a></p>";
    } else {
      html += "<a href='/on" + String(i) + "'><button>Включить</button></a></p>";
    }
  }
  html += "<p><form method='get' action='/settime'>Время <input type='text' name='id' placeholder='Номер'> - <input type='text' name='Hour' placeholder='Часы'>:<input type='text' name='Minute' placeholder='Минуты'> - <input type='text' name='onof' placeholder='Состояние'><br>";
  //html += "Время выключения: <input type='text' name='offHour' placeholder='Часы'>:<input type='text' name='offMinute' placeholder='Минуты'> - <input type='text' name='id' placeholder='Номер'><br>";
  html += "<input type='submit' value='Установить'></form></p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}


void handleSetTime() {
  if (server.args() > 0) {
    alarms[idalarm].id = server.hasArg("id") ? server.arg("id").toInt() : 1;
    alarms[idalarm].Hour = server.hasArg("Hour") ? server.arg("Hour").toInt() : -1;
    alarms[idalarm].Minute = server.hasArg("Minute") ? server.arg("Minute").toInt() : -1;
    alarms[idalarm].onof = server.hasArg("onof") ? server.arg("onof").toInt() : 1;

    // Serial.print("Серв ");
    // Serial.print(alarms[idalarm].id);
    // Serial.print(": ");
    // Serial.println(alarms[idalarm].Hour);

    idalarm++;
  }

  for (int i = 0; i < idalarm; i++){
    Serial.print("Серв ");
    Serial.print(alarms[i].id);
    Serial.print(" - ");
    Serial.print(alarms[i].Hour);
    Serial.print(": ");
    Serial.print(alarms[i].Minute);
    Serial.print(" - ");
    Serial.println(alarms[i].onof);
  }
  Serial.println();

  handleRoot();
}


void handleRelay() {
  String req = server.uri();
  int relayIndex = req.substring(3).toInt();
  Serial.println(relayIndex);

  if (relayIndex >= 0 && relayIndex < numRelays) {
    if (req.startsWith("/on"))
      relayStates[relayIndex] = true;
    else if (req.startsWith("/of"))
      relayStates[relayIndex] = false;

    digitalWrite(relayPins[relayIndex], relayStates[relayIndex]);
    
    Serial.print("Реле ");
    Serial.print(relayPins[relayIndex]);
    Serial.print(":");
    Serial.println(relayStates[relayIndex] ? "Включено" : "Выключено");
  }

  handleRoot();
}


void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Подключение к Wi-Fi...");
  }
  
  Serial.println("Подключено к Wi-Fi");
  Serial.print("IP-адрес: ");
  Serial.println(WiFi.localIP()); // Выводим IP-адрес в монитор последовательного порта

  // Настройка пинов реле как выходов
  for (int i = 0; i < numRelays; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);  // Начальное состояние - выключено
  }


  server.on("/style.css", HTTP_GET, [](){
    String style = R"=====(

      * {
        font-family: Arial;
        padding: 10px;
        text-align: center;
        font-size: 16px;
        margin: 4px 2px;
      }

      button {
        cursor: pointer;
        border-radius: 4px;
      }

      #offBtn {
        background-color: #f44336;
        color: white;
        border: none;
      }

      #onBtn {
        background-color: #4CAF50;
        color: white;
        border: none;
      }

      #submitBtn {
        background-color: #008CBA;
        color: white;
        padding: 10px 18px;
        border-radius: 4px;
      }


  
  )=====";
  server.send(200, "text/css", style);
  });

  server.on("/", handleRoot);
  server.on("/settime", handleSetTime);
  server.onNotFound(handleRelay);
  server.begin();

  timeClient.begin();
  
}


void loop() {
  server.handleClient();

  timeClient.update();
  currentHour = timeClient.getHours();
  currentMinute = timeClient.getMinutes();

  Serial.print(currentHour);
  Serial.print(":");
  Serial.println(currentMinute);

  for (int i = 0; i < idalarm; i++){
      
      if (currentHour == alarms[i].Hour && currentMinute == alarms[i].Minute){
        Serial.print("Y");
        relayStates[alarms[i].id] = alarms[i].onof;
        digitalWrite(relayPins[alarms[i].id], alarms[i].onof);
      }
  }


//
//
//  for (int i = 0; i < numRelays; i++) {
//    if (currentHour == onHour[i] && currentMinute == onMinute[i]){
//      relayStates[i] = true;
//      digitalWrite(relayPins[i], HIGH);
//    }
//    if (currentHour == offHour[i] && currentMinute == offMinute[i]) {
//      relayStates[i] = false;
//      digitalWrite(relayPins[i], HIGH);
//    }
//  }
//
//  // Можно добавить задержку между проверками времени для экономии ресурсов

    delay(3000);
}

