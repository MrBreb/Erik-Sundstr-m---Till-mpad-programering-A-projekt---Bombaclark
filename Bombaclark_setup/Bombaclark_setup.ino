#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>
#include <WebServer.h>
#include <queue> 

/**
* @file Bombaclark_setup.ino
* @brief Huvudfil för initiering av nätverksfunktioner och variabler
* @author Erik Sundström
* @date 2026-05-21
* Beskrivning: 
* Denna kod fungerar som bombaclarks initierings sekvens. Koden ansvarar
* för att konfigurera variabler och initiera de grundliga systemen som krävs för
* kommunikation mellan mikorkontrollern och AI-modellen. Användaren måste själv skriva
* in ditt nätverks SSID och Lösenord, sedan sätter koden upp en webbserver och en 
* kommandokö (std::queue).
* 
* Hårdvarukrav & I/O:
* - ESP32 mikrokontroller.
* - I2C-kommunikation initieras på pin 6 (SDA) och pin 7 (SCL) för att 
*   ansluta till PCA9685 servodrivern.
* - Seriell kommunikation sätts till 115200 baud för felsökning.
*/

const char* ssid = "Slerib";        ///< Ditt nätverks SSID
const char* password = "Bajsmacka"; ///< Ditt nätverks lösenord

WebServer server(80);
std::queue<String> commandQueue;  
bool isBusy = false;              

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Ställer in min- och max värdena för servon och PWM frekvensen åt PCA-drivern
#define SERVOMIN 150
#define SERVOMAX 500
#define SERVO_FREQ 50

// =============================================================================
// Olika förinställda och kalibreringsbara vinklar. mellan 0 och 180
// =============================================================================
int front_pos = 150;    ///< Höftvinkel för att flytta benen frammåt
int back_pos = 30;      ///< Höftvinkel för att flytta benen bakåt
int out_pos = 120;      ///< Höftvinkel för sidosteg utåt (Bort från kroppen i sidled)
int in_pos = 140;       ///< Höftvinkel för sidosteg inåt (Mot kroppen i sidled)
int up_pos = 60;        ///< Hur högt benet ska lyftas när det tar ett steg (Är låg för att ge bättre stabilitet)
int down_pos = 40;      ///< Hur lågt benet ska vara när det är i kontakt med marken
int neutral_pos = 90;   ///< Centrerad utgångspunkt för alla servon

// =============================================================================
// Olika förinställda och kallibreringsbara delayer (I millisekunder)
// =============================================================================
int base_delay = 100;   ///< Kort paus
int medium_delay = 300; ///< Medel paus
int big_delay = 500;    ///< Lång paus

/*
  @brief handle_command sparar och hanterar inskickade kommandon
  @note Denna funktionen sparar inskickade kommandon till en kö
  där de kan köras en och en, detta hindrar koden från att köra flera 
  kommandon direkt eller från att hoppa över vissa komandon
*/
void handle_command() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    Serial.println("Kommandot är mottaget: " + cmd);
    commandQueue.push(cmd);

    server.send(200, "text/plain", "Tillagd i kö");
  } else {
    server.send(200, "text/plain", "Inget kommando hittades");
  }
}

void idle();

/*
  @brief setup initierar Bombaclarks hårdvarukomponenter, nätverksanslutning och webbserver
  @note setup körs en gång vid kodens uppstartnig och utför föjlande steg
  - Startar seriell kommunikation och initierar pin 6 och 7 som I2C pins (För att skicka information till PCA9685:an)
  - Ansluter till det definerade nätverket och väntar tills anslutningen upptättats
  - Startar själva webbservern och mappar endpointen "/command" till handle_command
  - Sätter rätt oscilationsfrekvens och PWM-frekvens (50Hz) och initierar PCA-drivern
  Till sist körs en delay på 2 sekunder för att ge elektroniken tid att stabiliseras. Denna delayen är väldigt 
  viktig och om den tas bort kan det leda till att ESP32:an hamnar i en brownout loop.
  Notera att allting från och med "WiFi.begin(ssid, password);" till och med "server.begin();" 
  bör kommenteras ut när man kör manuella tester utan wifi
 */
void setup() {  
  Serial.begin(115200);
  Wire.begin(6, 7);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }

  Serial.println("\nAnsluten! IP-adress: ");
  Serial.println(WiFi.localIP());
  server.on("/command", handle_command); ///< Denna koden registrerar en så kallad "callback funktion" på webbservern för att hantera inkommande API-anrop
  server.begin();

  //Serial.println("Redo för testing!"); // Denna behöver endast vara igång när man kör manuella tester och kalibrerar för att se att koden är redo

  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);

  delay(2000);
}