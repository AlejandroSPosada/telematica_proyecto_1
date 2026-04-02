# Descripción general
El protocolo definido a continuación es un protocolo de aplicación para un sistema distribuido de monitoreo de IOT. Es un protocolo de red stateful basado en texto que se ejecuta sobre el modelo TCP/IP, hace uso de sockets implementando la API de sockets Berkeley. El protocolo regula la comunicación entre dos tipos de clientes (Operadores y Sensores) con un servidor central de monitoreo, el cuál recibe lecturas de múltiples sensores, almacena y procesa la información para reportar a los clientes operadores, ya sea alertas en tiempo real o respuestas a querys hechas por ellos.

## Formato de mensajes:
```cmd
<verbo> [field1] [field2] ... \n
```
- verbo: indica la acción correspondiente al mensaje
- fieldn: Cada campo requerido para realizar esa acción
- \n: Delimita el final de un mensaje

## Tabla de referencia de mensajes
Verbo        | Dirección           | Campos                                         | Ejemplo
-------------|---------------------|------------------------------------------------|---------
REGISTER     | Sensor -> Server    | sensor_type                                    | REGISTER pressure\n
CONNECT      | Sensor -> Server    | sensor_id, sensor_type                         | CONNECT 1 temperature\n
AUTH         | Operator -> Server  | username, password                             | AUTH johndoe 123passwordsupersafe\n
READ_BATCH   | Sensor -> Server    | sensor_id, timestamp, reading_count, readings  | READ_BATCH 1 2026-04-01T19:36:45 3 25.02 25.02 999.0\n
LIST         | Operator -> Server  | sensor_type or -                               | LIST\n o LIST temperature\n
HISTORY      | Operator -> Server  | sensor_id                                      | HISTORY 1\n
ALERTS       | Operator -> Server  |                                                | ALERTS\n
DISCONNECT   | Client -> Server    |                                                | DISCONNECT\n
RESPONSE     | Server -> Client    | response_code, client_id or -, response_message| RESPONSE 200 1 Sensor registered correctly\n
ALERT        | Server -> Operator  | sensor_id, alert_type, alert_message           | ALERT SENSOR_OFFLINE sensor_id Sensor disconnected unexpectedly\n

## Máquinas de estado

### Operator
Estado       | Mensaje recibido | Estado siguiente  | Acción del servidor
-------------|------------------|-------------------|---------------
UNIDENTIFIED | AUTH             | PENDING           | Authenticate user
UNIDENTIFIED | INVALID_MSG      | OFFLINE           | Send RESPONSE 400 Bad request, terminate connection
PENDING      | AUTH_SUCCESS     | CONNECTED         | Send RESPONSE 200 User authenticated, conection established
PENDING      | AUTH_FAIL        | OFFLINE           | Send RESPONSE 401 Could not authenticate user
CONNECTED    | DISCONNECT       | OFFLINE           | Disconnect user, end connection thread, Send RESPONSE 200 User connection terminated
CONNECTED    | LIST             | CONNECTED         | Send RESPONSE 200 3 1 temperature 25.6 2 pressure 23.0 3 temperature 25.1\n
CONNECTED    | HISTORY          | CONNECTED         | Send RESPONSE 200 2 2026-04-01T19:36:45 25.6 2026-04-01T19:36:46 30\n
CONNECTED    | ALERTS           | CONNECTED         | Send RESPONSE 200 2 1 ALERT BATCH_MISS 2026-04-01T19:36:46 2 ALERT ABNORMAL_READING 2026-04-01T19:36:58 ...
CONNECTED    | INVALID_MSG      | OFFLINE           | Send RESPONSE 400 Bad request, terminate connection
OFFLINE      | -                | -                 | -

### Sensor
Estado       | Mensaje recibido | Estado siguiente  | Acción del servidor
-------------|------------------|-------------------|---------------
UNIDENTIFIED | REGISTER         | CONNECTED         | Register new sensor, Send RESPONSE 200 Sensor registered correctly, connection established
UNIDENTIFIED | INVALID_MSG      | OFFLINE           | Send RESPONSE 400 Bad request, terminate connection
UNIDENTIFIED | CONNECT          | CONNECTED         | Connect to existing sensor, Send RESPONSE 200 Connection established
CONNECTED    | READ_BATCH       | CONNECTED         | Parse and store batch
CONNECTED    | TIMEOUT(5s)      | IDLE              | Put sensor on watchlist, send ALERT BATCH_MISS sensor_id Sensor missed a batch, on watchlist
CONNECTED    | INVALID_MSG      | OFFLINE           | Send RESPONSE 400 Bad request, terminate connection
IDLE         | TIMEOUT(10s)     | OFFLINE           | End connection, send ALERT SENSOR_OFFLINE sensor_id Sensor disconnected unexpectedly
IDLE         | READ_BATCH       | CONNECTED         | Parse and store data, Remove sensor from watchlist
IDLE         | INVALID_MSG      | OFFLINE           | Send RESPONSE 400 Bad request, terminate connection
OFFLINE      | -                | -                 | -