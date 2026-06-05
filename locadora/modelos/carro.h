#pragma once
#include <string>

enum class StatusCarro {
    DISPONIVEL,
    LOCADO,
    MANUTENCAO,
    RESERVADO
};

struct Carro {
    int id;
    std::string modelo;
    std::string marca;
    std::string placa;
    int ano;
    int categoriaId;   // índice na lista estática de categorias
    double diaria;
    StatusCarro status;

    Carro() : id(0), ano(0), categoriaId(0), diaria(0.0), status(StatusCarro::DISPONIVEL) {}

    Carro(int id, const std::string& marca, const std::string& modelo,
          const std::string& placa, int ano, int catId, double diaria)
        : id(id), modelo(modelo), marca(marca), placa(placa),
          ano(ano), categoriaId(catId), diaria(diaria), status(StatusCarro::DISPONIVEL) {}

    bool operator==(const Carro& outro) const { return id == outro.id; }

    std::string statusStr() const {
        switch (status) {
            case StatusCarro::DISPONIVEL:  return "Disponivel";
            case StatusCarro::LOCADO:      return "Locado";
            case StatusCarro::MANUTENCAO:  return "Manutencao";
            case StatusCarro::RESERVADO:   return "Reservado";
        }
        return "?";
    }
};
