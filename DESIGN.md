# Arquitectura de la Biblioteca Proto

Esta biblioteca implementa un runtime para lenguajes de programación, con un fuerte enfoque en estructuras de datos inmutables, un modelo de objetos basado en prototipos y un sistema de gestión de memoria concurrente de alto rendimiento.

## Principios de Diseño

El núcleo de la biblioteca se basa en los siguientes principios:

1.  **Inmutabilidad:** Todas las estructuras de datos fundamentales (listas, tuplas, cadenas) son inmutables. Las modificaciones no alteran las estructuras existentes, sino que crean nuevas versiones con los cambios aplicados (copy-on-write). Esto simplifica enormemente la programación concurrente al eliminar la necesidad de locks complejos para proteger los datos.
2.  **Gestión de Memoria Basada en Celdas:** Toda la memoria para los objetos del runtime se gestiona a través de celdas de tamaño fijo (64 bytes). Este enfoque elimina la fragmentación de memoria y permite el uso de un asignador (allocator) extremadamente simple y rápido.
3.  **Modelo de Objetos Basado en Prototipos:** En lugar de clases tradicionales, el sistema utiliza un modelo de objetos basado en prototipos, donde los objetos heredan propiedades y comportamientos directamente de otros objetos.
4.  **Alto Rendimiento y Concurrencia:** El diseño está optimizado para sistemas multi-core, minimizando la contención de locks y maximizando el paralelismo.

## Gestión de Memoria y Garbage Collection (GC)

El sistema de gestión de memoria es uno de los componentes más críticos y sofisticados de la biblioteca.

### Asignador de Memoria (Allocator)

-   **Pool por Thread:** Cada thread del sistema operativo posee su propio pool de celdas de memoria libres. Cuando un thread necesita crear un nuevo objeto, toma una celda de su pool local.
-   **Asignación Rápida:** Este diseño permite una asignación de memoria casi instantánea, ya que no requiere un lock global. La contención se minimiza, permitiendo que los threads se ejecuten en paralelo sin bloquearse mutuamente durante la creación de objetos.
-   **Obtención de Bloques:** Si el pool local de un thread se agota, solicita un nuevo bloque de celdas al `ProtoSpace` global, que gestiona el heap principal. El `ProtoSpace` también es responsable de la asignación inicial de grandes bloques de memoria del sistema operativo.

### Garbage Collector (GC) Concurrente

El GC está diseñado para minimizar las pausas y el impacto en el rendimiento de la aplicación.

-   **Thread Dedicado:** El GC se ejecuta en su propio thread (`gcThreadLoop` en `ProtoSpace.cpp`), operando en paralelo con los threads de la aplicación.
-   **Seguimiento de Asignaciones (`DirtySegment`):** Las celdas recién asignadas por los threads de la aplicación se encadenan en `DirtySegment`s, que son luego procesados por el GC. Esto permite al GC identificar eficientemente la memoria que necesita ser analizada.
-   **Sincronización de Threads (`synchToGC`):** Los threads de la aplicación utilizan el método `synchToGC` para coordinar con el GC. Durante una fase de "stop-the-world" parcial, los threads se detienen brevemente (`THREAD_STATE_STOPPED`) para permitir que el GC recolecte las raíces de forma segura.
-   **GC Híbrido (Stop-the-World Parcial):** El GC no detiene el mundo por completo para todo su ciclo. La fase de "stop-the-world" es muy breve y se utiliza únicamente para recolectar de forma segura las **raíces (roots)** del sistema. Las raíces incluyen los stacks de todos los threads activos y las referencias globales (como los objetos mutables gestionados por `mutableRoot` en `ProtoSpace`).
-   **Fases Concurrentes de Mark and Sweep:** Una vez que las raíces han sido recolectadas, las fases de marcado (`mark`) y limpieza (`sweep`) se ejecutan de forma concurrente mientras los threads de la aplicación continúan su ejecución. La inmutabilidad de los datos es clave aquí, ya que garantiza que las referencias entre objetos no cambiarán mientras el GC está trabajando.

### Ciclo de Vida de los Objetos y Limpieza por Ámbito

-   **Objetos de Corta Duración:** La biblioteca implementa una optimización para objetos de corta duración. Cuando un método o función finaliza, todas las celdas de memoria que fueron asignadas dentro de su ámbito y que no son parte del valor de retorno explícito, se devuelven a un "pool de análisis".
-   **Análisis Asíncrono:** Los objetos en este pool son candidatos para ser liberados. El GC analiza de forma asíncrona estas celdas para asegurarse de que no haya ninguna referencia viva a ellas desde las raíces del sistema. Si no hay referencias, la celda se devuelve al pool global de celdas libres, lista para ser reutilizada.
-   **Eficiencia:** Este mecanismo actúa como una forma de recolección generacional, permitiendo que la mayoría de los objetos (que suelen tener una vida corta) se recolecten de manera muy eficiente sin necesidad de un ciclo completo de mark-and-sweep sobre todo el heap.

## Estructuras de Datos Inmutables

El runtime `proto` proporciona implementaciones de estructuras de datos fundamentales que son inherentemente inmutables, lo que contribuye a la seguridad y la concurrencia del sistema.

### Listas (`ProtoList`)

-   Implementadas como árboles balanceados (posiblemente AVL o similar, dado el uso de `leftRotate`, `rightRotate` y `rebalance`).
-   Las operaciones como `appendFirst`, `appendLast`, `insertAt`, `setAt`, `removeAt`, `splitFirst`, `splitLast`, `removeFirst`, `removeLast` y `removeSlice` no modifican la lista original, sino que devuelven una nueva lista con los cambios. Esto garantiza la persistencia de la estructura de datos.
-   Optimizadas para acceso eficiente y modificaciones en entornos concurrentes.

### Tuplas (`ProtoTuple`)

-   Representan colecciones inmutables de elementos, similares a las tuplas en Python.
-   Implementadas como árboles de búsqueda (posiblemente un árbol B o similar, dado el uso de `TupleDictionary` para la gestión de la raíz de tuplas y la deduplicación).
-   Las tuplas son internadas (`tupleRoot` en `ProtoSpace`), lo que significa que las tuplas idénticas comparten la misma instancia en memoria, optimizando el uso de memoria y las comparaciones.
-   Proporcionan acceso eficiente a los elementos y operaciones de slicing.

### Cadenas (`ProtoString`)

-   Las cadenas son inmutables y se construyen sobre `ProtoTuple`, lo que significa que se benefician de las optimizaciones de inmutabilidad y deduplicación de las tuplas.
-   Las operaciones de cadena como concatenación, inserción y eliminación también devuelven nuevas instancias de cadena.

## Modelo de Objetos Basado en Prototipos

El sistema `proto` implementa un modelo de objetos flexible y dinámico basado en prototipos, similar a JavaScript o Self.

-   **Objetos (`ProtoObjectCell`):** Los objetos son representados por `ProtoObjectCellImplementation`, que contienen un enlace a su `parent` (prototipo) y un `ProtoSparseList` de `attributes`.
-   **Herencia Basada en Prototipos:** Los objetos heredan propiedades y métodos de sus objetos `parent`. La búsqueda de atributos (`getAttribute`) recorre la cadena de prototipos hasta encontrar el atributo o llegar al final de la cadena.
-   **Clonación y Creación de Hijos (`clone`, `newChild`):** Los objetos pueden ser clonados (`clone`) para crear nuevas instancias con los mismos atributos, o se pueden crear nuevos objetos que hereden directamente de un prototipo existente (`newChild`).
-   **Atributos Dinámicos:** Los atributos pueden ser añadidos o modificados dinámicamente en los objetos. Las operaciones `setAttribute` y `hasAttribute` gestionan estos atributos.
-   **Llamada a Métodos (`call`):** El mecanismo de llamada a métodos permite invocar funciones asociadas a objetos, resolviendo el método a través de la cadena de prototipos.
-   **Objetos Mutables:** Aunque las estructuras de datos fundamentales son inmutables, el sistema soporta la noción de objetos mutables (`mutable_ref` en `ProtoObjectCellImplementation` y `mutableRoot` en `ProtoSpace`). Esto permite que ciertos objetos se comporten de manera mutable, mientras que el GC gestiona su visibilidad y recolección de forma segura en un entorno concurrente.

