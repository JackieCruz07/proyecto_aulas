// Pestaña Aulas: alta, cambio de nombre y baja
import {enviar} from '../api.js';
import {datos, pedirRefresco} from '../datos.js';
import {$, esc, hora} from '../utilidades.js';

let editando = null;

export function iniciarAulas() {
  $('#formAula').addEventListener('submit', async e => {
    e.preventDefault();
    if (await enviar('/api/aulas', 'POST', {id: $('#aId').value, nombre: $('#aNombre').value}, 'Aula agregada')) {
      e.target.reset();
      pedirRefresco();
    }
  });

  $('#formEditarAula').addEventListener('submit', async e => {
    e.preventDefault();
    if (await enviar(`/api/aulas/${encodeURIComponent(editando.id)}`, 'PUT', {nombre: $('#eANombre').value}, 'Cambios guardados')) {
      $('#dlgAula').close();
      pedirRefresco();
    }
  });

  $('#tablaAulas').addEventListener('click', async e => {
    const botonEditar = e.target.closest('[data-editar-aula]');
    if (botonEditar) {
      editando = datos.estado.find(a => a.id === botonEditar.dataset.editarAula);
      $('#eANombre').value = editando.nombre;
      $('#dlgAula').showModal();
    }
    const botonEliminar = e.target.closest('[data-eliminar-aula]');
    if (botonEliminar) {
      const id = botonEliminar.dataset.eliminarAula;
      const aviso = `¿Eliminar el aula ${id} junto con todo su historial de conteos y registros? ` +
        'Esta acción no se puede deshacer.\n\nSi su ESP32 sigue encendido, el aula volverá a aparecer en cuanto reporte.';
      if (!confirm(aviso)) return;
      if (await enviar(`/api/aulas/${encodeURIComponent(id)}`, 'DELETE', null, 'Aula eliminada')) pedirRefresco();
    }
  });
}

function filaAula(a) {
  const estado = a.en_linea ? '<span class="pill">En línea</span>' : '<span class="pill apagada">Sin conexión</span>';
  const docentes = a.docentes.length ? esc(a.docentes.map(d => d.nombre).join(', ')) : '—';
  return `<tr>
    <td><b>${esc(a.id)}</b></td>
    <td>${esc(a.nombre)}</td>
    <td>${estado}</td>
    <td>${a.personas}</td>
    <td>${docentes}</td>
    <td>${hora(a.ultima_conexion)}</td>
    <td class="acciones">
      <button class="btn chico" data-editar-aula="${esc(a.id)}">Editar</button>
      <button class="btn chico peligro" data-eliminar-aula="${esc(a.id)}">Eliminar</button>
    </td>
  </tr>`;
}

export function dibujarAulas() {
  $('#tablaAulas').innerHTML = datos.estado.length
    ? datos.estado.map(filaAula).join('')
    : '<tr><td colspan="7" class="vacio">Todavía no hay aulas. Agrega la primera con el formulario de arriba.</td></tr>';
}
