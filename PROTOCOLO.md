# Protocolo de Aplicación - Sistema de Monitoreo IoT

## 1. Alcance
Este documento define el protocolo de aplicación basado en texto para la comunicación por TCP entre:
- Sensores y servidor central.
- Operadores y servidor central.

Este protocolo se implementa sobre sockets Berkeley (`SOCK_STREAM`) y es stateful.

Nota de alcance respecto al proyecto:
- El protocolo de este documento cubre el canal TCP de monitoreo.
- La interfaz web HTTP (GET, cabeceras, códigos HTTP) se especifica por separado.
- La localización de servicios debe hacerse por DNS (sin IPs hardcodeadas).

## 2. Reglas generales de formato

### 2.1 Formato de línea
Cada mensaje es exactamente una línea terminada en `\n`:

```cmd
<VERBO> [campo1] [campo2] ...\n
```

Reglas:
- Codificación: UTF-8.
- Separador de campos: un espacio (`0x20`).
- No se permiten tabulaciones.
- Longitud máxima de línea: 1024 bytes (incluyendo `\n`).
- Mensajes vacíos o sin `\n` son inválidos.

### 2.2 Tipos de campo
- `sensor_id`: entero positivo (`>= 1`).
- `sensor_type`: token alfanumérico con guion bajo, regex `^[a-zA-Z][a-zA-Z0-9_]{1,31}$`.
- `username`: token sin espacios, longitud 3 a 32.
- `password`: token sin espacios, longitud 8 a 64.
- `timestamp`: formato ISO-8601 UTC, `YYYY-MM-DDThh:mm:ssZ`.
- `reading`: número decimal con signo opcional.

### 2.3 Manejo de campos de texto libre
Cuando se requiera texto libre, se usa prefijo de longitud:

```cmd
<len>:<texto>
```

Ejemplo: `12:Sensor caido`

## 3. Comandos del cliente al servidor

Verbo | Dirección | Formato | Ejemplo
---|---|---|---
REGISTER | Sensor -> Server | `REGISTER <sensor_type>` | `REGISTER pressure`
CONNECT | Sensor -> Server | `CONNECT <sensor_id> <sensor_type>` | `CONNECT 1 temperature`
AUTH | Operator -> Server | `AUTH <username> <password>` | `AUTH johndoe 123passwordsupersafe`
READ_BATCH | Sensor -> Server | `READ_BATCH <sensor_id> <timestamp> <count> <r1> ... <rN>` | `READ_BATCH 1 2026-04-01T19:36:45Z 3 25.02 25.02 999.0`
LIST | Operator -> Server | `LIST` o `LIST <sensor_type>` | `LIST` / `LIST temperature`
HISTORY | Operator -> Server | `HISTORY <sensor_id> [limit]` | `HISTORY 1 10`
ALERTS | Operator -> Server | `ALERTS [since_timestamp]` | `ALERTS` / `ALERTS 2026-04-01T19:30:00Z`
DISCONNECT | Client -> Server | `DISCONNECT` | `DISCONNECT`

Reglas adicionales:
- En `READ_BATCH`, `count` debe coincidir exactamente con la cantidad de lecturas enviadas.
- Si el `sensor_id` de `READ_BATCH` no coincide con el sensor autenticado/conectado en ese socket, el servidor responde error.

## 4. Mensajes del servidor al cliente

### 4.1 Respuesta estándar

```cmd
RESPONSE <code> <request_id|-> <payload_type> [payload...]\n
```

Donde:
- `code`: código numérico (ver tabla siguiente).
- `request_id`: correlación opcional (en esta versión usar `-`).
- `payload_type`: tipo de respuesta estructurada.

Ejemplos:

```cmd
RESPONSE 200 - OK 1 Sensor_registered\n
RESPONSE 422 - ERR Invalid_timestamp\n
RESPONSE 404 - ERR Sensor_not_found\n
```

### 4.2 Alertas push a operadores

```cmd
ALERT <sensor_id> <alert_type> <timestamp> <len:message>\n
```

Ejemplos:

```cmd
ALERT 2 BATCH_MISS 2026-04-01T19:36:46Z 23:Sensor missed one batch\n
ALERT 2 SENSOR_OFFLINE 2026-04-01T19:36:58Z 33:Sensor disconnected unexpectedly\n
```

## 5. Códigos de respuesta

Código | Significado | Uso típico
---|---|---
200 | OK | Operación exitosa.
201 | CREATED | Sensor registrado exitosamente.
400 | BAD_REQUEST | Error de sintaxis o verbo inválido.
401 | UNAUTHORIZED | Credenciales inválidas en `AUTH`.
403 | FORBIDDEN | Cliente autenticado sin permisos.
404 | NOT_FOUND | `sensor_id` inexistente.
409 | CONFLICT | Sensor ya conectado / estado incompatible.
422 | UNPROCESSABLE_ENTITY | Campos válidos en sintaxis pero inválidos semánticamente.
500 | INTERNAL_ERROR | Falla interna del servidor.

## 6. Formatos de payload por operación

### 6.1 REGISTER
- Éxito: `RESPONSE 201 - SENSOR_REGISTERED <sensor_id> <sensor_type>`
- Error: `RESPONSE 409 - ERR Sensor_type_or_session_conflict`

### 6.2 CONNECT (sensor)
- Éxito: `RESPONSE 200 - SENSOR_CONNECTED <sensor_id> <sensor_type>`
- Error: `RESPONSE 404 - ERR Sensor_not_found`

### 6.3 AUTH (operator)
- Éxito: `RESPONSE 200 - AUTH_OK <username>`
- Error: `RESPONSE 401 - ERR Invalid_credentials`

### 6.4 LIST
- Éxito:

```cmd
RESPONSE 200 - SENSOR_LIST <count> [<sensor_id> <sensor_type> <last_reading> <last_ts>]...
```

### 6.5 HISTORY
- Éxito:

```cmd
RESPONSE 200 - HISTORY_DATA <sensor_id> <count> [<timestamp> <reading>]...
```

### 6.6 ALERTS
- Éxito:

```cmd
RESPONSE 200 - ALERT_LIST <count> [<sensor_id> <alert_type> <timestamp> <len:msg>]...
```

## 7. Máquina de estados del operador

Estado | Mensaje recibido | Estado siguiente | Acción del servidor
---|---|---|---
UNIDENTIFIED | AUTH | PENDING | Validar contra servicio externo de identidad.
UNIDENTIFIED | INVALID_MSG | OFFLINE | `RESPONSE 400 - ERR Bad_request`; cerrar conexión.
PENDING | AUTH_SUCCESS | CONNECTED | `RESPONSE 200 - AUTH_OK <username>`.
PENDING | AUTH_FAIL | OFFLINE | `RESPONSE 401 - ERR Invalid_credentials`; cerrar conexión.
CONNECTED | LIST / HISTORY / ALERTS | CONNECTED | Responder según payload definido.
CONNECTED | DISCONNECT | OFFLINE | `RESPONSE 200 - OK Operator_disconnected`; cerrar conexión.
CONNECTED | INVALID_MSG | OFFLINE | `RESPONSE 400 - ERR Bad_request`; cerrar conexión.

## 8. Máquina de estados del sensor

Estado | Evento/Mensaje | Estado siguiente | Acción del servidor
---|---|---|---
UNIDENTIFIED | REGISTER | CONNECTED | Registrar sensor y responder `201`.
UNIDENTIFIED | CONNECT | CONNECTED | Asociar socket a sensor existente.
UNIDENTIFIED | INVALID_MSG | OFFLINE | `RESPONSE 400 - ERR Bad_request`; cerrar conexión.
CONNECTED | READ_BATCH | CONNECTED | Validar, almacenar y detectar anomalías.
CONNECTED | TIMEOUT 5s | IDLE | Enviar `ALERT ... BATCH_MISS ...` a operadores.
CONNECTED | INVALID_MSG | OFFLINE | `RESPONSE 400 - ERR Bad_request`; cerrar conexión.
IDLE | READ_BATCH | CONNECTED | Almacenar y remover de watchlist.
IDLE | TIMEOUT 10s | OFFLINE | Enviar `ALERT ... SENSOR_OFFLINE ...`; cerrar conexión.
IDLE | INVALID_MSG | OFFLINE | `RESPONSE 400 - ERR Bad_request`; cerrar conexión.

## 9. Reglas de sesión y concurrencia
- Un socket representa una sola sesión (sensor u operador).
- Un `sensor_id` no puede tener dos sesiones activas simultáneas:
	- Si llega una segunda conexión, responder `409` y cerrar la nueva sesión.
- Tras `DISCONNECT`, el servidor responde y luego cierra el socket.
- Si el cliente cierra abruptamente, el servidor debe liberar recursos sin detener el proceso principal.

## 10. Manejo de errores y robustez de red
- Diferenciar error sintáctico (`400`) de error semántico (`422`).
- El servidor debe tolerar recepción fragmentada: acumular bytes hasta `\n`.
- Si en un `recv()` llegan múltiples líneas, procesarlas en orden FIFO.
- Si falla resolución DNS de dependencias externas, registrar error y continuar operando cuando sea posible.

## 11. Logging mínimo requerido
Por cada interacción se debe registrar en consola y archivo:
- IP y puerto origen.
- Mensaje recibido.
- Respuesta enviada.
- Timestamp del evento.
- Tipo de error (si aplica).

Formato sugerido:

```text
2026-04-01T19:36:45Z 192.168.1.10:53002 RX AUTH johndoe ****
2026-04-01T19:36:45Z 192.168.1.10:53002 TX RESPONSE 200 - AUTH_OK johndoe
```

## 12. Ejemplos canónicos

### 12.1 Alta de sensor nuevo
```cmd
S->SV: REGISTER temperature
SV->S: RESPONSE 201 - SENSOR_REGISTERED 7 temperature
```

### 12.2 Conexión de operador y consulta
```cmd
O->SV: AUTH operadorA claveSegura123
SV->O: RESPONSE 200 - AUTH_OK operadorA
O->SV: LIST
SV->O: RESPONSE 200 - SENSOR_LIST 2 1 temperature 25.6 2026-04-01T19:36:45Z 2 pressure 23.0 2026-04-01T19:36:40Z
```

### 12.3 Lote de lecturas
```cmd
S->SV: READ_BATCH 1 2026-04-01T19:36:45Z 3 25.02 25.20 999.0
SV->S: RESPONSE 200 - OK Batch_received
SV->O: ALERT 1 ABNORMAL_READING 2026-04-01T19:36:45Z 31:Outlier detected in current batch
```

---

Version: 2.0
Estado: listo para implementacion