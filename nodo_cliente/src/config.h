#ifndef CONFIG_H
#define CONFIG_H

// Datos de la Red WiFi
#define WIFI_SSID ""
#define WIFI_PASS ""

// Datos del Servidor MQTT
#define MQTT_BROKER "192.168.0.190"
#define MQTT_PORT 1883

// Datos de la Estación
#define ESTACION_ID "casa_mati"
#define LATITUD -34.9214
#define LONGITUD -57.9545
#define LOCALIDAD "Los Hornos"

// Variables de configuración para el envío de datos, true para enviar, false para no enviar
#define ENVIAR_TEMPERATURA true
#define ENVIAR_HUMEDAD true
#define ENVIAR_SENSACION_TERMICA true
#define ENVIAR_PRESION true
#define ENVIAR_LLUVIA true
#define ENVIAR_VIENTO true
#define ENVIAR_RADIACION_SOLAR true
#define ENVIAR_CALIDAD_AIRE true

// Clave que dio la página web
#define CLAVE 1

#endif
