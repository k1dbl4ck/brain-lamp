/*
 *  A brain for the light! 
 *  https://medium.com/p/c5b290c2e31a/edit 
 * 
 *  Author: Werner F. Venter
 *  Email: k1dbl4ck@gmail.com
 *  License: MIT
 * 
 *  This code was written to use with a Wemo D1 Mini NodeMCU board driving an RGB LED board (From Philips in this case)
 *  
 *  Features:
 *   - Uses LED to indicate state during startup and connection phases
 *   - WiFi auto connect
 *   - Web server
 *       - Hosts a colour picker
 *       - Accepts RGB colours in a GET request
 *   - MQTT client
 *       - Connects to MQTT broker
 *       - Subscribes to RGB colour codes in messages
 */

// Include the dependencies
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <FS.h>
#include "string"
#include <ctime>

// Enumerate the pins you will use (I always like using the GPIO address, and not the board map)
const int R_PIN = 5; //D1
const int G_PIN = 4; //D2
const int B_PIN = 0; //D3

// Enumerate WiFi credentials
const char *ssid = "...";
const char *password = "...";

// Enumerate MQTT broker IP
const char *mqtt_server = "192.168.0.200";

// Alloc dependencies
WiFiClient espClient;               // Allocate the WiFi client
PubSubClient mqttClient(espClient); // Allocate the MQTT client
ESP8266WebServer webServer(80);     // Allocate the Web Server

// Alloc functions
void initHardware();
void initFS();
void initWebServer();
void initWiFi();
void initMQTT();
void off();
void flashPin(int pin, int duration);
void onMQTT(char *topic, byte *payload, unsigned int length);
void onRGB();
void fade(int pin);
int col(int rgbValue);

/*
 * Setup
 * 
 * - Start serial comms for debugging (if debugger connected via Serial UART)
 * - Mount the filesystem for static web files
 * - Start the web server
 * - Start connecting to WiFi and MQTT
 * - Use the LED to indicate progress
 * 
 */
void setup()
{
    initHardware();
    initFS();
    initWebServer();
    initWiFi();
    initMQTT();
}

void initHardware()
{

    //Start serial port. Low baud, no matter
    Serial.begin(9600);

    //Give serial a few millseconds to get going
    delay(10);

    //Allocate GPIO behaviour to pins we enumerated at the top of this file
    pinMode(R_PIN, OUTPUT);
    pinMode(G_PIN, OUTPUT);
    pinMode(B_PIN, OUTPUT);

    //Turn the led off - i.e Set it to RGB(0,0,0)
    off();

    //Flash Red for 3 seconds to indicate startup
    flashPin(R_PIN, 3);

    //Take Red HIGH to indicate we are about to start connecting
    analogWrite(R_PIN, 1023);

    //Give it a second for UX reasons :}
    delay(1000);
}

void initFS()
{

    Serial.println("Mounting FS...");
    if (!SPIFFS.begin())
    {
        Serial.println("Failed to mount file system");
        return;
    }
    Serial.println("FS mounted.");

    Serial.println("CMMC READING ROOT DIRECTORY..");
    Dir root = SPIFFS.openDir("/web");

    while (root.next())
    {
        String fileName = root.fileName();
        File f = root.openFile("r");
        Serial.printf("%s: %d\r\n", fileName.c_str(), f.size());
    }
}

void initWebServer()
{

    webServer.on("/set", onRGB);
    webServer.serveStatic("/", SPIFFS, "/");
    webServer.begin();
}

void initWiFi()
{

    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Once WiFi is connected, turn the light low orange
    analogWrite(R_PIN, 500);
    analogWrite(G_PIN, 500);
    analogWrite(B_PIN, 0);
}

void initMQTT()
{

    // Start MQTT connection
    mqttClient.setServer(mqtt_server, 1883);
    mqttClient.setCallback(onMQTT);

    delay(2000);
    off();
}

void onRGB()
{
    String red = webServer.arg(0);   // read R argument
    String green = webServer.arg(1); // read G argument
    String blue = webServer.arg(2);  // read B argument

    Serial.println("Displaying rgb(" + red + "," + green + "," + blue + ")");
    analogWrite(R_PIN, col(red.toInt()));
    analogWrite(G_PIN, col(green.toInt()));
    analogWrite(B_PIN, col(blue.toInt()));
}

void onMQTT(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);

    //check if message is named colour
    std::string cName = "";
    for (int i = 0; i < length; i++)
    {
        cName = cName + (char)payload[i];
    }

    if (cName.find("lue") != std::string::npos)
    {

        analogWrite(R_PIN, col(0));
        analogWrite(G_PIN, col(0));
        analogWrite(B_PIN, col(255));
    }
    else if (cName.find("ed") != std::string::npos)
    {
        analogWrite(R_PIN, col(255));
        analogWrite(G_PIN, col(0));
        analogWrite(B_PIN, col(0));
    }
    else if (cName.find("reen") != std::string::npos)
    {

        analogWrite(R_PIN, col(0));
        analogWrite(G_PIN, col(255));
        analogWrite(B_PIN, col(0));
    }
    else if (cName.find("andom") != std::string::npos)
    {

        srand(time(NULL));
        int random;
        random = (rand() % 250) + 1;
        analogWrite(R_PIN, col(random));
        random = (rand() % 250) + 1;
        analogWrite(G_PIN, col(random));
        random = (rand() % 250) + 1;
        analogWrite(B_PIN, col(random));
    }
    else
    {

        int index = 0;
        String RGB[] = {"", "", ""};
        for (int i = 0; i < length; i++)
        {
            char current = (char)payload[i];
            if (current == ',')
            {
                index++;
            }
            else
            {
                RGB[index] = RGB[index] + current;
            }
        }

        Serial.println();
        Serial.println("-----------------------");

        Serial.println("rgb(" + RGB[0] + "," + RGB[1] + "," + RGB[2] + ")");
        analogWrite(R_PIN, col(RGB[0].toInt()));
        analogWrite(G_PIN, col(RGB[1].toInt()));
        analogWrite(B_PIN, col(RGB[2].toInt()));
    }
}

int col(int rgbValue)
{
    return rgbValue * 4;
}

void reconnect()
{
    // Loop until we're reconnected
    while (!mqttClient.connected())
    {
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (mqttClient.connect(clientId.c_str()))
        {
            Serial.println("connected");
            mqttClient.subscribe("lightBrain");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

/*
 * Fade
 * 
 * - For a better UX if required
 * 
 */
void fade(int pin)
{
    for (int u = 0; u < 1024; u++)
    {
        analogWrite(pin, 1023 - u);
        delay(1);
    }
    for (int u = 0; u < 1024; u++)
    {
        analogWrite(pin, u);
        delay(1);
    }
}

/*
 * Test
 * 
 * - Used during development, calling this function indicated good connections to the LED and all colours active. 
 * 
 */
void testRGB()
{
    // fade in and out of Red, Green, Blue
    analogWrite(R_PIN, 0); // Red off
    analogWrite(G_PIN, 0); // Green off
    analogWrite(B_PIN, 0); // Blue off

    fade(R_PIN); // Red fade effect
    fade(G_PIN); // Green fade effect
    fade(B_PIN); // Blue fade effect
}

/*
 * Off
 * 
 * - Take all pins low (Turn LED off - or black)
 * 
 */
void off()
{

    analogWrite(R_PIN, 0);
    analogWrite(G_PIN, 0);
    analogWrite(B_PIN, 0);
}

/*
 * Flash Pin
 * 
 * - Flash the passed pin for the passed duration in seconds
 * 
 */
void flashPin(int pin, int duration)
{

    for (int u = 0; u < duration; u++)
    {
        analogWrite(pin, 500);
        delay(500);
        analogWrite(pin, 0);
        delay(500);
    }
}

/*
 * Looper
 * 
 * - Poll Web Server to handle connections 
 * - Loop the MQTT client and reconnect if connection was lost
 * 
 */
void loop()
{

    webServer.handleClient();

    if (!mqttClient.connected())
    {
        reconnect();
    }
    mqttClient.loop();
}
