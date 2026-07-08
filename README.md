# Referencia de Requisitos y Arquitectura - Webserv (42)

Este documento centraliza los requisitos del subject de 42, las comprobaciones de la hoja de evaluación y la distribución del diseño de cabeceras implementado en el directorio `inc/`.

---

## Parte 1: Requisitos del Subject

### I. Reglas Generales
* **Resiliencia ante fallos:** El programa no debe crashear bajo ninguna circunstancia (incluyendo quedarse sin memoria) ni terminar inesperadamente. De ocurrir, la calificación del proyecto será 0.
* **Compilación y Makefile:** Compilación mediante Makefile con las reglas estándar: `$(NAME)`, `all`, `clean`, `fclean` y `re`. No debe realizar relinking innecesario.
* **Estándar y Flags:** Compilación estricta en C++98 usando `c++` con los flags `-Wall -Wextra -Werror -std=c++98`.
* **Uso del Lenguaje:** Utilizar funciones de C++ en la medida de lo posible (ej. `<cstring>` en lugar de `<string.h>`). Se permiten funciones de C autorizadas.
* **Restricciones:** Prohibido el uso de librerías externas o Boost.

### II. Especificaciones del Ejecutable
* **Nombre:** `webserv`
* **Entregables:** `Makefile`, cabeceras (`*.hpp`, `*.h`), archivos fuente (`*.cpp`), y archivos de configuración.
* **Uso:** `./webserv [archivo_de_configuracion]` (si no se especifica, debe usar una ruta por defecto).

#### Funciones Autorizadas
`execve`, `pipe`, `strerror`, `gai_strerror`, `errno`, `dup`, `dup2`, `fork`, `socketpair`, `htons`, `htonl`, `ntohs`, `ntohl`, `select`, `poll`, `epoll` (`epoll_create`, `epoll_ctl`, `epoll_wait`), `kqueue` (`kqueue`, `kevent`), `socket`, `accept`, `listen`, `send`, `recv`, `chdir`, `bind`, `connect`, `getaddrinfo`, `freeaddrinfo`, `setsockopt`, `getsockname`, `getprotobyname`, `fcntl`, `close`, `read`, `write`, `waitpid`, `kill`, `signal`, `access`, `stat`, `open`, `opendir`, `readdir` y `closedir`.

### III. I/O y Concurrencia
* **Naturaleza no bloqueante:** El servidor debe ser no bloqueante y gestionar de forma limpia las desconexiones de clientes.
* **Bucle de eventos único:** Se debe utilizar un único multiplexor (`epoll` para Linux) en el hilo principal para todas las operaciones de I/O de red, incluyendo el socket de escucha.
* **Monitoreo simultáneo:** El multiplexor debe controlar lecturas y escrituras de manera simultánea.
* **Control previo de descriptores:** Está prohibido leer (`read`/`recv`) o escribir (`write`/`send`) sobre descriptores de red asíncronos sin que el multiplexor haya notificado que están listos para la operación.
* **Archivos regulares:** Los archivos en disco están excluidos de esta regla; su lectura y escritura no requiere pasar por el multiplexor de eventos.
* **Restricciones de `errno`:** Prohibido evaluar el valor de `errno` inmediatamente después de llamadas de I/O para desviar el flujo lógico del servidor.

### IV. Reglas del Protocolo HTTP
* **Persistencia:** Las conexiones de clientes no deben quedarse suspendidas indefinidamente.
* **Compatibilidad:** Debe funcionar correctamente con navegadores web estándar (se sugiere contrastar con Nginx).
* **Gestión de errores:** Códigos de estado HTTP precisos. Deben definirse páginas de error por defecto si la configuración no las provee.
* **Uso de fork:** Limitado exclusivamente a la ejecución de scripts CGI.
* **Funcionalidad obligatoria:**
  * Servir archivos estáticos.
  * Permitir subida de archivos (upload).
  * Métodos GET, POST y DELETE.
  * Escuchar en múltiples puertos de forma simultánea.

### V. Reglas de Plataforma
* Modificación de descriptores a modo no bloqueante mediante `fcntl()`.
* **Flags de fcntl permitidos:** Únicamente `F_SETFL`, `O_NONBLOCK` y `FD_CLOEXEC`.

### VI. Archivo de Configuración
Debe soportar sintaxis tipo Nginx (bloques `server` y `location` sin expresiones regulares):
* Par IP:puerto de escucha.
* Rutas de páginas de error personalizadas.
* Límite de tamaño de cuerpo (`client_max_body_size`).
* Configuración de rutas (locations):
  * Métodos HTTP permitidos en la ruta.
  * Redirecciones HTTP.
  * Directorio raíz de búsqueda de archivos (root).
  * Autoindex (listado de directorios).
  * Archivo index por defecto.
  * Activación de subidas de archivos y directorio de almacenamiento.

### VII. CGI (Common Gateway Interface)
* Ejecución basada en la extensión del archivo (ej. `.php`, `.py`).
* Paso de metadatos mediante variables de entorno estándar.
* **Peticiones Chunked:** El servidor debe des-segmentar (un-chunk) el cuerpo antes de enviarlo al CGI.
* **Salida del CGI:** Si no incluye cabecera `Content-Length`, el fin de la transmisión vendrá marcado por el cierre de la tubería (EOF).
* Directorio de ejecución relativo al script para permitir accesos locales.

### VIII. Parte Bonus
* Soporte para Cookies y sesiones de usuario.
* Soporte para múltiples CGIs en paralelo.

---

## Parte 2: Hoja de Evaluación (Correction Sheet)

### I. Directrices de Evaluación
* **Crashes:** Cualquier fallo de segmentación o terminación inesperada durante la defensa supone un 0 directo.
* **Modificaciones en vivo:** Los evaluadores pueden pedir cambios sencillos en vivo para verificar la comprensión del código.
* **Fugas de memoria:** Evaluación estricta de leaks mediante valgrind. Cualquier leak invalida los puntos de la sección correspondiente.

### II. Comprobaciones Obligatorias
* **Control Crítico de Eventos:** El multiplexor debe controlar lectura y escritura al mismo tiempo en el bucle principal.
* **Límite de I/O:** Límite estricto de una única operación de lectura o de escritura por cliente por cada iteración del bucle del multiplexor.
* **Manejo de errores de socket:** Desconexión y borrado inmediato del cliente si `recv` o `send` devuelven un error o valor `<= 0`.
* **I/O no controlado:** Cualquier lectura o escritura en descriptores no preparados o que no pasen por el multiplexor (salvo archivos en disco) es penalizada con un 0.
* **Prueba de estrés (Siege):** Disponibilidad superior al 99.5% ejecutando `siege -b` indefinidamente sobre una página vacía. No debe haber aumento progresivo de memoria ni sockets colgados.

---

## Parte 3: Estructura de Cabeceras (inc/)

La arquitectura del proyecto está organizada en subcarpetas dentro de `inc/`:

```text
inc/
├── core/
│   ├── AEventHandler.hpp
│   ├── EpollManager.hpp
│   └── ServerHandler.hpp
├── http/
│   ├── HttpParser.hpp
│   ├── HttpRequest.hpp
│   └── HttpResponse.hpp
├── router/
│   ├── CgiHandler.hpp
│   ├── ClientHandler.hpp
│   ├── IMethodHandler.hpp
│   └── Router.hpp
└── types/
    └── ConfigStructures.hpp
```

### 1. Directorio inc/core/ (Red y Multiplexor)

#### AEventHandler.hpp
* **Clase:** `AEventHandler` (Abstracta, base para control de eventos)
* **Descripción:** Representación polimórfica de cualquier descriptor de archivo monitorizado por el multiplexor. Almacena el descriptor `fd_` y declara los métodos virtuales puros de callback.
* **Métodos:**
  * `int getFd() const`: Acceso al descriptor.
  * `virtual void onReadReady() = 0`: Listo para leer.
  * `virtual void onWriteReady() = 0`: Listo para escribir.
  * `virtual void onDisconnect() = 0`: Cierre o error en el descriptor.
  * `virtual bool isTimedOut(time_t current_time) const`: Validación de inactividad.

#### EpollManager.hpp
* **Clase:** `EpollManager` (No copiable)
* **Descripción:** Implementación de la cola de eventos y despacho basada en `epoll`. Mantiene el registro de manejadores activos (`handlers_`) mapeando `fd` a su respectivo `AEventHandler*`.
* **Métodos:**
  * `void init()`: Creación del descriptor epoll.
  * `void run()`: Bucle principal (`epoll_wait()`).
  * `void stop()`: Terminar el bucle.
  * `void addHandler(AEventHandler* handler, uint32_t events)`: Añadir al set de monitoreo.
  * `void updateHandlerEvents(AEventHandler* handler, uint32_t events)`: Modificar flags de escucha.
  * `void removeHandler(AEventHandler* handler)`: Quita al manejador de la monitorización.

#### ServerHandler.hpp
* **Clase:** `ServerHandler` (No copiable)
* **Descripción:** Socket pasivo de escucha. Al recibir eventos de lectura, llama a `accept()` de forma no bloqueante y registra el nuevo socket cliente creando una instancia de `ClientHandler`.

### 2. Directorio inc/http/ (Parseo y Protocolo)

#### HttpParser.hpp
* **Clase:** `HttpParser` (No copiable)
* **Descripción:** Parser incremental no bloqueante basado en una máquina de estados finitos (FSM) que extrae la información del protocolo a partir de buffers de red.
* **Métodos:**
  * `bool consume(std::vector<char>& raw_buffer, HttpRequest& req)`: Consume bytes del buffer y construye el objeto de petición.

#### HttpRequest.hpp
* **Clase:** `HttpRequest` (Forma canónica completa)
* **Descripción:** Objeto que encapsula los datos de la solicitud. Implementa el método `reset()` para limpieza lógica y reutilización del objeto bajo carga constante, y procesa cabeceras `Cookie`.

#### HttpResponse.hpp
* **Clase:** `HttpResponse` (Forma canónica completa)
* **Descripción:** Encapsula la construcción de respuestas. Contiene buffers internos para cabeceras y cuerpo, serialize para generar la salida final hacia la red, y `setCookie` para inyectar cookies.

### 3. Directorio inc/router/ (Lógica de Negocio y Ejecución)

#### Router.hpp
* **Clase:** `Router` (No copiable)
* **Descripción:** Resuelve los Virtual Hosts (mediante Host y puerto), busca la localización correspondiente (Location) con mayor coincidencia de prefijo, y despacha la petición al handler adecuado.

#### ClientHandler.hpp
* **Clase:** `ClientHandler` (No copiable, hereda de `AEventHandler`)
* **Descripción:** Representa la conexión del cliente. Mantiene los buffers de red (`rawInBuffer_`, `rawOutBuffer_`), su estado transaccional, y controla posibles fugas o llamadas CGI huérfanas mediante el registro de procesos activos asociados.
* **Métodos:**
  * `void registerCgi(pid_t pid, CgiReadHandler* read_h, CgiWriteHandler* write_h)`: Registro de control de CGI.
  * `void clearCgi()`: Limpieza y reap de procesos CGI activos.

#### CgiHandler.hpp
* **Clases:** `CgiReadHandler` y `CgiWriteHandler` (No copiables, heredan de `AEventHandler`)
* **Descripción:** Manejadores asíncronos para los pipes de lectura (stdout del proceso hijo) y escritura (stdin del proceso hijo) del CGI.

#### IMethodHandler.hpp
* **Clase:** `IMethodHandler` (Interface base)
* **Descripción:** Declara la interfaz común para el tratamiento de los verbos HTTP.

### 4. Directorio inc/types/ (Tipados)

#### ConfigStructures.hpp
* **Estructuras:** `LocationConfig`, `ServerConfig`, `VirtualHostGroup`, `ConfigMap`
* **Descripción:** Modelos de configuración del servidor. Soporta mapeo múltiple de CGIs (`cgi_handlers`) y rutas de almacenamiento para subidas de archivos.

---

## Parte 4: Distribución de Tareas

El reparto de responsabilidades se divide de manera balanceada de la siguiente forma:

### 1. Ruben - Infraestructura de Red, Sockets y Eventos
* Desarrollo del bucle de eventos (`EpollManager.cpp`).
* Implementación de la escucha pasiva y aceptación de sockets (`ServerHandler.cpp`).
* Configuración no bloqueante de sockets (`fcntl` con `O_NONBLOCK` / `FD_CLOEXEC` / `F_SETFL`).
* Gestión del ciclo de vida del socket del cliente en `ClientHandler.cpp` (callbacks `onReadReady` y `onWriteReady`, recepción y envíos parciales seguros, buffers `rawInBuffer_` y `rawOutBuffer_`).
* Gestión segura de cierres de conexión y control de inactividad de descriptores (timeouts).
* **(Infraestructura para Bonus):** Garantizar que `EpollManager` sea completamente genérico y dinámico (permitiendo registrar, modificar y eliminar cualquier clase que herede de `AEventHandler` de forma concurrente). Esto permite que el sistema soporte múltiples CGIs en paralelo nativamente sin que Ruben tenga que programar lógica de CGI.

### 2. Alex - Parsers y Estructuras de Datos
* Implementación del analizador sintáctico del archivo de configuración del servidor (`ConfigParser.cpp`).
* **(Bonus - Múltiples CGIs):** Parseo y estructuración de múltiples mapeos CGI por extensión en los bloques de localización del archivo de configuración.
* Desarrollo de la FSM del protocolo HTTP (`HttpParser.cpp`) con soporte para cuerpos normales e incremental chunked parsing.
* Implementación de los objetos de transferencia de datos de solicitud y respuesta (`HttpRequest.cpp` e `HttpResponse.cpp`), con reciclaje lógico y limpieza de buffers.
* **(Bonus - Cookies/Sesiones):** Parseo sintáctico de la cabecera `Cookie` en `HttpRequest` y método de inyección/serialización de cabeceras `Set-Cookie` en `HttpResponse`.

### 3. Angel - Enrutamiento, CGI y Cookies/Sesiones
* Enrutamiento de peticiones por host y coincidencia de prefijos en URI (`Router.cpp`).
* Integración no bloqueante de procesos CGI mediante lectura/escritura asíncrona en pipes registradas en el multiplexor (`CgiHandler.cpp` con `CgiReadHandler` y `CgiWriteHandler`).
* **(Bonus - Múltiples CGIs):** Control simultáneo de procesos CGI activos (`fork`, `execve`, y reap no bloqueante con `waitpid` y flag `WNOHANG`), gestionando de forma independiente múltiples procesos y tuberías activas por cliente.
* **(Bonus - Cookies/Sesiones):** Diseño y desarrollo de la clase `SessionManager` (base de datos en memoria para sesiones) y validación/control de acceso por sesión en el enrutamiento.

---

### Tabla General de Reparto del Proyecto (Incluido Bonus)

| Módulo / Requisito | Ruben (Red y Eventos) | Alex (Parsers y DTOs) | Angel (Enrutado y CGI) |
| :--- | :--- | :--- | :--- |
| **Multiplexor de Eventos** | Bucle central de eventos en [EpollManager](inc/core/EpollManager.hpp) e interfaz base [AEventHandler](inc/core/AEventHandler.hpp). | Ninguno. | Registro indirecto de descriptores polimórficos. |
| **Sockets y Conectividad** | Creación y configuración de socket de escucha en [ServerHandler](inc/core/ServerHandler.hpp), `accept` no bloqueante. | Ninguno. | Ninguno. |
| **I/O Físico del Cliente** | Gestión del descriptor del cliente en [ClientHandler](inc/router/ClientHandler.hpp), buffers de red raw y **envíos parciales** seguros. | Reciclaje lógico de buffers. | Ninguno. |
| **Archivo de Configuración** | Ninguno. | Analizador sintáctico completo en `ConfigParser.cpp` y estructurado de datos. | Ninguno. |
| **Protocolo HTTP (Core)** | Ninguno. | Parser incremental FSM en [HttpParser](inc/http/HttpParser.hpp), DTOs [HttpRequest](inc/http/HttpRequest.hpp) y [HttpResponse](inc/http/HttpResponse.hpp). | Ninguno. |
| **Enrutamiento y Lógica** | Ninguno. | Ninguno. | Selección de vhosts y locations en [Router](inc/router/Router.hpp), y verbos HTTP vía [IMethodHandler](inc/router/IMethodHandler.hpp). |
| **Ejecución CGI (Core + Bonus)** | Multiplexa los fds de pipes de forma transparente usando `AEventHandler`. | **(Bonus):** Parsea múltiples asociaciones CGI por extensión desde la configuración. | **(Bonus):** Lógica de CGI asíncrona en [CgiHandler](inc/router/CgiHandler.hpp), control de procesos y tuberías concurrentes. |
| **Cookies y Sesiones (Bonus)** | Soporte pasivo en sockets para flujos con estado. | **(Bonus):** Parseo de cabeceras `Cookie` y formateador de `Set-Cookie` en la capa HTTP. | **(Bonus):** Lógica del gestor de sesiones en memoria (`SessionManager`) y control de acceso. |
