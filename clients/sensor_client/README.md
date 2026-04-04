# Cliente Sensores IoT - Python

Este módulo implementa los clientes sensores del sistema distribuido de monitoreo IoT. Cada sensor se ejecuta en un hilo independiente, se registra en el servidor central de monitoreo y envía lecturas periódicas en formato batch siguiendo el protocolo de aplicación definido en PROTOCOLO.md.

## Estructura de archivos

- `config.py` - Parámetros de conexión al servidor, intervalos de envío y tamaño de batch
- `sensor.py` - Clase base Sensor con la máquina de estados del protocolo y la lógica de conexión, registro y envío
- `sensor_types.py` - Implementación de los cinco tipos de sensores con generación de valores simulados
- `main.py` - Punto de entrada del programa, crea los sensores y los lanza en hilos paralelos
- `mock_server.py` - Servidor de prueba que implementa el protocolo para desarrollo local sin depender del servidor C

## Tipos de sensores implementados

Tipo | Rango normal | Unidad
-----|-------------|-------
temperature | 15.0 a 35.0 | C
pressure | 950.0 a 1050.0 | hPa
humidity | 30.0 a 70.0 | porcentaje
vibration | 0.0 a 5.0 | mm/s
energy | 100.0 a 400.0 | W

Cada sensor tiene un 10% de probabilidad de generar un valor anomalo por fuera del rango normal para activar las alertas del servidor.

## Protocolo utilizado

Los mensajes que implementan los sensores siguen el formato definido en PROTOCOLO.md. Todos los mensajes terminan en salto de linea y los campos se separan por espacios.

Registro de un sensor nuevo:

    REGISTER temperature
    RESPONSE 200 1 Sensor registered correctly

Reconexion de un sensor existente:

    CONNECT 1 temperature
    RESPONSE 200 1 Connection established

Envio de un batch de mediciones:

    READ_BATCH 1 2026-04-03T19:31:16 3 22.50 23.10 21.80
    RESPONSE 200 1 Batch received

Desconexion limpia:

    DISCONNECT
    RESPONSE 200 - Connection terminated

## Como ejecutar

Para probar localmente con el servidor mock, abrir dos terminales. En la primera:

    python mock_server.py

En la segunda:

    python main.py

Para conectar al servidor C real, modificar en config.py el valor de SERVER_HOST con el nombre de dominio del servidor y verificar que SERVER_PORT coincida con el puerto configurado en el servidor. Nunca se deben usar direcciones IP directamente en el codigo.

## Parametros de configuracion

En config.py (o por variables de entorno) se pueden ajustar los siguientes valores:

- SERVER_HOST: nombre de dominio del servidor central, por defecto localhost para desarrollo
- SERVER_PORT: puerto TCP del servidor, por defecto 5000
- SEND_INTERVAL: segundos entre cada envio de batch, por defecto 5
- BATCH_SIZE: cantidad de lecturas por batch, por defecto 3