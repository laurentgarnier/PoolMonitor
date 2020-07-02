/**************************************************/
/*****       GESTION ECHANGE AVEC LINUX       ****/
#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>

/**************************************************/
/**************************************************/
/***** LECTURE TEMPERATURE ET HUMIDITE DHT11   ****/
#include <dht11.h>
/**************************************************/
/***** AFFICHAGE ECRAN OLED                   ****/
#include "ssd1306.h"
/****************************************/
/***** LECTURE TEMPERATURE DS18B20   ****/
#include <OneWire.h>
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

/*********************** CONSTANTES ************************************************/
#define VREF 5.0  // Tension de référence des niveaux logiques sert pour le calcul du PH
#define NOMBRE_POINTS_DE_MESURE  10 // sert pour le calcul du PH et ORP
#define NB_PAS_POUR_VREF 1024 // 1024 pour 5V pour les entrees analogiques 
/***********************************************************************************/

/*********************** VARIABLES GLOBALES ****************************************/
float temperatureEauEnCelcius;
float calibrationPH = 0;
float calibrationORP = 0;
float PH;
float ORP;
int timingDerniereMAJDesDonnees;
int PERIODE_ENVOI_DES_DONNEES;
dht11 DHT;
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
  ssd1306_128x32_i2c_init();
  ssd1306_fillScreen(0x00);
  ssd1306_setFixedFont(ssd1306xled_font6x8);

  server.listenOnLocalhost();
  server.begin();

  if (PERIODE_ENVOI_DES_DONNEES == 0)
    PERIODE_ENVOI_DES_DONNEES = 20; // par defaut 5mn

  timingDerniereMAJDesDonnees = millis();
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
    RepondreAuxDemandesHTTP(client);
  
  // Les mesures sont transmises toutes les PERIODE_ENVOI_DES_DONNEES
  if ((timingCourant - timingDerniereMAJDesDonnees) > (PERIODE_ENVOI_DES_DONNEES * 1000))
  {
    envoyerLesDonneesDansLeCloud(); 
    envoyerInfosSurLiaisonSerie();
    timingDerniereMAJDesDonnees = millis();
  }
  
  lireTemperatureEauDuBassin();
  ORP = lireORP(BROCHE_ORP, NOMBRE_POINTS_DE_MESURE, NB_PAS_POUR_VREF, VREF, calibrationORP);
  PH = lirePH(BROCHE_PH, NOMBRE_POINTS_DE_MESURE, NB_PAS_POUR_VREF, VREF, calibrationPH);
  lireTemperatureEtHumiditeDuLocal();
  lireNiveauEauDansLocal();
  gererAlarmeNiveauEauDansLocal();
  mettreAJourInfosSurAfficheur();
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
  int chk;
  Serial.print("Lecture DHT11, \t");
  chk = DHT.read(BROCHE_DHT11);    // READ DATA
  switch (chk){
    case DHTLIB_OK:
                Serial.print("OK,\t");
                break;
    case DHTLIB_ERROR_CHECKSUM:
                Serial.print("Checksum error,\t");
                break;
    case DHTLIB_ERROR_TIMEOUT:
                Serial.print("Time out error,\t");
                break;
    default:
                Serial.print("Unknown error,\t");
                break;
  }
  
  temperatureDuLocalEnCelcius = DHT.temperature;
  humiditeDuLocal = DHT.humidity;
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
    tracerDebug("Requete HTTP";
    // read the command
    String command = client.readString();
    command.trim();        //kill whitespace
    tracerDebug(command);
    if(command == "Monitor"){
      Process time;
      time.runShellCommand("date");
      String timeString = "";
      while (time.available()) {
        char c = time.read();
        timeString += c;
      }
      String reponse = "[{\"time\":\"" + timeString + "\",\"Capteur\":\"1\",\"ORP\":" + String(ORP);
      reponse += "\",\"PH\":" + String(PH) + ",\"TempEau\":" + String(temperatureEauEnCelcius);
      reponse += "\",\"TempLocal\":" + String(temperatureDuLocalEnCelcius)+ ",\"humidite\":" + String(humiditeDuLocal);
      reponse += "\",\"niveau\";" + String(niveauEau) + "}]";
      
      client.print(reponse);      
    }
    client.stop();
}
