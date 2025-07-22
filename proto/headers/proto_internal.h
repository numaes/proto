/* 
 * proto_internal.h
 *
 *  Created on: November, 2017 - Redesign January, 2024
 *      Author: Gustavo Adrian Marino <gamarino@numaes.com>
 */


#ifndef PROTO_INTERNAL_H
#define PROTO_INTERNAL_H

#include "../headers/proto.h"
#include <thread>

namespace proto
{
    // Forward declarations for implementation classes
    class Cell;
    class BigCell;
    class ProtoObjectCellImplementation;
    class ParentLinkImplementation;
    class ProtoListImplementation;
    class ProtoSparseListImplementation;
    class ProtoTupleImplementation;
    class ProtoStringImplementation;
    class ProtoMethodCellImplementation;
    class ProtoExternalPointerImplementation;
    class ProtoByteBufferImplementation;
    class ProtoThreadImplementation;

    // Union for pointer tagging
    union ProtoObjectPointer
    {
        struct
        {
            ProtoObject* oid;
        } oid;

        struct
        {
            unsigned long pointer_tag : 4;
            unsigned long value : 60;
        } op;

        struct
        {
            unsigned long pointer_tag : 4;
            unsigned long embedded_type : 4;
            long smallInteger : 56;
        } si;

        struct
        {
            unsigned long pointer_tag : 4;
            unsigned long embedded_type : 4;
            unsigned long smallDouble : 56;
        } sd;

        struct
        {
            unsigned long pointer_tag : 4;
            unsigned long embedded_type : 4;
            unsigned long unicodeValue : 56;
        } unicodeChar;

        struct
        {
            unsigned long pointer_tag : 4;
            unsigned long embedded_type : 4;
            unsigned long booleanValue : 1;
            unsigned long padding : 55;
        } booleanValue;

        struct
        {
            unsigned long pointer_tag : 4;
            unsigned long embedded_type : 4;
            unsigned char byteData;
        } byteValue;

        struct
        {
            unsigned long pointer_tag : 4;
            unsigned long embedded_type : 4;
            unsigned long year : 16;
            unsigned long month : 8;
            unsigned long day : 8;
            unsigned long padding : 24;
        } date;

        struct
        {
            unsigned long pointer_tag : 4;
            unsigned long embedded_type : 4;
            unsigned long timestamp : 56;
        } timestampValue;

        struct
        {
            unsigned long pointer_tag : 4;
            unsigned long embedded_type : 4;
            long timedelta : 56;
        } timedeltaValue;

        struct
        {
            unsigned long pointer_tag : 4;
            unsigned long hash : 60;
        } asHash;

        struct
        {
            unsigned long pointer_tag : 4;
            Cell* cell;
        } cell;
    };

    // Pointer tags
#define POINTER_TAG_EMBEDEDVALUE            0
#define POINTER_TAG_OBJECT                  1
#define POINTER_TAG_LIST                    2
#define POINTER_TAG_LIST_ITERATOR           3
#define POINTER_TAG_TUPLE                   4
#define POINTER_TAG_TUPLE_ITERATOR          5
#define POINTER_TAG_STRING                  6
#define POINTER_TAG_STRING_ITERATOR         7
#define POINTER_TAG_SPARSE_LIST             8
#define POINTER_TAG_SPARSE_LIST_ITERATOR    9
#define POINTER_TAG_BYTE_BUFFER             10
#define POINTER_TAG_EXTERNAL_POINTER        11
#define POINTER_TAG_METHOD                  12
#define POINTER_TAG_THREAD                  13

    // Embedded types
#define EMBEDED_TYPE_SMALLINT               0
#define EMBEDED_TYPE_SMALLDOUBLE            1
#define EMBEDED_TYPE_UNICODECHAR            2
#define EMBEDED_TYPE_BOOLEAN                3
#define EMBEDED_TYPE_BYTE                   4
#define EMBEDED_TYPE_DATE                   5
#define EMBEDED_TYPE_TIMESTAMP              6
#define EMBEDED_TYPE_TIMEDELTA              7

    // Iterator states
#define ITERATOR_NEXT_PREVIOUS              0
#define ITERATOR_NEXT_THIS                  1
#define ITERATOR_NEXT_NEXT                  2

    // Space states
#define SPACE_STATE_RUNNING                 0
#define SPACE_STATE_STOPPING_WORLD          1
#define SPACE_STATE_WORLD_TO_STOP           2
#define SPACE_STATE_WORLD_STOPPED           3
#define SPACE_STATE_ENDING                  4

    // Thread states
#define THREAD_STATE_UNMANAGED              0
#define THREAD_STATE_MANAGED                1
#define THREAD_STATE_STOPPING               2
#define THREAD_STATE_STOPPED                3
#define THREAD_STATE_ENDED                  4

#define TYPE_SHIFT                          4

    class Cell
    {
    public:
        static void* operator new(unsigned long size, ProtoContext* context);

        Cell(ProtoContext* context);
        virtual ~Cell();

        virtual void finalize(ProtoContext* context) = 0;
        virtual void processReferences(
            ProtoContext* context,
            void* self,
            void (*method)(
                ProtoContext* context,
                void* self,
                Cell* cell
            )
        ) = 0;

        virtual unsigned long getHash(ProtoContext* context) = 0;
        virtual ProtoObject* asObject(ProtoContext* context) = 0;

        Cell* nextCell;
    };

    class BigCell : public Cell
    {
    public:
        BigCell(ProtoContext* context);
        virtual ~BigCell();

        char padding[56];
    };

    class ParentLinkImplementation : public Cell
    {
    public:
        explicit ParentLinkImplementation(
            ProtoContext* context,
            ParentLinkImplementation* parent,
            ProtoObjectCellImplementation* object
        );
        ~ParentLinkImplementation() override;

        ProtoObject* asObject(ProtoContext* context) override;
        unsigned long getHash(ProtoContext* context) override;

        void finalize(ProtoContext* context) override;

        void processReferences(
            ProtoContext* context,
            void* self,
            void (*method)(
                ProtoContext* context,
                void* self,
                Cell* cell
            )
        ) override;

        ParentLinkImplementation* parent;
        ProtoObjectCellImplementation* object;
    };

    /**
     * @class ProtoObjectCellImplementation
     * @brief Implementación de una celda que representa un objeto Proto.
     *
     * Hereda de 'Cell' para la gestión de memoria y de 'ProtoObjectCell'
     * para la interfaz pública de objetos. Contiene una referencia a su cadena
     * de herencia (padres) y una lista de atributos.
     */
    class ProtoObjectCellImplementation : public Cell, public ProtoObjectCell
    {
    public:
        /**
         * @brief Constructor.
         * @param context El contexto de ejecución actual.
         * @param parent Puntero al primer eslabón de la cadena de herencia.
         * @param mutable_ref Un indicador de si el objeto es mutable.
         * @param attributes La lista dispersa de atributos del objeto.
         */
        ProtoObjectCellImplementation(
            ProtoContext* context,
            ParentLinkImplementation* parent,
            unsigned long mutable_ref,
            ProtoSparseListImplementation* attributes
        );

        /**
         * @brief Destructor virtual.
         */
        ~ProtoObjectCellImplementation() override;

        /**
         * @brief Crea una nueva celda de objeto con un padre adicional en su cadena de herencia.
         * @param context El contexto de ejecución actual.
         * @param newParentToAdd El objeto padre que se va a añadir.
         * @return Una *nueva* ProtoObjectCellImplementation con la cadena de herencia actualizada.
         */
        ProtoObjectCellImplementation* addParent(
            ProtoContext* context,
            ProtoObjectCellImplementation* newParentToAdd
        );

        /**
         * @brief Devuelve la representación de esta celda como un ProtoObject.
         * @param context El contexto de ejecución actual.
         * @return Un ProtoObject que representa este objeto.
         */
        ProtoObject* asObject(ProtoContext* context) override;

        /**
         * @brief Finalizador para el recolector de basura.
         *
         * Este método es llamado por el GC antes de liberar la memoria de la celda.
         * @param context El contexto de ejecución actual.
         */
        void finalize(ProtoContext* context) override;

        /**
         * @brief Procesa las referencias internas para el recolector de basura.
         *
         * Recorre las referencias al padre y a los atributos para que el GC
         * pueda marcar los objetos alcanzables.
         */
        void processReferences(
            ProtoContext* context,
            void* self,
            void (*method)(ProtoContext* context, void* self, Cell* cell)
        ) override;
        long unsigned getHash(ProtoContext* context) override;

    private:
        ParentLinkImplementation* parent;
        unsigned long mutable_ref;
        ProtoSparseListImplementation* attributes;
    };

    // --- ProtoListIterator ---
    // Concrete implementation for ProtoObject*
    class ProtoListIteratorImplementation : public Cell, public ProtoListIterator
    {
    public:
        ProtoListIteratorImplementation(
            ProtoContext* context,
            ProtoListImplementation* base,
            unsigned long currentIndex
        );
        ~ProtoListIteratorImplementation() override;

        virtual int hasNext(ProtoContext* context);
        virtual ProtoObject* next(ProtoContext* context);
        virtual ProtoListIterator* advance(ProtoContext* context);

        virtual ProtoObject* asObject(ProtoContext* context);
        virtual unsigned long getHash(ProtoContext* context);

        virtual void finalize(ProtoContext* context);

        virtual void processReferences(
            ProtoContext* context,
            void* self,
            void (*method)(
                ProtoContext* context,
                void* self,
                Cell* cell
            )
        );

    private:
        ProtoListImplementation* base;
        unsigned long currentIndex;
    };

    // --- ProtoList ---
    // Concrete implementation for ProtoObject*
    class ProtoListImplementation : public Cell, public ProtoList
    {
    public:
        ProtoListImplementation(
            ProtoContext* context,
            ProtoObject* value = PROTO_NONE,
            ProtoListImplementation* previous = NULL,
            ProtoListImplementation* next = NULL
        );
        ~ProtoListImplementation() override;

        virtual ProtoObject* getAt(ProtoContext* context, int index);
        virtual ProtoObject* getFirst(ProtoContext* context);
        virtual ProtoObject* getLast(ProtoContext* context);
        virtual ProtoList* getSlice(ProtoContext* context, int from, int to);
        virtual unsigned long getSize(ProtoContext* context);

        virtual bool has(ProtoContext* context, ProtoObject* value);
        virtual ProtoList* setAt(ProtoContext* context, int index, ProtoObject* value = PROTO_NONE);
        virtual ProtoList* insertAt(ProtoContext* context, int index, ProtoObject* value);

        virtual ProtoList* appendFirst(ProtoContext* context, ProtoObject* value);
        virtual ProtoList* appendLast(ProtoContext* context, ProtoObject* value);

        virtual ProtoList* extend(ProtoContext* context, ProtoList* other);

        virtual ProtoList* splitFirst(ProtoContext* context, int index);
        virtual ProtoList* splitLast(ProtoContext* context, int index);

        virtual ProtoList* removeFirst(ProtoContext* context);
        virtual ProtoList* removeLast(ProtoContext* context);
        virtual ProtoList* removeAt(ProtoContext* context, int index);
        virtual ProtoList* removeSlice(ProtoContext* context, int from, int to);

        virtual ProtoTuple* asTuple(ProtoContext* context);
        virtual ProtoObject* asObject(ProtoContext* context);
        virtual unsigned long getHash(ProtoContext* context);
        virtual ProtoListIterator* getIterator(ProtoContext* context);

        virtual void finalize(ProtoContext* context);
        virtual void processReferences(
            ProtoContext* context,
            void* self,
            void (*method)(
                ProtoContext* context,
                void* self,
                Cell* cell
            )
        );

        ProtoListImplementation* previous;
        ProtoListImplementation* next;

        ProtoObject* value;
        unsigned long hash;

        unsigned long count : 52;
        unsigned long height : 8;
        unsigned long type : 4;
    };

    // --- ProtoSparseList ---
    // Concrete implementation for ProtoObject*
    class ProtoSparseListIteratorImplementation : public Cell, public ProtoSparseListIterator
    {
    public:
        ProtoSparseListIteratorImplementation(
            ProtoContext* context,
            int state,
            ProtoSparseListImplementation* current,
            ProtoSparseListIteratorImplementation* queue = NULL
        );
        ~ProtoSparseListIteratorImplementation() override;

        virtual int hasNext(ProtoContext* context);
        virtual unsigned long nextKey(ProtoContext* context);
        virtual ProtoObject* nextValue(ProtoContext* context);

        virtual ProtoSparseListIterator* advance(ProtoContext* context);
        virtual ProtoObject* asObject(ProtoContext* context);
        virtual unsigned long getHash(ProtoContext* context);

        virtual void finalize(ProtoContext* context);
        virtual void processReferences(
            ProtoContext* context,
            void* self,
            void (*method)(
                ProtoContext* context,
                void* self,
                Cell* cell
            )
        );

    private:
        int state;
        ProtoSparseListImplementation* current;
        ProtoSparseListIteratorImplementation* queue;
    };

    class ProtoSparseListImplementation : public Cell, public ProtoSparseList
    {
    public:
        ProtoSparseListImplementation(
            ProtoContext* context,
            unsigned long index = 0,
            ProtoObject* value = PROTO_NONE,
            ProtoSparseListImplementation* previous = NULL,
            ProtoSparseListImplementation* next = NULL
        );
        ~ProtoSparseListImplementation() override;

        virtual bool has(ProtoContext* context, unsigned long index);
        virtual ProtoObject* getAt(ProtoContext* context, unsigned long index);
        virtual ProtoSparseList* setAt(ProtoContext* context, unsigned long index, ProtoObject* value);
        virtual ProtoSparseList* removeAt(ProtoContext* context, unsigned long index);
        virtual int isEqual(ProtoContext* context, ProtoSparseList* otherDict);
        virtual ProtoObject* getAtOffset(ProtoContext* context, int offset);

        virtual unsigned long getSize(ProtoContext* context);
        virtual ProtoObject* asObject(ProtoContext* context);
        virtual unsigned long getHash(ProtoContext* context);
        virtual ProtoSparseListIterator* getIterator(ProtoContext* context);

        virtual void processElements(
            ProtoContext* context,
            void* self,
            void (*method)(
                ProtoContext* context,
                void* self,
                unsigned long index,
                ProtoObject* value
            )
        );

        virtual void processValues(
            ProtoContext* context,
            void* self,
            void (*method)(
                ProtoContext* context,
                void* self,
                ProtoObject* value
            )
        );

        virtual void finalize(ProtoContext* context);
        virtual void processReferences(
            ProtoContext* context,
            void* self,
            void (*method)(
                ProtoContext* context,
                void* self,
                Cell* cell
            )
        );

        ProtoSparseListImplementation* previous;
        ProtoSparseListImplementation* next;

        unsigned long index;
        ProtoObject* value;
        unsigned long hash;

        unsigned long count : 52;
        unsigned long height : 8;
        unsigned long type : 4;
    };

    // --- ProtoTuple ---
    // Implementación de tuplas, potencialmente usando una estructura de "rope" para eficiencia.
    class ProtoTupleImplementation : public Cell, public ProtoTuple
    {
    public:
        // Constructor
        ProtoTupleImplementation(
            ProtoContext* context,
            unsigned long elementCount,
            ProtoObject** data
        );

        ProtoTupleImplementation(
            ProtoContext* context,
            unsigned long elementCount,
            ProtoTupleImplementation** indirect
        );

        // Destructor
        ~ProtoTupleImplementation() override;

        // --- Métodos de la interfaz ProtoTuple ---
        ProtoObject* getAt(ProtoContext* context, int index) override;
        unsigned long getSize(ProtoContext* context) override { return elementCount; }
        ProtoList* asList(ProtoContext* context) override;
        static ProtoTuple* tupleFromList(ProtoContext* context, ProtoList* list);
        ProtoTupleIterator* getIterator(ProtoContext* context) override;
        ProtoTuple* setAt(ProtoContext* context, int index, ProtoObject* value);

        // --- Métodos de la interfaz Cell ---
        ProtoObject* asObject(ProtoContext* context) override;
        unsigned long getHash(ProtoContext* context) override;
        void finalize(ProtoContext* context) override;
        void processReferences(
            ProtoContext* context,
            void* self,
            void (*method)(ProtoContext*, void*, Cell*)
        ) override;

    private:
        unsigned long elementCount{};
        unsigned long height{}; // 0 para un nodo hoja, >0 para un nodo interno

        union
        {
            ProtoObject** data{}; // Para nodos hoja
            ProtoTupleImplementation** indirect; // Para nodos internos
        };
    };

    // --- Iterador de Tuplas ---
    // Implementación concreta para ProtoTupleIterator
    class ProtoTupleIteratorImplementation : public Cell, public ProtoTupleIterator
    {
    public:
        // Constructor
        ProtoTupleIteratorImplementation(
            ProtoContext* context,
            ProtoTupleImplementation* base,
            unsigned long currentIndex
        );

        // Destructor
        ~ProtoTupleIteratorImplementation() override;

        // --- Métodos de la interfaz ProtoTupleIterator ---
        int hasNext(ProtoContext* context) override;
        ProtoObject* next(ProtoContext* context) override;
        ProtoTupleIterator* advance(ProtoContext* context) override;

        // --- Métodos de la interfaz Cell ---
        ProtoObject* asObject(ProtoContext* context) override;
        unsigned long getHash(ProtoContext* context) override;
        void finalize(ProtoContext* context) override;
        void processReferences(
            ProtoContext* context,
            void* self,
            void (*method)(ProtoContext*, void*, Cell*)
        ) override;

    private:
        ProtoTupleImplementation* base; // La tupla que se está iterando
        unsigned long currentIndex; // La posición actual en la tupla
    };

    // --- ProtoString ---
    // Implementación de string inmutable, basada en una tupla de caracteres.
    class ProtoStringImplementation : public Cell, public ProtoString
    {
    public:
        // Constructor
        ProtoStringImplementation(
            ProtoContext* context,
            ProtoTuple* baseTuple
        );

        // Destructor
        ~ProtoStringImplementation() override;

        // --- Métodos de la interfaz ProtoString ---
        ProtoObject* getAt(ProtoContext* context, int index) override;
        unsigned long getSize(ProtoContext* context);
        ProtoString* getSlice(ProtoContext* context, int from, int to) override;
        ProtoString* setAt(ProtoContext* context, int index, ProtoObject* value) override;
        ProtoString* insertAt(ProtoContext* context, int index, ProtoObject* value) override;
        ProtoString* appendLast(ProtoContext* context, ProtoString* otherString) override;
        ProtoString* appendFirst(ProtoContext* context, ProtoString* otherString) override;
        ProtoString* removeSlice(ProtoContext* context, int from, int to) override;
        ProtoList* asList(ProtoContext* context) override;
        ProtoStringIterator* getIterator(ProtoContext* context) override;

        // --- Métodos de la interfaz Cell ---
        ProtoObject* asObject(ProtoContext* context) override;
        unsigned long getHash(ProtoContext* context) override;
        void finalize(ProtoContext* context) override;
        void processReferences(
            ProtoContext* context,
            void* self,
            void (*method)(ProtoContext*, void*, Cell*)
        ) override;

    private:
        ProtoTuple* baseTuple; // La tupla subyacente que almacena los caracteres.
    };


    // --- ProtoStringIterator ---
    // Implementación concreta para el iterador de ProtoString.
    class ProtoStringIteratorImplementation : public Cell, public ProtoStringIterator
    {
    public:
        // Constructor
        ProtoStringIteratorImplementation(
            ProtoContext* context,
            ProtoStringImplementation* base,
            unsigned long currentIndex
        );

        // Destructor
        ~ProtoStringIteratorImplementation() override;

        // --- Métodos de la interfaz ProtoStringIterator ---
        int hasNext(ProtoContext* context) override;
        ProtoObject* next(ProtoContext* context) override;
        ProtoStringIterator* advance(ProtoContext* context) override;

        // --- Métodos de la interfaz Cell ---
        ProtoObject* asObject(ProtoContext* context) override;
        unsigned long getHash(ProtoContext* context) override; // Heredado de Cell, importante para la consistencia.
        void finalize(ProtoContext* context) override;
        void processReferences(
            ProtoContext* context,
            void* self,
            void (*method)(ProtoContext*, void*, Cell*)
        ) override;

    private:
        ProtoStringImplementation* base; // La string que se está iterando.
        unsigned long currentIndex; // La posición actual en la string.
    };

    // --- ProtoByteBufferImplementation ---
    // Implementación de un búfer de bytes que puede gestionar su propia memoria
    // o envolver un búfer existente.
    class ProtoByteBufferImplementation : public Cell, public ProtoByteBuffer
    {
    public:
        // Constructor: crea o envuelve un búfer de memoria.
        // Si 'buffer' es nulo, se asignará nueva memoria.
        ProtoByteBufferImplementation(
            ProtoContext* context,
            unsigned long size,
            char* buffer = nullptr
        );

        // Destructor: libera la memoria si la clase es propietaria.
        ~ProtoByteBufferImplementation() override;

        // --- Métodos de la interfaz ProtoByteBuffer ---
        char getAt(ProtoContext* context, int index) override;
        void setAt(ProtoContext* context, int index, char value) override;

        // --- Métodos de la interfaz Cell ---
        void processReferences(
            ProtoContext* context,
            void* self,
            void (*method)(ProtoContext*, void*, Cell*)
        ) override;
        void finalize(ProtoContext* context) override;
        ProtoObject* asObject(ProtoContext* context) override;
        unsigned long getHash(ProtoContext* context);

    private:
        // Función auxiliar para validar y normalizar índices.
        bool normalizeIndex(int& index);

        unsigned long size; // El tamaño del búfer en bytes.
        char* buffer; // Puntero a la memoria del búfer.
        bool freeOnExit; // Flag que indica si el destructor debe liberar `buffer`.
    };

    // --- ProtoMethodCellImplementation ---
    // Implementación de un puntero a un método c
    class ProtoMethodCellImplementation : public Cell, public ProtoMethodCell
    {
    public:
        ProtoMethodCellImplementation(ProtoContext* context, ProtoMethod method);

        ProtoObject* invoke(ProtoContext* context, ProtoList* args, ProtoSparseList* kwargs);
        ProtoObject* asObject(ProtoContext* context) override;
        unsigned long getHash(ProtoContext* context) override;
        void finalize(ProtoContext* context) override;
        void processReferences(ProtoContext* context, void* self,
                               void (*method)(ProtoContext* context, void* self, Cell* cell));

    private:
        ProtoMethod method{};
    };

    /**
 * @class ProtoExternalPointerImplementation
 * @brief Implementación de una celda que contiene un puntero opaco a datos externos.
 *
 * Esta clase encapsula un puntero `void*` que no es gestionado por el recolector
 * de basura de Proto. Es útil para integrar Proto con bibliotecas o datos C/C++
 * externos, permitiendo que estos punteros se pasen como objetos de primera clase.
 */
    class ProtoExternalPointerImplementation : public Cell, public ProtoExternalPointer
    {
    public:
        /**
         * @brief Constructor.
         * @param context El contexto de ejecución actual.
         * @param pointer El puntero externo (void*) que se va a encapsular.
         */
        ProtoExternalPointerImplementation(ProtoContext* context, void* pointer);

        /**
         * @brief Destructor.
         */
        ~ProtoExternalPointerImplementation() override;

        /**
         * @brief Obtiene el puntero externo encapsulado.
         * @param context El contexto de ejecución actual.
         * @return El puntero (void*) almacenado.
         */
        void* getPointer(ProtoContext* context) override;

        /**
         * @brief Devuelve la representación de esta celda como un ProtoObject.
         * @param context El contexto de ejecución actual.
         * @return Un ProtoObject que representa este puntero externo.
         */
        ProtoObject* asObject(ProtoContext* context) override;

        /**
         * @brief Finalizador para el recolector de basura.
         *
         * No realiza ninguna acción, ya que el puntero externo no es gestionado por el GC.
         * @param context El contexto de ejecución actual.
         */
        void finalize(ProtoContext* context) override;
        unsigned long getHash(ProtoContext* context);

        /**
         * @brief Procesa las referencias para el recolector de basura.
         *
         * El cuerpo está vacío porque el puntero externo no es una referencia
         * que el recolector de basura deba seguir.
         */
        void processReferences(
            ProtoContext* context,
            void* self,
            void (*method)(ProtoContext* context, void* self, Cell* cell)
        ) override;

    private:
        void* pointer; // El puntero opaco a los datos externos.
    };

    // --- Declaración de la Clase ProtoThreadImplementation ---

    class ProtoThreadImplementation : public Cell
    {
    public:
        // --- Miembros de Datos ---
        ProtoString* name;
        ProtoSpace* space;
        std::thread* osThread;
        BigCell* freeCells;
        ProtoContext* currentContext;
        int state;
        int unmanagedCount;

        // --- Constructor y Destructor ---
        ProtoThreadImplementation(
            ProtoContext* context,
            ProtoString* name,
            ProtoSpace* space,
            ProtoMethod code,
            ProtoList* args,
            ProtoSparseList* kwargs
        );

        ~ProtoThreadImplementation();

        // --- Métodos de la Interfaz Pública ---
        void setCurrentContext(ProtoContext* context);
        void setUnmanaged();
        void setManaged();
        void detach(ProtoContext* context);
        void join(ProtoContext* context);
        void exit(ProtoContext* context);

        // --- Sincronización y Gestión de Memoria ---
        void synchToGC();
        Cell* allocCell();

        // --- Métodos Virtuales Sobrescritos de la Clase Base 'Cell' ---
        void finalize(ProtoContext* context) override;
        void processReferences(
            ProtoContext* context,
            void* self,
            void (*method)(ProtoContext* context, void* self, Cell* cell)
        ) override;
        ProtoObject* asObject(ProtoContext* context) override;
    };
}

#endif /* PROTO_INTERNAL_H */