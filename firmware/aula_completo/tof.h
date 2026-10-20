// Dos sensores VL53L1X ARRIBA del marco de la puerta (A = lado de fuera,
// B = lado de dentro, apuntando al umbral): detectan quién cruza y en qué
// sentido. El conteo oficial lo decide verificacion.cpp (radar + ToF fusionados).
#pragma once

namespace Tof {
  void iniciar();                   // arranca los dos sensores, les asigna dirección y calibra
  void actualizar();                // llamar en cada vuelta de loop()
  bool disponible();                // true si los dos sensores arrancaron bien
  bool proximoEvento(int& dir, unsigned long& ms);  // drena un evento: +1 entrada (A->B), -1 salida (B->A)
  int  personas();                  // conteo propio del ToF (entradas - salidas, nunca negativo)
  void reiniciar();                 // pone el conteo del ToF en 0 y vacía los eventos
  void recalibrar();                // vuelve a medir la distancia "libre" (la puerta debe estar vacía)
  void alternarDiagnostico();       // muestra u oculta las distancias en el Monitor Serie
}
