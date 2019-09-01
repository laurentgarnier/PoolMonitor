class ORPMeasure
{
public :
	float lireORP(int brocheDeLaSonde, int nombreDePointsDeMesure, int nbPasPourVRef, int vRef, float calibration);

private:
	float calculerORP(float* mesures, int nombreDePointsDeMesure, int nbPasPourVRef, int vRef, float calibration);
};


