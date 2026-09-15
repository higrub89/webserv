# Registro Técnico de Modificaciones y Remediación Arquitectural

Este documento detalla todas las modificaciones, correcciones de errores críticos y nuevas implementaciones realizadas sobre la base de código de **Webserv** para cumplir con la Norma 42, el Subject v24.0, los estándares POSIX/C++98 y la suite oficial de pruebas (`./tester/tester`).

---

## 1. Núcleo de Red y Bucle de Eventos (`src/core/`)

### 1.1. `EpollManager.cpp`
* **Corrección de Concurrencia Bidireccional**: Se eliminó la bifurcación excluyente `else if (events & EPOLLOUT)` posterior a `EPOLLIN`. Ahora un socket preparado para lectura y escritura atiende ambos eventos en el mismo ciclo de despacho sin latencia artificial.
* **Protección contra Cierres Prematuros (`EPOLLHUP`)**:
  - Antes: Si `revents` incluía `EPOLLHUP` o `EPOLLRDHUP`, se desconectaba inmediatamente ignorando datos pendientes en el socket o tubería.
  - Ahora: Se comprueba `!(revents & EPOLLIN) && (revents & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))`. Si hay datos pendientes (`EPOLLIN`), se procesa la lectura completa antes de ejecutar la desconexión.
* **Eliminación de Use-After-Free en `cleanupTimeouts()`**:
  - Antes: Se iteraba sobre `handlers_` y se eliminaban elementos directamente invalidando iteradores y desreferenciando punteros destruidos.
  - Ahora: Se recolectan los descriptores expirados en un `std::vector<int> expiredFds` previo a la desconexión, eliminando cualquier condición de puntero colgante o doble liberación.

### 1.2. `ClientHandler.cpp` e `inc/core/ClientHandler.hpp`
* **Eliminación del Cuello de Botella $O(N^2)$ en Transmisión de Sockets**:
  - Antes: Cada llamada a `send()` ejecutaba `rawOutBuffer_.erase(begin(), begin() + n)`. En respuestas pesadas (e.g. 100 MB generados por CGI), cada `send()` ejecutaba un `memmove` de hasta 100 MB, acumulando más de **150 Gigabytes de copias de memoria en RAM** en el hilo principal y congelando el servidor.
  - Ahora: Se implementó un cursor `size_t rawOutBufferOffset_`. El buffer se transmite de forma contigua sin reubicar memoria y se vacía con `clear()` únicamente cuando `rawOutBufferOffset_ == rawOutBuffer_.size()`, reduciendo la complejidad a $O(N)$.
* **Preservación de Buffer en Keep-Alive y Pipelining**:
  - Antes: Al terminar una respuesta, `resetForKeepAlive()` limpiaba indiscriminadamente `rawInBuffer_`, destruyendo peticiones HTTP pipelined que ya habían sido leídas en el mismo paquete TCP.
  - Ahora: `rawInBuffer_` conserva los bytes remanentes y, si existen, invoca inmediatamente al parser para procesar la siguiente transacción encolada.
* **Optimización de Entrada de Red**:
  - Se incrementó el tamaño del buffer de lectura por `recv()` de 8 KB a 64 KB (`READ_BUF_SIZE 65536`), reduciendo drásticamente las transiciones de contexto en pruebas de alta tasa de transferencia.

### 1.3. `ServerHandler.cpp` e `inc/core/ServerHandler.hpp`
* **Enlace Físico a IPs de Configuración (Bind Físico)**:
  - Antes: Se hardcodeaba `address_.sin_addr.s_addr = INADDR_ANY`, ignorando la interfaz IP especificada en el archivo `.conf`.
  - Ahora: El constructor recibe la IP del grupo (`ServerGroup.ip`) y enlaza el socket mediante `inet_pton(AF_INET, ip_.c_str(), &address_.sin_addr)`, soportando configuraciones multired (e.g. `127.0.0.1`, interfaces dedicadas o `0.0.0.0`).

---

## 2. Subsistema HTTP (`src/http/`)

### 2.1. `HttpParser.cpp` e `inc/http/HttpParser.hpp`
* **Compatibilidad Estricta HTTP/1.0 y HTTP/1.1**:
  - La cabecera `Host` sólo es mandatoria para peticiones `HTTP/1.1` (RFC 2616 §14.23). Peticiones `HTTP/1.0` sin `Host` son aceptadas válidamente.
  - En `HTTP/1.0`, la conexión por defecto es `Connection: close`, a menos que se especifique explícitamente `Connection: keep-alive`.
* **Protección contra Corrupción de Buffer en `readLine`**:
  - Antes: Si una cabecera llegaba fragmentada entre dos paquetes TCP sin `\r\n` o `\n`, el puntero de lectura avanzaba sin consumir la línea pero borraba el buffer, corrompiendo la siguiente lectura.
  - Ahora: Si no se detecta fin de línea, `pos` se restablece intacto y se pospone el parseo hasta la llegada de más bytes.
* **Límites Dinámicos de Cuerpo**:
  - Se añadió el método `setMaxBodySize(size_t)` para desacoplar el límite de tamaño de cuerpo fijo del servidor y ajustarlo dinámicamente según la configuración.

### 2.2. `HttpError.cpp`
* **Resolución de Rutas de Páginas de Error**:
  - Antes: Rutas como `error_page 404 /errors/404.html;` intentaban abrirse en la raíz del sistema de archivos (`/errors/...`) provocando fallo y emitiendo el HTML por defecto.
  - Ahora: Se intenta abrir la ruta directamente y, si no existe, se resuelve respecto al `root_dir` del servidor virtual (`server.root_dir + "/" + errorPagePath`).

---

## 3. Subsistema CGI (`src/cgi/`)

### 3.1. `CgiExecutor.cpp`
* **Rutas Virtuales de Script**:
  - Se removió la verificación restrictiva `access(script_path, F_OK)` previa a `fork()`. Esto permite que endpoints virtuales requeridos por la suite de 42 (como `POST /directory/youpla.bla`, archivo que no existe en disco pero cuya extensión dispara el binario CGI) invoquen correctamente el intérprete.
* **Normalización de Rutas e Intérprete**:
  - Desglose del prefijo del bloque `location` según Subject §IV.3.
  - Resolución del intérprete relativo a absoluto usando la variable `PWD` extraída directamente de `envp` (sin invocar funciones prohibidas como `getenv` o `realpath`).
  - Variables de entorno añadidas conforme a RFC 3875: `PATH_INFO`, `PATH_TRANSLATED`, `SCRIPT_NAME`, `QUERY_STRING` y headers HTTP normalizados en minúsculas.

### 3.2. `CgiReadHandler.cpp` e `inc/cgi/CgiReadHandler.hpp`
* **Cosecha de Proceso sin `SIGKILL` Prematuro**:
  - Antes: Al detectar EOF en la tubería (`bytes_read == 0`), se ejecutaba un `kill(cgiPid_, SIGKILL)` antes de esperar la terminación normal, provocando que `WIFEXITED` fuera falso y retornando un falso 500.
  - Ahora: Se invoca `waitpid(cgiPid_, &status, 0)` de forma limpia para recolectar el código de salida real del proceso hijo.
* **Cálculo Dinámico de `Content-Length`**:
  - Si el script CGI no emite cabecera `Content-Length`, el servidor acumula la salida hasta el cierre del pipe y genera automáticamente la cabecera con el tamaño exacto del cuerpo, evitando bloqueos por Keep-Alive en clientes HTTP como Go (`http.Client`).
* **Ampliación de Buffer**:
  - Se incrementó `kBufferSize` a 64 KB (`65536`), acelerando la lectura de respuestas grandes (100 MB).

### 3.3. `CgiWriteHandler.cpp`
* **Manejo de E/S No Bloqueante en Tuberías**:
  - Antes: Si la tubería de entrada del CGI se llenaba (buffer Linux de 64 KB), `write()` retornaba `-1` con `errno == EAGAIN`, lo cual era interpretado erróneamente como un error fatal que abortaba la conexión.
  - Ahora: Se gestiona `EAGAIN` y `EWOULDBLOCK` retornando sin error para esperar el próximo evento `EPOLLOUT`.
  - Ante rotura de tubería (`EPIPE`, cuando el proceso CGI cierra su entrada estándar antes de leer todo el cuerpo), se desregistra el handler y se cierra el pipe sin abortar al cliente, permitiendo leer la respuesta emitida por el script.

---

## 4. Métodos HTTP y Enrutado (`src/methods/` y `src/router/`)

### 4.1. `Router.cpp` e `inc/router/Router.hpp`
* **Precedencia de Extensiones CGI**:
  - Se reordenó el flujo de despacho para que la coincidencia de extensión CGI ocurra **antes** del filtrado de `allowed_methods` del bloque `location`. Esto permite ejecutar peticiones `POST` sobre rutas cuya configuración estática sólo define `methods GET;`, cumpliendo la directiva del tester: *"Any file with .bla as extension must answer to POST request by calling the cgi_test executable"*.
* **Control de `client_max_body_size` por Ruta**:
  - Verificación estricta del límite de cuerpo configurado en el bloque `location`. Si una petición excede el tamaño máximo permitido para la ruta específica, se responde de inmediato con `413 Payload Too Large`.

### 4.2. `GetExecutor.cpp`
* Desglose del prefijo `location.path` de la URI antes de componer la ruta en el sistema de ficheros.
* Si el fichero índice configurado no existe en un directorio y `autoindex` está desactivado, retorna `404 Not Found` (en lugar de `403 Forbidden`).
* Si `upload_enable on` está configurado en la ubicación, busca ficheros tanto en `root_dir` como en `upload_store`.

### 4.3. Implementación de `PostExecutor` (`PostExecutor.hpp` y `PostExecutor.cpp`)
* **Gestión de Cargas (`upload_enable on`)**:
  - Soporte de subida en formato `multipart/form-data`: extracción de delimitador (*boundary*), parseo de cabeceras de parte (`Content-Disposition`) y extracción del nombre de fichero.
  - Soporte de cargas binarias en bruto (*raw octet-stream*): extracción del nombre de fichero desde la URI.
  - Saneamiento de nombres de archivo para mitigar ataques de *directory traversal* (eliminación de componentes de ruta relativos `../`).
  - Escritura atómica en `location.upload_store` y emisión de respuesta `201 Created` con cabecera `Location`.
* **Peticiones Estándar**:
  - Respuesta `200 OK` para endpoints sin subida de ficheros pero con método POST autorizado (e.g. `/post_body`).

### 4.4. Implementación de `DeleteExecutor` (`DeleteExecutor.hpp` y `DeleteExecutor.cpp`)
* Resolución de la ruta del archivo comprobando `root_dir` y `upload_store`.
* Verificación de existencia (`404 Not Found`), permisos de escritura (`403 Forbidden`) y protección contra borrado de directorios (`403 Forbidden`).
* Eliminación física mediante la función del sistema autorizada `unlink()` y respuesta `200 OK`.

---

## 5. Módulo Bonus de Sesiones y Cookies (`SessionManager` y `Router`)

* **Estructura de datos (`SessionData`)**: Registra en memoria la IP del cliente (`clientIp`), número de visitas (`visitCount`), marca de creación (`createdAt`) y marca de última actividad (`lastActivityTime`).
* **Ciclo de vida en memoria (`SessionManager`)**:
  - Generación de identificadores de sesión alfanuméricos pseudoaleatorios de 32 caracteres.
  - Almacenamiento en memoria no bloqueante (`std::map<std::string, SessionData>`).
  - Recuperación de sesiones (`getSession`), incremento automático del contador de visitas y actualización de actividad.
  - Limpieza periódica de sesiones expiradas por inactividad (`cleanExpiredSessions`).
* **Integración con HTTP (`Router::dispatch`)**:
  - Extracción transparente de la cookie `session_id` desde `HttpRequest::getCookies()`.
  - En peticiones sin sesión o caducadas: creación de nueva sesión y emisión de `Set-Cookie: session_id=...; Path=/; Max-Age=1800; HttpOnly` en `HttpResponse`.
  - Registro informativo en tiempo real mediante `Logger::info` reportando el ID de sesión, IP del cliente, número de visitas y total de sesiones activas.

---

## 6. Despliegue de Entorno y Configuración

* **`config/tester.conf`**: Añadida directiva `index youpi.bad_extension;` en `location /` requerida por el tester oficial.
* **`config/default.conf`**: Especificada la raíz propia `root ./www/cgi-bin;` para el bloque `location /cgi-bin`.
* **Árbol de Directorios `./www`**:
  - `www/index.html`: Página de bienvenida estática.
  - `www/errors/404.html` y `www/errors/50x.html`: Páginas de error personalizadas.
  - `www/uploads/`: Directorio de almacenamiento para pruebas de subida y borrado.
  - `www/example/index.html`: Virtual host secundario para validación de nombres de servidor.
  - `www/cgi-bin/hello.py`: Script CGI en Python 3 para pruebas de ejecución dinámica.

---

## 7. Makefile y Documentación

* **`Makefile`**: Incorporados a la regla de compilación los módulos `PostExecutor.cpp`, `DeleteExecutor.cpp` y `SessionManager.cpp`.
* **`README.md`**: Actualizado íntegramente al inglés según el estándar de entrega del Subject v24.0:
  - Cabecera obligatoria en cursiva: `*This project has been created as part of the 42 curriculum by rhiguita.*`
  - Secciones requeridas: `Description`, `Instructions` y `Resources`.
  - Declaración formal de uso ético de herramientas de Inteligencia Artificial conforme al Capítulo III del Subject.

---

## 8. Verificación y Resultados Finales

### 8.1. Suite Oficial `./tester/tester`
* **Resultado**: **100% Superado**.
* Superadas todas las pruebas funcionales, peticiones con cuerpos masivos de 100 MB hacia scripts CGI, y pruebas de estrés concurrentes (100.000 peticiones GET y 20 workers paralelos de CGI).
* Mensaje final emitido por el evaluador oficial:
  ```text
  ********************************************************************************
  GG, So far so good! Run your own tests now! :D
  ********************************************************************************
  ```

### 8.2. Diagnóstico de Memoria Valgrind
* **Resultado**: **0 bytes en fuga, 0 errores en 0 contextos**.
  ```text
  ==158045== HEAP SUMMARY:
  ==158045==     in use at exit: 0 bytes in 0 blocks
  ==158045==   total heap usage: 381 allocs, 381 frees, 154,343 bytes allocated
  ==158045== All heap blocks were freed -- no leaks are possible
  ==158045== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
  ```
