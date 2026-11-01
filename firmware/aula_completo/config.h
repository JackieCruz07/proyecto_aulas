#pragma once
#include <Arduino.h>

// ---- Red y servidor ----
const char* const WIFI_NOMBRE = "MateBook JACKIE";
const char* const WIFI_CLAVE  = "12345678";
const char* const SERVIDOR    = "http://192.168.43.111:5000";
const char* const AULA_ID     = "D103";
const char* const API_KEY     = "04OCT2026";

// ---- Pines ----
// Radar LD2450
constexpr int RADAR_RX_PIN = 16; 
constexpr int RADAR_TX_PIN = 17;
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

// =====================================================================
//  CÓMO ESTÁ MONTADO EL EQUIPO (define el sentido de las cuentas)
//
//  TOF A y B: ARRIBA del marco de la puerta, uno tras otro en el sentido
//  en que camina la gente: A del lado de FUERA, B del lado de DENTRO
//  (apuntando hacia abajo, al umbral).
//
//  RADAR LD2450: en el marco de la puerta, a unos 135 cm del piso, DENTRO
//  del salón y mirando HACIA ADENTRO. Por eso:
//    - Quien ENTRA se acerca al radar y luego se aleja   -> CERCA a LEJOS
//    - Quien SALE viene de adentro hacia el radar         -> LEJOS a CERCA
//
//  Y quién cuenta y quién confirma:
//    - ENTRADA: la cuentan los ToF (cruce A->B) y el RADAR confirma.
//    - SALIDA:  la cuenta el RADAR (lejos->cerca) y los ToF confirman (B->A).
// =====================================================================

// ---- Cobertura del radar: el pasillo de la puerta (milímetros) ----
// Medidas del salón (puerta de 85 cm, mesas a ~150 cm del marco):
// el radar solo debe vigilar el pasillo de la puerta, NO las mesas.
constexpr int  LINEA_Y        = 700;   // línea a 70 cm del radar: al pasarla se considera que entró/salió
constexpr int  HISTERESIS     = 120;   // zona muerta ±120 mm alrededor de la línea (evita rebotes)
constexpr int  Y_MIN          = 200;   // zona ciega del radar: se ignora todo lo más cerca que esto
constexpr int  Y_MAX          = 1800;  // se ignora a más de 1.8 m: así no ve a la gente de las mesas (150 cm)
constexpr int  X_MAX          = 600;   // ancho del pasillo seguido (±): la puerta mide 85 cm, no hace falta más
constexpr int  ANGULO_MAXIMO  = 25;    // solo se sigue a quien esté dentro de ±25° del eje del radar
                                       // (el LD2450 ve ±60°; estrechar el cono evita "personas de más")
constexpr bool INVERTIR       = false; // true si en tu montaje entradas y salidas quedan al revés

// Filtros contra ruido y cambios de "ranura" del radar (cuando hay varias personas a la vez)
constexpr int  MIN_FRAMES_EN_ZONA = 3;    // tramas seguidas en la nueva zona antes de creer un cruce
                                          // (3 = ~300 ms: evita que el movimiento de una silla cuente)
constexpr int  FRAMES_PERDIDO     = 20;   // tramas sin ver a la persona antes de olvidarla (~2 s)
constexpr int  MAX_SALTO_MM       = 400;  // un salto mayor que esto en una trama es imposible caminando:
                                          // la ranura cambió de persona y se vuelve a seguirla desde cero
constexpr unsigned long MIN_MS_CRUCE_RANURA = 800;  // dos cruces de la misma ranura no vienen más seguidos

// ---- Envío al servidor ----
constexpr unsigned long INTERVALO_LATIDO_MS = 60000UL;   // cada minuto avisa que sigue vivo
constexpr unsigned long TIEMPO_WIFI_MS      = 15000UL;   // cuánto esperar el WiFi al arrancar

// ---- Lector RFID ----
constexpr unsigned long AUSENCIA_TARJETA_MS = 800;     // la tarjeta debe alejarse este tiempo para leerla de nuevo
constexpr unsigned long REVISION_LECTOR_MS  = 10000UL; // cada cuánto se comprueba que el lector responde

// ---- Sensores VL53L1X ARRIBA del marco de la puerta (confirman los cruces) ----
// Van uno tras otro en el sentido de caminata: A = lado de fuera, B = lado de dentro.
// El orden en que se activan dice quién cruza y en qué sentido: A->B = entrada, B->A = salida.
constexpr bool TOF_ACTIVO = true;          // false si todavía no los conectas
constexpr int  TOF_SDA_PIN = 21;           // bus I2C compartido por los dos sensores
constexpr int  TOF_SCL_PIN = 22;
constexpr int  TOF_XSHUT_A_PIN = 25;       // cada sensor tiene su propio pin XSHUT
constexpr int  TOF_XSHUT_B_PIN = 26;
constexpr uint8_t TOF_DIRECCION_A = 0x30;  // direcciones I2C que se les asignan al arrancar
constexpr uint8_t TOF_DIRECCION_B = 0x31;
constexpr int  TOF_MODO = 2;               // 0 corto (hasta 1.3 m), 1 medio (hasta 3 m), 2 largo (hasta 4 m)
                                           // arriba del marco apuntando al piso hacen falta ~2 m: modo largo
constexpr int  TOF_MARGEN_MM = 250;        // cuánto debe acortarse la distancia para contar "hay alguien"
constexpr int  TOF_MIN_BASE_MM = 1200;     // distancia libre mínima al calibrar: si mide menos, el sensor
                                           // no está alto ni apuntando al umbral (puerta vacía al calibrar)
constexpr int  TOF_MAX_BASE_MM = 3000;     // si mide más que esto, el sensor apunta demasiado lejos
constexpr unsigned long TOF_ENTRE_SENSORES_MS = 1500;   // tiempo máximo entre que se activa un sensor y el otro
constexpr unsigned long TOF_LIBERAR_MS = 300;           // tiempo libre necesario para cerrar un paso
constexpr bool TOF_INVERTIR = false;       // true si A y B están puestos al revés (B del lado de fuera)

// ---- Fusión radar + ToF: quién cuenta y quién confirma ----
// ENTRADA: propone el ToF (A->B); el radar confirma si además ve a la persona
//          pasar de cerca a lejos. Sin confirmación se descarta (se asomaron).
// SALIDA:  propone el radar (lejos->cerca); el ToF confirma si además ve el
//          cruce por la puerta (B->A). Sin confirmación se descarta (se
//          acercaron a la puerta y se regresaron sin salir).
constexpr unsigned long CONFIRMACION_ESPERA_MS = 5000; // tiempo máximo que el confirmador puede tardar
                                                       // (5 s: cubre a quien se detiene un momento en la puerta)
constexpr int  CONFIRMACION_ADELANTO_MS = 400;         // el confirmador puede aparecer hasta 400 ms "antes"
                                                       // del evento que confirma (rezagos de medición)
constexpr int  MAX_GRUPO = 2;        // un confirmador puede validar hasta 2 cruces seguidos de una pareja
                                     // que cruza la puerta juntita
// Si el radar no manda tramas durante este tiempo, se considera caído y el ToF cuenta solo (con aviso).
constexpr unsigned long RADAR_SIN_DATOS_MS = 5000;
