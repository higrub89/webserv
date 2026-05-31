# 📚 Análisis Detallado - Proyecto WebServer

## 📖 Tabla de Contenidos

1. [Visión General](#visión-general)
2. [Arquitectura del Sistema](#arquitectura-del-sistema)
3. [Componentes Principales](#componentes-principales)
4. [Desglose Detallado de Funcionalidades](#desglose-detallado-de-funcionalidades)
5. [División de Trabajo Posible](#división-de-trabajo-posible)
6. [Dependencias Entre Módulos](#dependencias-entre-módulos)
7. [Stack Tecnológico](#stack-tecnológico)
8. [Timeline Recomendado](#timeline-recomendado)
9. [Estrategia de Testing](#estrategia-de-testing)
10. [Consideraciones Importantes](#consideraciones-importantes)

---

## 🎯 Visión General

Un WebServer es una aplicación de red que:
- Escucha en uno o más puertos TCP/IP
- Acepta conexiones HTTP de clientes
- Parsea y procesa solicitudes HTTP
- Genera respuestas HTTP válidas
- Gestiona múltiples clientes simultáneamente
- Sirve contenido estático (archivos, directorios, etc.)
- Ejecuta scripts CGI (si aplica)
- Maneja errores y excepciones

### Requisitos Típicos (según normas 42)
- ✅ Funciona en C/C++
- ✅ Sin frameworks externos (uso mínimo de librerías)
- ✅ Manejo de múltiples clientes concurrentes
- ✅ Configuración similar a Nginx
- ✅ Soporte para GET, POST, DELETE
- ✅ Validación HTTP completa
- ✅ Servir archivos estáticos
- ✅ CGI opcional pero valorado

---

## 🏗️ Arquitectura del Sistema

```
┌─────────────────────────────────────────────────────────────────┐
│                         WEBSERVER                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐           │
│  │   MAIN       │  │   CONFIG     │  │   UTILS      │           │
│  │   (Entrada)  │  │   (Parsing)  │  │   (Helpers)  │           │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘           │
│         │                  │                  │                   │
│         └──────────────────┼──────────────────┘                   │
│                            │                                      │
│                    ┌───────▼────────┐                            │
│                    │  SOCKET LAYER  │                            │
│                    │  (Networking)  │                            │
│                    └───────┬────────┘                            │
│                            │                                      │
│         ┌──────────────────┼──────────────────┐                  │
│         │                  │                  │                  │
│    ┌────▼────┐      ┌─────▼─────┐     ┌─────▼─────┐            │
│    │  HTTP   │      │  REQUEST  │     │ RESPONSE  │            │
│    │ PARSER  │      │  HANDLER  │     │  BUILDER  │            │
│    └────┬────┘      └─────┬─────┘     └─────┬─────┘            │
│         │                  │                  │                  │
│         └──────────────────┼──────────────────┘                  │
│                            │                                      │
│         ┌──────────────────┼──────────────────┐                  │
│         │                  │                  │                  │
│    ┌────▼────┐      ┌─────▼─────┐     ┌─────▼─────┐            │
│    │  FILE   │      │  ROUTING  │     │   ERROR   │            │
│    │ SERVING │      │  (URLs)   │     │ HANDLING  │            │
│    └─────────┘      └───────────┘     └───────────┘            │
│                                                                   │
│    ┌─────────────────────────────────┐                          │
│    │       CGI EXECUTOR (Bonus)      │                          │
│    └─────────────────────────────────┘                          │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🔧 Componentes Principales

### 1. **CONFIGURADOR (Config Parser)**

#### Responsabilidades:
- Leer y parsear archivo de configuración (.conf)
- Validar sintaxis y estructura
- Soportar múltiples bloques de servidor
- Gestionar rutas y ubicaciones (locations)
- Aplicar permisos y restricciones
- Almacenar configuración en estructuras de datos eficientes

#### Entradas:
- Archivo de configuración (formato nginx-like)

#### Salidas:
- Estructura de configuración en memoria
- Lista de servidores a iniciar
- Reglas de routing
- Límites y restricciones

#### Funcionalidades Específicas:
```
- Directivas de servidor (server {})
  ├─ listen: puerto y host
  ├─ server_name: nombre del servidor
  ├─ root: directorio raíz
  └─ ...otros

- Directivas de ubicación (location {})
  ├─ alias/index
  ├─ allow_methods: GET, POST, DELETE
  ├─ cgi_pass: ruta a CGI
  ├─ redirect: redirecciones
  └─ ...otros

- Directivas globales
  ├─ client_max_body_size
  ├─ max_connections
  ├─ timeouts
  └─ ...otros
```

---

### 2. **CAPA DE SOCKETS (Socket Layer & Networking)**

#### Responsabilidades:
- Crear sockets TCP/IP
- Configurar y bindear sockets a puertos
- Escuchar conexiones entrantes
- Gestionar múltiples clientes de forma concurrente
- Multiplexing de I/O (select, poll, epoll, kqueue)
- Leer datos de los sockets
- Enviar datos a través de los sockets
- Cerrar conexiones apropiadamente

#### Entradas:
- Configuración (puertos, hosts)
- Datos brutos del cliente

#### Salidas:
- Conexiones establecidas
- Datos listos para parsear
- Capacidad de enviar respuestas

#### Funcionalidades Específicas:
```
- Inicialización de sockets
  ├─ socket()
  ├─ setsockopt() (SO_REUSEADDR, etc.)
  ├─ bind()
  └─ listen()

- Multiplexing
  ├─ select() / poll() / epoll()
  ├─ Monitoreo de múltiples descriptores
  ├─ Timeout management
  └─ Event handling

- Gestión de conexiones
  ├─ accept()
  ├─ Non-blocking I/O
  ├─ Keep-Alive management
  ├─ Connection timeouts
  └─ Graceful shutdown
```

---

### 3. **PARSER HTTP (HTTP Request Parser)**

#### Responsabilidades:
- Parsear la línea de solicitud (Request Line)
- Parsear headers HTTP
- Parsear cuerpo (body) de solicitudes
- Validar formato HTTP
- Extraer información de la solicitud
- Manejo de errores HTTP

#### Entradas:
- Datos brutos TCP/IP (strings)

#### Salidas:
- Estructura de solicitud parseada
- Headers en diccionario/mapa
- Body disponible
- Errores de validación

#### Funcionalidades Específicas:
```
- Request Line Parsing
  ├─ Método HTTP (GET, POST, DELETE, PUT, etc.)
  ├─ URI/Path
  ├─ Query String
  ├─ HTTP Version (1.0, 1.1)
  └─ Validación de formato

- Header Parsing
  ├─ Content-Type
  ├─ Content-Length
  ├─ Host
  ├─ Connection (keep-alive)
  ├─ User-Agent
  ├─ Accept, Accept-Encoding
  └─ Otros headers estándar

- Body Parsing
  ├─ Leer según Content-Length
  ├─ Chunked Transfer Encoding
  ├─ URL-encoded forms
  ├─ Multipart forms
  └─ Binary data

- Validación
  ├─ Formato HTTP válido
  ├─ Headers requeridos
  ├─ Método permitido según config
  └─ Tamaño máximo de body
```

---

### 4. **MANEJADOR DE SOLICITUDES (Request Handler)**

#### Responsabilidades:
- Enrutar solicitudes a la ubicación correcta
- Determinar qué hacer con cada solicitud
- Aplicar reglas de configuración
- Coordinar entre parser HTTP, servidor de archivos y ejecutor CGI
- Determinar respuesta a enviar

#### Entradas:
- Solicitud HTTP parseada
- Configuración del servidor
- Sistema de archivos

#### Salidas:
- Acción a ejecutar
- Datos para construir respuesta

#### Funcionalidades Específicas:
```
- Enrutamiento (Routing)
  ├─ Matching de rutas exactas
  ├─ Matching de prefijos
  ├─ Regex matching (si aplica)
  ├─ Resolución de ambigüedades
  └─ Prioridades

- Resolución de ubicaciones
  ├─ Encontrar location correspondiente
  ├─ Aplicar restricciones de permisos
  ├─ Validar métodos permitidos
  └─ Determinar tipo de acción

- Acciones posibles
  ├─ Servir archivo
  ├─ Listar directorio
  ├─ Ejecutar CGI
  ├─ Redirigir
  ├─ Retornar error
  └─ Reescribir URL
```

---

### 5. **CONSTRUCTOR DE RESPUESTAS (Response Builder)**

#### Responsabilidades:
- Construir la línea de estado (Status Line)
- Generar headers HTTP correctos
- Construir cuerpo de respuesta
- Serializar respuesta para enviar
- Manejar diferentes tipos de contenido

#### Entradas:
- Acción determinada
- Datos a enviar
- Configuración

#### Salidas:
- Respuesta HTTP completa y válida
- Datos listos para enviar por socket

#### Funcionalidades Específicas:
```
- Status Line
  ├─ HTTP/1.1 200 OK
  ├─ HTTP/1.1 404 Not Found
  ├─ HTTP/1.1 500 Internal Server Error
  └─ Otros códigos de estado

- Headers de Respuesta
  ├─ Content-Type (detection automático)
  ├─ Content-Length
  ├─ Content-Encoding
  ├─ Cache-Control
  ├─ Last-Modified
  ├─ Connection (keep-alive)
  ├─ Set-Cookie (si aplica)
  └─ Otros headers estándar

- Content Types
  ├─ text/html
  ├─ text/plain
  ├─ application/json
  ├─ image/png, image/jpeg
  ├─ application/octet-stream
  └─ Otros tipos

- Serialización
  ├─ Construir respuesta en bytes
  ├─ Manejo de cuerpo binario
  ├─ Manejo de cuerpo texto
  └─ Envío eficiente
```

---

### 6. **SERVIDOR DE ARCHIVOS (File Serving)**

#### Responsabilidades:
- Leer archivos del sistema de archivos
- Validar permisos y seguridad
- Listar directorios
- Generar índices HTML
- Detectar tipos de contenido
- Manejar rutas relativas/absolutas

#### Entradas:
- Ruta solicitada
- Configuración (root directory)
- Solicitud HTTP

#### Salidas:
- Contenido del archivo
- Información de directorio
- Errores (403, 404, etc.)

#### Funcionalidades Específicas:
```
- Lectura de archivos
  ├─ Validación de permisos
  ├─ Prevención de directory traversal
  ├─ Lectura eficiente
  ├─ Streaming para archivos grandes
  └─ Error handling

- Listado de directorios
  ├─ Generar HTML con lista
  ├─ Ordenamiento de archivos
  ├─ Información de tamaño y fecha
  ├─ Enlaces relativos
  └─ Configuración de índices

- MIME Type Detection
  ├─ Por extensión de archivo
  ├─ Por magic bytes (si aplica)
  ├─ Fallback a application/octet-stream
  └─ Configuración personalizada

- Validación de Seguridad
  ├─ Prevenir salida de directorio raíz
  ├─ Validar existencia de archivo
  ├─ Verificar permisos de lectura
  └─ Detectar enlaces simbólicos peligrosos
```

---

### 7. **EJECUTOR CGI (CGI Executor) - Bonus**

#### Responsabilidades:
- Ejecutar scripts CGI (Python, PHP, Bash, etc.)
- Pasar variables de entorno HTTP
- Capturar salida del script
- Redirigir entrada/salida
- Manejar timeouts
- Limpiar procesos hijo

#### Entradas:
- Solicitud HTTP
- Ruta del script CGI
- Configuración

#### Salidas:
- Salida del script
- Código de estado
- Errores

#### Funcionalidades Específicas:
```
- Preparación de entorno CGI
  ├─ Variables PATH_INFO, PATH_TRANSLATED
  ├─ QUERY_STRING, REQUEST_METHOD
  ├─ HTTP_* headers
  ├─ CONTENT_LENGTH, CONTENT_TYPE
  └─ Otras variables estándar

- Ejecución de scripts
  ├─ fork() / execve()
  ├─ Gestión de stdin/stdout/stderr
  ├─ Timeouts y límites
  ├─ Señales de terminación
  └─ Limpieza de procesos

- Manejo de resultados
  ├─ Capturar stdout del script
  ├─ Parsear headers CGI
  ├─ Combinar con response builder
  ├─ Manejar errores de ejecución
  └─ Logging
```

---

### 8. **MANEJO DE ERRORES (Error Handling)**

#### Responsabilidades:
- Generar páginas de error HTTP
- Validar solicitudes
- Manejar excepciones
- Logging de errores
- Códigos de estado apropiados

#### Códigos de Estado Comunes:
```
2xx Success
├─ 200 OK
├─ 201 Created
└─ 204 No Content

3xx Redirection
├─ 301 Moved Permanently
├─ 302 Found
└─ 304 Not Modified

4xx Client Error
├─ 400 Bad Request
├─ 403 Forbidden
├─ 404 Not Found
├─ 405 Method Not Allowed
├─ 408 Request Timeout
├─ 413 Payload Too Large
└─ 414 URI Too Long

5xx Server Error
├─ 500 Internal Server Error
├─ 501 Not Implemented
├─ 502 Bad Gateway
├─ 503 Service Unavailable
└─ 504 Gateway Timeout
```

---

### 9. **UTILIDADES Y HELPERS (Utils)**

#### Responsabilidades:
- Funciones auxiliares de string
- Conversión de tipos
- Logging
- Debugging
- Manejo de memoria
- Estructuras de datos comunes

#### Funcionalidades Específicas:
```
- String Utilities
  ├─ Trimming, splitting
  ├─ URL decoding/encoding
  ├─ Base64 encoding/decoding
  ├─ Conversión a mayúsculas/minúsculas
  └─ Búsqueda y reemplazo

- Conversiones
  ├─ String a número
  ├─ Número a string
  ├─ Booleanos
  └─ Tipos personalizados

- Logging
  ├─ Debug, Info, Warning, Error
  ├─ Niveles de log
  ├─ Timestamps
  └─ Archivo vs consola

- Estructuras de datos
  ├─ Listas ligadas
  ├─ Hash maps/diccionarios
  ├─ Colas
  └─ Pilas

- Manejo de memoria
  ├─ Allocación segura
  ├─ Liberación correcta
  ├─ Detección de memory leaks
  └─ Validaciones
```

---

### 10. **PROGRAMA PRINCIPAL (Main)**

#### Responsabilidades:
- Punto de entrada
- Inicialización del sistema
- Loop principal del servidor
- Gestión de señales (SIGINT, SIGTERM)
- Shutdown graceful

#### Flujo Básico:
```
main()
├─ Parsear argumentos (archivo config)
├─ Inicializar configuración
├─ Crear y configurar sockets
├─ Entrar en loop principal
│  ├─ Multiplexing (select/poll)
│  ├─ Aceptar nuevas conexiones
│  ├─ Leer datos de clientes
│  ├─ Procesar solicitudes
│  └─ Enviar respuestas
├─ Manejar señales
├─ Liberar recursos
└─ Exit
```

---

## 📋 Desglose Detallado de Funcionalidades

### **TIER 1: Funcionalidades Básicas (MVP)**

Estas son las funcionalidades mínimas para un servidor web funcional:

#### T1.1 - Configuración Básica
- [ ] Parser de archivo .conf simple
- [ ] Soporte para múltiples puertos
- [ ] Directorio raíz (root)
- [ ] Métodos HTTP permitidos por ruta

#### T1.2 - Networking Básico
- [ ] Crear socket TCP
- [ ] Bindear a puerto
- [ ] Aceptar conexiones
- [ ] Leer datos de cliente
- [ ] Escribir datos a cliente

#### T1.3 - Parser HTTP Básico
- [ ] Parsear request line
- [ ] Parsear headers principales
- [ ] Validación básica

#### T1.4 - Servir Archivos
- [ ] Leer archivos del disco
- [ ] Detectar MIME type simple
- [ ] Retornar 404 si no existe

#### T1.5 - Respuestas HTTP
- [ ] Generar status line válida
- [ ] Generar headers básicos
- [ ] Content-Length correcto
- [ ] Códigos de error (200, 404, 500)

---

### **TIER 2: Funcionalidades Intermedias**

#### T2.1 - Multiplexing Avanzado
- [ ] Manejo de múltiples clientes simultáneamente
- [ ] Select/poll/epoll según SO
- [ ] Non-blocking I/O
- [ ] Timeouts de conexión

#### T2.2 - Parser HTTP Avanzado
- [ ] Parsear POST body
- [ ] Content-Length validation
- [ ] Chunked Transfer Encoding
- [ ] URL-encoded forms
- [ ] Multipart forms

#### T2.3 - Enrutamiento
- [ ] Definir locations en config
- [ ] Matching de rutas
- [ ] Diferentes tratamientos por ruta
- [ ] Restricciones de permisos

#### T2.4 - Servir Directorios
- [ ] Listar directorios
- [ ] Generar índice HTML automático
- [ ] Redirigir a índices (index.html, index.php)
- [ ] Ordenamiento de archivos

#### T2.5 - Métodos HTTP Avanzados
- [ ] POST (crear archivos)
- [ ] DELETE (eliminar archivos)
- [ ] PUT (actualizar archivos)
- [ ] HEAD (sin cuerpo)

#### T2.6 - Keep-Alive
- [ ] Mantener conexiones abiertas
- [ ] Reutilizar conexiones
- [ ] Timeout correcto
- [ ] Close en demanda

---

### **TIER 3: Funcionalidades Avanzadas**

#### T3.1 - CGI Scripts
- [ ] Ejecutar scripts Python/PHP/Bash
- [ ] Pasar variables de entorno HTTP
- [ ] Capturar salida de script
- [ ] Timeouts de ejecución
- [ ] Manejo de errores de script

#### T3.2 - Redirecciones
- [ ] Redirecciones 301/302
- [ ] Reescritura de URL
- [ ] Alias de directorios
- [ ] Loops de redirección

#### T3.3 - Validación Avanzada
- [ ] Validación de headers HTTP
- [ ] Límite de tamaño de body
- [ ] Límite de URI length
- [ ] Limite de número de headers

#### T3.4 - Seguridad
- [ ] Prevención de directory traversal
- [ ] Validación de permisos de archivo
- [ ] Límites de rate limiting
- [ ] Validación de entrada

#### T3.5 - Logging y Debugging
- [ ] Access log (formato common log)
- [ ] Error log
- [ ] Debug mode
- [ ] Timestamps precisos

#### T3.6 - Características Avanzadas HTTP
- [ ] Content-Encoding (gzip)
- [ ] Cache headers (ETag, Last-Modified)
- [ ] Partial Content (Range requests)
- [ ] Compresión de respuestas

---

## 🔀 División de Trabajo Posible

### **Opción 1: Por Componente (Recomendado para equipo cohesivo)**

Cada persona es responsable de un componente principal:

```
Persona A: Configurador + Utilidades
├─ Config parser
├─ Validación de configuración
├─ Estructuras de datos para config
├─ Utils generales (strings, logging)
└─ Archivos de configuración de prueba

Persona B: Capa de Red + Socket
├─ Creación de sockets
├─ Multiplexing (select/poll/epoll)
├─ Gestión de conexiones
├─ Non-blocking I/O
└─ Lectura/escritura de datos

Persona C: HTTP + Handlers
├─ Parser HTTP
├─ Request handler/router
├─ Response builder
├─ Servir archivos
├─ Manejo de errores HTTP
```

**Ventajas:**
- Especialización clara
- Menos conflictos en merge
- Responsabilidad bien definida

**Desventajas:**
- Alta interdependencia
- Integración compleja
- Un componente lento ralentiza todo

---

### **Opción 2: Por Funcionalidad (Mejor para equipos menos coordinados)**

Cada persona maneja un "flujo" de funcionalidad:

```
Persona A: Entrada + Configuración
├─ main.c / main.cpp
├─ Argumentos de línea de comandos
├─ Parser de configuración
├─ Inicialización general
└─ Loop principal

Persona B: Solicitud + Procesamiento
├─ Socket layer (aceptar conexiones)
├─ Parser HTTP completo
├─ Request handler / enrutador
└─ Validaciones

Persona C: Respuesta + Salida
├─ File serving
├─ Response builder
├─ Error handling
├─ CGI executor (si aplica)
```

**Ventajas:**
- Menos "esperas" de otros
- Flujo lineal claro
- Puede testear independientemente

**Desventajas:**
- Responsabilidades divididas
- Testing integrado más complejo

---

### **Opción 3: Por Funcionalidad Horizontal (Flexible)**

```
Persona A: Core Networking + Socket Management
├─ Sockets TCP/IP
├─ Multiplexing
├─ Lectura/escritura de datos brutos
└─ Connection management

Persona B: Parsing + Validación
├─ Config file parsing
├─ HTTP request parsing
├─ Validación general
└─ Estructuras de datos

Persona C: Processing + Response
├─ File system operations
├─ Business logic (routing, handling)
├─ Response generation
└─ Error handling
```

**Ventajas:**
- Balance de trabajo
- Componentes más independientes
- Fácil de paralelizar

**Desventajas:**
- Requiere API clara entre componentes
- Más coordinación necesaria

---

## 🔗 Dependencias Entre Módulos

```
DEPENDENCIAS DIRECTAS:

main.c
├─ depende de: config.h, socket.h
└─ proporciona: inicialización

config.c
├─ depende de: utils.h
└─ proporciona: estructuras de configuración

socket.c
├─ depende de: http_parser.h, request_handler.h
└─ proporciona: datos brutos y escritura

http_parser.c
├─ depende de: utils.h
└─ proporciona: solicitud parseada

request_handler.c
├─ depende de: config.h, http_parser.h, file_serving.h, cgi_executor.h
└─ proporciona: respuesta determinada

response_builder.c
├─ depende de: http_parser.h, request_handler.h, utils.h
└─ proporciona: respuesta HTTP completa

file_serving.c
├─ depende de: utils.h, config.h
└─ proporciona: contenido de archivos

error_handling.c
├─ depende de: response_builder.h
└─ proporciona: páginas de error

cgi_executor.c (Bonus)
├─ depende de: http_parser.h, utils.h
└─ proporciona: salida de scripts

utils.c
├─ depende de: ninguno
└─ proporciona: funciones auxiliares


FLUJO DE DATOS EN UNA SOLICITUD:

Cliente TCP
    ↓
socket.c (accept, read)
    ↓
http_parser.c (parse request)
    ↓
request_handler.c (determine action)
    ├─ → file_serving.c (si es archivo)
    ├─ → cgi_executor.c (si es CGI)
    └─ → error_handling.c (si hay error)
    ↓
response_builder.c (build response)
    ↓
socket.c (write)
    ↓
Cliente TCP
```

---

## 💻 Stack Tecnológico

### **Lenguaje**
- C o C++
- Estándar C99 o C++11 como mínimo

### **Librerías Permitidas**
- `libc` estándar (obligatorio)
- `libsocket` (networking)
- Posiblemente `libcgi` (si CGI es requerido)

### **Herramientas de Desarrollo**
- Compilador: `gcc` o `clang`
- Build: `Makefile`
- Control de versiones: `git`
- Testing: `curl`, `telnet`, herramientas personalizadas
- Debugging: `gdb`, `valgrind`

### **Estructuras de Datos Recomendadas**
```c
// Config storage
typedef struct {
    int port;
    char *server_name;
    char *root;
    // ... otros campos
} ServerConfig;

// HTTP Request
typedef struct {
    char *method;           // GET, POST, DELETE
    char *uri;              // /path/to/resource
    char *query_string;     // param=value&foo=bar
    char *version;          // HTTP/1.1
    Map *headers;           // Diccionario de headers
    char *body;             // Cuerpo de solicitud
    size_t body_length;
} HttpRequest;

// HTTP Response
typedef struct {
    int status_code;        // 200, 404, etc.
    Map *headers;           // Diccionario de headers
    char *body;             // Cuerpo de respuesta
    size_t body_length;
} HttpResponse;
```

---

## 📅 Timeline Recomendado

### **Semana 1: Setup y Arquitectura**
- [ ] Configurar repositorio git
- [ ] Definir interfaz entre módulos (headers .h)
- [ ] Crear estructura de archivos
- [ ] Implement parsing de configuración básico
- [ ] Setup de Makefile

**Entregables:**
- Repositorio con estructura clara
- Archivo de configuración de ejemplo
- Headers compartidos (.h files)

---

### **Semana 2: Componentes Independientes**
- [ ] Socket layer básico
- [ ] HTTP parser básico
- [ ] File serving básico
- [ ] Response builder básico
- [ ] Testing unitario de cada componente

**Entregables:**
- Cada módulo compilable y testeable
- Test cases para funciones críticas
- Documentation de APIs

---

### **Semana 3: Integración**
- [ ] Conectar componentes
- [ ] Testing integrado
- [ ] Debug de flujos complejos
- [ ] Manejo de múltiples clientes
- [ ] Errores y edge cases

**Entregables:**
- Servidor funcional básico
- Capacidad de servir archivos
- Manejo de POST/DELETE
- Tests integrados

---

### **Semana 4: Pulido y Bonus**
- [ ] Optimizaciones de performance
- [ ] CGI executor (si tiempo)
- [ ] Logging y debugging
- [ ] Security review
- [ ] Testing final

**Entregables:**
- Servidor estable y robusto
- Documentación completa
- Test suite exhaustiva

---

## 🧪 Estrategia de Testing

### **Testing Unitario (Por Componente)**

```
test_config.c
├─ Parser de configuración válida
├─ Detección de configuración inválida
├─ Valores por defecto
└─ Límites y edge cases

test_http_parser.c
├─ Parsing de GET requests
├─ Parsing de POST requests
├─ Headers válidos e inválidos
├─ Body parsing
└─ Errores de formato

test_file_serving.c
├─ Leer archivo existente
├─ Retornar 404 para no existente
├─ MIME type detection
├─ Directory listing
└─ Security (no directory traversal)
```

### **Testing de Integración**

```
test_integration.c
├─ Servidor inicia correctamente
├─ Cliente se conecta
├─ GET request completa
├─ POST request completa
├─ DELETE request completa
├─ Múltiples clientes simultáneamente
└─ Conexiones Keep-Alive
```

### **Herramientas de Testing Externo**

```bash
# Simple GET request
curl http://localhost:8080/index.html

# Con headers específicos
curl -H "Content-Type: application/json" http://localhost:8080/

# POST request
curl -X POST -d "param=value" http://localhost:8080/endpoint

# DELETE request
curl -X DELETE http://localhost:8080/file.txt

# Conexión raw (telnet)
telnet localhost 8080

# Test de concurrencia
ab -n 1000 -c 10 http://localhost:8080/

# Test de stress
wrk -t4 -c100 -d30s http://localhost:8080/
```

---

## ⚠️ Consideraciones Importantes

### **Seguridad**

1. **Directory Traversal Prevention**
   - Validar todas las rutas
   - Resolver rutas canónicas
   - Prevenir `../` attacks

2. **Buffer Overflow Prevention**
   - Validar tamaños de entrada
   - Usar límites de body size
   - Validar longitud de headers

3. **Input Validation**
   - Escapar caracteres especiales
   - Validar métodos HTTP
   - Validar URIs

4. **Privacidad**
   - No servir archivos sensitivos
   - Validar permisos de archivo
   - Logging seguro

---

### **Performance**

1. **I/O Efficiency**
   - Usar non-blocking I/O
   - Multiplexing correcto
   - Buffering de datos

2. **Memory Management**
   - Evitar memory leaks
   - Liberar memoria de conexiones cerradas
   - Límites de memoria

3. **Concurrencia**
   - Manejo de múltiples clientes
   - Timeouts apropiados
   - Pool de conexiones (si aplica)

---

### **Estándares HTTP**

1. **RFC Compliance**
   - RFC 7230 (HTTP/1.1 Message Syntax)
   - RFC 7231 (HTTP/1.1 Semantics)
   - RFC 7232 (HTTP/1.1 Conditional Requests)

2. **Métodos Soportados**
   - GET: lectura
   - HEAD: GET sin body
   - POST: creación
   - DELETE: eliminación
   - PUT: reemplazo (si aplica)

3. **Headers Importantes**
   - Host: requerido
   - Content-Length: importante
   - Content-Type: MIME type
   - Connection: keep-alive/close
   - User-Agent: identificación del cliente

---

### **Manejo de Errores**

1. **Errores de Configuración**
   - Validar sintaxis
   - Directorio raíz válido
   - Permisos de archivo

2. **Errores de Red**
   - Conexión rechazada
   - Timeout de conexión
   - Socket errors

3. **Errores HTTP**
   - Bad requests
   - Métodos no soportados
   - Recursos no encontrados

4. **Errores del Sistema**
   - Out of memory
   - File not found
   - Permission denied

---

### **Logging y Debugging**

1. **Niveles de Log**
   - ERROR: algo falló
   - WARN: posible problema
   - INFO: información general
   - DEBUG: detalles de ejecución

2. **Información a Loguear**
   - Hora exacta
   - Nivel de severidad
   - Descripción del evento
   - Contexto relevante (PID, conexión, etc.)

3. **Salidas**
   - Archivo de log
   - Consola (stderr)
   - Rotación de logs (si aplica)

---

### **Configuración Recomendada de Ejemplo**

```nginx
server {
    listen 8080;
    server_name localhost;
    
    root /var/www/html;
    
    location / {
        allow_methods GET POST;
        index index.html index.php;
    }
    
    location /api {
        allow_methods GET POST DELETE;
        cgi_pass /usr/bin/php;
    }
    
    location /uploads {
        allow_methods POST DELETE;
    }
    
    error_page 404 /404.html;
    error_page 500 /500.html;
}

server {
    listen 8081;
    server_name api.local;
    root /var/www/api;
    
    location / {
        allow_methods GET POST PUT DELETE;
    }
}

client_max_body_size 1m;
max_connections 100;
request_timeout 30s;
```

---

## 📚 Recursos y Referencias

### **Documentación HTTP**
- Mozilla HTTP Guide: https://developer.mozilla.org/en-US/docs/Web/HTTP
- HTTP/1.1 Specification: https://tools.ietf.org/html/rfc7230
- HTTP Status Codes: https://httpwg.org/specs/rfc7231.html#status.codes

### **Networking en C/C++**
- Beej's Guide to Network Programming: https://beej.us/guide/bgnet/
- POSIX Socket API: https://pubs.opengroup.org/onlinepubs/9699919799/
- Linux man pages: man socket, man select, man poll

### **Herramientas de Testing**
- curl: https://curl.se/
- Apache Bench: https://httpd.apache.org/docs/current/programs/ab.html
- wrk: https://github.com/wg/wrk

### **Debugging**
- gdb: https://www.gnu.org/software/gdb/
- valgrind: https://valgrind.org/
- strace: https://strace.io/

---

## ✅ Checklist de Tareas

### **Pre-Proyecto**
- [ ] Leer este documento completamente
- [ ] Definir requisitos exactos del proyecto
- [ ] Discutir la división de trabajo
- [ ] Configurar repositorio git
- [ ] Crear estructura de directorios

### **Fase 1: Setup**
- [ ] Crear headers compartidos (.h)
- [ ] Definir APIs entre módulos
- [ ] Configurar Makefile
- [ ] Setup de testing framework
- [ ] Primeros compilables

### **Fase 2: Implementación**
- [ ] Config parser funcional
- [ ] Socket layer básico
- [ ] HTTP parser funcional
- [ ] Request handler básico
- [ ] Response builder básico
- [ ] File serving básico

### **Fase 3: Integración**
- [ ] Todos los módulos juntos
- [ ] Testing integrado
- [ ] Múltiples clientes concurrentes
- [ ] Debug de flows
- [ ] Optimizaciones básicas

### **Fase 4: Pulido**
- [ ] Performance improvements
- [ ] Security review
- [ ] CGI implementation (bonus)
- [ ] Documentation completa
- [ ] Testing exhaustivo
- [ ] Norming (código limpio, bien estructurado)

---

## 🎯 Conclusión

Este proyecto requiere:
- **Coordinación**: API clara entre módulos
- **Testing**: Unitario e integrado
- **Documentación**: Código auto-documentado + README
- **Debugging**: Herramientas y paciencia
- **Iteración**: Pequeños pasos, testing frecuente

**Recomendación final:** Comienzen definiendo las APIs entre módulos, luego cada persona puede trabajar relativamente independientemente. Reuniones diarias de 15 minutos para sincronización.

¡Mucho éxito con el proyecto! 🚀
