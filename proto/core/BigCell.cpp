/*
* BigCell.cpp
 *
 *  Created on: 2024
 *      Author: Gemini Code Assist
 *
 *  Este archivo proporciona una definición no-inline para el constructor
 *  y destructor de BigCell. Su propósito es darle al compilador un lugar
 *  único para generar la v-table y la información de RTTI para la clase,
 *  solucionando errores de "undefined reference to typeinfo".
 */
#include "../headers/proto_internal.h"

namespace proto {
    BigCell::BigCell(ProtoContext* context) : Cell(context) {}
    BigCell::~BigCell() = default;
} // namespace proto