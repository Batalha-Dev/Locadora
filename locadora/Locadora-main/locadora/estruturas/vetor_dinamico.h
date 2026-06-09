#pragma once
#include <stdexcept>

// Array dinâmico simples — base para ordenação e busca binária.
template <typename T>
class VetorDinamico {
private:
    T*  dados;
    int tamanho;
    int capacidade;

    void redimensionar() {
        capacidade *= 2;
        T* novo = new T[capacidade];
        for (int i = 0; i < tamanho; i++) novo[i] = dados[i];
        delete[] dados;
        dados = novo;
    }

public:
    explicit VetorDinamico(int capInicial = 16)
        : tamanho(0), capacidade(capInicial) {
        dados = new T[capacidade];
    }

    ~VetorDinamico() { delete[] dados; }

    VetorDinamico(const VetorDinamico&) = delete;
    VetorDinamico& operator=(const VetorDinamico&) = delete;

    // Move constructor — permite retorno por valor sem copiar
    VetorDinamico(VetorDinamico&& outro) noexcept
        : dados(outro.dados), tamanho(outro.tamanho), capacidade(outro.capacidade) {
        outro.dados = nullptr;
        outro.tamanho = 0;
        outro.capacidade = 0;
    }

    void adicionar(const T& item) {
        if (tamanho == capacidade) redimensionar();
        dados[tamanho++] = item;
    }

    T& operator[](int i) {
        if (i < 0 || i >= tamanho) throw std::out_of_range("Indice invalido");
        return dados[i];
    }
    const T& operator[](int i) const {
        if (i < 0 || i >= tamanho) throw std::out_of_range("Indice invalido");
        return dados[i];
    }

    int  getTamanho() const { return tamanho; }
    bool vazio()      const { return tamanho == 0; }
    void limpar()           { tamanho = 0; }
};
