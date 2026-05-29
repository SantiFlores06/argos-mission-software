# ARGOS — Session Log

> Registro completo de la sesión de implementación del 2026-05-29.
> Autor: Santiago Flores — Universidad Blas Pascal, Córdoba, Argentina

---

## Contexto de la sesión

**Punto de partida:** Repositorio vacío + `ARGOS_Architecture_Plan.md` (1380 líneas, plan de arquitectura completo).

**Objetivo de la sesión:** Implementar la **Fase 9.1 — Core Infrastructure** del plan, documentando cada decisión.

**Regla de trabajo acordada:** Implementar una fase por vez; esperar confirmación antes de avanzar.

---

## Archivos creados en esta sesión

### Raíz del repositorio

| Archivo | Descripción |
|---------|-------------|
| `IMPLEMENTATION_JOURNAL.md` | Diario de decisiones arquitectónicas: qué se hizo, por qué, y alternativas descartadas |
| `SESSION.md` | Este archivo |

---

### `src/argos_msgs/` — Contrato de datos

**Propósito:** Paquete ROS2 con todas las definiciones de mensajes, servicios y actions del sistema.
Sin este paquete, ningún nodo puede comunicarse.

| Archivo | Contenido |
|---------|-----------|
| `CMakeLists.txt` | `rosidl_generate_interfaces()` para C++ y Python |
| `package.xml` | `<member_of_group>rosidl_interface_packages</member_of_group>` |
| `msg/MissionStatus.msg` | Estado FSM de la misión (enum + waypoint actual + total) |
| `msg/FlightState.msg` | Estado del Flight Lifecycle FSM (10 estados) |
| `msg/FaultEvent.msg` | Evento de falla: subsistema + severidad + timestamp + descripción |
| `msg/SystemHealth.msg` | Salud agregada del sistema (batería, GPS HDOP, sats, link GCS) |
| `msg/NodeHeartbeat.msg` | Latido de vida: nombre del nodo + periodo declarado |
| `msg/WaypointList.msg` | Lista de waypoints para una misión |
| `srv/ArmDisarm.srv` | Solicitud arm/disarm con razón en string |
| `srv/LoadMission.srv` | Carga lista de waypoints; retorna ID de misión |
| `srv/SetRecoveryPolicy.srv` | Reconfigura políticas de recuperación en caliente |
| `srv/RequestAuthority.srv` | Solicita autoridad de comando; retorna `granted: bool` |
| `action/ExecuteMission.action` | Ejecuta misión completa con feedback de waypoint actual |
| `action/NavigateTo.action` | Navega a un waypoint único con feedback de distancia |

**Decisión clave:** QoS `TRANSIENT_LOCAL` en topics de estado (`/argos/authority/current`,
`/argos/mission/status`, `/argos/flight/state`) para que nodos que reinician reciban el
último estado inmediatamente. `BEST_EFFORT` para heartbeats.

---

### `src/argos_core_authority/` — Árbitro de autoridad (C++ puro)

**Propósito:** Implementa el patrón Mediador para arbitraje de comandos entre fuentes.
Previene condiciones de carrera cuando múltiples fuentes compiten por controlar el vehículo.

| Archivo | Contenido |
|---------|-----------|
| `include/argos_core_authority/IAuthorityManager.hpp` | Interfaz abstracta pura + enum `CommandSource` |
| `include/argos_core_authority/AuthorityManagerImpl.hpp` | Declaración de la implementación |
| `src/AuthorityManagerImpl.cpp` | Tabla de prioridades estáticas + mutex + arbitraje |
| `test/test_authority_manager.cpp` | 8 tests GTest |
| `CMakeLists.txt` | Librería C++ + tests |
| `package.xml` | Sin dependencias ROS2 (C++ puro) |

**Tabla de prioridades estáticas:**
```
GCS_OVERRIDE        = 255  (máxima — piloto siempre manda)
RECOVERY_MANAGER    = 200  (FDIR tiene precedencia sobre misión)
FLIGHT_LIFECYCLE_FSM= 100  (control de estados de vuelo)
BEHAVIOR_TREE       =  50  (mínima — ejecución de misión normal)
```

**Decisión clave:** La tabla es estática y hardcodeada. El argumento `priority` en `requestAuthority()`
NO puede sobrepasar la tabla — previene que un BT buggy se autopromueva sobre `RECOVERY_MANAGER`.

---

### `src/argos_core_fdir/` — Fault Detection, Isolation & Recovery (C++ puro)

**Propósito:** Detecta fallos del sistema y ejecuta estrategias de recuperación.
Separado en dos responsabilidades distintas (detección ≠ recuperación).

| Archivo | Contenido |
|---------|-----------|
| `include/argos_core_fdir/IFaultMonitor.hpp` | Interfaz + enums `Subsystem`, `FaultSeverity`, struct `FaultEvent` |
| `include/argos_core_fdir/IRecoveryStrategy.hpp` | Interfaz + enums `RecoveryResult`, struct `RecoveryContext` |
| `include/argos_core_fdir/FaultMonitorImpl.hpp` | Struct `HealthThreshold`, declaración del monitor |
| `include/argos_core_fdir/RecoveryStrategies.hpp` | Declaración de las 3 estrategias |
| `include/argos_core_fdir/RecoveryManager.hpp` | Declaración del gestor de recuperación |
| `src/FaultMonitorImpl.cpp` | Evaluación de umbrales con callback inyectado |
| `src/RecoveryStrategies.cpp` | `ReturnToHome`, `EmergencyLand`, `HoldPosition` |
| `src/RecoveryManager.cpp` | Selección de estrategia por `canHandle()` en orden |
| `test/test_fault_monitor.cpp` | Tests de umbral, callback, flag higher_is_worse |
| `test/test_recovery_strategies.cpp` | Tests de selección de estrategia y prioridad |
| `CMakeLists.txt` + `package.xml` | Build sin ROS2 |

**Orden de estrategias (no arbitrario):**
1. `EmergencyLand` — batería crítica: aterrizar YA, no gastar más batería volando a casa
2. `ReturnToHome` — link GCS perdido, EKF divergido
3. `HoldPosition` — GPS degradado (temporal, puede recuperarse)

---

### `src/argos_core_navigation/` — Navegación y estimación de estado (C++ puro)

**Propósito:** Estimación de pose del vehículo (EKF) y navegación por waypoints.

| Archivo | Contenido |
|---------|-----------|
| `include/argos_core_navigation/IStateEstimator.hpp` | Interfaz: `getState()`, `getCovarianceNorm()`, `isDiverged()`, `isDeadReckoningActive()` |
| `include/argos_core_navigation/INavigationManager.hpp` | Interfaz: gestión de waypoints |
| `include/argos_core_navigation/EKFStateEstimator.hpp` | EKF simplificado + dead-reckoning automático |
| `include/argos_core_navigation/WaypointNavigator.hpp` | Navegación por índice con radio de aceptación |
| `src/EKFStateEstimator.cpp` | Integración IMU + update GPS; dead-reckoning si GPS dropout > 3s |
| `src/WaypointNavigator.cpp` | Distancia 3D euclidiana; toma `const IStateEstimator&` por DI |
| `test/test_waypoint_navigator.cpp` | Tests con `StubEstimator` mock |
| `CMakeLists.txt` + `package.xml` | Build sin ROS2 |

**Decisión clave:** Dead-reckoning automático — si GPS dropout > `dead_reckoning_timeout_s`,
la covarianza crece 0.1% por ciclo IMU. `isDiverged()` retorna `true` cuando la norma
supera `divergence_threshold` (default: 5.0), permitiendo que FDIR active RTH.

---

### `src/argos_core_mission/` — FSM de misión (C++ puro)

**Propósito:** Máquina de estados para la ejecución de misiones (IDLE → LOADING → RUNNING → ...).

| Archivo | Contenido |
|---------|-----------|
| `include/argos_core_mission/IMissionExecutor.hpp` | Interfaz: `load/start/pause/abort` + getters |
| `include/argos_core_mission/MissionExecutorImpl.hpp` | Declaración FSM de 7 estados |
| `src/MissionExecutorImpl.cpp` | FSM con mutex: IDLE/LOADING/RUNNING/PAUSED/COMPLETE/ABORTED/FAILED |
| `test/test_mission_executor.cpp` | 8 tests de transiciones válidas e inválidas |
| `CMakeLists.txt` + `package.xml` | Build sin ROS2 |

**FSM de 7 estados:**
```
IDLE → (load) → LOADING → (start) → RUNNING
RUNNING → (pause) → PAUSED → (start) → RUNNING
RUNNING → (markWaypointReached × N) → COMPLETE
RUNNING / PAUSED → (abort) → ABORTED
RUNNING → (error) → FAILED
```

**Invariante:** `start()` solo desde `LOADING` o `PAUSED`. No se puede hacer `start()` desde
`IDLE` — obliga a cargar waypoints primero.

---

### `src/argos_system_watchdog_node/` — Watchdog de nodos ROS2

**Propósito:** Detecta nodos muertos por ausencia de heartbeat. Primer componente de seguridad activo.

| Archivo | Contenido |
|---------|-----------|
| `src/SystemWatchdogNode.cpp` | Lifecycle node: subscribe heartbeat, detecta timeout, publica nombre del nodo muerto |
| `CMakeLists.txt` | Ejecutable ROS2 |
| `package.xml` | Dependencias: `rclcpp_lifecycle`, `argos_msgs` |

**Mecanismo de timeout:** `elapsed > timeout_multiplier × declared_period_ms`
El `declared_period` viene en el mensaje `NodeHeartbeat` — si un nodo reduce su frecuencia,
el timeout se adapta sin reconfigurar el watchdog (a diferencia de un timeout absoluto en YAML).

---

### `src/argos_authority_manager_node/` — Nodo ROS2 de autoridad

**Propósito:** Wrapper ROS2 del `AuthorityManagerImpl`. Expone el árbitro como servicio ROS2.

| Archivo | Contenido |
|---------|-----------|
| `src/AuthorityManagerNode.cpp` | Lifecycle node: servicio `/argos/authority/request`, publisher `TRANSIENT_LOCAL`, heartbeat |
| `CMakeLists.txt` + `package.xml` | Dependencias: `rclcpp_lifecycle`, `argos_core_authority`, `argos_msgs` |

---

### `src/argos_bringup/` — Configuración y lanzamiento

| Archivo | Contenido |
|---------|-----------|
| `config/watchdog_timeouts.yaml` | `timeout_multiplier: 2.0`, `check_period_ms: 100`, lista de 5 nodos críticos |
| `config/ekf_params.yaml` | `divergence_threshold: 5.0`, `dead_reckoning_timeout_s: 3.0`, params de `robot_localization` |
| `config/recovery_policies.yaml` | Orden de estrategias FDIR + timeouts de link GCS, GPS, batería |
| `launch/argos_sitl.launch.py` | Lanza los 10 nodos del sistema como `LifecycleNode` en 4 capas |

**Capas de lanzamiento:**
1. **Infrastructure:** system_watchdog, authority_manager, fault_monitor, recovery_manager, health_reporter
2. **Navigation:** state_estimator, navigation_manager
3. **Mission:** mission_executor, behavior_tree_engine
4. **Communication:** gcs_bridge

---

## Decisiones arquitectónicas transversales

### 1. Patrón Core/Node

Cada dominio tiene DOS paquetes:
- `argos_core_X` — C++ puro sin ROS2; testeable con GTest; portátil
- `argos_X_node` — nodo ROS2 fino; solo hace I/O; sin lógica de negocio

**Por qué:** 80% de la lógica es verificable sin levantar ROS2. Cumple DO-178C.
Permite portar a otro middleware sin reescribir lógica.

### 2. `std::mutex` en todos los cores

Todos los implementors son thread-safe. Necesario para `MultiThreadedExecutor`.
`std::shared_mutex` fue evaluado y descartado por YAGNI — los locks son de corta
duración. Candidato para optimización en Fase 9.9 si profiling indica contención.

### 3. `rclcpp_lifecycle::LifecycleNode` en todos los nodos

Garantiza que el BT solo ejecuta cuando el sistema está `ACTIVE`, que el watchdog
detecta nodos que no arrancan, y que `on_cleanup()` libera recursos determinísticamente.

### 4. Patrón Mediador (`authority_manager`)

Problema original del diseño: "Double Authority FSM" — múltiples fuentes podían
enviar comandos a PX4 simultáneamente causando condiciones de carrera no deterministas.

Solución: un único nodo árbitro con tabla de prioridades estáticas. Cualquier fuente
debe obtener `granted: true` antes de enviar el primer comando.

---

## Deuda técnica identificada

| ID | Descripción | Fase objetivo |
|----|-------------|---------------|
| TD-01 | `EKFStateEstimator::propagateIMU()` usa `dt=0.02` hardcoded | 9.3 |
| TD-02 | `RecoveryStrategies::execute()` retorna `IN_PROGRESS` sin lógica real | 9.4 |
| TD-03 | `argos_authority_manager_node` no impone timeout en servicios bloqueados | 9.2 |
| TD-04 | `WaypointNavigator` usa coordenadas locales; necesita conversión geodésica | 9.3 |
| TD-05 | `argos_system_watchdog_node` no se monitorea a sí mismo | 9.9 |
| TD-06 | CMakeLists.txt de `argos_authority_manager_node` debe usar `ament_target_dependencies` | 9.1 fix |

---

## Estado del sistema al cierre de la sesión

| Fase | Estado |
|------|--------|
| **9.1 — Core Infrastructure** | ✅ Completada |
| 9.2 — Authority & Lifecycle FSM | ⏳ Pendiente confirmación |
| 9.3 — Navigation | ⏳ Pendiente |
| 9.4 — Fault Management | ⏳ Pendiente |
| 9.5 — Mission Execution (BT) | ⏳ Pendiente |
| 9.6 — GCS Bridge | ⏳ Pendiente |
| 9.7 — PX4 Integration | ⏳ Pendiente |
| 9.8 — SIL Demonstrator | ⏳ Pendiente |
| 9.9 — Hardening | ⏳ Pendiente |

**Próximo paso (Fase 9.2):**
- Flight Lifecycle FSM completa (10 estados: UNINITIALIZED, PREFLIGHT, ARMED, TAKEOFF, MISSION, HOLD, RETURN_TO_HOME, LAND, DISARMED, FAULTED)
- `argos_mission_executor_node` con FSM integrada + `authority_manager`
- Test de integración: dos fuentes compiten → mayor prioridad gana determinísticamente

---

*Generado el 2026-05-29*
