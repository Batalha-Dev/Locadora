#pragma once
#include <stdexcept>

template <typename T>
class Fila {
private:
    struct No {
        T dado;
        No* proximo;
        No(const T& d) : dado(d), proximo(nullptr) {}
    };

    No* frente;
    No* fundo;
    int tamanho;

public:
    Fila() : frente(nullptr), fundo(nullptr), tamanho(0) {}

    ~Fila() {
        while (!vazia()) desenfileirar();
    }

    Fila(const Fila&) = delete;
    Fila& operator=(const Fila&) = delete;

    void enfileirar(const T& dado) {
        No* novo = new No(dado);
        if (!fundo) {
            frente = fundo = novo;
        } else {
            fundo->proximo = novo;
            fundo = novo;
        }
        tamanho++;
    }

    T desenfileirar() {
        if (vazia()) throw std::underflow_error("Fila vazia");
        No* temp = frente;
        T dado = temp->dado;
        frente = frente->proximo;
        if (!frente) fundo = nullptr;
        delete temp;
        tamanho--;
        return dado;
    }

    const T& espiar() const {
        if (vazia()) throw std::underflow_error("Fila vazia");
        return frente->dado;
    }

    int getTamanho() const { return tamanho; }
    bool vazia() const { return tamanho == 0; }
};
