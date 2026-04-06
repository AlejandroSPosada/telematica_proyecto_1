# Sistema Distribuido de Monitoreo de Sensores IoT

## Información del Proyecto

**Institución:** EAFIT  
**Materia:** Internet, Arquitectura y Protocolos  
**Tipo:** Proyecto de Aplicación de Conceptos de Servicios de Aplicación y Capa de Transporte

## Integrantes del Equipo

- Alejandro Sepúlveda Posada
- Jacobo Giraldo Zuluaga
- Laura Sofía Aceros Monsalve
- Paula Andrea Arroyave Acevedo

## Descripción General

Este proyecto implementa un sistema distribuido de monitoreo de sensores IoT desplegado en la nube. El objetivo es aplicar conceptos sobre servicios de aplicación y capa de transporte en una aplicación que simula un sistema real de monitoreo industrial.

El sistema integra múltiples servicios de red coordinados, incluyendo:

- Resolución de nombres para localizar servicios
- Autenticación de usuarios
- Intercambio de datos en tiempo real
- Servicio web para interacción con usuarios
- Infraestructura en la nube (AWS)
- Contenedores Docker para despliegue y portabilidad

## Objetivos

- Diseñar e implementar un protocolo de aplicación personalizado para la comunicación entre componentes
- Aplicar conceptos de sockets y abstracción en programación de aplicaciones distribuidas
- Deplegue de aplicación en infraestructura en la nube utilizando servicios de AWS
- Integración de múltiples servicios de red en un sistema funcional
- Implementación de servidor capaz de manejar múltiples clientes simultáneos

## Arquitectura del Sistema

### Componentes Principales

#### 1. Sensores (Clientes)
Dispositivos IoT que envían información periódicamente al servidor de monitoreo. Incluyen:
- Sensores de temperatura
- Sensores de vibración
- Sensores de consumo energético
- Sensores de humedad
- Sensores de estado operativo

#### 2. Operadores del Sistema
Aplicaciones cliente que permiten a los ingenieros:
- Visualizar sensores activos
- Recibir alertas en tiempo real
- Consultar mediciones recientes
- Ejecutar acciones de supervisión

#### 3. Servidor Central de Monitoreo
Aplicación servidor responsable de:
- Recibir datos de los sensores
- Procesar las mediciones
- Detectar eventos anómalos
- Notificar a los operadores conectados
- Manejar múltiples clientes simultáneamente

## Características Técnicas

### Sockets
Se utilizan dos tipos de sockets de la API Berkeley:

- **SOCK_STREAM**: Flujos fiables orientados a conexión (TCP) para garantizar entrega de mensajes
- **SOCK_DGRAM**: Datagramas sin conexión (UDP) para casos donde la latencia es prioritaria

### Protocolo de Aplicación
Protocolo basado en texto que permite operaciones como:
- Registro de sensores
- Envío de mediciones
- Notificación de alertas
- Consulta de estado del sistema

### Interfaz Web
Servidor HTTP básico que permite:
- Inicio de sesión de usuarios
- Consulta del estado general del sistema
- Visualización de sensores activos
- Interpretación correcta de cabeceras HTTP
- Manejo de peticiones GET
- Códigos de estado HTTP apropiados

### Autenticación
Consulta a un servicio externo de identidad para verificar:
- Existencia del usuario
- Rol dentro del sistema

### Resolución de Nombres
Todos los servicios se localizan mediante resolución de nombres de dominio DNS, sin direcciones IP codificadas.

## Requisitos Técnicos

### Servidor
- Implementado en **C**
- Utiliza API de Sockets Berkeley
- Soporta múltiples clientes simultáneos
- Implementa concurrencia con hilos
- Ejecutable desde consola: `./server puerto archivoDeLogs`
- Registra peticiones, respuestas y errores en consola y archivo

### Clientes
- Desarrollados en al menos dos lenguajes de programación diferentes
- Conectan al servidor mediante sockets
- Envían y reciben mensajes según protocolo

### Cliente Operador
- Interfaz gráfica que muestra:
  - Sensores activos
  - Mediciones en tiempo real
  - Alertas generadas

### Simulación de Sensores
- Mínimo 5 sensores simulados
- Cada sensor conecta al servidor
- Envía mediciones periódicas
- Se identifica con tipo específico

## Despliegue en la Nube

### Información de Despliegue Actual
- **Proveedor de Nube:** Amazon Web Services (AWS)
- **Instancia:** EC2 (t2.micro, Ubuntu 22.04 LTS)
- **IP Pública:** 52.21.234.109
- **Dominio:** telematica-iot-monitoring.duckdns.org
- **Proveedor DNS:** Duck DNS (www.duckdns.org)
- **Servicios:**
  - Servidor TCP: Puerto 5000
  - Servidor HTTP: Puerto 8081
  - Servicio Auth: Puerto 9090 (interno)

### Tecnologías Utilizadas
- **AWS (Amazon Web Services)**
  - Instancia EC2 para cómputo
  - Route 53 (Hosted Zone configurada como requisito académico; no se usó como solución final debido a restricciones de permisos en AWS Academy que impidieron el registro del dominio)
  - Configuración de Security Groups
  
- **Docker & Docker Compose**
  - Contenedores para todos los servicios
  - Dockerfile incluido en `docker/`
  - docker-compose.yml para orquestación
  
- **Duck DNS**
  - Servicio gratuito de DNS dinámico para resolución de nombres
  - Permite acceso a través de dominio en lugar de IP
  - Domain: `telematica-iot-monitoring.duckdns.org`

### Características de Despliegue
- El servidor se ejecuta dentro de un contenedor Docker
- Acceso remoto a la instancia mediante SSH
- El sistema es accesible desde Internet mediante infraestructura en la nube
- Despliegue reproducible en diferentes entornos
- Configuración de servicios mediante docker-compose
- No hay IPs ni hosts hardcodeados en el código

### Resolución de Nombres
**Solución implementada:** Duck DNS

**Justificación:** El enunciado requiere usar Amazon Route 53 para la gestión DNS. Se configuró una Hosted Zone en Route 53 cumpliendo ese requisito académico, pero las cuentas de AWS Academy tienen permisos limitados que impiden registrar dominios públicos dentro de la plataforma.

Como solución práctica se usó Duck DNS, un servicio gratuito de DNS dinámico que cumple el mismo propósito técnico: asociar un nombre de dominio a la IP de la instancia EC2. Esto es necesario porque AWS Academy asigna una IP diferente cada vez que la instancia se reinicia. Con Duck DNS el dominio `telematica-iot-monitoring.duckdns.org` siempre apunta a la IP correcta sin modificar el código.

En ningún punto del sistema se usan IPs directas, todo se resuelve por nombre de dominio.

Como resumen, Duck DNS fue seleccionado como proveedor de DNS dinámico porque:
1. Servicio gratuito y confiable
2. Propagación instantánea de cambios DNS (vs 24-48h de Route 53)
3. Fácil actualización de registros
4. No requiere permisos especiales de AWS para registrar dominio

**Requisito del proyecto:** Route 53 fue configurado (Hosted Zone creada), cumpliendo con el requisito académico, pero se utiliza Duck DNS como solución práctica para despliegue.

## Logging

El servidor implementa un sistema de registro que captura:

- Peticiones entrantes
- Respuestas enviadas
- Errores ocurridos

Cada registro incluye:
- Dirección IP del cliente
- Puerto de origen
- Mensaje recibido
- Respuesta enviada
- Timestamp del evento

Los registros se muestran en consola y se almacenan en archivo de logs dentro del contenedor.

## Estructura del Repositorio

```
proyecto-iot/
├── auth/                      # Servicio de autenticación
│   └── auth_service.py
├── clients/
│   ├── operador_client/       # Cliente operador (Java)
│   │   ├── Main.java
│   │   └── README.md
│   └── sensor_client/         # Cliente sensores (Python)
│       ├── main.py
│       ├── sensor.py
│       ├── config.py
│       └── README.md
├── server/                    # Servidor central (C)
│   ├── server.c
│   ├── server.h
│   ├── http_server.c
│   ├── http_server.h
│   ├── Makefile
│   └── server (ejecutable)
├── web/                       # Interfaz web (Python)
│   └── main.py
├── docker/                    # Archivos Docker
│   ├── Dockerfile             # Servidor y web
│   ├── Dockerfile.auth        # Servicio auth
│   └── Dockerfile.sensors     # Sensores
├── docs/                      # Documentación
│   ├── DEPLOYMENT_GUIDE.md    # Guía de despliegue completa
│   └── deployment_guide.md
├── docker-compose.yml         # Orquestación de contenedores
├── PROTOCOLO.md               # Especificación del protocolo
├── PROYECTO.md                # Enunciado del proyecto
└── README.md                  # Este archivo
```

## Quick Start

### Desarrollo Local

```bash
# Clonar repositorio
git clone <url> proyecto-iot
cd proyecto-iot

# Compilar servidor
cd server
make

# En otra terminal, compilar cliente operador
cd clients/operador_client
javac Main.java
java Main

# En otra terminal, ejecutar sensor client
cd clients/sensor_client
python main.py
```

### Despliegue en AWS

Para instrucciones detalladas de despliegue en AWS con Docker, ver:
**[DEPLOYMENT_GUIDE.md](docs/DEPLOYMENT_GUIDE.md)**

Resumen rápido:
```bash
# En la instancia EC2:
git clone <url> proyecto-iot
cd proyecto-iot
docker compose up -d

# Verificar servicios
docker compose ps

# Ver logs
docker compose logs -f
```

Luego conectar:
- **Web:** http://52.21.234.109:8081 o http://telematica-iot-monitoring.duckdns.org:8081
- **Cliente Java:** Host = telematica-iot-monitoring.duckdns.org, Puerto = 5000

## Documentación

- **[PROTOCOLO.md](PROTOCOLO.md)** - Especificación completa del protocolo de aplicación
- **[PROYECTO.md](PROYECTO.md)** - Enunciado del proyecto y requisitos
- **[docs/DEPLOYMENT_GUIDE.md](docs/DEPLOYMENT_GUIDE.md)** - Guía paso a paso para despliegue en AWS
- **[clients/operador_client/README.md](clients/operador_client/README.md)** - Documentación del cliente operador
- **[clients/sensor_client/README.md](clients/sensor_client/README.md)** - Documentación del cliente sensores

## Credenciales por Defecto

Para testing y desarrollo:
- **Usuario:** admin
- **Contraseña:** admin123

## Consideraciones de Seguridad

**Nota:** Las credenciales por defecto y configuración son solo para propósitos educativos. Para producción:
1. Cambiar todas las contraseñas
2. Ajustar Security Groups para restringir acceso
3. Usar HTTPS en lugar de HTTP
4. Implementar validación adicional de entrada
5. Auditar logs regularmente

## Autores

- Alejandro Sepúlveda Posada
- Jacobo Giraldo Zuluaga
- Laura Sofía Aceros Monsalve
- Paula Andrea Arroyave Acevedo

## Institución

**EAFIT** - Escuela de Administración, Finanzas e Institutos Tecnológicos  
Materia: Internet, Arquitectura y Protocolos  
2026

- Puerto de origen
- Mensaje recibido
- Respuesta enviada

Los registros se muestran en consola y se almacenan en un archivo de logs.

## Estructura del Repositorio

```
proyecto1/
├── README.md                 # Este archivo
├── PROYECTO.pdf             # Especificación del proyecto
├── PROTOCOLO.md             # Especificación del protocolo de aplicación
├── server/                  # Código del servidor (C)
│   ├── server.c
│   ├── Makefile
│   └── ...
├── clients/                 # Código de los clientes
│   ├── sensor_client/       # Cliente sensor (lenguaje 1)
│   │   └── ...
│   ├── operator_client/     # Cliente operador (lenguaje 2)
│   │   └── ...
│   └── ...
├── web/                     # Servidor HTTP y interfaz web
│   └── ...
├── docker/                  # Configuración Docker
│   ├── Dockerfile
│   └── ...
└── docs/                    # Documentación adicional
    └── deployment_guide.md  # Guía de despliegue en AWS
```

## Instrucciones de Uso

### Compilación del Servidor

```bash
cd server
make
```

### Ejecución del Servidor

```bash
./server [puerto_tcp] [puerto_http] [archivo_de_logs]
```

Ejemplo:
```bash
./server 5000 8081 server.log
```

### Compilación y Ejecución de Clientes

Consultar la documentación específica en cada carpeta de cliente.

Comandos rápidos:

```bash
# Cliente sensor (Go)
cd clients/sensor_client
go run .

# Cliente operador (Java + Swing)
cd clients/operator_client
javac Main.java
java Main
```

## Docker (Linux) - Un solo comando

```bash
docker compose up --build
```

Esto levanta automaticamente:

- `auth-service` en `9090`
- `monitor-server` (protocolo TCP en `5000`, dashboard HTTP en `8081`)
- `sensor-client` con 5 sensores simulados enviando lecturas

Acceso web:

- URL: `http://localhost:8081`
- Usuario: `admin`
- Password: `admin123`

Para detener el stack:

```bash
docker compose down
```

## Despliegue en AWS

Consultar [docs/deployment_guide.md](docs/deployment_guide.md) para instrucciones detalladas de despliegue en instancias EC2 de AWS.

## Especificación del Protocolo de Aplicación

Consultar [PROTOCOLO.md](PROTOCOLO.md) para la especificación completa del protocolo de comunicación entre componentes.

## Características del Sistema

✓ Manejo de múltiples sensores y operadores simultáneos  
✓ Resolución de nombres (DNS)  
✓ Autenticación de usuarios  
✓ Detección de eventos anómalos  
✓ Notificaciones en tiempo real  
✓ Interfaz web para consultas  
✓ Logging completo de eventos  
✓ Despliegue en la nube  
✓ Contenedorización con Docker  

## Evaluación

La evaluación se realiza mediante sustentación presencial del sistema con los siguientes criterios:

| Componente | Ponderación |
|-----------|-------------|
| Programación del Servidor | 30% |
| Programación de Clientes | 25% |
| Funcionamiento del Sistema | 20% |
| Despliegue en AWS y Docker | 15% |
| Documentación | 10% |

