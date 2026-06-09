#pragma once
#include "pilha.h"

// Gerador automatico de IDs apoiado em uma PILHA.
//
// A ideia: o sistema NUNCA pede o id ao usuario. Os ids ficam guardados numa
// pilha de "ids disponiveis" que se REABASTECE sozinha em blocos. Cada cadastro
// desempilha o proximo id; cada remocao pode devolver (empilhar) o id para ser
// reaproveitado.
//
//   obter()    -> desempilha o proximo id (reabastece a pilha se ela esvaziar)
//   liberar(id)-> empilha de volta um id removido (reaproveitamento)
//   reservar(id)-> ao carregar de arquivo, garante que ids novos nao colidam
class GeradorId {
private:
    Pilha<int> disponiveis;   // topo = proximo id a ser usado
    int        proximoNovo;   // proximo id "inedito" ainda nao colocado na pilha
    static const int BLOCO = 8;

    // Reabastece a pilha com um bloco de ids novos. Empilha em ordem
    // decrescente para que o TOPO fique com o menor id (saida crescente).
    void abastecer() {
        for (int i = proximoNovo + BLOCO - 1; i >= proximoNovo; i--)
            disponiveis.empilhar(i);
        proximoNovo += BLOCO;
    }

public:
    explicit GeradorId(int inicio = 1) : proximoNovo(inicio) {}

    // Proximo id automatico.
    int obter() {
        if (disponiveis.vazia()) abastecer();
        return disponiveis.desempilhar();
    }

    // Devolve um id removido para a pilha (sera reaproveitado no proximo obter).
    void liberar(int id) { disponiveis.empilhar(id); }

    // Mantem o gerador a frente de ids ja existentes (carregados de arquivo).
    // Deve ser chamado durante a carga, antes de qualquer obter().
    void reservar(int id) { if (id >= proximoNovo) proximoNovo = id + 1; }

    int proximoInedito()    const { return proximoNovo; }
    int idsDisponiveis()    const { return disponiveis.getTamanho(); }
};
