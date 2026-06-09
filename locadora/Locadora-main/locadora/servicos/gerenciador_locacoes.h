#pragma once
#include "../estruturas/lista_encadeada.h"
#include "../estruturas/pilha.h"
#include "../estruturas/gerador_id.h"
#include "../modelos/locacao.h"
#include "../modelos/operacao.h"
#include "gerenciador_frota.h"
#include "gerenciador_clientes.h"

class GerenciadorLocacoes {
private:
    ListaEncadeada<Locacao> locacoes;
    Pilha<Operacao>         historicoOps;
    GerenciadorFrota&       frota;
    GerenciadorClientes&    clientes;
    GeradorId               geradorId;   // ids de locacao automaticos via pilha

    // ─── Consistência ────────────────────────────────────────────
    // Verifica se o cliente já possui uma locação ativa
    bool clienteTemLocacaoAtiva(int idCliente) {
        bool encontrado = false;
        locacoes.paraCada([&](Locacao& l) {
            if (l.idCliente == idCliente && l.status == StatusLocacao::ATIVA)
                encontrado = true;
        });
        return encontrado;
    }

public:
    GerenciadorLocacoes(GerenciadorFrota& f, GerenciadorClientes& c)
        : frota(f), clientes(c) {}

    // Insere locação já existente (carregada de arquivo)
    void inserirExistente(const Locacao& l) {
        locacoes.inserirFim(l);
        geradorId.reservar(l.id);
        // Restaura status do carro se locação está ativa
        if (l.status == StatusLocacao::ATIVA)
            frota.marcarLocado(l.idCarro);
    }

    // ─── Reconciliação de consistência (frota ↔ locações) ─────────
    // Após carregar de arquivo, garante que o status de cada veículo
    // corresponde às locações ATIVAS. Corrige os dois sentidos da
    // inconsistência e retorna o nº de correções aplicadas.
    int reconciliarFrota() {
        int correcoes = 0;

        // (1) Veículo com locação ATIVA deve estar LOCADO (exceto em MANUTENCAO)
        locacoes.paraCada([&](Locacao& l) {
            if (l.status != StatusLocacao::ATIVA) return;
            Carro* c = frota.buscarPorId(l.idCarro);
            if (c && c->status != StatusCarro::LOCADO &&
                     c->status != StatusCarro::MANUTENCAO) {
                c->status = StatusCarro::LOCADO;
                correcoes++;
            }
        });

        // (2) Veículo LOCADO sem nenhuma locação ATIVA deve voltar a DISPONIVEL
        frota.getFrota().paraCada([&](Carro& c) {
            if (c.status != StatusCarro::LOCADO) return;
            bool temAtiva = false;
            locacoes.paraCada([&](Locacao& l) {
                if (l.idCarro == c.id && l.status == StatusLocacao::ATIVA)
                    temAtiva = true;
            });
            if (!temAtiva) { c.status = StatusCarro::DISPONIVEL; correcoes++; }
        });

        return correcoes;
    }

    Locacao* registrarLocacao(int idCliente, int idCarro, int dias) {
        Cliente* cli = clientes.buscarPorId(idCliente);
        Carro*   car = frota.buscarPorId(idCarro);

        if (!cli || !car)                              return nullptr;
        if (car->status != StatusCarro::DISPONIVEL)    return nullptr;
        if (dias <= 0)                                 return nullptr;
        if (clienteTemLocacaoAtiva(idCliente))         return nullptr; // consistência

        time_t agora = std::time(nullptr);
        int id = geradorId.obter();
        locacoes.inserirFim({id, idCliente, idCarro, agora, dias, car->diaria});
        frota.marcarLocado(idCarro);

        Locacao* loc = locacoes.buscar([id](Locacao& l){ return l.id == id; });
        historicoOps.empilhar({TipoOp::REGISTRAR_LOCACAO, loc->id,
            "Locacao #" + std::to_string(loc->id) +
            " | Cliente " + std::to_string(idCliente) +
            " | Carro "   + std::to_string(idCarro)});

        return loc;
    }

    bool registrarDevolucao(int idLocacao) {
        Locacao* loc = buscarPorId(idLocacao);
        if (!loc || loc->status != StatusLocacao::ATIVA) return false;

        loc->dataFimReal = std::time(nullptr);
        loc->status = StatusLocacao::CONCLUIDA;

        Carro* car = frota.buscarPorId(loc->idCarro);
        if (car) {
            // Recalcula o valor pela duracao real, cobrando no minimo 1 diaria.
            // Usa a MESMA regra exibida na tela de devolucao (consistencia
            // entre o que o usuario ve e o que e persistido).
            loc->valorTotal = loc->valorCobrado(car->diaria, loc->dataFimReal);
            frota.marcarDisponivel(loc->idCarro);

            // Atende próximo da fila de espera se houver
            int proxCli = frota.proximoDaFila(car->categoriaId);
            (void)proxCli; // notificação seria enviada aqui
        }

        historicoOps.empilhar({TipoOp::REGISTRAR_DEVOLUCAO, idLocacao,
            "Devolucao locacao #" + std::to_string(idLocacao)});

        return true;
    }

    bool cancelarLocacao(int idLocacao) {
        Locacao* loc = buscarPorId(idLocacao);
        if (!loc || loc->status != StatusLocacao::ATIVA) return false;

        loc->status = StatusLocacao::CANCELADA;
        frota.marcarDisponivel(loc->idCarro);

        historicoOps.empilhar({TipoOp::CANCELAR_LOCACAO, idLocacao,
            "Cancelamento locacao #" + std::to_string(idLocacao)});

        return true;
    }

    // ─── Desfazer ────────────────────────────────────────────────
    std::string desfazerUltima() {
        if (historicoOps.vazia()) return "";
        Operacao op = historicoOps.desempilhar();

        switch (op.tipo) {
            case TipoOp::REGISTRAR_LOCACAO: {
                Locacao* loc = buscarPorId(op.idReferencia);
                if (loc && loc->status == StatusLocacao::ATIVA) {
                    loc->status = StatusLocacao::CANCELADA;
                    frota.marcarDisponivel(loc->idCarro);
                }
                break;
            }
            case TipoOp::REGISTRAR_DEVOLUCAO: {
                Locacao* loc = buscarPorId(op.idReferencia);
                if (loc && loc->status == StatusLocacao::CONCLUIDA) {
                    loc->status = StatusLocacao::ATIVA;
                    loc->dataFimReal = 0;
                    frota.marcarLocado(loc->idCarro);
                }
                break;
            }
            default: break;
        }
        return op.descricao;
    }

    // ─── Consultas básicas ───────────────────────────────────────
    Locacao* buscarPorId(int id) {
        return locacoes.buscar([id](Locacao& l){ return l.id == id; });
    }

    ListaEncadeada<Locacao>& getLocacoes() { return locacoes; }

    double receitaTotal() {
        double total = 0;
        locacoes.paraCada([&](Locacao& l) {
            if (l.status == StatusLocacao::CONCLUIDA) total += l.valorTotal;
        });
        return total;
    }

    double receitaPrevista() {
        double total = 0;
        locacoes.paraCada([&](Locacao& l) {
            if (l.status == StatusLocacao::ATIVA) total += l.valorTotal;
        });
        return total;
    }

    int locacoesAtivas() {
        int count = 0;
        locacoes.paraCada([&](Locacao& l) {
            if (l.status == StatusLocacao::ATIVA) count++;
        });
        return count;
    }
};
