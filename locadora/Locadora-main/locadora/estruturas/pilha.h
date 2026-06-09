#pragma once
#include <stdexcept>

template <typename T>
class Pilha {
private:
    struct No {
        T dado;
        No* abaixo;
        No(const T& d) : dado(d), abaixo(nullptr) {}
    };

    No* topo;
    int tamanho;

public:
    Pilha() : topo(nullptr), tamanho(0) {}

    ~Pilha() {
        while (!vazia()) desempilhar();
    }

    Pilha(const Pilha&) = delete;
    Pilha& operator=(const Pilha&) = delete;

    void empilhar(const T& dado) {
        No* novo = new No(dado);
        novo->abaixo = topo;
        topo = novo;
        tamanho++;
    }

    T desempilhar() {
        if (vazia()) throw std::underflow_error("Pilha vazia");
        No* temp = topo;
        T dado = temp->dado;
        topo = topo->abaixo;
        delete temp;
        tamanho--;
        return dado;
    }

    const T& espiar() const {
        if (vazia()) throw std::underflow_error("Pilha vazia");
        return topo->dado;
    }

    int getTamanho() const { return tamanho; }
    bool vazia() const { return tamanho == 0; }
};
