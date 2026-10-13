// Radar LD2450: lee las posiciones y cuenta entradas y salidas
#pragma once

namespace Conteo {
  void iniciar();
  void actualizar();                 // llamar en cada vuelta de loop()
  int  personas();                   // personas contadas dentro del aula
  void reiniciar();                  // vuelve el contador a 0
  unsigned long bytesRecibidos();    // bytes que han llegado del radar (para diagnóstico)
  unsigned long tramasRecibidas();   // tramas completas y válidas del radar
  void alternarDiagnostico();        // muestra u oculta las posiciones en el Monitor Serie
  bool hayCambioPendiente();         // true si el conteo cambió y aún no se envía
  void limpiarCambioPendiente();
}
