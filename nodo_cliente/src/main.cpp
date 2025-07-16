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

uint8_t buffer[256]; // CBOR buffer
CborEncoder encoder, mapEncoder, locEncoder;
void publicarDatosSensor() {
    float temperatura = random(10, 40);
    float humedad = random(0, 100);
    float viento = random(0, 40);
    float sensacionTermica = temperatura - (viento * 0.2);
    float presion = random(980, 1050);
    float lluvia = random(0, 10);
    float radiacion = random(0, 1000);
    float calidadAire = random(0, 500);

    cbor_encoder_init(&encoder, buffer, sizeof(buffer), 0);
    cbor_encoder_create_map(&encoder, &mapEncoder, CborIndefiniteLength);

    cbor_encode_text_stringz(&mapEncoder, "station_id");
    cbor_encode_text_stringz(&mapEncoder, ESTACION_ID);

    if (ENVIAR_TEMPERATURA) {
        cbor_encode_text_stringz(&mapEncoder, "temperatura");
        cbor_encode_float(&mapEncoder, temperatura);
    }
    if (ENVIAR_HUMEDAD) {
        cbor_encode_text_stringz(&mapEncoder, "humedad");
        cbor_encode_float(&mapEncoder, humedad);
    }
    if (ENVIAR_SENSACION_TERMICA) {
        cbor_encode_text_stringz(&mapEncoder, "sensacion_termica");
        cbor_encode_float(&mapEncoder, sensacionTermica);
    }
    if (ENVIAR_PRESION) {
        cbor_encode_text_stringz(&mapEncoder, "presion");
        cbor_encode_float(&mapEncoder, presion);
    }
    if (ENVIAR_LLUVIA) {
        cbor_encode_text_stringz(&mapEncoder, "lluvia");
        cbor_encode_float(&mapEncoder, lluvia);
    }
    if (ENVIAR_VIENTO) {
        cbor_encode_text_stringz(&mapEncoder, "viento");
        cbor_encode_float(&mapEncoder, viento);
    }
    if (ENVIAR_RADIACION_SOLAR) {
        cbor_encode_text_stringz(&mapEncoder, "radiacion_solar");
        cbor_encode_float(&mapEncoder, radiacion);
    }
    if (ENVIAR_CALIDAD_AIRE) {
        cbor_encode_text_stringz(&mapEncoder, "calidad_aire");
        cbor_encode_float(&mapEncoder, calidadAire);
    }

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
