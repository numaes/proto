#include "../headers/proto_internal.h"

namespace proto
{
    // --- ProtoMethodCellImplementation ---

    /**
     * @brief Constructor
     * @param context El contexto de ejecución actual.
     * @param method Método a utilizar
     * @param kwargs Una lista dispersa de argumentos por palabra clave.
     */

    ProtoMethodCellImplementation::ProtoMethodCellImplementation(ProtoContext* context, ProtoMethod method): Cell(
        context)
    {
        this->method = method;
    };

    /**
     * @brief Invoca el método C nativo encapsulado por esta celda.
     *
     * Esta función actúa como un puente entre el sistema de objetos de Proto
     * y el código C nativo. Delega la llamada al puntero de función 'method'
     * que se almacenó durante la construcción.
     *
     * @param context El contexto de ejecución actual.
     * @param args Una lista de argumentos posicionales para la función.
     * @param kwargs Una lista dispersa de argumentos por palabra clave.
     * @return El ProtoObject resultante de la ejecución del método nativo.
     */
    ProtoObject* ProtoMethodCellImplementation::invoke(
        ProtoContext* context,
        ProtoList* args,
        ProtoSparseList* kwargs
    )
    {
        // La lógica principal es simplemente llamar al puntero de función almacenado.
        // Se asume que this->method no es nulo, lo cual debería garantizarse en el constructor.
        return this->method(context, this->asObject(context), static_cast<ParentLink*>(nullptr), args, kwargs);
    }

    /**
     * @brief Devuelve la representación de esta celda como un ProtoObject genérico.
     *
     * Este método es esencial para que la celda se pueda manejar como un objeto
     * de primera clase dentro del sistema Proto. Asigna el puntero 'this' y
     * establece el tag de puntero correcto para identificarlo como un método.
     *
     * @param context El contexto de ejecución actual.
     * @return Un ProtoObject que representa esta celda de método.
     */
    ProtoObject* ProtoMethodCellImplementation::asObject(ProtoContext* context)
    {
        ProtoObjectPointer p;
        p.oid.oid = (ProtoObject*)this;
        p.op.pointer_tag = POINTER_TAG_METHOD; // Tag para identificar celdas de método
        return p.oid.oid;
    }

    /**
     * @brief Obtiene el código hash para esta celda.
     *
     * Reutiliza la implementación de la clase base 'Cell', que genera un hash
     * basado en la dirección de memoria del objeto. Esto es eficiente y garantiza
     * la unicidad del hash para cada instancia de celda.
     *
     * @param context El contexto de ejecución actual.
     * @return El valor del hash como un unsigned long.
     */
    unsigned long ProtoMethodCellImplementation::getHash(ProtoContext* context)
    {
        return Cell::getHash(context);
    }

    /**
     * @brief Finalizador para la celda de método.
     *
     * Este método se llama antes de que el recolector de basura reclame la memoria
     * del objeto. En este caso, no se necesita ninguna limpieza especial, ya que
     * el puntero a la función no es un recurso que deba liberarse manualmente.
     *
     * @param context El contexto de ejecución actual.
     */
    void ProtoMethodCellImplementation::finalize(ProtoContext* context)
    {
        // No se requiere ninguna acción de finalización.
    }

    /**
     * @brief Procesa las referencias para el recolector de basura.
     *
     * Esta implementación está vacía porque una ProtoMethodCellImplementation
     * contiene un puntero a una función C, no referencias a otras 'Cells' que
     * el recolector de basura necesite rastrear. Es crucial que este método
     * no procese 'this' para evitar bucles infinitos en el GC.
     *
     * @param context El contexto de ejecución actual.
     * @param self Puntero al objeto que se está procesando.
     * @param method Función de callback del GC para marcar referencias.
     */
    void ProtoMethodCellImplementation::processReferences(
        ProtoContext* context,
        void* self,
        void (*method)(ProtoContext* context, void* self, Cell* cell)
    )
    {
        // Esta celda no contiene referencias a otras celdas, por lo que el cuerpo está vacío.
    };

    ProtoObject* ProtoMethodCell::getSelf(ProtoContext* context) {
        return nullptr;
    }

    ProtoMethod ProtoMethodCell::getMethod(ProtoContext* context) {
        return nullptr;
    }
} // namespace proto
