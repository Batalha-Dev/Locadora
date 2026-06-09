#pragma once
#include <string>

// Catalogo de marcas e modelos selecionaveis no cadastro de veiculos.
// Um Modelo pertence sempre a uma Marca (modelo.marcaId -> marca.id).

struct Marca {
    int id;
    std::string nome;

    Marca() : id(0) {}
    Marca(int id, const std::string& nome) : id(id), nome(nome) {}

    bool operator==(const Marca& outro) const { return id == outro.id; }
};

struct Modelo {
    int id;
    int marcaId;
    std::string nome;

    Modelo() : id(0), marcaId(0) {}
    Modelo(int id, int marcaId, const std::string& nome)
        : id(id), marcaId(marcaId), nome(nome) {}

    bool operator==(const Modelo& outro) const { return id == outro.id; }
};
