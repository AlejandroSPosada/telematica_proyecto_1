# Cliente Sensores IoT - Go

Este módulo implementa los clientes sensores del sistema distribuido de monitoreo IoT. Cada sensor se ejecuta en una goroutine independiente, se registra en el servidor central de monitoreo y envía lecturas periódicas en formato batch siguiendo el protocolo de aplicación definido en [PROTOCOLO.md](../../PROTOCOLO.md).

## Estructura de archivos

- `go.mod` - Módulo Go del cliente sensor
- `main.go` - Punto de entrada del programa, crea los cinco sensores y los lanza en goroutines
- `README.md` - Esta documentación

Los archivos Python antiguos quedaron como referencia histórica, pero el cliente activo es el de Go.

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

Los mensajes que implementan los sensores siguen el formato definido en [PROTOCOLO.md](../../PROTOCOLO.md). Todos los mensajes terminan en salto de linea y los campos se separan por espacios.

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

La forma recomendada de ejecutarlo es con Docker Compose desde la raiz del proyecto:

```bash
docker compose up --build
```

Si quieres correr solo el sensor client en Go, desde este directorio:

```bash
go run .
```

Para conectar al servidor central real, configurar `SERVER_HOST` y `SERVER_PORT` con variables de entorno. No se deben usar direcciones IP codificadas.

## Variables de entorno

En Go se pueden ajustar los siguientes valores:

- `SERVER_HOST`: host del servidor central, por defecto `localhost`
- `SERVER_PORT`: puerto TCP del servidor, por defecto `5000`
- `SEND_INTERVAL`: segundos entre cada envio de batch, por defecto `5`
- `BATCH_SIZE`: cantidad de lecturas por batch, por defecto `3`

Ejemplo:

```bash
SERVER_HOST=monitor-server SERVER_PORT=5000 go run .
```

## Comportamiento

- Se crean cinco sensores simulados: temperature, pressure, humidity, vibration y energy.
- Cada sensor obtiene un ID al registrarse.
- Si pierde la conexión, intenta reconectarse y volver a enviar batches.
- Cada batch incluye lecturas normales o anomalas para activar alertas en el servidor.