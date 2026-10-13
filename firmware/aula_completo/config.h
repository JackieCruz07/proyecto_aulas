#pragma once
#include <Arduino.h>

// ---- Red y servidor ----
const char* const WIFI_NOMBRE = "MateBook JACKIE";
const char* const WIFI_CLAVE  = "12345678";
const char* const SERVIDOR    = "http://192.168.0.103:5000";
const char* const AULA_ID     = "D103";
const char* const API_KEY     = "04OCT2026";

// ---- Pines ----
// Radar LD2450
constexpr int RADAR_RX_PIN = 16;
constexpr int RADAR_TX_PIN = 17;
// Lector RFID RC522
constexpr int RFID_SS_PIN   = 5;
constexpr int RFID_RST_PIN  = 27;
constexpr int RFID_SCK_PIN  = 18;
constexpr int RFID_MISO_PIN = 19;
constexpr int RFID_MOSI_PIN = 23;
// LED azul integrado
constexpr int LED_PIN = 2;

// ---- Velocidad del radar ----
constexpr unsigned long RADAR_BAUDIOS = 256000;   // velocidad de fábrica del LD2450
constexpr bool RADAR_BAUDIOS_AUTO = true;         // si no llegan tramas, prueba otras velocidades al arrancar

// ---- Ajustes del conteo (milímetros) ----
constexpr int  LINEA_Y        = 1500;   // distancia de la línea imaginaria al radar
constexpr int  HISTERESIS     = 150;    // zona muerta alrededor de la línea
constexpr int  X_MAX          = 1000;   // se ignora lo que esté más a los lados (±)
constexpr int  Y_MAX          = 4000;   // se ignora lo que esté más lejos
constexpr int  FRAMES_PERDIDO = 15;     // tramas sin ver a la persona antes de olvidarla (~1.5 s)
constexpr bool INVERTIR       = false;  // true si entradas y salidas te quedan al revés

// ---- Envío al servidor ----
constexpr unsigned long INTERVALO_LATIDO_MS = 60000UL;   // cada minuto avisa que sigue vivo
constexpr unsigned long TIEMPO_WIFI_MS      = 15000UL;   // cuánto esperar el WiFi al arrancar

// ---- Lector RFID ----
constexpr unsigned long AUSENCIA_TARJETA_MS = 800;     // la tarjeta debe alejarse este tiempo para leerla de nuevo
constexpr unsigned long REVISION_LECTOR_MS  = 10000UL; // cada cuánto se comprueba que el lector responde
