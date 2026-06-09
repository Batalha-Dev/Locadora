#pragma once
#include "../estruturas/vetor_dinamico.h"
#include "../estruturas/ordenacao.h"
#include "../modelos/locacao.h"
#include "../modelos/carro.h"
#include "../modelos/cliente.h"
#include "gerenciador_frota.h"
#include "gerenciador_clientes.h"
#include "gerenciador_locacoes.h"

// Todas as consultas analíticas do sistema.
// Separa responsabilidade de "buscar/ordenar/filtrar" dos gerenciadores.
namespace Consultas {

// ─── Histórico por cliente ────────────────────────────────────────────────────
// Retorna locações do cliente ordenadas por dataInicio (mais recente primeiro).
inline VetorDinamico<Locacao> historicoPorCliente(int idCliente,
                                                   GerenciadorLocacoes& locacoes) {
    VetorDinamico<Locacao> resultado;

    locacoes.getLocacoes().paraCada([&](Locacao& l) {
        if (l.idCliente == idCliente)
            resultado.adicionar(l);
    });

    Ordenacao::ordenar(resultado, [](const Locacao& a, const Locacao& b) {
        return a.dataInicio > b.dataInicio; // mais recente primeiro
    });

    return resultado;
}

// ─── Histórico por veículo ────────────────────────────────────────────────────
inline VetorDinamico<Locacao> historicoPorVeiculo(int idCarro,
                                                   GerenciadorLocacoes& locacoes) {
    VetorDinamico<Locacao> resultado;

    locacoes.getLocacoes().paraCada([&](Locacao& l) {
        if (l.idCarro == idCarro)
            resultado.adicionar(l);
    });

    Ordenacao::ordenar(resultado, [](const Locacao& a, const Locacao& b) {
        return a.dataInicio > b.dataInicio;
    });

    return resultado;
}

// ─── Locações ativas ordenadas por data de início ─────────────────────────────
inline VetorDinamico<Locacao> locacoesAtivasOrdenadas(GerenciadorLocacoes& locacoes) {
    VetorDinamico<Locacao> resultado;

    locacoes.getLocacoes().paraCada([&](Locacao& l) {
        if (l.status == StatusLocacao::ATIVA)
            resultado.adicionar(l);
    });

    Ordenacao::ordenar(resultado, [](const Locacao& a, const Locacao& b) {
        return a.dataInicio < b.dataInicio; // mais antigas primeiro (prioridade)
    });

    return resultado;
}

// ─── Busca binária de locação por ID ─────────────────────────────────────────
// Ordena por id e aplica busca binária. Demonstra a estrutura do algoritmo.
inline int buscarLocacaoPorId(VetorDinamico<Locacao>& vetor, int idAlvo) {
    // Garante que está ordenado por id antes de chamar
    Ordenacao::ordenar(vetor, [](const Locacao& a, const Locacao& b) {
        return a.id < b.id;
    });
    return Ordenacao::buscaBinaria(vetor, idAlvo,
                                   [](const Locacao& l) { return l.id; });
}

// ─── Veículos disponíveis por categoria ───────────────────────────────────────
inline VetorDinamico<Carro> disponiveisPorCategoria(int categoriaId,
                                                     GerenciadorFrota& frota) {
    VetorDinamico<Carro> resultado;

    frota.getFrota().paraCada([&](Carro& c) {
        if (c.categoriaId == categoriaId && c.status == StatusCarro::DISPONIVEL)
            resultado.adicionar(c);
    });

    return resultado;
}

// ─── Todos os veículos disponíveis ────────────────────────────────────────────
inline VetorDinamico<Carro> todosDisponiveis(GerenciadorFrota& frota) {
    VetorDinamico<Carro> resultado;

    frota.getFrota().paraCada([&](Carro& c) {
        if (c.status == StatusCarro::DISPONIVEL)
            resultado.adicionar(c);
    });

    // Ordena por categoria, depois por modelo
    Ordenacao::ordenar(resultado, [](const Carro& a, const Carro& b) {
        if (a.categoriaId != b.categoriaId) return a.categoriaId < b.categoriaId;
        return a.modelo < b.modelo;
    });

    return resultado;
}

// ─── Locações em atraso ───────────────────────────────────────────────────────
inline VetorDinamico<Locacao> locacoesEmAtraso(GerenciadorLocacoes& locacoes) {
    VetorDinamico<Locacao> resultado;
    time_t agora = std::time(nullptr);

    locacoes.getLocacoes().paraCada([&](Locacao& l) {
        if (l.status == StatusLocacao::ATIVA && agora > l.dataFimPrevista)
            resultado.adicionar(l);
    });

    // Ordena pelo maior atraso primeiro
    Ordenacao::ordenar(resultado, [](const Locacao& a, const Locacao& b) {
        return a.dataFimPrevista < b.dataFimPrevista;
    });

    return resultado;
}

} // namespace Consultas
