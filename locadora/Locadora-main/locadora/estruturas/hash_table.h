#pragma once
#include <string>
#include <stdexcept>

// Hash com encadeamento separado. Capacidade deve ser primo para menos colisões.
template <typename K, typename V>
class HashTable {
private:
    struct Par {
        K    chave;
        V    valor;
        Par* proximo;
        Par(const K& k, const V& v) : chave(k), valor(v), proximo(nullptr) {}
    };

    Par** buckets;
    int   capacidade;
    int   tamanho;

    // ─── Funções de hash ─────────────────────────────────────────
    // djb2 para strings
    int calcularHash(const std::string& chave) const {
        unsigned long h = 5381;
        for (char c : chave) h = h * 33 + (unsigned char)c;
        return (int)(h % (unsigned long)capacidade);
    }

    // Divisão direta para inteiros
    int calcularHash(int chave) const {
        return ((chave % capacidade) + capacidade) % capacidade;
    }

public:
    explicit HashTable(int cap = 53)
        : capacidade(cap), tamanho(0) {
        buckets = new Par*[capacidade]();  // inicializa com nullptr
    }

    ~HashTable() {
        limpar();
        delete[] buckets;
    }

    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;

    void inserir(const K& chave, const V& valor) {
        int idx = calcularHash(chave);
        Par* atual = buckets[idx];

        // Atualiza se já existe
        while (atual) {
            if (atual->chave == chave) { atual->valor = valor; return; }
            atual = atual->proximo;
        }

        // Insere no início do bucket (O(1))
        Par* novo = new Par(chave, valor);
        novo->proximo = buckets[idx];
        buckets[idx] = novo;
        tamanho++;
    }

    V* buscar(const K& chave) {
        int idx = calcularHash(chave);
        Par* atual = buckets[idx];
        while (atual) {
            if (atual->chave == chave) return &atual->valor;
            atual = atual->proximo;
        }
        return nullptr;
    }

    bool remover(const K& chave) {
        int idx = calcularHash(chave);
        Par* atual  = buckets[idx];
        Par* anterior = nullptr;

        while (atual) {
            if (atual->chave == chave) {
                if (anterior) anterior->proximo = atual->proximo;
                else buckets[idx] = atual->proximo;
                delete atual;
                tamanho--;
                return true;
            }
            anterior = atual;
            atual = atual->proximo;
        }
        return false;
    }

    int  getTamanho()   const { return tamanho; }
    int  getCapacidade() const { return capacidade; }
    // Fator de carga: ideal < 0,75
    float fatorCarga()  const { return (float)tamanho / capacidade; }

    void limpar() {
        for (int i = 0; i < capacidade; i++) {
            Par* atual = buckets[i];
            while (atual) {
                Par* prox = atual->proximo;
                delete atual;
                atual = prox;
            }
            buckets[i] = nullptr;
        }
        tamanho = 0;
    }
};
