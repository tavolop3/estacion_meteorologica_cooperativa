#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
const int MSG_INTERVAL = 2000;

void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Conectando a ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi conectado");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Intentando conexión MQTT...");
        if (client.connect(ESTACION_ID)) {
            Serial.println("conectado");
        } else {
            Serial.print("falló, rc=");
            Serial.print(client.state());
            Serial.println(" intentando de nuevo en 5 segundos");
            delay(5000);
        }
    }
}

void publicarDatosSensor() {
    float temperatura = random(10, 40);
    float humedad = random(0, 100);

    JsonDocument jsonDoc;
    jsonDoc["station_id"] = ESTACION_ID;
    jsonDoc["temperatura"] = temperatura;
    jsonDoc["humedad"] = humedad;

    JsonObject location = jsonDoc["ubicacion"].to<JsonObject>();
    location["lat"] = LATITUD;
    location["lon"] = LONGITUD;

    char buffer[256];
    size_t n = serializeJson(jsonDoc, buffer);

    client.publish("barrio/estaciones/datos", buffer, n);

    Serial.print("Publicando mensaje: ");
    Serial.println(buffer);
}

void setup() {
    Serial.begin(9600);
    setup_wifi();
    client.setServer(MQTT_BROKER, MQTT_PORT);
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    long now = millis();
    if (now - lastMsg > MSG_INTERVAL) {
        lastMsg = now;
        publicarDatosSensor();
    }
}