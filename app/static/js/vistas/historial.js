// Pestaña Historial: gráfica de ocupación y registro de docentes
import {api} from '../api.js';
import {datos} from '../datos.js';
import {$, aFecha, duracion, esc, fechaHora, hora, llenarSelect, plural} from '../utilidades.js';

let grafica = null;
const CAMPOS_FILTRO = ['#fAula', '#fDocente', '#fDesde', '#fHasta'];

export function iniciarHistorial() {
  $('#gAula').addEventListener('change', cargarGrafica);
  $('#gHoras').addEventListener('change', cargarGrafica);
  CAMPOS_FILTRO.forEach(s => $(s).addEventListener('change', cargarRegistros));
  $('#limpiarFiltros').addEventListener('click', () => {
    CAMPOS_FILTRO.forEach(s => { $(s).value = ''; });
    cargarRegistros();
  });
  $('#exportar').addEventListener('click', () => { location.href = '/api/registros.csv?' + filtrosRegistros(); });
}

export function actualizarSelectores() {
  const aulas = datos.estado.map(a => ({v: a.id, t: a.nombre}));
  llenarSelect($('#gAula'), aulas);
  llenarSelect($('#fAula'), aulas, 'Todas las aulas');
  llenarSelect($('#fDocente'), datos.docentes.map(d => ({v: d.id, t: d.nombre})), 'Todos los docentes');
}

/* ---------- Gráfica ---------- */
function crearGrafica(etiquetas, valores) {
  return new Chart($('#grafica'), {
    type: 'line',
    data: {
      labels: etiquetas,
      datasets: [{label: 'Personas', data: valores, stepped: true, borderColor: '#6B0B0C',
                  backgroundColor: 'rgba(107,11,12,0.10)', borderWidth: 2, fill: true, pointRadius: 0}],
    },
    options: {
      animation: false, responsive: true, maintainAspectRatio: false,
      plugins: {legend: {display: false}},
      scales: {
        y: {beginAtZero: true, ticks: {precision: 0, color: '#2D120D'}, grid: {color: 'rgba(45,18,13,0.10)'}},
        x: {ticks: {maxTicksLimit: 8, color: '#2D120D'}, grid: {display: false}},
      },
    },
  });
}

// Pico, promedio (ponderado por el tiempo que duró cada valor) y última lectura
function resumenDeLecturas(lecturas) {
  let pico = lecturas[0], suma = 0, tiempo = 0;
  lecturas.forEach((l, i) => {
    if (l.personas > pico.personas) pico = l;
    const t0 = aFecha(l.fecha).getTime();
    const t1 = i + 1 < lecturas.length ? aFecha(lecturas[i + 1].fecha).getTime() : Date.now();
    suma += l.personas * Math.max(0, t1 - t0);
    tiempo += Math.max(0, t1 - t0);
  });
  const promedio = tiempo ? suma / tiempo : lecturas[0].personas;
  const ultima = lecturas[lecturas.length - 1].personas;
  return `Pico de <b>${plural(pico.personas, 'persona', 'personas')}</b> a las ${hora(pico.fecha)}. ` +
         `Promedio de <b>${promedio.toFixed(1)}</b> en el periodo. ` +
         `Última lectura: <b>${plural(ultima, 'persona', 'personas')}</b>.`;
}

export async function cargarGrafica() {
  const texto = $('#lecturaGrafica');
  const aula = $('#gAula').value;
  if (!aula) { texto.textContent = 'Aún no hay aulas para graficar.'; return; }
  if (typeof Chart === 'undefined') {
    texto.textContent = 'No se pudo cargar la librería de gráficas. Este equipo necesita conexión a internet.';
    return;
  }
  const lecturas = (await api(`/api/historial?aula=${encodeURIComponent(aula)}&horas=${$('#gHoras').value}`)).datos;
  const etiquetas = lecturas.map(l => fechaHora(l.fecha));
  const valores = lecturas.map(l => l.personas);

  if (!grafica) {
    grafica = crearGrafica(etiquetas, valores);
  } else {
    grafica.data.labels = etiquetas;
    grafica.data.datasets[0].data = valores;
    grafica.update();
  }
  if (lecturas.length) texto.innerHTML = resumenDeLecturas(lecturas);
  else texto.textContent = 'Todavía no hay lecturas en este periodo.';
}

/* ---------- Registro de docentes ---------- */
function filtrosRegistros() {
  const p = new URLSearchParams();
  const asignar = (clave, selector) => { if ($(selector).value) p.set(clave, $(selector).value); };
  asignar('aula', '#fAula');
  asignar('docente_id', '#fDocente');
  asignar('desde', '#fDesde');
  asignar('hasta', '#fHasta');
  return p;
}

function filaRegistro(r) {
  const salida = r.salida ? fechaHora(r.salida) : '<span class="pill">En el aula</span>';
  const accion = r.salida ? '' :
    `<button class="btn chico" data-cerrar-registro="${r.id}" data-nombre="${esc(r.nombre)}">Marcar salida</button>`;
  return `<tr>
    <td>${esc(r.nombre)}</td><td>${esc(r.aula_id)}</td>
    <td>${fechaHora(r.entrada)}</td><td>${salida}</td>
    <td>${duracion(r.entrada, r.salida)}</td>
    <td class="acciones">${accion}</td>
  </tr>`;
}

export async function cargarRegistros() {
  const p = filtrosRegistros();
  p.set('limite', '200');
  const filas = (await api('/api/registros?' + p)).datos;
  $('#tablaRegistros').innerHTML = filas.length
    ? filas.map(filaRegistro).join('')
    : '<tr><td colspan="6" class="vacio">No hay registros con estos filtros.</td></tr>';
}
