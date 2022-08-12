
/*************************************************
 *
 *            LIRE LE PH
 *
 * **********************************************/
float lirePH(int brocheDeLaSonde, int nombreDePointsDeMesure, int nbPasPourVRef, int vRef, float calibration)
{
	// Lecture du PH selon la doc du capteur https://www.dfrobot.com/wiki/index.php/PH_meter_V1.1_SKU:SEN0161#Sample_Code
	// https://wiki.dfrobot.com/PH_meter_SKU__SEN0161_


	// Stockage des points de mesure analogiques
	int* analogValuesPH = new int[nombreDePointsDeMesure];
	int tempValue;

	// Acquisition de nombreDePointsDeMesure valeurs de PH avec une tempo de 10ms entre chaque acquisition 
	for(int indexBuffer = 0; indexBuffer < nombreDePointsDeMesure; indexBuffer++)
	{
		analogValuesPH[indexBuffer] = analogRead(brocheDeLaSonde);
    //Serial.println("valeur analogique : " + String(analogValuesPH[indexBuffer]));
		delay(10);
	}
	
	// Tri des valeurs de la plus petite à la plus grande
	for(int indexBuffer = 0; indexBuffer < nombreDePointsDeMesure - 1; indexBuffer++)        
	{
		for(int indexBufferSuivant = indexBuffer+1; indexBufferSuivant < nombreDePointsDeMesure; indexBufferSuivant++)
		{
			if(analogValuesPH[indexBuffer] > analogValuesPH[indexBufferSuivant])
			{
				tempValue = analogValuesPH[indexBuffer];
				analogValuesPH[indexBuffer] = analogValuesPH[indexBufferSuivant];
				analogValuesPH[indexBufferSuivant] = tempValue;
			}
		}
	}	
	
	// récuparation d'une valeur stable en applicant l'algo de filtre median (https://fr.wikipedia.org/wiki/Filtre_m%C3%A9dian)
	//float moyenneDesMesures = avergearray(analogValuesPH, nombreDePointsDeMesure);
	//float voltage = moyenneDesMesures / (nbPasPourVRef / vRef);
	//float ph = 3.5 * voltage + calibration;
	//float ph = calculerPH(PHanalogBuffer, nombreDePointsDeMesure, nbPasPourVRef, vRef, calibration);
	
	unsigned long int somme = 0;

	// On retire les 2 plus petites valeurs et les 2 plus grandes valeurs
	for(int indexBuffer = 2;indexBuffer < nombreDePointsDeMesure - 2; indexBuffer++)                      
		somme += analogValuesPH[indexBuffer];
	
	float PhmV = (float)somme * vRef / nbPasPourVRef / (nombreDePointsDeMesure-4); //convert the analog into millivolt
	float ph = 3.5 * PhmV;     
	
	free(analogValuesPH);
  
	return ph * calibrationPH;
}
