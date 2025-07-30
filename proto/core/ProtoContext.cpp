/*
 * ProtoContext.cpp
 *
 *  Created on: 2020-5-1
 *      Author: gamarino
 */

#include "../headers/proto_internal.h"

#include <thread>
#include <cstdlib>   // Para std::malloc
#include <algorithm> // Para std::max

namespace proto
{
    // ADVERTENCIA: Estas variables globales pueden causar problemas en un entorno
    // multihilo y dificultan el razonamiento del estado. Considera encapsularlas
    // dentro de la clase ProtoSpace.
    std::atomic<bool> literalMutex(false);
    BigCell* literalFreeCells = nullptr;
    unsigned literalFreeCellsIndex = 0;
    ProtoContext globalContext;

    // --- Constructor y Destructor ---

    ProtoContext::ProtoContext(
        ProtoContext* previous,
        ProtoObject** localsBase,
        unsigned int localsCount,
        ProtoThread* thread,
        ProtoSpace* space
    ) : previous(previous),
        space(space),
        thread(thread),
        localsBase(localsBase),
        localsCount(localsCount),
        lastAllocatedCell(nullptr),
        allocatedCellsCount(0),
        lastReturnValue(nullptr)
    {
        if (previous)
        {
            // Si hay un contexto anterior, heredar su espacio y hilo.
            this->space = previous->space;
            this->thread = previous->thread;
        }

        if (this->thread)
        {
            // Actualizar el contexto actual del hilo a través de un método público.
            this->thread->setCurrentContext(this);
        }

        if (this->localsBase)
        {
            // CORRECCIÓN CRÍTICA: Bucle corregido para evitar desbordamiento de búfer.
            // El bucle original (i <= localsCount) escribía un elemento de más.
            for (unsigned int i = 0; i < this->localsCount; ++i)
            {
                this->localsBase[i] = nullptr;
            }
        }
    }


    ProtoContext::~ProtoContext()
    {
        if (this->previous && this->space && this->lastAllocatedCell)
        {
            // Al destruir un contexto, se informa al espacio sobre las celdas
            // que se asignaron en él para que el GC pueda analizarlas.
            this->space->analyzeUsedCells(this->lastAllocatedCell);
        }
    }

    // --- Gestión de Celdas y GC ---

    void ProtoContext::setReturnValue(ProtoContext* context, ProtoObject* returnValue)
    {
        if (this->previous)
        {
            // Asegura que el valor de retorno se mantenga como una referencia válida
            // cuando este contexto se destruya, asignándolo al contexto anterior.
            this->previous->lastReturnValue = returnValue;
        }
    }

    void ProtoContext::checkCellsCount()
    {
        if (this->allocatedCellsCount >= this->space->maxAllocatedCellsPerContext)
        {
            this->space->analyzeUsedCells(this->lastAllocatedCell);
            this->lastAllocatedCell = nullptr;
            this->allocatedCellsCount = 0;
            this->space->triggerGC();
        }
    }

    Cell* ProtoContext::allocCell()
    {
        Cell* newCell;
        if (this->thread)
        {
            newCell = ((ProtoThreadImplementation*)(this->thread))->allocCell();
            ::new (newCell) Cell(this);
            newCell = static_cast<Cell*>(newCell);
            this->allocatedCellsCount++;
            this->checkCellsCount();
        }
        else
        {
            // ADVERTENCIA: Esta rama usa malloc directamente, lo que evita el GC.
            // Esto es probablemente un remanente de código antiguo y podría ser una fuente de fugas de memoria.
            // Todas las asignaciones de celdas deberían pasar por el gestor de memoria del espacio.
            void* newChunk = std::malloc(sizeof(BigCell));
            ::new (newChunk) Cell(this);
            newCell = static_cast<Cell*>(newChunk);
        }

        // Las celdas se encadenan en una lista simple para su seguimiento dentro del contexto.
        newCell->nextCell = this->lastAllocatedCell;
        this->lastAllocatedCell = newCell;
        return newCell;
    }

    void ProtoContext::addCell2Context(Cell* newCell)
    {
        newCell->nextCell = this->lastAllocatedCell;
        this->lastAllocatedCell = newCell;
    }

    // --- Constructores de Tipos Primitivos (from...) ---

    ProtoObject* ProtoContext::fromInteger(int value)
    {
        ProtoObjectPointer p;
        p.oid.oid = nullptr;
        p.si.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
        p.si.embedded_type = EMBEDED_TYPE_SMALLINT;
        p.si.smallInteger = value;
        return p.oid.oid;
    }

    ProtoObject* ProtoContext::fromDouble(double value)
    {
        ProtoObjectPointer p;
        p.oid.oid = nullptr;
        p.sd.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
        p.sd.embedded_type = EMBEDED_TYPE_FLOAT;

        union
        {
            unsigned long lv;
            double dv;
        } u;
        u.dv = value;
        p.sd.floatValue = u.lv >> TYPE_SHIFT;
        return p.oid.oid;
    }

    ProtoObject* ProtoContext::fromUTF8Char(const char* utf8OneCharString)
    {
        ProtoObjectPointer p;
        p.oid.oid = nullptr;
        p.unicodeChar.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
        p.unicodeChar.embedded_type = EMBEDED_TYPE_UNICODECHAR;

        unsigned unicodeValue = 0U;
        unsigned char firstByte = utf8OneCharString[0];

        if ((firstByte & 0x80) == 0)
        {
            // 1-byte char (ASCII)
            unicodeValue = firstByte;
        }
        else if ((firstByte & 0xE0) == 0xC0)
        {
            // 2-byte char
            unicodeValue = ((firstByte & 0x1F) << 6) | (utf8OneCharString[1] & 0x3F);
        }
        else if ((firstByte & 0xF0) == 0xE0)
        {
            // 3-byte char
            unicodeValue = ((firstByte & 0x0F) << 12) | ((utf8OneCharString[1] & 0x3F) << 6) | (utf8OneCharString[2] &
                0x3F);
        }
        else if ((firstByte & 0xF8) == 0xF0)
        {
            // 4-byte char
            // CORRECCIÓN CRÍTICA: Se corrigió un error de copiado y pegado.
            // La versión anterior usaba utf8OneCharString[1] dos veces.
            unicodeValue = ((firstByte & 0x07) << 18) | ((utf8OneCharString[1] & 0x3F) << 12) | ((utf8OneCharString[2] &
                0x3F) << 6) | (utf8OneCharString[3] & 0x3F);
        }

        p.unicodeChar.unicodeValue = unicodeValue;
        return p.oid.oid;
    }

    ProtoString* ProtoContext::fromUTF8String(const char* zeroTerminatedUtf8String)
    {
        const char* currentChar = zeroTerminatedUtf8String;
        ProtoList* charList = this->newList();

        while (*currentChar)
        {
            charList = charList->appendLast(this, this->fromUTF8Char(currentChar));

            // Avanzar el puntero según el número de bytes del carácter UTF-8
            if ((*currentChar & 0x80) == 0) currentChar += 1;
            else if ((*currentChar & 0xE0) == 0xC0) currentChar += 2;
            else if ((*currentChar & 0xF0) == 0xE0) currentChar += 3;
            else if ((*currentChar & 0xF8) == 0xF0) currentChar += 4;
            else currentChar += 1; // Carácter inválido, avanzar 1 para evitar bucle infinito
        }

        // La creación de una cadena implica convertir la lista de caracteres en una tupla.
        return new(this) ProtoStringImplementation(
            this,
            ProtoTupleImplementation::tupleFromList(this, charList)
        );
    }

    // --- Constructores de Tipos de Colección (new...) ---
    // AJUSTADO: Se eliminó la sintaxis de plantillas.

    ProtoList* ProtoContext::newList()
    {
        return new(this) ProtoListImplementation(this);
    }

    ProtoTuple* ProtoContext::newTuple()
    {
        return new(this) ProtoTupleImplementation(this, 0, static_cast<ProtoObject**>(nullptr));
    }

    ProtoSparseList* ProtoContext::newSparseList()
    {
        return new(this) ProtoSparseListImplementation(this);
    }

    // --- Otros Constructores (from...) ---

    ProtoObject* ProtoContext::fromBoolean(bool value)
    {
        ProtoObjectPointer p;
        p.oid.oid = nullptr;
        p.booleanValue.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
        p.booleanValue.embedded_type = EMBEDED_TYPE_BOOLEAN;
        p.booleanValue.booleanValue = value;
        return p.oid.oid;
    }

    ProtoObject* ProtoContext::fromByte(char c)
    {
        ProtoObjectPointer p;
        p.oid.oid = nullptr;
        p.byteValue.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
        p.byteValue.embedded_type = EMBEDED_TYPE_BYTE;
        p.byteValue.byteData = c;
        return p.oid.oid;
    }

    ProtoObject* ProtoContext::fromDate(unsigned year, unsigned month, unsigned day)
    {
        ProtoObjectPointer p;
        p.oid.oid = nullptr;
        p.date.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
        p.date.embedded_type = EMBEDED_TYPE_DATE;
        p.date.year = year;
        p.date.month = month;
        p.date.day = day;
        return p.oid.oid;
    }

    ProtoObject* ProtoContext::fromTimestamp(unsigned long timestamp)
    {
        ProtoObjectPointer p;
        p.oid.oid = nullptr;
        p.timestampValue.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
        p.timestampValue.embedded_type = EMBEDED_TYPE_TIMESTAMP;
        p.timestampValue.timestamp = timestamp;
        return p.oid.oid;
    }

    ProtoObject* ProtoContext::fromTimeDelta(long timedelta)
    {
        ProtoObjectPointer p;
        p.oid.oid = nullptr;
        p.timedeltaValue.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
        p.timedeltaValue.embedded_type = EMBEDED_TYPE_TIMEDELTA;
        p.timedeltaValue.timedelta = timedelta;
        return p.oid.oid;
    }

    ProtoMethodCell* ProtoContext::fromMethod(ProtoObject* self, ProtoMethod method)
    {
        return new(this) ProtoMethodCellImplementation(this, method);
    }

    ProtoExternalPointer* ProtoContext::fromExternalPointer(void* pointer)
    {
        return new(this) ProtoExternalPointerImplementation(this, pointer);
    }

    ProtoByteBuffer* ProtoContext::fromBuffer(unsigned long length, char* buffer)
    {
        return new(this) ProtoByteBufferImplementation(this, length, buffer);
    }

    ProtoByteBuffer* ProtoContext::newBuffer(unsigned long length)
    {
        return new(this) ProtoByteBufferImplementation(this, length, NULL);
    }
} // namespace proto
