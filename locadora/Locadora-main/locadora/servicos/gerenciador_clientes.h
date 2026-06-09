#pragma once
#include "../estruturas/lista_encadeada.h"
#include "../estruturas/hash_table.h"
#include "../estruturas/gerador_id.h"
#include "../modelos/cliente.h"

class GerenciadorClientes {
private:
    ListaEncadeada<Cliente>     clientes;
    HashTable<std::string, int> hashPorCpf; // cpf → id
    HashTable<int, int>         hashPorId;  // id  → id
    GeradorId                   geradorId;  // ids automaticos via pilha

public:
    GerenciadorClientes() {}

    Cliente* cadastrar(const std::string& nome, const std::string& cpf,
                       const std::string& telefone, const std::string& email) {
        if (hashPorCpf.buscar(cpf)) return nullptr; // CPF já existe

        int id = geradorId.obter();
        clientes.inserirFim({id, nome, cpf, telefone, email});

        hashPorCpf.inserir(cpf, id);
        hashPorId.inserir(id, id);

        return clientes.buscar([id](Cliente& c){ return c.id == id; });
    }

    void inserirExistente(const Cliente& c) {
        clientes.inserirFim(c);
        hashPorCpf.inserir(c.cpf, c.id);
        hashPorId.inserir(c.id, c.id);
        geradorId.reservar(c.id);
    }

    bool remover(int id, bool reutilizarId = true) {
        Cliente* c = buscarPorId(id);
        if (!c) return false;
        hashPorCpf.remover(c->cpf);
        hashPorId.remover(id);
        bool ok = clientes.removerSe([id](const Cliente& x){ return x.id == id; });
        if (ok && reutilizarId) geradorId.liberar(id);  // id volta para a pilha
        return ok;
    }

    Cliente* buscarPorId(int id) {
        if (!hashPorId.buscar(id)) return nullptr;
        return clientes.buscar([id](Cliente& c){ return c.id == id; });
    }

    Cliente* buscarPorCpf(const std::string& cpf) {
        int* id = hashPorCpf.buscar(cpf);
        if (!id) return nullptr;
        return clientes.buscar([id](Cliente& c){ return c.id == *id; });
    }

    ListaEncadeada<Cliente>& getClientes() { return clientes; }

    int total() const { return clientes.getTamanho(); }
};
