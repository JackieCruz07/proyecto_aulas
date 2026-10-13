// Acción compartida por el Resumen y el Historial: marcar la salida de un docente
import {enviar} from './api.js';
import {pedirRefresco} from './datos.js';

export function iniciarAcciones() {
  document.addEventListener('click', async e => {
    const boton = e.target.closest('[data-cerrar-registro]');
    if (!boton) return;
    if (!confirm(`¿Marcar la salida de ${boton.dataset.nombre} ahora mismo?`)) return;
    if (await enviar(`/api/registros/${boton.dataset.cerrarRegistro}/cerrar`, 'POST', null, 'Salida registrada')) {
      pedirRefresco();
    }
  });
}
