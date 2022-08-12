#include "WifiManagement.h"
#include "Arduino.h"
#include <WiFi.h>

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
void initWifi(char* ssid, char* password)
{
	if (WiFi.status() == WL_CONNECTED) return;
	Serial.println("Tentative de connection sur SSID: " + String(ssid));

	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED)
	{
		Serial.println("\r\nEchec, nouvelle tentative dans 10 secondes\r\n");
		WiFi.begin(ssid, password);
		delay(10000);
	}
	Serial.println("Connecte au reseau " + String(ssid) + " (" + WiFi.localIP().toString() + ")");
}
