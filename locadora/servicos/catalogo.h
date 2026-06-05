#pragma once
#include <string>
#include <cctype>
#include "../estruturas/lista_encadeada.h"
#include "../estruturas/vetor_dinamico.h"
#include "../estruturas/gerador_id.h"
#include "../modelos/marca.h"

// Catalogo de marcas/modelos. Fornece a lista pronta usada no cadastro de
// veiculos (o usuario SELECIONA, nao digita texto livre) e permite cadastrar
// novas marcas e modelos. Ids gerados automaticamente via pilha (GeradorId).
class Catalogo {
private:
    ListaEncadeada<Marca>  marcas;
    ListaEncadeada<Modelo> modelos;
    GeradorId genMarca;
    GeradorId genModelo;

    static std::string minusculo(const std::string& s) {
        std::string r;
        for (char c : s) r += (char)std::tolower((unsigned char)c);
        return r;
    }

public:
    // ─── Marcas ──────────────────────────────────────────────────
    Marca* adicionarMarca(const std::string& nome) {
        if (nome.empty() || buscarMarcaPorNome(nome)) return nullptr; // sem duplicada
        int id = genMarca.obter();
        marcas.inserirFim({id, nome});
        return marcas.buscar([id](Marca& m){ return m.id == id; });
    }

    void inserirMarcaExistente(const Marca& m) {
        marcas.inserirFim(m);
        genMarca.reservar(m.id);
    }

    Marca* buscarMarca(int id) {
        return marcas.buscar([id](Marca& m){ return m.id == id; });
    }

    Marca* buscarMarcaPorNome(const std::string& nome) {
        std::string alvo = minusculo(nome);
        return marcas.buscar([&](Marca& m){ return minusculo(m.nome) == alvo; });
    }

    ListaEncadeada<Marca>& getMarcas() { return marcas; }

    // ─── Modelos ─────────────────────────────────────────────────
    Modelo* adicionarModelo(int marcaId, const std::string& nome) {
        if (nome.empty() || !buscarMarca(marcaId)) return nullptr;
        if (buscarModeloNaMarca(marcaId, nome)) return nullptr; // sem duplicada na marca
        int id = genModelo.obter();
        modelos.inserirFim({id, marcaId, nome});
        return modelos.buscar([id](Modelo& m){ return m.id == id; });
    }

    void inserirModeloExistente(const Modelo& m) {
        modelos.inserirFim(m);
        genModelo.reservar(m.id);
    }

    Modelo* buscarModelo(int id) {
        return modelos.buscar([id](Modelo& m){ return m.id == id; });
    }

    Modelo* buscarModeloNaMarca(int marcaId, const std::string& nome) {
        std::string alvo = minusculo(nome);
        return modelos.buscar([&](Modelo& m){
            return m.marcaId == marcaId && minusculo(m.nome) == alvo;
        });
    }

    // Modelos de uma marca (para listar na hora do cadastro).
    VetorDinamico<Modelo> modelosDaMarca(int marcaId) {
        VetorDinamico<Modelo> r;
        modelos.paraCada([&](Modelo& m){ if (m.marcaId == marcaId) r.adicionar(m); });
        return r;
    }

    ListaEncadeada<Modelo>& getModelos() { return modelos; }

    int totalMarcas()  { return marcas.getTamanho(); }
    int totalModelos() { return modelos.getTamanho(); }
    bool vazio()       { return marcas.vazia(); }

    // Popula o catalogo com marcas/modelos comuns do mercado brasileiro.
    // Usado apenas quando nao ha arquivo salvo.
    void carregarPadrao() {
        struct { const char* marca; const char* modelos[6]; } base[] = {
            {"Volkswagen", {"Gol", "Polo", "Virtus", "T-Cross", "Nivus", nullptr}},
            {"Chevrolet",  {"Onix", "Onix Plus", "Tracker", "Spin", "S10", nullptr}},
            {"Fiat",       {"Mobi", "Argo", "Cronos", "Strada", "Toro", nullptr}},
            {"Toyota",     {"Yaris", "Corolla", "Corolla Cross", "Hilux", nullptr, nullptr}},
            {"Hyundai",    {"HB20", "Creta", nullptr, nullptr, nullptr, nullptr}},
            {"Honda",      {"City", "Civic", "HR-V", "Fit", nullptr, nullptr}},
            {"Jeep",       {"Renegade", "Compass", "Commander", nullptr, nullptr, nullptr}},
            {"Renault",    {"Kwid", "Sandero", "Duster", nullptr, nullptr, nullptr}},
            {"Ford",       {"Ranger", "Territory", nullptr, nullptr, nullptr, nullptr}},
            {"BMW",        {"320i", "X1", "X3", nullptr, nullptr, nullptr}},
        };
        for (auto& b : base) {
            Marca* m = adicionarMarca(b.marca);
            if (!m) continue;
            for (const char* mod : b.modelos)
                if (mod) adicionarModelo(m->id, mod);
        }
    }
};
