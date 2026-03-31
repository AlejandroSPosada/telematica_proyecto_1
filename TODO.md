# TODO - Proyecto 1

Checklist operativo para desarrollar el proyecto paso a paso segun [PROYECTO.md](PROYECTO.md).

## Fase 0 - Arranque y organizacion

- [ ] Definir arquitectura base: sensores, operadores, servidor central.
- [ ] Definir tecnologias por componente (recordar: servidor central obligatorio en C).
- [ ] Crear estructura de carpetas real del repositorio (`server`, `clients`, `web`, `docker`, `docs`).
- [ ] Definir convenciones de ramas y mensajes de commit.
- [ ] Crear matriz de responsabilidades por integrante.

## Fase 1 - Protocolo de aplicacion (texto)

- [ ] Definir formato de mensaje (comando, campos, separador, fin de linea).
- [ ] Definir operaciones minimas:
	- [ ] Registro de sensor.
	- [ ] Envio de medicion.
	- [ ] Consulta de estado del sistema.
	- [ ] Notificacion de alertas a operadores.
- [ ] Definir codigos de respuesta y errores de protocolo.
- [ ] Definir ejemplos de intercambio completos (sensor-servidor y operador-servidor).
- [ ] Escribir especificacion en `PROTOCOLO.md`.

## Fase 2 - Servidor central (C + Berkeley Sockets)

- [ ] Crear servidor en C con parametros de ejecucion: `./server puerto archivoDeLogs`.
- [ ] Implementar socket principal y `bind/listen/accept` (si usan TCP para control principal).
- [ ] Implementar concurrencia para multiples clientes (hilos o estrategia equivalente).
- [ ] Implementar parser del protocolo de texto.
- [ ] Implementar estado interno de sensores activos y ultimas mediciones.
- [ ] Implementar deteccion basica de anomalias (reglas por umbral).
- [ ] Implementar notificaciones a operadores conectados.
- [ ] Implementar manejo de errores de red sin terminar el proceso abruptamente.

## Fase 3 - Logging obligatorio del servidor

- [ ] Registrar peticiones entrantes.
- [ ] Registrar respuestas enviadas.
- [ ] Registrar errores ocurridos.
- [ ] Incluir en cada log: IP, puerto, mensaje recibido y respuesta enviada.
- [ ] Imprimir logs en consola y persistirlos en archivo.

## Fase 4 - Cliente de sensores (`clients/sensor_client`)

- [ ] Implementar cliente sensor que se registre ante el servidor.
- [ ] Simular al menos 5 sensores con tipos distintos.
- [ ] Enviar mediciones periodicas por sensor.
- [ ] Incluir reconexion o reintentos ante fallos de red.
- [ ] Verificar que el servidor reciba y procese las mediciones simultaneas.

## Fase 5 - Cliente operador (`clients/operator_client`)

- [ ] Implementar autenticacion via servicio externo (sin usuarios locales en servidor central).
- [ ] Implementar consulta de sensores activos.
- [ ] Implementar visualizacion de mediciones recientes.
- [ ] Implementar recepcion de alertas en tiempo real.
- [ ] Construir interfaz grafica sencilla para operador.

## Fase 6 - Servicio web HTTP basico (`web`)

- [ ] Implementar servidor HTTP basico.
- [ ] Soportar peticiones GET.
- [ ] Interpretar cabeceras HTTP correctamente.
- [ ] Responder con codigos de estado HTTP adecuados.
- [ ] Exponer vistas minimas: login, estado general, sensores activos.

## Fase 7 - Resolucion de nombres y autenticacion externa

- [ ] Remover cualquier IP hardcodeada del codigo.
- [ ] Configurar consumo de servicios por nombre de dominio.
- [ ] Manejar excepciones de resolucion DNS sin caida del sistema.
- [ ] Configurar o simular servicio de identidad externo para roles de usuario.

## Fase 8 - Docker

- [ ] Crear `Dockerfile` funcional del servidor.
- [ ] Verificar build local de imagen.
- [ ] Verificar ejecucion local del contenedor con puertos publicados.
- [ ] Documentar comando de build y run utilizados en sustentacion.

## Fase 9 - AWS y acceso desde Internet

- [ ] Crear/usar instancia EC2.
- [ ] Configurar reglas de seguridad y puertos requeridos.
- [ ] Desplegar contenedor en EC2.
- [ ] Configurar DNS (idealmente Route 53) para acceso por dominio.
- [ ] Validar conexion desde clientes externos reales.

## Fase 10 - Pruebas integrales y estabilidad

- [ ] Probar carga con multiples sensores y operadores simultaneos.
- [ ] Probar desconexiones y reconexiones de clientes.
- [ ] Probar caidas parciales (DNS o servicio externo) y recuperacion.
- [ ] Verificar coordinacion correcta de mensajes entre entidades.
- [ ] Corregir cuellos de botella y errores de concurrencia.

## Fase 11 - Documentacion y entrega

- [ ] Actualizar `README.md` con arquitectura final, ejecucion y despliegue.
- [ ] Completar `PROTOCOLO.md` con especificacion final y ejemplos.
- [ ] Escribir guia de despliegue AWS paso a paso.
- [ ] Verificar que el repositorio incluya: codigo fuente, `Dockerfile`, protocolo e instrucciones.
- [ ] Validar checklist de entrega:
	- [ ] Enlace al repositorio privado.
	- [ ] Codigo fuente completo.
	- [ ] Documentacion del protocolo.
	- [ ] Archivo `Dockerfile`.
	- [ ] Instrucciones de despliegue en AWS.
	- [ ] Configuracion de dominio o DNS usado.
- [ ] Congelar cambios antes de la fecha limite (sin commits posteriores).

## Checklist de sustentacion (ensayo final)

- [ ] Mostrar arquitectura implementada.
- [ ] Demostrar servidor en C funcionando con multiples clientes.
- [ ] Demostrar simulacion de sensores y alertas en tiempo real.
- [ ] Demostrar build de imagen Docker en vivo.
- [ ] Demostrar ejecucion de contenedor en AWS.
- [ ] Demostrar acceso al sistema desde cliente externo por dominio.

