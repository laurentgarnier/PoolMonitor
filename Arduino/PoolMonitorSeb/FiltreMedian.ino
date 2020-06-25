
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
int filtreMedian(int* tableauDesPoints, int nombreDePoints)
{
	int* tableauTemporaire = new int[nombreDePoints];
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

float filtreMedianEnFloat(float* tableauDesPoints, int nombreDePoints)
{
	float* tableauTemporaire = new float[nombreDePoints];
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
