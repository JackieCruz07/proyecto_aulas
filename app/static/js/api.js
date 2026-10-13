// Comunicación con el servidor
import {aviso} from './ui.js';

export async function api(url, opciones) {
  const r = await fetch(url, opciones);
  if (r.status === 401) {            // la sesión venció: volver al acceso
    location.href = '/login';
    throw new Error('sesion');
  }
  let datos = {};
  try { datos = await r.json(); } catch (e) { /* respuesta sin cuerpo */ }
  return {ok: r.ok && datos.ok !== false, datos};
}

// Envía un cambio (crear, editar, eliminar) y avisa el resultado
export async function enviar(url, metodo, cuerpo, textoOk) {
  const r = await api(url, {
    method: metodo,
    headers: {'Content-Type': 'application/json'},
    body: cuerpo ? JSON.stringify(cuerpo) : undefined,
  });
  r.ok ? aviso(textoOk) : aviso(r.datos.error || 'No se pudo completar la acción.', true);
  return r.ok;
}
