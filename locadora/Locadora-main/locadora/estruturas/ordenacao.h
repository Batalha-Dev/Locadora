#pragma once
#include "vetor_dinamico.h"

// Merge Sort genérico sobre VetorDinamico<T>.
// Comparador: função/lambda bool comp(const T& a, const T& b) → true se a < b.
namespace Ordenacao {

template <typename T, typename Comp>
void merge(VetorDinamico<T>& v, int esq, int meio, int dir, Comp comp) {
    int n1 = meio - esq + 1;
    int n2 = dir - meio;

    // Subarrays temporários alocados na heap para não estourar a pilha
    T* L = new T[n1];
    T* R = new T[n2];

    for (int i = 0; i < n1; i++) L[i] = v[esq + i];
    for (int j = 0; j < n2; j++) R[j] = v[meio + 1 + j];

    int i = 0, j = 0, k = esq;
    while (i < n1 && j < n2) {
        if (comp(L[i], R[j])) v[k++] = L[i++];
        else                   v[k++] = R[j++];
    }
    while (i < n1) v[k++] = L[i++];
    while (j < n2) v[k++] = R[j++];

    delete[] L;
    delete[] R;
}

template <typename T, typename Comp>
void mergeSort(VetorDinamico<T>& v, int esq, int dir, Comp comp) {
    if (esq >= dir) return;
    int meio = esq + (dir - esq) / 2;
    mergeSort(v, esq, meio, comp);
    mergeSort(v, meio + 1, dir, comp);
    merge(v, esq, meio, dir, comp);
}

// Ponto de entrada: ordena o vetor inteiro
template <typename T, typename Comp>
void ordenar(VetorDinamico<T>& v, Comp comp) {
    if (v.getTamanho() > 1)
        mergeSort(v, 0, v.getTamanho() - 1, comp);
}

// ─── Busca binária ────────────────────────────────────────────────────────────
// Requer vetor ORDENADO pelo mesmo critério de chave usado aqui.
// Extrator: função que retorna a chave comparável de cada elemento.
// Retorna índice do elemento ou -1 se não encontrado.
template <typename T, typename Chave, typename Extrator>
int buscaBinaria(VetorDinamico<T>& v, const Chave& alvo, Extrator extrair) {
    int esq = 0, dir = v.getTamanho() - 1;
    while (esq <= dir) {
        int meio = esq + (dir - esq) / 2;
        Chave chaveMeio = extrair(v[meio]);
        if (chaveMeio == alvo)  return meio;
        if (chaveMeio < alvo)   esq = meio + 1;
        else                    dir = meio - 1;
    }
    return -1;
}

} // namespace Ordenacao
