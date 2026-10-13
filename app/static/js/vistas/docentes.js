// Pestaña Docentes: alta, edición y baja
import {enviar} from '../api.js';
import {datos, pedirRefresco} from '../datos.js';
import {$, esc, hora} from '../utilidades.js';

let editando = null;

export function iniciarDocentes() {
  $('#formDocente').addEventListener('submit', async e => {
    e.preventDefault();
    if (await enviar('/api/docentes', 'POST', {nombre: $('#dNombre').value, uid: $('#dUid').value}, 'Docente agregado')) {
      e.target.reset();
      pedirRefresco();
    }
  });

  $('#formEditarDocente').addEventListener('submit', async e => {
    e.preventDefault();
    const cambios = {nombre: $('#eDNombre').value, uid: $('#eDUid').value};
    if (await enviar(`/api/docentes/${editando.id}`, 'PUT', cambios, 'Cambios guardados')) {
      $('#dlgDocente').close();
      pedirRefresco();
    }
  });

  $('#tablaDocentes').addEventListener('click', async e => {
    const botonEditar = e.target.closest('[data-editar-docente]');
    if (botonEditar) {
      editando = datos.docentes.find(d => d.id == botonEditar.dataset.editarDocente);
      $('#eDNombre').value = editando.nombre;
      $('#eDUid').value = editando.uid;
      $('#dlgDocente').showModal();
    }
    const botonEliminar = e.target.closest('[data-eliminar-docente]');
    if (botonEliminar) {
      const d = datos.docentes.find(x => x.id == botonEliminar.dataset.eliminarDocente);
      if (!confirm(`¿Eliminar a ${d.nombre}? Su historial de entradas y salidas se conserva.`)) return;
      if (await enviar(`/api/docentes/${d.id}`, 'DELETE', null, 'Docente eliminado')) pedirRefresco();
    }
  });
}

function filaDocente(d) {
  const ubicacion = d.aula_actual
    ? `<span class="pill">En ${esc(d.aula_actual)} desde las ${hora(d.desde)}</span>`
    : '<span class="pill apagada">Fuera de las aulas</span>';
  return `<tr>
    <td>${esc(d.nombre)}</td>
    <td class="uid">${esc(d.uid)}</td>
    <td>${ubicacion}</td>
    <td class="acciones">
      <button class="btn chico" data-editar-docente="${d.id}">Editar</button>
      <button class="btn chico peligro" data-eliminar-docente="${d.id}">Eliminar</button>
    </td>
  </tr>`;
}

export function dibujarDocentes() {
  $('#tablaDocentes').innerHTML = datos.docentes.length
    ? datos.docentes.map(filaDocente).join('')
    : '<tr><td colspan="4" class="vacio">Todavía no hay docentes. Agrega el primero con el formulario de arriba.</td></tr>';
}
