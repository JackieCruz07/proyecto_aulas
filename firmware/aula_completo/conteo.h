// Radar LD2450: sigue a las personas dentro del pasillo de la puerta y emite
// eventos de cruce (entrada = cerca->lejos, salida = lejos->cerca).
// El conteo oficial lo decide verificacion.cpp (radar + ToF fusionados).
#pragma once

namespace Conteo {
  void iniciar();
  void actualizar();                 // llamar en cada vuelta de loop()
  bool proximoEvento(int& dir, unsigned long& ms);  // drena un evento: +1 entrada, -1 salida
  int  personasSolo();               // lo que contaría el radar sin ToF (solo diagnóstico)
  void reiniciar();                  // vuelve el conteo provisional a 0 y vacía los eventos
  unsigned long bytesRecibidos();    // bytes que han llegado del radar (para diagnóstico)
  unsigned long tramasRecibidas();   // tramas completas y válidas del radar
  unsigned long ultimoDatoMs();      // cuándo llegó la última trama (0 = nunca)
  void alternarDiagnostico();        // muestra u oculta las posiciones en el Monitor Serie
}
