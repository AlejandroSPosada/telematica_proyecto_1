# Cliente Operador (Java Desktop)

Aplicacion de escritorio en Java Swing que se conecta al servidor TCP del monitor IoT usando el protocolo de aplicacion basado en texto.

## Requisitos

- Java 11 o superior
- Servidor ejecutandose con socket TCP activo (ej. puerto 5000)

## Compilacion

```bash
cd clients/operador_client
javac Main.java
```

## Ejecucion

```bash
java Main
```

## Uso

1. Ingresa host, puerto, usuario y password.
2. Pulsa "Conectar / Autenticar".
3. Veras sensores y alertas (auto-refresh cada 5 segundos).
4. Selecciona un sensor y pulsa "Cargar historial".

## Comandos del protocolo usados

- `AUTH <user> <pass>`
- `LIST` y `LIST <tipo>`
- `ALERTS`
- `HISTORY <sensor_id>`

El cliente mantiene una conexion persistente y procesa alertas push en tiempo real (`ALERT ...`) enviadas por el servidor.
