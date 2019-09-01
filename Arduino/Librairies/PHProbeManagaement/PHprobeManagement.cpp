#include "PHprobeManagement.h"
#include "FiltreMedian.h"
#include "Arduino.h"

/*************************************************
 *
 *            LIRE LE PH
 *
 * **********************************************/
float lirePH(int brocheDeLaSonde, int nombreDePointsDeMesure, int nbPasPourVRef, int vRef, float calibration)
{
	// Lecture du PH selon la doc du capteur https://www.dfrobot.com/wiki/index.php/PH_meter_V1.1_SKU:SEN0161#Sample_Code
	// https://wiki.dfrobot.com/PH_meter_SKU__SEN0161_

	int PHanalogBufferIndex = 0;

	// Stockage des points de mesure analogiques
	int* PHanalogBuffer = new int[nombreDePointsDeMesure];

	while (PHanalogBufferIndex < nombreDePointsDeMesure)
	{
		int valeurPH = analogRead(brocheDeLaSonde);
		PHanalogBuffer[PHanalogBufferIndex] = valeurPH;
		PHanalogBufferIndex++;
		delay(20);
	}
	// récuparation d'une valeur stable en applicant l'algo de filtre median (https://fr.wikipedia.org/wiki/Filtre_m%C3%A9dian)
	float moyenneDesMesures = avergearray(PHanalogBuffer, nombreDePointsDeMesure);
	float voltage = moyenneDesMesures / (nbPasPourVRef / vRef);
	float ph = 3.5 * voltage + calibration;
	//float ph = calculerPH(PHanalogBuffer, nombreDePointsDeMesure, nbPasPourVRef, vRef, calibration);
	free(PHanalogBuffer);

	return ph;
}
