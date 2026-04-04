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

### Teknologías Utilizadas
- **AWS (Amazon Web Services)**
  - Instancia EC2 para cómputo
  - Route 53 para gestión DNS
  - Configuración de puertos de red
  
- **Docker**
  - Contenedor con todas las dependencias
  - Dockerfile incluido en el repositorio
  
### Características de Despliegue
- El servidor se ejecuta dentro de un contenedor Docker
- Acceso remoto a la instancia
- El sistema es accesible desde Internet mediante infraestructura en la nube
- Despliegue reproducible en diferentes entornos

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

