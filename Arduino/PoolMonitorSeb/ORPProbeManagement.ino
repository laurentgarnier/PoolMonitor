
/*************************************************
 *
 *            LIRE OXYDOREDUCTION
 *
 * **********************************************/
float lireORP(int brocheDeLaSonde, int nombreDePointsDeMesure, int nbPasPourVRef, int vRef, float calibration)
{
    int valeur = analogRead(brocheDeLaSonde) * (vRef * 1000) / nbPasPourVRef / 1000.0;   // Convertit 0.0 to 5.0 V
    return (((2.5 - valeur) / 1.037) * 1000.0) + (calibration - 250);
}
//{
//	// Lecture de l'ORP selon la doc du capteur https://www.phidgets.com/docs/1130_User_Guide
//
// Serial.println("Lire ORP");
//	int ORPanalogBufferIndex = 0;
//	float* ORPanalogBuffer = new float[nombreDePointsDeMesure];
//
//	while (ORPanalogBufferIndex < nombreDePointsDeMesure)
//	{
//		int valeurORP = analogRead(brocheDeLaSonde);
//
//		float tension = ((float)valeurORP * vRef) / nbPasPourVRef;
//		ORPanalogBuffer[ORPanalogBufferIndex] = tension;
//		ORPanalogBufferIndex++;
//		delay(20);
//	}
//
//	float orp = calculerORP(ORPanalogBuffer, nombreDePointsDeMesure, nbPasPourVRef, vRef, calibration);
//	free(ORPanalogBuffer);
//  Serial.println("fin Lire ORP : " + String(orp));
//	return orp;
//
//}
//
//float calculerORP(float* mesures, int nombreDePointsDeMesure, int nbPasPourVRef, int vRef, float calibration)
//{
//  Serial.println("Calculer ORP");
//	float voltageORP = filtreMedianEnFloat(mesures, nombreDePointsDeMesure);// + calibrationORP;
//
////	float* ORPMoyen = new float[nombreDePointsDeMesure];
////	int indexORPMoyen = 0;
////
////	ORPMoyen[indexORPMoyen] = (2.5 * voltageORP) / 1.035;
////	indexORPMoyen++;
////
////	if (indexORPMoyen > nombreDePointsDeMesure) indexORPMoyen = 0;
////Serial.println("Fin calculer ORP");
////	float orp = filtreMedianEnFloat(ORPMoyen, nombreDePointsDeMesure) * 1000;
////  free(ORPMoyen);
//  return (2.5 * voltageORP) / 1.035;
//}
