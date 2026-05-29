# ARGOS — Implementation Journal

> Documenta **qué** se implementó, **por qué** se tomó cada decisión arquitectónica,
> y **qué alternativas** existían para cada problema clave.
> Autor: Santiago Flores — Universidad Blas Pascal, Córdoba, Argentina

---

## Índice

1. [Fase 9.1 — Core Infrastructure](#fase-91--core-infrastructure)
   - [argos_msgs](#argos_msgs)
   - [argos_core_authority](#argos_core_authority)
   - [argos_core_fdir](#argos_core_fdir)
   - [argos_core_navigation](#argos_core_navigation)
   - [argos_core_mission](#argos_core_mission)
   - [argos_system_watchdog_node](#argos_system_watchdog_node)
   - [argos_authority_manager_node](#argos_authority_manager_node)
2. [Fase 9.2 — Authority & Lifecycle](#fase-92--authority--lifecycle) *(pendiente)*
3. [Fase 9.3 — Navigation](#fase-93--navigation) *(pendiente)*
4. [Fase 9.4 — Fault Management](#fase-94--fault-management) *(pendiente)*
5. [Decisiones transversales](#decisiones-transversales)
6. [Deuda técnica registrada](#deuda-técnica-registrada)

---

## Fase 9.1 — Core Infrastructure

**Objetivo:** Crear los contratos de datos (mensajes ROS2), las librerías de lógica pura (sin ROS2),
y el watchdog de sistema. Todo lo demás del sistema depende de estos artefactos.

**Estado:** ✅ Implementado

---

### `argos_msgs`

**Qué se hizo:**
Paquete ROS2 que genera bindings C++ y Python para:
- 6 mensajes: `MissionStatus`, `FlightState`, `FaultEvent`, `SystemHealth`, `NodeHeartbeat`, `WaypointList`
- 4 servicios: `ArmDisarm`, `LoadMission`, `SetRecoveryPolicy`, `RequestAuthority`
- 2 actions: `ExecuteMission`, `NavigateTo`

**Por qué este diseño:**
Concentrar todas las definiciones de interfaz en un solo paquete permite que cualquier nodo pueda
depender de `argos_msgs` sin crear dependencias circulares. Los mensajes son el contrato entre nodos;
cambiarlos tiene impacto conocido y aislado.

**Decisión: `TRANSIENT_LOCAL` QoS en topics de estado**

Problema: si un nodo se reinicia (por ejemplo, tras un watchdog timeout), ¿cómo recibe el último
estado del sistema sin esperar al próximo publish?

| Opción | Pro | Contra |
|--------|-----|--------|
| `VOLATILE` (default) | Simple | Nodo recién iniciado no recibe estado hasta el próximo publish |
| **`TRANSIENT_LOCAL`** ✅ | Late-joiner recibe último mensaje inmediatamente | Requiere publisher y subscriber con misma QoS |
| Servicio de consulta | Control explícito | Añade latencia y complejidad |

**Decisión:** `TRANSIENT_LOCAL` en `/argos/authority/current`, `/argos/mission/status`, `/argos/flight/state`.
`BEST_EFFORT` en heartbeats — perder un heartbeat es aceptable; saturar la red no.

---

### `argos_core_authority`

**Qué se hizo:**
- `IAuthorityManager` — interfaz abstracta pura
- `AuthorityManagerImpl` — implementación con tabla de prioridades estáticas + mutex
- 8 tests unitarios GTest

**Por qué separar interfaz de implementación:**
Dependency Inversion (DIP). Los nodos que consumen la autoridad dependen de `IAuthorityManager`,
no de `AuthorityManagerImpl`. En tests se puede inyectar un mock sin modificar el código de producción.
Esto también permite cumplir DO-178C: la lógica de negocio es verificable sin entorno ROS2.

**Decisión: Tabla de prioridades estáticas vs. prioridades dinámicas**

El plan original especifica `priority: uint8_t` en `requestAuthority()`. Dos estrategias posibles:

| Estrategia | Descripción | Pro | Contra |
|------------|-------------|-----|--------|
| **Prioridades estáticas** ✅ | Tabla hardcodeada: GCS=255, Recovery=200, FSM=100, BT=50 | Determinista; imposible que BT se eleve sobre GCS en runtime | Menos flexible si se añaden fuentes |
| Prioridades dinámicas puras | El caller decide la prioridad en cada llamada | Flexible | Un nodo buggy puede escalarse artificialmente |
| Híbrido (elegido) | Tabla estática tiene precedencia; `priority` arg solo desempata | Determinismo + algo de flexibilidad | Dos niveles de prioridad a entender |

**Implementación elegida:** La tabla estática es el árbitro final. El argumento `priority` en
`requestAuthority()` sirve para desempates dentro de la misma fuente, no para overridear la tabla.
Esto previene que un nodo de BT buggy se autopromueva sobre `RECOVERY_MANAGER`.

**Invariante crítica implementada:**
```cpp
// resolveConflict: la tabla estática tiene precedencia sobre el argumento runtime
uint8_t static_challenger = priority_table_.find(challenger)->second;
uint8_t static_holder     = priority_table_.find(current_authority_)->second;
if (static_challenger > static_holder) { ... }
```

**Alternativa descartada — Lock-free con `std::atomic`:**
Se consideró usar `std::atomic<CommandSource>` para eliminar el mutex. Se descartó porque la
operación de arbitraje involucra leer el holder actual Y escribir el nuevo holder como unidad
atómica — necesita compare-and-swap sobre un struct, que no cabe en un `std::atomic` estándar
sin usar `compare_exchange` con riesgo de ABA problem. El mutex es más simple y correcto.

---

### `argos_core_fdir`

**Qué se hizo:**
- `IFaultMonitor`, `IRecoveryStrategy` — interfaces abstractas
- `FaultMonitorImpl` — evaluación de umbrales con callback de falla
- `ReturnToHomeStrategy`, `EmergencyLandStrategy`, `HoldPositionStrategy`
- `RecoveryManager` — selección y gestión de estrategia activa
- Tests unitarios completos

**Decisión: Separar detección de recuperación en clases distintas**

El diseño original tenía `argos_fdir` como un nodo único que hacía detección Y recuperación.
Esto crea un punto único de falla: si el nodo falla durante la recuperación, no hay quien lo detecte.

| Diseño | Pro | Contra |
|--------|-----|--------|
| Un nodo FDIR unificado | Simple | Si muere durante recovery, el sistema está ciego |
| **Dos nodos separados** ✅ | FaultMonitor puede detectar fallos del RecoveryManager | Más complejidad de integración |
| Tres nodos (+ Health Reporter) | Máxima separación | Overhead de comunicación |

**Implementación:** `FaultMonitorImpl` solo evalúa métricas y emite eventos. `RecoveryManager`
solo ejecuta estrategias. El `argos_system_watchdog` monitorea a ambos.

**Decisión: Strategy Pattern para recuperación**

El `RecoveryManager` tiene un `vector<unique_ptr<IRecoveryStrategy>>` y llama a `canHandle()`
en orden. El primero que puede manejar el fallo "gana".

Alternativas:
| Opción | Pro | Contra |
|--------|-----|--------|
| Switch/case por subsistema | Simple | No extensible; modificar para añadir estrategia |
| **Strategy + canHandle()** ✅ | Open/Closed Principle; agregar estrategia = agregar clase | Orden de registro importa |
| Map subsistema → estrategia | Explícito | Cada subsistema solo tiene una estrategia |

**Orden de estrategias registradas (crítico):**
```
1. EmergencyLand  — batería crítica; aterrizar AHORA, no volar a casa
2. ReturnToHome   — link GCS perdido, EKF divergido
3. HoldPosition   — GPS degradado (temporal)
```
El orden no es arbitrario: batería crítica debe resolverse con EmergencyLand, no con RTH
(volar más consume batería). Esto se documenta en `recovery_policies.yaml`.

**Decisión: Callback vs. Publisher en FaultMonitorImpl**

`FaultMonitorImpl` es C++ puro (sin ROS2). Para notificar faults al nodo adaptador ROS2:

| Opción | Pro | Contra |
|--------|-----|--------|
| `std::function` callback ✅ | Core lib sin ROS2; testeable | Un solo subscriber |
| ROS2 topic (desde el core) | Múltiples subscribers nativos | Viola la separación core/adapter |
| Cola thread-safe + polling | Desacopla timing | Más complejo |

**Implementación:** Callback `std::function<void(const FaultEvent&)>` inyectado desde el nodo adaptador.
El nodo adaptador convierte el `FaultEvent` en un `argos_msgs::msg::FaultEvent` y lo publica.

---

### `argos_core_navigation`

**Qué se hizo:**
- `IStateEstimator` — interfaz con `getCovarianceNorm()` e `isDiverged()` (ausentes en diseño original)
- `INavigationManager` — interfaz de gestión de waypoints
- `EKFStateEstimator` — implementación con integración IMU + update GPS, dead-reckoning automático
- `WaypointNavigator` — navegación por índice con radio de aceptación configurable
- Tests unitarios con `StubEstimator` mock

**Decisión: EKF stub vs. robot_localization directo**

El `EKFStateEstimator` en el core es un EKF simplificado (integración constante + update GPS naive).
La implementación de producción usa `robot_localization::EKF` desde el nodo adaptador ROS2.

¿Por qué el stub?
1. Permite tests unitarios de `WaypointNavigator` sin ROS2 ni `robot_localization`
2. `IStateEstimator` define el contrato: cualquier implementación funciona
3. El nodo `argos_state_estimator_node` wrappea `robot_localization` y alimenta al EKFStateEstimator,
   o bien expone directamente su estado como implementación de `IStateEstimator`

| Opción | Pro | Contra |
|--------|-----|--------|
| Core con EKF completo (Eigen) | Portable | Duplica robot_localization; mantenimiento doble |
| **Core con stub + adapter ROS2** ✅ | Tests puros; robot_localization en producción | El stub no es el mismo código que producción |
| Solo robot_localization | Una implementación | No testeable sin ROS2 |

**Decisión: Dead-reckoning automático en EKFStateEstimator**

Si no se recibe GPS por más de `dead_reckoning_timeout_s` (default: 3s), `isDeadReckoningActive()`
retorna `true` y la covarianza crece por factor 1.001 por ciclo IMU.

Esto permite que `FaultMonitorImpl` detecte independientemente el estado del EKF, y que
`HoldPositionStrategy` sepa que la posición es estimada (no medida).

**Decisión: Distancia 3D vs. 2D en WaypointNavigator**

`distanceToWaypoint()` calcula distancia euclidiana 3D. Alternativas:

| Opción | Use case | Limitación |
|--------|----------|------------|
| 2D (xy) | Vehículos terrestres; altitud no importa | UAV puede estar al waypoint xy pero lejos en z |
| **3D euclidiana** ✅ | UAV con control de altitud | acceptance_radius más grande necesario |
| Cilíndrica (2D + ventana z) | UAV en entornos con altitud variable | Dos radios a configurar |

Para un UAV con control de altitud explícito (despegue a altitud de misión), 3D euclidiana
con `acceptance_radius_m = 1.0` es suficiente.

---

### `argos_core_mission`

**Qué se hizo:**
- `IMissionExecutor` — interfaz con los 4 estados de control (load/start/pause/abort) y 4 getters
- `MissionExecutorImpl` — FSM de 7 estados con mutex
- 8 tests unitarios

**Decisión: Estados de misión como enum vs. FSM formal**

`MissionStatus` es un enum de 7 valores. La FSM es implícita (implementada como validaciones
en cada método). Alternativas:

| Opción | Pro | Contra |
|--------|-----|--------|
| Enum + validaciones inline ✅ | Simple; suficiente para 7 estados | Transiciones no están centralizadas |
| Tabla de transiciones | Explícito; auditable | Overhead para 7 estados |
| boost::statechart / sc-xml | Formal; soporte para jerarquías | Dependencia extra; overkill en core |

Para el core puro, el enum con validaciones inline es suficiente. La FSM principal del sistema
(Flight Lifecycle con 10 estados) sí justifica una tabla de transiciones y se implementa en
`argos_mission_executor_node`.

**Invariante:** `startMission()` solo funciona desde `LOADING` o `PAUSED`. No se puede hacer
`start()` directamente desde `IDLE` — se requiere `load()` primero. Esto previene misiones
sin waypoints.

---

### `argos_system_watchdog_node`

**Qué se hizo:**
- Lifecycle node ROS2 que subscribe a `/argos/watchdog/heartbeat`
- Detecta timeouts: `elapsed > timeout_multiplier × declared_period`
- Publica nombre del nodo muerto en `/argos/watchdog/node_timeout`
- Configurable via YAML: qué nodos monitorear, multiplicador de timeout, periodo de chequeo

**Decisión: Watchdog centralizado vs. distribuido**

| Diseño | Pro | Contra |
|--------|-----|--------|
| **Un watchdog central** ✅ | Simple; si un nodo muere, el watchdog sigue vivo | El watchdog mismo es SPOF |
| Watchdog por nodo (peer watchdog) | Más robusto | Complejidad N²; cascading failures |
| PX4-style resource manager | Estándar aerospace | Requiere OS con soporte para RM |

**Mitigación del SPOF del watchdog:** El watchdog es un `LifecycleNode` intencionalmente
simple — solo lee timestamps y publica strings. Tiene cero dependencias en lógica de misión.
Si muere, el sistema entra en estado FAULTED por ausencia de heartbeat del propio watchdog
(monitoreado por el `argos_fault_monitor_node`).

**Decisión: `timeout_multiplier` en lugar de timeout absoluto**

Cada nodo declara su `period` en el mensaje `NodeHeartbeat`. El watchdog calcula el timeout
como `timeout_multiplier × period`. Ventaja: si un nodo reduce su frecuencia (por carga CPU),
el timeout se adapta automáticamente sin reconfigurar el watchdog.

**Alternativa descartada:** Timeout absoluto por nodo en el YAML. Problema: si el nodo cambia
su frecuencia de heartbeat, el YAML queda desincronizado.

---

### `argos_authority_manager_node`

**Qué se hizo:**
- Nodo ROS2 lifecycle que wrappea `AuthorityManagerImpl`
- Expone `/argos/authority/request` como servicio
- Publica `/argos/authority/current` con QoS `TRANSIENT_LOCAL`
- Envía heartbeat a `/argos/watchdog/heartbeat`

**Decisión: Servicio vs. Topic para solicitar autoridad**

| Opción | Pro | Contra |
|--------|-----|--------|
| **Servicio (Request/Response)** ✅ | El caller sabe inmediatamente si fue granted o denied | Bloquea si el authority_manager no responde |
| Topic de solicitud + topic de respuesta | No bloquea | Race condition si dos nodos solicitan simultáneamente |
| Acción ROS2 | Feedback durante la solicitud | Overhead para una operación que es instantánea |

La solicitud de autoridad es inherentemente síncrona: el caller necesita saber si puede
proceder antes de enviar el primer comando a PX4. Un servicio es la abstracción correcta.

---

## Decisiones transversales

### Separación `argos_core_*` / `argos_*_node`

La decisión más importante del workspace. Cada dominio tiene:
- `argos_core_X` — librería C++ pura, sin ROS2, testeable con GTest directo
- `argos_X_node` — nodo ROS2 que wrappea el core, hace I/O con topics/servicios

**Por qué:**
- El 80% de la lógica de negocio se prueba sin levantar un nodo ROS2
- Cumple DO-178C: code coverage medible con gcovr puro
- Permite portar a otra capa de middleware (zenoh, DDS nativo) sin reescribir lógica

### `std::mutex` en todos los cores

Todos los métodos de los implementors son thread-safe via `std::mutex`. Esto es necesario
porque los nodos ROS2 con `MultiThreadedExecutor` pueden llamar callbacks desde hilos distintos.

**Alternativa evaluada:** `std::shared_mutex` para operaciones de lectura frecuente.
Se descartó en esta fase por YAGNI — los locks son de corta duración y no justifican la
complejidad extra. Candidato para optimización en Fase 9.9 (Hardening) si profiling indica
contención.

### Lifecycle nodes en todos los nodos ROS2

Todos los nodos implementan `rclcpp_lifecycle::LifecycleNode`. Esto garantiza:
- El BT solo ejecuta cuando el sistema está en `ACTIVE`
- El watchdog puede detectar nodos que no alcanzan `ACTIVE` en el arranque
- `on_cleanup()` libera recursos determinísticamente

**Alternativa:** Nodos regulares `rclcpp::Node` con flags internos de "ready". Descartada
porque violaría la especificación ROS2 Managed Node y haría el arranque no determinista.

---

## Deuda técnica registrada

| ID | Descripción | Fase objetivo |
|----|-------------|---------------|
| TD-01 | `EKFStateEstimator::propagateIMU()` usa `dt=0.02` hardcoded. En producción debe derivarse de `rclcpp::Clock` | 9.3 |
| TD-02 | `RecoveryStrategies::execute()` retorna `IN_PROGRESS` — el nodo adapter debe implementar la lógica de polling | 9.4 |
| TD-03 | `argos_authority_manager_node` no impone timeout en servicios bloqueados | 9.2 |
| TD-04 | `WaypointNavigator::distanceToWaypoint()` usa coordenadas locales (x,y,z). Necesita conversión geodésica para GPS lat/lon/alt | 9.3 |
| TD-05 | `argos_system_watchdog_node` no se monitorea a sí mismo — necesita un mecanismo de auto-heartbeat | 9.9 |
| TD-06 | `CMakeLists.txt` de `argos_authority_manager_node` usa `find_package(argos_core_authority)` que requiere que el paquete esté instalado; en el workspace colcon se usa `ament_target_dependencies` | 9.1 fix |

---

## Próximos pasos

### Fase 9.2 — Authority & Lifecycle
- [ ] Implementar Flight Lifecycle FSM completa (10 estados, tabla de transiciones)
- [ ] `argos_mission_executor_node` con FSM + integración authority_manager
- [ ] Test de integración: dos fuentes compiten → mayor prioridad gana

### Fase 9.3 — Navigation
- [ ] `argos_state_estimator_node` wrappea robot_localization
- [ ] `argos_navigation_manager_node` con offboard keepalive
- [ ] Conversión geodésica lat/lon/alt → NED local

### Fase 9.4 — Fault Management
- [ ] `argos_fault_monitor_node` suscrito a todos los health topics
- [ ] `argos_recovery_manager_node` con las tres estrategias
- [ ] `argos_health_reporter_node`

---

*Última actualización: Fase 9.1 completada — 2026-05-29*
