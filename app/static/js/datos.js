// Datos compartidos por todas las vistas y aviso de "hay que refrescar"
import {api} from './api.js';

export const datos = {
  estado: [],     // aulas con su conteo y docentes presentes
  docentes: [],   // docentes registrados
};

export async function cargarEstado() {
  datos.estado = (await api('/api/estado')).datos;
}

export async function cargarDocentes() {
  datos.docentes = (await api('/api/docentes')).datos;
}

// Las vistas piden refrescar después de un cambio; principal.js escucha y recarga
const canal = new EventTarget();
export const pedirRefresco = () => canal.dispatchEvent(new Event('refrescar'));
export const alPedirRefresco = fn => canal.addEventListener('refrescar', fn);
