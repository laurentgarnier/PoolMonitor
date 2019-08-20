/**************************************************/
/***** LECTURE TEMPERATURE ET HUMIDITE DHT11   ****/
#include <DHT.h>
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
#define NOMBRE_POINTS_DE_MESURE_PH  40 // sert pour le calcul du PH
/***********************************************************************************/

/*********************** VARIABLES GLOBALES ****************************************/
float temperatureEauEnCelcius;
float calibrationPH = 0;
float calibrationORP = -690;
int tableauMesuresPHEnVolt[NOMBRE_POINTS_DE_MESURE_PH];    // Stockage des points de mesure analogiques
int indexTableauMesuresPHEnVolt = 0;
float voltagePHMoyen;
float PH;
int periodeEchantillonagePHEnMs = 20;
unsigned long dateDerniereMesurePH = millis();
float ORP;
int timingDerniereMAJDesDonnees;
int PERIODE_ENVOI_DES_DONNEES;
int nombreCycle = 0;
DHT dht(BROCHE_DHT11, DHT11);
float temperatureDuLocalEnCelcius;
float humiditeDuLocal;
float niveauEau;
/***********************************************************************************/

/*************************************************

              SETUP

 * **********************************************/
void setup(void)
{

  Serial.begin(9600); // USB
  ssd1306_128x32_i2c_init();
  ssd1306_fillScreen(0x00);
  ssd1306_setFixedFont(ssd1306xled_font6x8);

  if (PERIODE_ENVOI_DES_DONNEES == 0)
    PERIODE_ENVOI_DES_DONNEES = 20; // par defaut 5mn

  timingDerniereMAJDesDonnees = millis();
}

/*************************************************

              LOOP

 * **********************************************/
void loop(void)
{
  // Les mesures sont transmises toutes les PERIODE_ENVOI_DES_DONNEES
  if ((millis() - timingDerniereMAJDesDonnees) > (PERIODE_ENVOI_DES_DONNEES * 1000))
  {
    envoyerLesDonneesDansLeCloud(); 
    envoyerInfosSurLiaisonSerie();
    
    timingDerniereMAJDesDonnees = millis();
  }
  
  lireTemperatureEauDuBassin();
  lireORP();
  lirePH();
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
  temperatureDuLocalEnCelcius = dht.readTemperature();
  humiditeDuLocal = dht.readHumidity();
}

/*************************************************

              LIRE LE PH

 * **********************************************/
void lirePH(void)
{
  // La mesure de PH ne se fait pas à tous les cycles
  if(millis() - dateDerniereMesurePH < periodeEchantillonagePHEnMs) return;

  // Lecture du PH selon la doc du capteur https://www.dfrobot.com/wiki/index.php/PH_meter_V1.1_SKU:SEN0161#Sample_Code
  tableauMesuresPHEnVolt[indexTableauMesuresPHEnVolt++] = analogRead(BROCHE_PH);

  if (indexTableauMesuresPHEnVolt == NOMBRE_POINTS_DE_MESURE_PH) indexTableauMesuresPHEnVolt = 0;
  // récuperation d'une valeur stable en applicant l'algo de filtre median (https://fr.wikipedia.org/wiki/Filtre_m%C3%A9dian)
  voltagePHMoyen = filtreMedian(tableauMesuresPHEnVolt, NOMBRE_POINTS_DE_MESURE_PH) * VREF / 1024; 

  PH = 3.5 * voltagePHMoyen + calibrationPH;
}

/*************************************************

              LIRE OXYREDUCTION

 * **********************************************/
void lireORP(void)
{
  // Lecture de l'ORP selon la doc du capteur https://www.phidgets.com/docs/1130_User_Guide
  int voltageORP = analogRead(BROCHE_ORP) + calibrationORP;
  ORP = (2.5 - voltageORP) / 1.037;
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

              FILTRE MEDIAN

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
  if ((nombreDePoints & 1) > 0)
    valeurMoyenne = tableauTemporaire[(nombreDePoints - 1) / 2];
  else
    valeurMoyenne = (tableauTemporaire[nombreDePoints / 2] + tableauTemporaire[nombreDePoints / 2 - 1]) / 2;

  return valeurMoyenne;
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
   miseAJourAfficheur(" " + String(PH) + " | " + String(ORP), 0, 8);
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
