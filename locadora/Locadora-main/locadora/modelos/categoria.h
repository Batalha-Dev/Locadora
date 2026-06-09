#pragma once
#include <string>

struct Categoria {
    int id;
    std::string nome;
    std::string descricao;
    double diariaPadrao;

    Categoria() : id(0), diariaPadrao(0.0) {}
    Categoria(int id, const std::string& nome, const std::string& desc, double diaria)
        : id(id), nome(nome), descricao(desc), diariaPadrao(diaria) {}
};
