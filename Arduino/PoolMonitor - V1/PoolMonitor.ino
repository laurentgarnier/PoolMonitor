/***** AFFICHAGE ECRAN OLED          ****/
#include "ssd1306.h"
/****************************************/
/***** LECTURE TEMPERATURE DS18B20   ****/
#include <OneWire.h>
#include <DallasTemperature.h>
/****************************************/
#include <WiFi.h>

#include <PHprobeManagement.h>
#include <ORPprobeManagement.h>
#include <WifiManagement.h>

/****************************************/

/*********************** DECLARATIONS DES BROCHES UTILISEES **************************/

#define BROCHE_ORP                        39
#define BROCHE_PH                         34
#define BROCHE_CAPTEURS_TEMPERATURE       15
// SDA AFFICHEUR OLED en I2C              21 
// SCL AFFICHEUR OLED en I2C              22 
#define BROCHE_CALIBRATION_PH             33 
#define BROCHE_CALIBRATION_ORP            36 

/************************************************************************************/

/*********************** CONSTANTES ************************************************/
#define VREF 3.3  // Tension de référence des niveaux logiques
#define NOMBRE_POINTS_DE_MESURE  40 // Nombre de mesures pour PH et ORP
#define NB_PAS_POUR_VREF 4096 // 4096 pour 3.3V pour les entrees analogiques 

DeviceAddress capteurTemperatureExterieure = { 0x28, 0xFF, 0x8A, 0x5E, 0xC0, 0x17, 0x5, 0x1D };
DeviceAddress capteurTemperaturePiscine = { 0x28, 0xFF, 0xD2, 0x26, 0xC0, 0x17, 0x5, 0x2 };
/***********************************************************************************/

/*********************** VARIABLES GLOBALES ****************************************/
float temperatureEauEnCelcius;
float temperatureExterieureEnCelcius;

float PH;
float calibrationPH = 0.0;
float ORP;

int timingDernierEnvoiDesDonnees;
int timingDernierEnvoiSurLiaisonSerie = 0;

int cptEnvoiDansLeCloud = 0;

const int periodeEnvoiDansLeCloudEnSeconde = 600; // Toutes les 10mn
const int periodeEnvoiSurLiaisonSerieEnMs = 60000; // Toutes les minutes

static char* ssid = "CG";
static char* pass = "12ImpEglAnt69";
WiFiClient wifi;

IPAddress serveur(192, 168, 10, 200);

/***********************************************************************************/

/*************************************************
 *
 *            SETUP
 *
 * **********************************************/
void setup(void)
{
	Serial.begin(9600); // USB

	ssd1306_128x32_i2c_init();
	ssd1306_fillScreen(0x00);
	ssd1306_setFixedFont(ssd1306xled_font6x8);

	initWifi(ssid, pass);
  enregistrerCapteur();

	timingDernierEnvoiDesDonnees = millis();
}

/*************************************************
 *
 *            LOOP
 *
 * **********************************************/
void loop(void)
{
	int timingCourant = millis();

	lireTemperature();
	//  lireORP();
	PH = lirePH(BROCHE_PH, NOMBRE_POINTS_DE_MESURE, NB_PAS_POUR_VREF, VREF, calibrationPH);

	// Les mesures sont transmises toutes les periodeEnvoiDansLeCloudEnSeconde
	if ((timingCourant - timingDernierEnvoiDesDonnees) > (periodeEnvoiDansLeCloudEnSeconde * 1000))
	{
		envoyerLesDonneesDansLeCloud();
		timingDernierEnvoiDesDonnees = millis();
	}

	miseAJourAfficheur("Temp eau : " + String(temperatureEauEnCelcius) + " *C", 0, 0);
	miseAJourAfficheur("Temp ext : " + String(temperatureExterieureEnCelcius) + " *C", 0, 8);
	miseAJourAfficheur("PH       : " + String(PH), 0, 16);
	// miseAJourAfficheur("ORP  : " + String(ORP), 0, 16);  

	if (verifierStatutWifi())
		miseAJourAfficheur("WIFI : OK - " + String(cptEnvoiDansLeCloud), 0, 24);
	else
		miseAJourAfficheur(F("WIFI : NOK"), 0, 24);

	if ((timingCourant - timingDernierEnvoiSurLiaisonSerie) > periodeEnvoiSurLiaisonSerieEnMs) {
		tracerDebug("\r\nTemp     : " + String(temperatureEauEnCelcius) + "*C\r\nPH       : " + String(PH) + "\r\nORP      : " + String(ORP) + "\r\nTemp ext : " + String(temperatureExterieureEnCelcius) + "*C\r\n");
		timingDernierEnvoiSurLiaisonSerie = timingCourant;
	}
}

/*************************************************
 *
 *            LIRE LA TEMPERATURE
 *
 * **********************************************/
void lireTemperature(void)
{
	OneWire oneWire(BROCHE_CAPTEURS_TEMPERATURE);
	DallasTemperature capteursTemperature(&oneWire); //Utilistion du bus Onewire pour les capteurs

	capteursTemperature.begin(); //Activation des capteurs de temperature
	//Lecture des températures de tous les capteurs
	capteursTemperature.requestTemperatures();
	// Le capteur de temperature de la piscine
	temperatureEauEnCelcius = capteursTemperature.getTempC(capteurTemperaturePiscine);
	temperatureExterieureEnCelcius = capteursTemperature.getTempC(capteurTemperatureExterieure);
}



/*************************************************
 *
 *            ENVOI DES DONNEES DANS LE CLOUD
 *
 * **********************************************/
void envoyerLesDonneesDansLeCloud() {

	tracerDebug(F("\nEnvoi cloud"));
	if (!verifierStatutWifi()) return;

	tracerDebug(F("Connection serveur cloud..."));
	if (!wifi.connect(serveur, 1880))
	{
		tracerDebug(F("Echec de la connection"));
		return;
	}
	else
		tracerDebug("Connection au serveur " + String(serveur) + " OK!");

	String datas = "[{\"Temp\":" + String(temperatureEauEnCelcius) + ",\"ORP\":" + String(ORP) + ",\"PH\":" + String(PH) + ",\"TempExt\":" + String(temperatureExterieureEnCelcius) + "},{\"Capteur\":1}]\n";

	String requete = F("POST /Monitor");
	requete.concat(F("\n"\
		"Host: 192.168.10.200:1880\n"\
		"Content-Type: application/json\n"\
		"Content-Length: "));
	requete.concat(datas.length());
	requete.concat(F("\n\n"));
	requete.concat(datas);

	tracerDebug("Envoi :\n" + requete);
	wifi.println(requete);

	while (wifi.connected())
	{
		String line = wifi.readStringUntil('\n');
		if (line == "\r")
		{
			tracerDebug("\nheaders received");
			break;
		}
	}
	// Reponse du serveur
	String reponse;

	while (wifi.available())
	{
		char c = wifi.read();
		reponse.concat(c);
	}

	if (reponse.indexOf("{\"Temp\":") > 0)
		cptEnvoiDansLeCloud++;

	tracerDebug(reponse);
	wifi.stop();
}



/*************************************************
 *
 *            ENVOI DES DONNEES DANS LE CLOUD
 *
 * **********************************************/
void enregistrerCapteur() {

  tracerDebug(F("\nEnregistrement"));
  if (!verifierStatutWifi()) return;

  tracerDebug(F("Connection serveur ..."));
  if (!wifi.connect(serveur, 1880))
  {
    tracerDebug(F("Echec de la connection"));
    return;
  }
  else
    tracerDebug("Connection au serveur " + String(serveur) + " OK!");

  String datas = "{\"sensorID\": \"Piscine\",\"IP\": \"" + WiFi.localIP().toString() + "\"}\n";

  String requete = F("POST /RegisterSensor");
  requete.concat(F("\n"\
    "Host: 192.168.10.200:1880\n"\
    "Content-Type: application/json\n"\
    "Content-Length: "));
  requete.concat(datas.length());
  requete.concat(F("\n\n"));
  requete.concat(datas);

  tracerDebug("Envoi :\n" + requete);
  wifi.println(requete);

  while (wifi.connected())
  {
    String line = wifi.readStringUntil('\n');
    if (line == "\r")
    {
      tracerDebug("\nheaders received");
      break;
    }
  }
  // Reponse du serveur
  String reponse;

  while (wifi.available())
  {
    char c = wifi.read();
    reponse.concat(c);
  }

  tracerDebug(reponse);
  tracerDebug("Enregistrement OK");
  wifi.stop();
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
 *
 *            TRACER POUR DEBUG
 *
 * **********************************************/
void tracerDebug(String message)
{
	Serial.println(message);
}
