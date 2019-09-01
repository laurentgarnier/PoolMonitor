#include "ORPprobeManagement.h"
#include "FiltreMedian.h"
#include "Arduino.h"


/*************************************************
 *
 *            LIRE OXYDOREDUCTION
 *
 * **********************************************/
float ORPMeasure::lireORP(int brocheDeLaSonde, int nombreDePointsDeMesure, int nbPasPourVRef, int vRef, float calibration)
{
	// Lecture de l'ORP selon la doc du capteur https://www.phidgets.com/docs/1130_User_Guide
	int ORPanalogBufferIndex = 0;
	float* ORPanalogBuffer = new float[nombreDePointsDeMesure];

	while (ORPanalogBufferIndex < nombreDePointsDeMesure)
	{
		int valeurORP = analogRead(brocheDeLaSonde);

		float tension = ((float)valeurORP * vRef) / nbPasPourVRef;
		ORPanalogBuffer[ORPanalogBufferIndex] = tension;
		ORPanalogBufferIndex++;
		delay(20);
	}

	float orp = calculerORP(ORPanalogBuffer, nombreDePointsDeMesure, nbPasPourVRef, vRef, calibration);
	free(ORPanalogBuffer);
	return orp;

}

float ORPMeasure::calculerORP(float* mesures, int nombreDePointsDeMesure, int nbPasPourVRef, int vRef, float calibration)
{
	float voltageORP = filtreMedianEnFloat(mesures, nombreDePointsDeMesure);// + calibrationORP;

	float* ORPMoyen = new float[nombreDePointsDeMesure];
	int indexORPMoyen = 0;

	ORPMoyen[indexORPMoyen] = (2.5 * voltageORP) / 1.035;
	indexORPMoyen++;

	if (indexORPMoyen > nombreDePointsDeMesure) indexORPMoyen = 0;

	return filtreMedianEnFloat(ORPMoyen, nombreDePointsDeMesure) * 1000;
}
