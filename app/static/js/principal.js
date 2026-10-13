// Punto de entrada del panel: pestañas, reloj y actualización periódica
import {iniciarAcciones} from './acciones.js';
import {alPedirRefresco, cargarDocentes, cargarEstado} from './datos.js';
import {$, $$} from './utilidades.js';
import {dibujarAulas, iniciarAulas} from './vistas/aulas.js';
import {dibujarDocentes, iniciarDocentes} from './vistas/docentes.js';
import {actualizarSelectores, cargarGrafica, cargarRegistros, iniciarHistorial} from './vistas/historial.js';
import {dibujarResumen, iniciarResumen} from './vistas/resumen.js';

const PESTAÑAS = ['resumen', 'historial', 'docentes', 'aulas'];
const CADA_CUANTO_MS = 5000;
let pestañaActual = 'resumen';

async function refrescar() {
  try {
    await cargarEstado();
    dibujarResumen();
    dibujarAulas();
    if (pestañaActual === 'historial' || pestañaActual === 'docentes') await cargarDocentes();
    if (pestañaActual === 'docentes') dibujarDocentes();
    if (pestañaActual === 'historial') {
      actualizarSelectores();
      await Promise.all([cargarGrafica(), cargarRegistros()]);
    }
  } catch (e) {
    if (e.message !== 'sesion') console.error(e);
  }
}

function mostrarPestaña(nombre) {
  if (!PESTAÑAS.includes(nombre)) nombre = 'resumen';
  pestañaActual = nombre;
  $$('[role=tab]').forEach(b => b.setAttribute('aria-selected', b.dataset.tab === nombre));
  $$('.vista').forEach(v => { v.hidden = v.id !== 'vista-' + nombre; });
  history.replaceState(null, '', '#' + nombre);
  refrescar();
}

function iniciarPestañas() {
  const botones = $$('[role=tab]');
  botones.forEach((boton, i) => {
    boton.addEventListener('click', () => mostrarPestaña(boton.dataset.tab));
    boton.addEventListener('keydown', e => {      // flechas del teclado para cambiar de pestaña
      if (e.key !== 'ArrowRight' && e.key !== 'ArrowLeft') return;
      const paso = e.key === 'ArrowRight' ? 1 : botones.length - 1;
      const siguiente = botones[(i + paso) % botones.length];
      siguiente.focus();
      mostrarPestaña(siguiente.dataset.tab);
    });
  });
}

function iniciarReloj() {
  const pintar = () => {
    $('#reloj').textContent = new Date().toLocaleTimeString('es-MX', {hour: 'numeric', minute: '2-digit', hour12: true});
  };
  pintar();
  setInterval(pintar, 15000);
}

// Arranque
iniciarAcciones();
iniciarResumen();
iniciarHistorial();
iniciarDocentes();
iniciarAulas();
iniciarPestañas();
iniciarReloj();
$$('[data-cerrar]').forEach(b => b.addEventListener('click', () => b.closest('dialog').close()));
alPedirRefresco(refrescar);
setInterval(refrescar, CADA_CUANTO_MS);
mostrarPestaña(location.hash.slice(1));
