#pragma once
#include <stdexcept>

template <typename T>
class ListaEncadeada {
private:
    struct No {
        T dado;
        No* anterior;
        No* proximo;
        No(const T& d) : dado(d), anterior(nullptr), proximo(nullptr) {}
    };

    No* cabeca;
    No* cauda;
    int tamanho;

public:
    ListaEncadeada() : cabeca(nullptr), cauda(nullptr), tamanho(0) {}

    ~ListaEncadeada() { limpar(); }

    // Impede cópia acidental
    ListaEncadeada(const ListaEncadeada&) = delete;
    ListaEncadeada& operator=(const ListaEncadeada&) = delete;

    void inserirFim(const T& dado) {
        No* novo = new No(dado);
        if (!cauda) {
            cabeca = cauda = novo;
        } else {
            novo->anterior = cauda;
            cauda->proximo = novo;
            cauda = novo;
        }
        tamanho++;
    }

    void inserirInicio(const T& dado) {
        No* novo = new No(dado);
        if (!cabeca) {
            cabeca = cauda = novo;
        } else {
            novo->proximo = cabeca;
            cabeca->anterior = novo;
            cabeca = novo;
        }
        tamanho++;
    }

    bool remover(const T& dado) {
        No* atual = cabeca;
        while (atual) {
            if (atual->dado == dado) {
                if (atual->anterior) atual->anterior->proximo = atual->proximo;
                else cabeca = atual->proximo;
                if (atual->proximo) atual->proximo->anterior = atual->anterior;
                else cauda = atual->anterior;
                delete atual;
                tamanho--;
                return true;
            }
            atual = atual->proximo;
        }
        return false;
    }

    // Busca por predicado: buscar([](Carro& c){ return c.id == 5; })
    template <typename Pred>
    T* buscar(Pred pred) {
        No* atual = cabeca;
        while (atual) {
            if (pred(atual->dado)) return &atual->dado;
            atual = atual->proximo;
        }
        return nullptr;
    }

    template <typename Pred>
    bool removerSe(Pred pred) {
        No* atual = cabeca;
        while (atual) {
            if (pred(atual->dado)) {
                No* alvo = atual;
                if (alvo->anterior) alvo->anterior->proximo = alvo->proximo;
                else cabeca = alvo->proximo;
                if (alvo->proximo) alvo->proximo->anterior = alvo->anterior;
                else cauda = alvo->anterior;
                delete alvo;
                tamanho--;
                return true;
            }
            atual = atual->proximo;
        }
        return false;
    }

    // Percorre todos os elementos com uma função
    template <typename Func>
    void paraCada(Func func) {
        No* atual = cabeca;
        while (atual) {
            func(atual->dado);
            atual = atual->proximo;
        }
    }

    template <typename Func>
    void paraCada(Func func) const {
        No* atual = cabeca;
        while (atual) {
            func(atual->dado);
            atual = atual->proximo;
        }
    }

    int getTamanho() const { return tamanho; }
    bool vazia() const { return tamanho == 0; }

    void limpar() {
        No* atual = cabeca;
        while (atual) {
            No* prox = atual->proximo;
            delete atual;
            atual = prox;
        }
        cabeca = cauda = nullptr;
        tamanho = 0;
    }
};
