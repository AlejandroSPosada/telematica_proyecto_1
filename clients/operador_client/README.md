# Cliente Operador (Java Desktop)

Aplicación de escritorio en Java Swing que se conecta al servidor TCP del monitor IoT usando el protocolo de aplicación basado en texto.

## Requisitos

- Java 11 o superior
- Servidor ejecutándose con socket TCP activo (puerto por defecto: 5000)
- Acceso de red al servidor (local o remoto)

## Configuración

El cliente se conecta por defecto a:
- **Host:** `telematica-iot-monitoring.duckdns.org` (resolución DNS mediante Duck DNS)
- **Puerto:** `5000`
- **Usuario:** `admin`
- **Contraseña:** `admin123`

Estos valores pueden modificarse directamente en la UI antes de conectar.

## Compilación

```bash
cd clients/operador_client
javac Main.java
```

## Ejecución

```bash
java Main
```

Se abrirá la ventana de la interfaz gráfica.

## Uso Básico

1. **Conectar al servidor:**
   - Verifica que los campos Host, Puerto, Usuario y Contraseña sean correctos
   - Pulsa **"Conectar / Autenticar"**
   - Aparecerá un diálogo de confirmación si la conexión es exitosa

2. **Visualizar sensores:**
   - Los sensores activos se muestran en la tabla "Sensores"
   - Puedes filtrar por tipo (temperatura, presión, vibración, humedad, energía)
   - Cada sensor muestra su último valor medido

3. **Recibir alertas:**
   - Las alertas se muestran en tiempo real en la tabla "Alertas"
   - Se actualiza automáticamente cada 5 segundos
   - Las alertas push se reciben instantáneamente cuando el servidor las envía

4. **Consultar historial:**
   - Selecciona un sensor en la tabla "Sensores"
   - Pulsa **"Cargar historial"**
   - Se muestran las últimas mediciones del sensor seleccionado

## Protocolo TCP

El cliente utiliza los siguientes comandos del protocolo de aplicación:

- `AUTH <usuario> <contraseña>` - Autenticar operador
- `LIST` - Obtener lista de todos los sensores
- `LIST <tipo>` - Obtener sensores de un tipo específico
- `HISTORY <sensor_id>` - Obtener historial de mediciones de un sensor
- `ALERTS` - Obtener lista de alertas recientes
- `DISCONNECT` - Cerrar conexión

## Características Técnicas

- **Conexión persistente:** Mantiene un socket TCP abierto durante toda la sesión
- **Threading:** Las operaciones de red se ejecutan en hilos secundarios para no bloquear la UI
- **Auto-refresh:** Actualiza la lista de sensores cada 5 segundos automáticamente
- **Alertas push:** Procesa alertas entrantes en tiempo real desde el servidor
- **Timeout configurable:** Espera 30 segundos por respuesta del servidor (ajustable en código)
- **Logging:** Registra todas las operaciones en `operator_client.log`

## Estructura del Código

- `OperatorApp` - Ventana principal y UI (Swing)
- `OperatorProtocolClient` - Manejo de conexión TCP y protocolo
- `AppLogger` - Sistema de logging a archivo
- `ProtocolParsers` - Parseo de respuestas del protocolo
- Clases de datos: `SensorRow`, `AlertRow`, `HistoryRow`

## Resolución de Nombres

El cliente utiliza **Duck DNS** para la resolución del nombre de dominio:
- DNS Provider: Duck DNS (duckdns.org)
- Dominio: `telematica-iot-monitoring.duckdns.org`
- IP: 52.21.234.109 (EC2 en AWS)

No hay direcciones IP hardcodeadas en el código, permitiendo cambiar la configuración en tiempo de ejecución a través de la UI.

## Troubleshooting

| Problema | Solución |
|----------|----------|
| "No autorizado (401)" | Verifica usuario/contraseña |
| "Timeout esperando respuesta" | El servidor no responde; aumenta timeout en `RESPONSE_TIMEOUT_SECONDS` |
| "No hay conexión activa" | Conecta al servidor primero usando "Conectar / Autenticar" |
| "Error de resolución DNS" | Verifica conectividad a internet y que Duck DNS esté accesible |

