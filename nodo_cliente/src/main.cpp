#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include <tinycbor.h>

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

    uint8_t buffer[256]; // CBOR buffer
    CborEncoder encoder, mapEncoder, locEncoder;

    cbor_encoder_init(&encoder, buffer, sizeof(buffer), 0);
    
    // mapa principal
    cbor_encoder_create_map(&encoder, &mapEncoder, 5);
    
    cbor_encode_text_stringz(&mapEncoder, "station_id");
    cbor_encode_text_stringz(&mapEncoder, ESTACION_ID);

    cbor_encode_text_stringz(&mapEncoder, "temperatura");
    cbor_encode_float(&mapEncoder, temperatura);

    cbor_encode_text_stringz(&mapEncoder, "humedad");
    cbor_encode_float(&mapEncoder, humedad);

    cbor_encode_text_stringz(&mapEncoder, "localidad");
    cbor_encode_text_stringz(&mapEncoder, LOCALIDAD);

    cbor_encode_text_stringz(&mapEncoder, "ubicacion");
    cbor_encoder_create_map(&mapEncoder, &locEncoder, 2);
    cbor_encode_text_stringz(&locEncoder, "lat");
    cbor_encode_float(&locEncoder, LATITUD);
    cbor_encode_text_stringz(&locEncoder, "lon");
    cbor_encode_float(&locEncoder, LONGITUD);
    cbor_encoder_close_container(&mapEncoder, &locEncoder);

    cbor_encoder_close_container(&encoder, &mapEncoder);

    size_t encodedLength = cbor_encoder_get_buffer_size(&encoder, buffer);

    String topic = "barrio/estaciones/" + String(CLAVE);
    
    Serial.print("Publicando en el canal: ");
    Serial.println(topic);

    client.publish(topic.c_str(), buffer, encodedLength);
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
