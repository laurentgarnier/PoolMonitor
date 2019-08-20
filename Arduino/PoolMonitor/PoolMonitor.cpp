/***** AFFICHAGE ECRAN OLED                   ****/
#include "ssd1306.h"
/****************************************/
/***** LECTURE TEMPERATURE DS18B20   ****/
#include <OneWire.h>
#include <DallasTemperature.h>
/****************************************/
/*****  LIAISON wifi                 ****/
#include <WiFi.h>
/****************************************/

/*********************** DECLARATIONS DES BROCHES UTILISEES **************************/

#define BROCHE_ORP                        39
#define BROCHE_PH                         34
#define BROCHE_CAPTEURS_TEMPERATURE       15
#define BROCHE_BOUTON_CALIBRATION         16
// SDA AFFICHEUR OLED en I2C              21 
// SCL AFFICHEUR OLED en I2C              22 
#define BROCHE_CALIBRATION_PH             33 
#define BROCHE_CALIBRATION_ORP            36 

/************************************************************************************/

/*********************** CONSTANTES ************************************************/
#define VREF 3.3  // Tension de référence des niveaux logiques sert pour le calcul du PH
#define NOMBRE_POINTS_DE_MESURE  40 // sert pour le calcul du PH
#define NB_PAS_POUR_VREF 4096 // 4096 pour 3.3V pour les entrees analogiques 

DeviceAddress capteurTemperatureExterieure = { 0x28, 0xFF, 0x8A, 0x5E, 0xC0, 0x17, 0x5, 0x1D };
DeviceAddress capteurTemperaturePiscine = { 0x28, 0xFF, 0xD2, 0x26, 0xC0, 0x17, 0x5, 0x2 };
/***********************************************************************************/

/*********************** VARIABLES GLOBALES ****************************************/
float temperatureEauEnCelcius;
float temperatureExterieureEnCelcius;
float calibrationPH = 0;
float calibrationORP = 0;
float PH;
float ORP;
int PHanalogBuffer[NOMBRE_POINTS_DE_MESURE];    // Stockage des points de mesure analogiques
float ORPanalogBuffer[NOMBRE_POINTS_DE_MESURE];
float ORPMoyen[NOMBRE_POINTS_DE_MESURE];
int indexORPMoyen = 0;

int timingDernierEnvoiDesDonnees;
int timingDernierEnvoiSurLiaisonSerie = 0;
int timingDerniereLecturePH = 0;
int cptEnvoiDansLeCloud = 0;

const int periodeEnvoiDansLeCloudEnSeconde = 600; // Toutes les 10mn
const int periodeEnvoiSurLiaisonSerieEnMs = 60000; // Toutes les minutes
const int periodeLecturePHEnMs = 30000; // Toutes les 30 secondes

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

	pinMode(BROCHE_BOUTON_CALIBRATION, INPUT);
	initWifi();
	//calibrer();

	timingDernierEnvoiDesDonnees = millis();
}

/*************************************************
 *
 *            LOOP
 *
 * **********************************************/
void loop(void)
{
	int modeCalibration = digitalRead(BROCHE_BOUTON_CALIBRATION);
	if (modeCalibration == HIGH)
	{
		ssd1306_fillScreen(0x00);
		calibrer();
		return;
	}

	int timingCourant = millis();

	lireTemperature();
	//  lireORP();
	lirePH();

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
 *            VERIFIER STATUT wifi
 *
 * **********************************************/
bool verifierStatutWifi()
{
	return (WiFi.status() == WL_CONNECTED);
}

/*************************************************
 *
 *            INIT wifi
 *
 * **********************************************/
void initWifi()
{
	if (WiFi.status() == WL_CONNECTED) return;

	tracerDebug("Tentative de connection sur SSID: " + String(ssid));

	WiFi.begin(ssid, pass);
	while (WiFi.status() != WL_CONNECTED)
	{
		tracerDebug("\r\nEchec, nouvelle tentative dans 10 secondes\r\n");
		WiFi.begin(ssid, pass);
		delay(10000);
	}
	tracerDebug("Connecte au reseau " + String(ssid) + " (" + WiFi.localIP().toString() + ")");
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
 *            CALIBRATION DES CAPTEURS
 *
 * **********************************************/
void calibrer()
{
	float valeurBrocheCalibrationPH = analogRead(BROCHE_CALIBRATION_PH);

	calibrationPH = ((valeurBrocheCalibrationPH - (NB_PAS_POUR_VREF / 2)) * VREF) / NB_PAS_POUR_VREF;

	calculerPH();
	float valeurBrocheCalibrationORP = analogRead(BROCHE_CALIBRATION_ORP);

	calibrationORP = ((valeurBrocheCalibrationORP - (NB_PAS_POUR_VREF / 2)) * VREF) / NB_PAS_POUR_VREF;

	calculerORP();

	//miseAJourAfficheur("PH   : " + String(PH), 0, 8);
	//miseAJourAfficheur("ORP  : " + String(ORP), 0, 16);

	tracerDebug("mode calibration");
	tracerDebug("CalPH : " + String(calibrationPH) + " brocheCalPH : " + String(valeurBrocheCalibrationPH));
	tracerDebug("CalORP : " + String(calibrationORP) + " brocheCalORP : " + String(valeurBrocheCalibrationORP));
}

/*************************************************
 *
 *            LIRE LE PH
 *
 * **********************************************/

void lirePH(void)
{
	// Lecture du PH selon la doc du capteur https://www.dfrobot.com/wiki/index.php/PH_meter_V1.1_SKU:SEN0161#Sample_Code
	// https://wiki.dfrobot.com/PH_meter_SKU__SEN0161_

	// if((millis() - timingDerniereLecturePH) < periodeLecturePHEnMs) return;
	int PHanalogBufferIndex = 0;

	while (PHanalogBufferIndex < NOMBRE_POINTS_DE_MESURE)
	{
		int valeurPH = analogRead(BROCHE_PH);
		PHanalogBuffer[PHanalogBufferIndex] = valeurPH;
		PHanalogBufferIndex++;
		delay(20);
	}
	calculerPH();

	timingDerniereLecturePH = millis();
}

/*************************************************
 *
 *            CALCUL DU PH
 *
 * **********************************************/
void calculerPH()
{
	// récuparation d'une valeur stable en applicant l'algo de filtre median (https://fr.wikipedia.org/wiki/Filtre_m%C3%A9dian)
	float moyenne = avergearray(PHanalogBuffer, NOMBRE_POINTS_DE_MESURE);
	float voltage = moyenne / (NB_PAS_POUR_VREF / VREF);
	PH = 3.5 * voltage + calibrationPH;
	tracerDebug("Mesure moyenne : " + String(moyenne) + " - voltage : " + String(voltage) + " - Calibration : " + String(calibrationPH) + " - PH : " + String(PH));
}

/*************************************************
 *
 *            LIRE OXYDOREDUCTION
 *
 * **********************************************/
void lireORP(void)
{
	// Lecture de l'ORP selon la doc du capteur https://www.phidgets.com/docs/1130_User_Guide
	int ORPanalogBufferIndex = 0;

	while (ORPanalogBufferIndex < NOMBRE_POINTS_DE_MESURE)
	{
		int valeurORP = analogRead(BROCHE_ORP);

		float tension = ((float)valeurORP * VREF) / NB_PAS_POUR_VREF;

		ORPanalogBuffer[ORPanalogBufferIndex] = tension;
		ORPanalogBufferIndex++;
		delay(20);
	}
	calculerORP();

}

void calculerORP()
{
	float voltageORP = filtreMedianEnFloat(ORPanalogBuffer, NOMBRE_POINTS_DE_MESURE);// + calibrationORP;

	ORPMoyen[indexORPMoyen] = (2.5 * voltageORP) / 1.035;
	indexORPMoyen++;
	if (indexORPMoyen > NOMBRE_POINTS_DE_MESURE) indexORPMoyen = 0;

	ORP = filtreMedianEnFloat(ORPMoyen, NOMBRE_POINTS_DE_MESURE) * 1000;

	//tracerDebug("Mesure moyenne : " + String(voltageORP) + " - Calibration : " + String(calibrationORP) + " - ORP : " + String(ORP));

}

double avergearray(int* arr, int number) {
	int i;
	int max, min;
	double avg;
	long amount = 0;
	if (number <= 0) {
		Serial.println("Error number for the array to avraging!/n");
		return 0;
	}
	if (number < 5) {   //less than 5, calculated directly statistics
		for (i = 0; i < number; i++) {
			amount += arr[i];
		}
		avg = amount / number;
		return avg;
	}
	else {
		if (arr[0] < arr[1]) {
			min = arr[0]; max = arr[1];
		}
		else {
			min = arr[1]; max = arr[0];
		}
		for (i = 2; i < number; i++) {
			if (arr[i] < min) {
				amount += min;        //arr<min
				min = arr[i];
			}
			else {
				if (arr[i] > max) {
					amount += max;    //arr>max
					max = arr[i];
				}
				else {
					amount += arr[i]; //min<=arr<=max
				}
			}//if
		}//for
		avg = (double)amount / (number - 2);
	}//if
	return avg;
}

/*************************************************
 *
 *            FILTRE MEDIAN
 *
 * **********************************************/
int filtreMedian(int tableauDesPoints[], int nombreDePoints)
{
	int tableauTemporaire[nombreDePoints];
	for (byte i = 0; i < nombreDePoints; i++)
	{
		tableauTemporaire[i] = tableauDesPoints[i];
	}
	int i, j, valeurMoyenne;
	for (j = 0; j < nombreDePoints - 1; j++)
	{
		for (i = 0; i < nombreDePoints - j - 1; i++)
		{
			if (tableauTemporaire[i] > tableauTemporaire[i + 1])
			{
				valeurMoyenne = tableauTemporaire[i];
				tableauTemporaire[i] = tableauTemporaire[i + 1];
				tableauTemporaire[i + 1] = valeurMoyenne;
			}
		}
	}
	// nombre de points est pair ou impair
	if ((nombreDePoints & 1) > 0)
		valeurMoyenne = tableauTemporaire[(nombreDePoints - 1) / 2];
	else
		valeurMoyenne = (tableauTemporaire[nombreDePoints / 2] + tableauTemporaire[nombreDePoints / 2 - 1]) / 2;

	return valeurMoyenne;
}

float filtreMedianEnFloat(float tableauDesPoints[], int nombreDePoints)
{
	float tableauTemporaire[nombreDePoints];
	for (byte i = 0; i < nombreDePoints; i++)
	{
		tableauTemporaire[i] = tableauDesPoints[i];
	}
	int i, j;
	float valeurMoyenne;
	for (j = 0; j < nombreDePoints - 1; j++)
	{
		for (i = 0; i < nombreDePoints - j - 1; i++)
		{
			if (tableauTemporaire[i] > tableauTemporaire[i + 1])
			{
				valeurMoyenne = tableauTemporaire[i];
				tableauTemporaire[i] = tableauTemporaire[i + 1];
				tableauTemporaire[i + 1] = valeurMoyenne;
			}
		}
	}
	if ((nombreDePoints & 1) > 0)
		valeurMoyenne = tableauTemporaire[(nombreDePoints - 1) / 2];
	else
		valeurMoyenne = (tableauTemporaire[nombreDePoints / 2] + tableauTemporaire[nombreDePoints / 2 - 1]) / 2;

	return valeurMoyenne;
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
