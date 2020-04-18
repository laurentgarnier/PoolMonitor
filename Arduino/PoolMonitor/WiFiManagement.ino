//#include "Arduino.h"
//#include <WiFi.h>

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

//void initWifiWithStaticIP(char* ssid, char* password, char* hostName, IPAddress ip, IPAddress gateway)
//{
//	if (WiFi.status() == WL_CONNECTED) return;
//
//	// config static IP
//	WiFi.setAutoConnect(false);   // devrait empêcher la connexion auto mais c'est pas le cas
//	WiFi.disconnect();  //Donc on déceonnecte manuellement pour être sûre
//	
//	WiFi.hostname(hostName);
//	// IPAddress ip(192, 168, 10, 245);
//	// IPAddress gateway(192, 168, 10, 3);
//	Serial.print(F("Setting static ip to : ")); 
//	Serial.println(ip);
//	IPAddress subnet(255, 255, 255, 0);
//	IPAddress dns(208, 67, 222, 222); // DNS OpenDns
//	WiFi.config(ip, gateway, subnet, dns);
//
//	WiFi.begin(ssid, password);
//
//	while (WiFi.status() != WL_CONNECTED)
//	{
//		Serial.println("\r\nEchec, nouvelle tentative dans 10 secondes\r\n");
//		WiFi.begin(ssid, password);
//		delay(10000);
//	}
//	Serial.println("Connecte au reseau " + String(ssid) + " (" + WiFi.localIP().toString() + ")");
//}
