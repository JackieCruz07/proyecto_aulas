// Funciones pequeñas de apoyo: selección de elementos, formatos de fecha y texto

export const $ = s => document.querySelector(s);
export const $$ = s => [...document.querySelectorAll(s)];

// Evita que un nombre con símbolos rompa el HTML
export const esc = t => String(t ?? '').replace(/[&<>"']/g,
  c => ({'&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'}[c]));

export const aFecha = f => new Date(f.replace(' ', 'T'));

export const hora = f => f
  ? aFecha(f).toLocaleTimeString('es-MX', {hour: 'numeric', minute: '2-digit', hour12: true})
  : '—';

export const fechaHora = f => f
  ? aFecha(f).toLocaleString('es-MX', {day: '2-digit', month: '2-digit', hour: 'numeric', minute: '2-digit', hour12: true})
  : '—';

export const plural = (n, singular, plural) => `${n} ${n === 1 ? singular : plural}`;

export function duracion(entrada, salida) {
  const fin = salida ? aFecha(salida) : new Date();
  const min = Math.max(0, Math.round((fin - aFecha(entrada)) / 60000));
  const h = Math.floor(min / 60), m = min % 60;
  return h ? `${h} h ${m} min` : `${m} min`;
}

// Llena un <select> sin tocarlo si no cambió (así no se cierra el menú si está abierto)
export function llenarSelect(el, items, textoTodas) {
  const firma = JSON.stringify([items, textoTodas]);
  if (el.dataset.firma === firma) return;
  const actual = el.value;
  el.innerHTML = (textoTodas ? `<option value="">${esc(textoTodas)}</option>` : '') +
    items.map(i => `<option value="${esc(i.v)}">${esc(i.t)}</option>`).join('');
  el.dataset.firma = firma;
  if ([...el.options].some(o => o.value === actual)) el.value = actual;
}
