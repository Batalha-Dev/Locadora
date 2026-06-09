#pragma once
#include <string>

struct Cliente {
    int id;
    std::string nome;
    std::string cpf;
    std::string telefone;
    std::string email;
    bool ativo;

    Cliente() : id(0), ativo(true) {}

    Cliente(int id, const std::string& nome, const std::string& cpf,
            const std::string& telefone, const std::string& email)
        : id(id), nome(nome), cpf(cpf), telefone(telefone), email(email), ativo(true) {}

    bool operator==(const Cliente& outro) const { return id == outro.id; }
};
