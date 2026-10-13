// Avisos breves en la parte inferior de la pantalla
import {$} from './utilidades.js';

let temporizador;

export function aviso(texto, error = false) {
  const el = $('#aviso');
  el.textContent = texto;
  el.classList.toggle('error', error);
  el.classList.add('ver');
  clearTimeout(temporizador);
  temporizador = setTimeout(() => el.classList.remove('ver'), 3200);
}
