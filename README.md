<div align="center">

# ARGOS
### Sistema Autónomo Resiliente de Operaciones en Vuelo

*Software de misión embarcado para UAVs — sin depender de tierra*

![Estado](https://img.shields.io/badge/estado-fase%209.1%20completa-orange)
![ROS2](https://img.shields.io/badge/ROS2-Humble%20%2F%20Iron-informational)
![Lang](https://img.shields.io/badge/C%2B%2B17-informational)
![Licencia](https://img.shields.io/badge/licencia-Propietaria-red)

</div>

---

## Por qué "ARGOS"

En la mitología griega, **Argos Panoptes** (*el que todo lo ve*) era un gigante con
cien ojos que nunca dormía. Vigilaba, protegía y nunca fallaba en su guardia.

Me pareció el nombre justo para un sistema cuyo trabajo es exactamente ese: nunca
perder de vista el estado del vehículo, detectar cada falla antes de que escale y
mantener la misión en curso aunque nadie esté mirando desde tierra.

---

## Qué es esto

ARGOS es una **capa de software de misión autónoma** que corre en un companion
computer a bordo del UAV. Se sienta entre el flight controller (PX4) y la GCS,
y actúa como el cerebro local del vehículo.

```
┌────────────────────────────────┐
│         GCS  (tierra)          │
└────────────────┬───────────────┘
                 │  enlace cifrado
┌────────────────▼───────────────┐
│    ARGOS  (companion computer) │
└────────────────┬───────────────┘
                 │
┌────────────────▼───────────────┐
│    PX4  (flight controller)    │
└────────────────────────────────┘
```

Si PX4 mantiene el vehículo en el aire, ARGOS decide **qué hacer**, **cuándo** y
**qué hacer cuando algo sale mal** — sin requerir una conexión activa con tierra.

---

## El problema que intenta resolver

Los fabricantes nacionales construyen airframes capaces e integran FCs robustos.
Lo que no tienen es una **capa de misión local, auditable y nuestra**. Hoy la
lógica de misión depende de software importado que no fue pensado para operar de
forma completamente autónoma, no puede auditarse ni certificarse, y no es de nadie
en este país.

Si se cae el enlace con la GCS → comportamiento indefinido. El código fuente no es
tuyo. No hay producto nacional que resuelva esto.

ARGOS intenta llenar ese vacío.

---

## Qué hace (en concreto)

- Ejecuta misiones precargadas sin requerir conexión activa con la GCS
- Monitorea subsistemas críticos en RT y responde a fallas de forma automática
- Gestiona el ciclo de vida de vuelo completo via FSM determinista (10 estados)
- Canal de comunicación cifrado y autenticado con la estación en tierra
- Logs estructurados y reportes de misión para análisis post-vuelo

---

## Stack técnico

**ROS2 Humble/Iron · C++17 · colcon · CMake · Google Test**

Target de deployment: RPi4 o Jetson Nano junto a un FC PX4.

Patrón arquitectónico: `argos_core_*` (C++17 puro, sin ROS2) + `argos_*_node`
(adaptadores lifecycle finos sobre `rclcpp_lifecycle::LifecycleNode`). Así la
lógica de negocio es testeable sin levantar un grafo ROS2 completo.

---

## Referentes

Arquitectónicamente ARGOS es pariente de Auterion OS, NASA cFS, Anduril Lattice y
Shield AI Hivemind. Ninguno de esos es auditable ni adaptable por la industria de
defensa argentina. Este sí.

---

## Estado actual

| Fase | Descripción | Estado |
|------|-------------|--------|
| **9.1** | Core infra — msgs, autoridad, FDIR, nav, misión, nodos ROS2 lifecycle | ✅ Completa |
| 9.2 | Authority & Lifecycle FSM — FSM 10 estados + `mission_executor_node` | 🔲 Siguiente |
| 9.3 | Navigation nodes | 🔲 |
| 9.4 | Fault Management nodes | 🔲 |
| 9.5 | Mission Execution (Behavior Tree) | 🔲 |
| 9.6 | GCS Bridge | 🔲 |
| 9.7 | PX4 Integration (MAVLink / uXRCE) | 🔲 |
| 9.8 | SIL Demo — misión completa en PX4 SITL + Gazebo Harmonic | 🔲 |
| 9.9 | Hardening — sanitizers, fuzzing, cobertura | 🔲 |

La Fase 9.1 entregó 9 packages en `src/` con interfaces puras C++17,
implementaciones con GTest y dos nodos lifecycle funcionales.

---

## Claude Code en este proyecto

Usé **[Claude Code](https://claude.ai/code)** como par de programación a lo largo
de toda la implementación. No como generador de código a ciegas, sino como colega
senior con quien discutir decisiones antes de escribir una línea.

Lo usé principalmente para:

- **Diseño** — definir el patrón Core/Node, las interfaces C++17, la separación de
  responsabilidades entre subsistemas y los trade-offs de cada decisión
- **Implementación** — generar clases, mocks de GTest y CMakeLists.txt respetando
  las restricciones de la arquitectura que habíamos acordado
- **Review técnico** — detectar deuda técnica (ej. uso incorrecto de `find_package`
  vs `ament_target_dependencies` en un workspace colcon), analizar alternativas
- **Git** — organizar commits por feature, reescribir historial, resolver
  divergencias entre rama local y remota

Cada decisión de diseño pasó por mí. Claude Code es una herramienta, no el
ingeniero a cargo.

---

## Licencia

Software propietario. Repositorio público solo a modo de referencia. Sin
autorización escrita del autor no se otorga ningún derecho de uso, copia,
modificación ni distribución.

Ver [LICENSE](./LICENSE).

---

**Santiago Flores**  
Ingeniería en Informática — UBP, Córdoba, Argentina  
[github.com/SantiFlores06](https://github.com/SantiFlores06)
