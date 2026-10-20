"""
Configuración del servidor.

Los valores se pueden cambiar aquí directamente o, de forma más segura,
con variables de entorno (AULAS_API_KEY, AULAS_CLAVE_PANEL, AULAS_CLAVE_SECRETA).
"""
import os
from datetime import timedelta

RAIZ = os.path.abspath(os.path.dirname(__file__))


class Config:
    # Archivo de la base de datos SQLite
    BASE_DATOS = os.environ.get("AULAS_BASE_DATOS", os.path.join(RAIZ, "aulas.db"))
    # Clave que usan los ESP32
    API_KEY = os.environ.get("AULAS_API_KEY", "04OCT2026")
    # Contraseña para entrar al panel web
    CLAVE_PANEL = os.environ.get("AULAS_CLAVE_PANEL", "1234")
    # Texto largo cualquiera, sirve para firmar la sesión del navegador
    SECRET_KEY = os.environ.get("AULAS_CLAVE_SECRETA", "Doggie")
    # ---------------------------------

    PERMANENT_SESSION_LIFETIME = timedelta(hours=12)

    # Si un aula no reporta en este tiempo, se marca "sin conexión"
    SEGUNDOS_EN_LINEA = 150
    # Ignora dos pasadas de la misma tarjeta casi seguidas
    SEGUNDOS_ANTIREBOTE = 3
