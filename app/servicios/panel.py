"""Consultas que alimentan el panel: estado, historial, registros y exportación."""
import csv
import io
from datetime import datetime, timedelta

from flask import current_app

from ..repositorios import aulas, conteos, docentes, registros
from ..utilidades import (FORMATO, a_fecha, fecha_valida, limpiar_aula,
                          segundos_desde)


def estado_aulas():
    resultado = []
    for aula in aulas.listar():
        ultima = aula["ultima_conexion"]
        en_linea = bool(ultima) and segundos_desde(ultima) < current_app.config["SEGUNDOS_EN_LINEA"]
        resultado.append({
            "id": aula["id"],
            "nombre": aula["nombre"],
            "personas": aula["personas"] or 0,
            "ultima_conexion": ultima,
            "en_linea": en_linea,
            "docentes": [dict(d) for d in registros.abiertos_de_aula(aula["id"])],
        })
    return resultado


def historial_conteos(aula, horas):
    horas = min(max(horas, 1), 24 * 30)
    desde = (datetime.now() - timedelta(hours=horas)).strftime(FORMATO)
    filas = conteos.listar_desde(limpiar_aula(aula), desde)
    return [{"fecha": f["fecha"], "personas": f["personas"]} for f in filas]


def normalizar_filtros(args):
    """Convierte los parámetros de la URL en filtros limpios."""
    return {
        "aula": limpiar_aula(args.get("aula")) or None,
        "docente_id": args.get("docente_id", type=int) or None,
        "desde": fecha_valida(args.get("desde")),
        "hasta": fecha_valida(args.get("hasta")),
    }


def listar_registros(filtros, limite):
    limite = min(max(limite, 1), 1000)
    return [dict(f) for f in registros.buscar(filtros, limite)]


def listar_docentes():
    return [dict(d) for d in docentes.listar_con_ubicacion()]


def exportar_registros_csv(filtros):
    buffer = io.StringIO()
    escritor = csv.writer(buffer)
    escritor.writerow(["Docente", "Aula", "Entrada", "Salida", "Duración (min)"])
    for f in registros.buscar(filtros, 10000):
        duracion = ""
        if f["salida"]:
            duracion = round((a_fecha(f["salida"]) - a_fecha(f["entrada"])).total_seconds() / 60)
        escritor.writerow([f["nombre"], f["aula_id"], f["entrada"], f["salida"] or "", duracion])
    # El BOM inicial hace que Excel muestre bien los acentos
    return "\ufeff" + buffer.getvalue()
