#pragma once
#include <stdexcept>

template <typename T, int CAPACIDADE>
class ListaEstatica {
private:
    T dados[CAPACIDADE];
    int tamanho;

public:
    ListaEstatica() : tamanho(0) {}

    void inserir(const T& dado) {
        if (tamanho >= CAPACIDADE)
            throw std::overflow_error("Lista estatica cheia");
        dados[tamanho++] = dado;
    }

    bool remover(int indice) {
        if (indice < 0 || indice >= tamanho) return false;
        for (int i = indice; i < tamanho - 1; i++)
            dados[i] = dados[i + 1];
        tamanho--;
        return true;
    }

    T& operator[](int i) {
        if (i < 0 || i >= tamanho) throw std::out_of_range("Indice invalido");
        return dados[i];
    }

    const T& operator[](int i) const {
        if (i < 0 || i >= tamanho) throw std::out_of_range("Indice invalido");
        return dados[i];
    }

    template <typename Pred>
    T* buscar(Pred pred) {
        for (int i = 0; i < tamanho; i++)
            if (pred(dados[i])) return &dados[i];
        return nullptr;
    }

    int getTamanho() const { return tamanho; }
    int getCapacidade() const { return CAPACIDADE; }
    bool vazia() const { return tamanho == 0; }
    bool cheia() const { return tamanho == CAPACIDADE; }
};
