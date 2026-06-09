#pragma once
#include <string>
#include <ctime>

enum class StatusLocacao {
    ATIVA,
    CONCLUIDA,
    CANCELADA
};

struct Locacao {
    int id;
    int idCliente;
    int idCarro;
    time_t dataInicio;
    time_t dataFimPrevista;
    time_t dataFimReal;
    double valorTotal;
    StatusLocacao status;

    Locacao() : id(0), idCliente(0), idCarro(0),
                dataInicio(0), dataFimPrevista(0), dataFimReal(0),
                valorTotal(0.0), status(StatusLocacao::ATIVA) {}

    Locacao(int id, int idCliente, int idCarro, time_t inicio, int dias, double diaria)
        : id(id), idCliente(idCliente), idCarro(idCarro),
          dataInicio(inicio), dataFimReal(0), status(StatusLocacao::ATIVA) {
        dataFimPrevista = inicio + dias * 86400;
        valorTotal = dias * diaria;
    }

    bool operator==(const Locacao& outro) const { return id == outro.id; }

    // Dias inteiros decorridos entre a retirada e 'fim' (apenas informativo).
    int diasDecorridos(time_t fim) const {
        if (fim <= dataInicio) return 0;
        return (int)((fim - dataInicio) / 86400);
    }

    // Dias efetivamente cobrados: no minimo 1 diaria e qualquer fracao de dia
    // conta como dia cheio (teto). Centraliza a regra de cobranca para que a
    // tela de devolucao e a persistencia usem SEMPRE o mesmo valor.
    int diasCobrados(time_t fim) const {
        if (fim <= dataInicio) return 1;
        long long seg = (long long)(fim - dataInicio);
        int dias = (int)(seg / 86400);
        if (seg % 86400 != 0) dias++;          // teto: dia iniciado = dia cheio
        return dias < 1 ? 1 : dias;
    }

    // Valor a cobrar por uma devolucao em 'fim', dada a diaria do veiculo.
    double valorCobrado(double diaria, time_t fim) const {
        return diasCobrados(fim) * diaria;
    }

    std::string statusStr() const {
        switch (status) {
            case StatusLocacao::ATIVA:      return "Ativa";
            case StatusLocacao::CONCLUIDA:  return "Concluida";
            case StatusLocacao::CANCELADA:  return "Cancelada";
        }
        return "?";
    }
};
