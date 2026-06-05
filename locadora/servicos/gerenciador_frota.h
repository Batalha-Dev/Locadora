#pragma once
#include "../estruturas/lista_encadeada.h"
#include "../estruturas/lista_estatica.h"
#include "../estruturas/fila.h"
#include "../estruturas/hash_table.h"
#include "../estruturas/gerador_id.h"
#include "../modelos/carro.h"
#include "../modelos/categoria.h"

const int MAX_CATEGORIAS = 6;

class GerenciadorFrota {
private:
    ListaEncadeada<Carro>              frota;
    ListaEstatica<Categoria, MAX_CATEGORIAS> categorias;
    Fila<int>                          filaEspera[MAX_CATEGORIAS];

    // Hash tables para busca O(1) por placa e por id
    HashTable<std::string, int>        hashPorPlaca;   // placa → id
    HashTable<int, int>                hashPorId;      // id    → id (confirma existência)

    GeradorId geradorId;   // ids automaticos via pilha (usuario nunca digita)

    void carregarCategoriasPadrao() {
        categorias.inserir({0, "Economico",    "Carros compactos e populares",  80.0});
        categorias.inserir({1, "Intermediario","Sedas e hatches medios",       120.0});
        categorias.inserir({2, "SUV",          "Utilitarios esportivos",       180.0});
        categorias.inserir({3, "Luxo",         "Veiculos premium",             350.0});
        categorias.inserir({4, "Pickup",       "Caminhonetes",                 160.0});
        categorias.inserir({5, "Van",          "Vans e minivans",              200.0});
    }

public:
    GerenciadorFrota() {
        carregarCategoriasPadrao();
    }

    // ─── Categorias ──────────────────────────────────────────────
    const ListaEstatica<Categoria, MAX_CATEGORIAS>& getCategorias() const {
        return categorias;
    }
    Categoria* buscarCategoria(int id) {
        return categorias.buscar([id](const Categoria& c){ return c.id == id; });
    }

    // ─── Frota ───────────────────────────────────────────────────
    Carro* cadastrar(const std::string& marca, const std::string& modelo,
                     const std::string& placa, int ano, int categoriaId) {
        if (!buscarCategoria(categoriaId)) return nullptr;
        // Placa duplicada não é permitida
        if (hashPorPlaca.buscar(placa)) return nullptr;

        Categoria* cat = buscarCategoria(categoriaId);
        double diaria = cat->diariaPadrao;

        int id = geradorId.obter();
        frota.inserirFim({id, marca, modelo, placa, ano, categoriaId, diaria});

        hashPorPlaca.inserir(placa, id);
        hashPorId.inserir(id, id);

        return frota.buscar([id](Carro& c){ return c.id == id; });
    }

    // Carrega carro já existente (vindo de arquivo) sem gerar novo id
    void inserirExistente(const Carro& c) {
        frota.inserirFim(c);
        hashPorPlaca.inserir(c.placa, c.id);
        hashPorId.inserir(c.id, c.id);
        geradorId.reservar(c.id);
    }

    bool remover(int id, bool reutilizarId = true) {
        Carro* c = buscarPorId(id);
        if (!c) return false;
        hashPorPlaca.remover(c->placa);
        hashPorId.remover(id);
        bool ok = frota.removerSe([id](const Carro& x){ return x.id == id; });
        if (ok && reutilizarId) geradorId.liberar(id);  // id volta para a pilha
        return ok;
    }

    // Busca O(1) via hash
    Carro* buscarPorId(int id) {
        int* encontrado = hashPorId.buscar(id);
        if (!encontrado) return nullptr;
        return frota.buscar([id](Carro& c){ return c.id == id; });
    }

    // Busca O(1) via hash
    Carro* buscarPorPlaca(const std::string& placa) {
        int* id = hashPorPlaca.buscar(placa);
        if (!id) return nullptr;
        return frota.buscar([id](Carro& c){ return c.id == *id; });
    }

    ListaEncadeada<Carro>& getFrota() { return frota; }

    bool marcarLocado(int id) {
        Carro* c = buscarPorId(id);
        if (!c || c->status != StatusCarro::DISPONIVEL) return false;
        c->status = StatusCarro::LOCADO;
        return true;
    }

    bool marcarDisponivel(int id) {
        Carro* c = buscarPorId(id);
        if (!c) return false;
        c->status = StatusCarro::DISPONIVEL;
        return true;
    }

    bool marcarManutencao(int id) {
        Carro* c = buscarPorId(id);
        if (!c || c->status == StatusCarro::LOCADO) return false;
        c->status = StatusCarro::MANUTENCAO;
        return true;
    }

    // ─── Fila de espera ──────────────────────────────────────────
    void entrarFilaEspera(int idCliente, int categoriaId) {
        if (categoriaId < 0 || categoriaId >= MAX_CATEGORIAS) return;
        filaEspera[categoriaId].enfileirar(idCliente);
    }

    int proximoDaFila(int categoriaId) {
        if (categoriaId < 0 || categoriaId >= MAX_CATEGORIAS) return -1;
        if (filaEspera[categoriaId].vazia()) return -1;
        return filaEspera[categoriaId].desenfileirar();
    }

    int tamanhoFila(int categoriaId) const {
        if (categoriaId < 0 || categoriaId >= MAX_CATEGORIAS) return 0;
        return filaEspera[categoriaId].getTamanho();
    }

    int contarDisponiveis(int categoriaId) {
        int count = 0;
        frota.paraCada([&](Carro& c) {
            if (c.categoriaId == categoriaId && c.status == StatusCarro::DISPONIVEL)
                count++;
        });
        return count;
    }

    float fatorCargaHash() const { return hashPorPlaca.fatorCarga(); }
};
