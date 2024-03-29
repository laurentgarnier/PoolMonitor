/**************************************************/
/*****       GESTION ECHANGE AVEC LINUX       ****/
#include <Bridge.h>
#include <BridgeServer.h>
#include <BridgeClient.h>

/**************************************************/
/**************************************************/
/***** LECTURE TEMPERATURE ET HUMIDITE DHT11   ****/
// https://github.com/adafruit/DHT-sensor-library
#include <DHT.h>
/**************************************************/
/***** AFFICHAGE ECRAN OLED                   ****/
// https://github.com/lexus2k/ssd1306
#include "ssd1306.h"
/****************************************/
/***** LECTURE TEMPERATURE DS18B20   ****/
// https://www.pjrc.com/teensy/td_libs_OneWire.html
#include <OneWire.h>
// https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <DallasTemperature.h>
/****************************************/

/*********************** DECLARATIONS DES BROCHES UTILISEES **************************/

#define BROCHE_PH           A0
#define BROCHE_ORP          A1
#define BROCHE_NIVEAU       A2

#define BROCHE_BUZZER       2
#define BROCHE_TEMPERATURE  6
#define BROCHE_DHT11        8

/************************************************************************************/

#define DHT_TYPE DHT11

/*********************** CONSTANTES ************************************************/
#define VREF 5.0  // Tension de référence des niveaux logiques sert pour le calcul du PH
#define NOMBRE_POINTS_DE_MESURE  10 // sert pour le calcul du PH et ORP
#define NB_PAS_POUR_VREF 1024 // 1024 pour 5V pour les entrees analogiques pour Yun
/***********************************************************************************/

/*********************** VARIABLES GLOBALES ****************************************/
float temperatureEauEnCelcius;
float calibrationPH = 0;
float calibrationORP = 0;
float PH;
float ORP;

DHT dht11(BROCHE_DHT11, DHT_TYPE);

float temperatureDuLocalEnCelcius;
float humiditeDuLocal;
float niveauEau;
/***********************************************************************************/

/*********************** LIEN HTTP         ****************************************/
// Listen on default port 5555, the webserver on the Yún
// will forward there all the HTTP requests for us.
BridgeServer server;
/***********************************************************************************/

/*************************************************

              SETUP

 * **********************************************/
void setup(void)
{
  Serial.begin(9600); // USB
  Bridge.begin();
  delay(1000);
//  ssd1306_128x32_i2c_init();
//  ssd1306_fillScreen(0x00);
//  ssd1306_setFixedFont(ssd1306xled_font6x8);
  dht11.begin();
  
  server.listenOnLocalhost();
  server.begin();
}

/*************************************************

              LOOP

 * **********************************************/
void loop(void)
{
  int timingCourant = millis();
  
  // Get clients coming from server
  BridgeClient client = server.accept();
  if (client) 
  {
    RepondreAuxDemandesHTTP(client);
    client.stop();
  }

  lireTemperatureEauDuBassin();
  ORP = lireORP(BROCHE_ORP, NOMBRE_POINTS_DE_MESURE, NB_PAS_POUR_VREF, VREF, calibrationORP);
  PH = lirePH(BROCHE_PH, NOMBRE_POINTS_DE_MESURE, NB_PAS_POUR_VREF, VREF, calibrationPH);
  lireTemperatureEtHumiditeDuLocal();
  lireNiveauEauDansLocal();
  gererAlarmeNiveauEauDansLocal();
  //mettreAJourInfosSurAfficheur();
  
}

/*************************************************

              LIRE LA TEMPERATURE DU BASSIN
                        DS18B20
 * **********************************************/
void lireTemperatureEauDuBassin(void)
{
  OneWire oneWire(BROCHE_TEMPERATURE);
  DallasTemperature capteurTemperature(&oneWire); //Utilistion du bus Onewire pour les capteurs
  capteurTemperature.begin(); //Activation du capteur de temperature
  //Lecture de la température
  capteurTemperature.requestTemperatures();
  temperatureEauEnCelcius = capteurTemperature.getTempCByIndex(0);
}

/*************************************************

              LIRE LA TEMPERATURE ET HUMIDITE
                        DHT 11
 * **********************************************/
void lireTemperatureEtHumiditeDuLocal(void)
{
  temperatureDuLocalEnCelcius = dht11.readTemperature();
  humiditeDuLocal = dht11.readHumidity();

  if (isnan(humiditeDuLocal) || isnan(temperatureDuLocalEnCelcius))
  {
    Serial.println("Failed to read from DHT sensor!");
    temperatureDuLocalEnCelcius = 0.0;
    humiditeDuLocal = 0.0;
  }
}

/*************************************************

              LIRE NIVEAU EAU DANS LOCAL
                      Moisture sensor
 * **********************************************/
void lireNiveauEauDansLocal()
{
  /*# valeurs du capteur
  # 0  ~300     sec
  # 300~700     humide
  # 700~950     dans l'eau     */
  
  niveauEau = analogRead(BROCHE_NIVEAU);
}

/*************************************************

              ALARME NIVEAU EAU DANS LOCAL
                     
 * **********************************************/
void gererAlarmeNiveauEauDansLocal()
{
  if(niveauEau > 500) digitalWrite(BROCHE_BUZZER, HIGH);
}

/*************************************************

              ENVOI DES DONNEES DANS LE CLOUD

 * **********************************************/
void envoyerLesDonneesDansLeCloud(void) 
{

}

/*************************************************

              MISE A JOUR INFOS AFFICHEUR

 * **********************************************/
void miseAJourAfficheur(String valeur, int x, int y)
{
  char stringBuff[16];
  valeur.toCharArray(stringBuff, 16);
  ssd1306_printFixed(x, y, stringBuff, STYLE_NORMAL);
}

/*************************************************

              TRACER POUR DEBUG

 * **********************************************/
void tracerDebug(String message)
{
  Serial.println("DEBUG|" + String(millis()) + " - " + message);
}

/*************************************************

              AFFICHAGE DES INFOS SUR AFFICHEUR

 * **********************************************/
void mettreAJourInfosSurAfficheur()
{
   miseAJourAfficheur("Eau : " + String(temperatureEauEnCelcius) + " *C", 0, 0);
   miseAJourAfficheur("PH " + String(PH) + " | " + String(ORP), 0, 8);
   miseAJourAfficheur("Loc : " + String(temperatureDuLocalEnCelcius) + " *C", 0, 16);
   miseAJourAfficheur("Loc : " + String(humiditeDuLocal) + " %", 0, 24);
}

/*************************************************

              TRACER LIAISON SERIE

 * **********************************************/
void envoyerInfosSurLiaisonSerie(void)
{
  String messageDebug = "\r\n=============== INFOS BASSIN ==============\r\n";
  messageDebug += "Temperature : " + String(temperatureEauEnCelcius) + "°C\r\n";
  messageDebug += "PH          : " + String(PH) + "\r\n";
  messageDebug += "ORP         : " + String(ORP) + " mV\r\n";
  messageDebug += "=============== INFOS LOCAL  ==============\r\n";
  messageDebug += "Temperature : " + String(temperatureDuLocalEnCelcius) + " °C\r\n";
  messageDebug += "Humidite    : " + String(humiditeDuLocal) + " %\r\n";
  messageDebug += "Niveau eau  : " + String(niveauEau) + "\r\n";

  tracerDebug(messageDebug);
}

/*************************************************

              REPONSE JSON AUX DEMANDES arduino/Monitor

 * **********************************************/
void RepondreAuxDemandesHTTP(BridgeClient client)
{
    tracerDebug("Requete HTTP");
    // read the command
    //String command = client.readString();
    String command = client.readStringUntil('/');  //read the incoming data 
    command.trim();        //kill whitespace
    tracerDebug(command);
        
    if(command == "Monitor"){
          String reponse = "{\"Capteur\":\"1\",\"ORP\":\"" + String(ORP);
      reponse += "\",\"PH\":\"" + String(PH) + "\",\"TempEau\":\"" + String(temperatureEauEnCelcius);
      reponse += "\",\"TempLocal\":\"" + String(temperatureDuLocalEnCelcius)+ "\",\"humidite\":\"" + String(humiditeDuLocal);
      reponse += "\",\"niveau\":\"" + String(niveauEau) + "\"}";

      client.println("Status: 200");
      client.println("Content-type: application/json; charset=UTF-8");
      client.println();
      client.println(reponse);      
    }

    if(command == "SetPH"){
       calibrationPH = client.readStringUntil('/').toFloat();  // read the incoming data
       client.println("Status: 200");
       client.println("Content-type: application/json; charset=UTF-8");
       client.println();
       client.println("Valeur de calibration PH : " + String(calibrationPH));
       tracerDebug(String(calibrationPH));
    }

     if(command == "SetORP"){
       calibrationORP = client.readStringUntil('/').toFloat();  // read the incoming data
       client.println("Status: 200");
       client.println("Content-type: application/json; charset=UTF-8");
       client.println();
       client.println("Valeur de calibration ORP : " + String(calibrationORP));
       tracerDebug(String(calibrationORP));
    }
 }
