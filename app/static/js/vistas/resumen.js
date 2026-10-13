// Pestaña Resumen: frase de lectura y tablero de aulas
import {datos} from '../datos.js';
import {$, $$, esc, hora, plural} from '../utilidades.js';

let filtro = 'todas';

export function iniciarResumen() {
  $$('.segmentos button').forEach(boton => boton.addEventListener('click', () => {
    filtro = boton.dataset.filtro;
    $$('.segmentos button').forEach(b => b.setAttribute('aria-pressed', b === boton));
    dibujarResumen();
  }));
  $('#buscar').addEventListener('input', dibujarResumen);
}

function aulasVisibles() {
  const q = $('#buscar').value.trim().toLowerCase();
  return datos.estado.filter(a => {
    const docentes = a.docentes.map(d => d.nombre).join(' ').toLowerCase();
    if (q && !(a.nombre.toLowerCase().includes(q) || a.id.toLowerCase().includes(q) || docentes.includes(q))) return false;
    if (filtro === 'con') return a.docentes.length > 0;
    if (filtro === 'sin') return a.docentes.length === 0;
    if (filtro === 'off') return !a.en_linea;
    return true;
  });
}

const alumnosDe = a => Math.max(0, a.personas - a.docentes.length);

function frase() {
  const total = datos.estado.length;
  if (!total) return 'Todavía no hay aulas registradas.';
  const enLinea = datos.estado.filter(a => a.en_linea).length;
  const conDocente = datos.estado.filter(a => a.docentes.length > 0).length;
  const alumnos = datos.estado.reduce((suma, a) => suma + alumnosDe(a), 0);
  return `<b>${enLinea}</b> de <b>${total}</b> ${total === 1 ? 'aula en línea' : 'aulas en línea'}. ` +
         `<b>${conDocente}</b> con docente. <b>${alumnos}</b> ${alumnos === 1 ? 'alumno' : 'alumnos'} en total.`;
}

function filaAula(a) {
  const alumnos = alumnosDe(a);
  const detalle = a.en_linea
    ? 'de ' + plural(a.personas, 'persona detectada', 'personas detectadas')
    : 'dato de las ' + hora(a.ultima_conexion);
  const docentes = a.docentes.length
    ? a.docentes.map(d => `<div><b>${esc(d.nombre)}</b><span>desde las ${hora(d.entrada)}</span></div>`).join('')
    : '<span class="ninguno">Sin docente</span>';
  const botones = a.docentes.map(d =>
    `<button class="btn chico" data-cerrar-registro="${d.registro_id}" data-nombre="${esc(d.nombre)}">Marcar salida</button>`
  ).join('');

  return `<li class="fila">
    <div>
      <span class="nombre">${esc(a.nombre)}</span>
      <span class="estado ${a.en_linea ? '' : 'fuera'}">${a.en_linea ? 'En línea' : 'Sin conexión'}</span>
    </div>
    <div>
      <span class="cifra">${alumnos}</span>
      <span class="detalle">${alumnos === 1 ? 'alumno' : 'alumnos'}<br>${detalle}</span>
    </div>
    <div class="docente">${docentes}</div>
    <div>${botones}</div>
  </li>`;
}

export function dibujarResumen() {
  $('#lectura').innerHTML = frase();
  const visibles = aulasVisibles();
  $('#tablero').innerHTML = visibles.length
    ? visibles.map(filaAula).join('')
    : `<li class="vacio">${datos.estado.length
        ? 'Ninguna aula coincide con el filtro.'
        : 'Las aulas aparecen solas cuando un ESP32 reporta datos, o puedes crearlas en la pestaña Aulas.'}</li>`;
}
