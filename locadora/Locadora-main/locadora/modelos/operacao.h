#pragma once
#include <string>
#include <ctime>

enum class TipoOp {
    CADASTRAR_CARRO,
    REMOVER_CARRO,
    CADASTRAR_CLIENTE,
    REMOVER_CLIENTE,
    REGISTRAR_LOCACAO,
    REGISTRAR_DEVOLUCAO,
    CANCELAR_LOCACAO
};

struct Operacao {
    TipoOp tipo;
    int idReferencia;   // id do carro, cliente ou locacao afetado
    std::string descricao;
    time_t timestamp;

    Operacao() : tipo(TipoOp::CADASTRAR_CARRO), idReferencia(0), timestamp(0) {}
    Operacao(TipoOp t, int id, const std::string& desc)
        : tipo(t), idReferencia(id), descricao(desc), timestamp(std::time(nullptr)) {}
};
