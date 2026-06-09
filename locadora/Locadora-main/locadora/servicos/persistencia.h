#pragma once
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include "../modelos/carro.h"
#include "../modelos/cliente.h"
#include "../modelos/locacao.h"
#include "../modelos/marca.h"
#include "../estruturas/lista_encadeada.h"

// Persistência em CSV simples.
// Cada entidade é salva em dados/carros.csv, clientes.csv, locacoes.csv.
namespace Persistencia {

const std::string DIR = "dados/";

// ─── Helpers ─────────────────────────────────────────────────────────────────
inline std::string campo(const std::string& s) {
    // Escapa vírgulas dentro de campos
    return '"' + s + '"';
}

inline std::string lerCampo(std::istringstream& ss) {
    std::string tok;
    if (ss.peek() == '"') {
        ss.ignore(); // remove "
        std::getline(ss, tok, '"');
        ss.ignore(); // remove ,
    } else {
        std::getline(ss, tok, ',');
    }
    return tok;
}

inline void criarDiretorio() {
#ifdef _WIN32
    system("if not exist dados mkdir dados");
#else
    system("mkdir -p dados");
#endif
}

// ─── Carros ──────────────────────────────────────────────────────────────────
inline bool salvarCarros(ListaEncadeada<Carro>& frota) {
    criarDiretorio();
    std::ofstream f(DIR + "carros.csv");
    if (!f.is_open()) return false;

    f << "id,marca,modelo,placa,ano,categoriaId,diaria,status\n";
    frota.paraCada([&](Carro& c) {
        f << c.id << ","
          << campo(c.marca)  << ","
          << campo(c.modelo) << ","
          << campo(c.placa)  << ","
          << c.ano << ","
          << c.categoriaId << ","
          << c.diaria << ","
          << (int)c.status << "\n";
    });
    return true;
}

inline bool carregarCarros(ListaEncadeada<Carro>& frota, int& proximoId) {
    std::ifstream f(DIR + "carros.csv");
    if (!f.is_open()) return false;

    std::string linha;
    std::getline(f, linha); // cabeçalho

    while (std::getline(f, linha)) {
        if (linha.empty()) continue;
        std::istringstream ss(linha);
        Carro c;
        std::string tmp;

        std::getline(ss, tmp, ','); c.id = std::stoi(tmp);
        c.marca      = lerCampo(ss);
        c.modelo     = lerCampo(ss);
        c.placa      = lerCampo(ss);
        std::getline(ss, tmp, ','); c.ano = std::stoi(tmp);
        std::getline(ss, tmp, ','); c.categoriaId = std::stoi(tmp);
        std::getline(ss, tmp, ','); c.diaria = std::stod(tmp);
        std::getline(ss, tmp);     c.status = (StatusCarro)std::stoi(tmp);

        frota.inserirFim(c);
        if (c.id >= proximoId) proximoId = c.id + 1;
    }
    return true;
}

// ─── Clientes ────────────────────────────────────────────────────────────────
inline bool salvarClientes(ListaEncadeada<Cliente>& clientes) {
    criarDiretorio();
    std::ofstream f(DIR + "clientes.csv");
    if (!f.is_open()) return false;

    f << "id,nome,cpf,telefone,email,ativo\n";
    clientes.paraCada([&](Cliente& c) {
        f << c.id << ","
          << campo(c.nome)     << ","
          << campo(c.cpf)      << ","
          << campo(c.telefone) << ","
          << campo(c.email)    << ","
          << (c.ativo ? 1 : 0) << "\n";
    });
    return true;
}

inline bool carregarClientes(ListaEncadeada<Cliente>& clientes, int& proximoId) {
    std::ifstream f(DIR + "clientes.csv");
    if (!f.is_open()) return false;

    std::string linha;
    std::getline(f, linha); // cabeçalho

    while (std::getline(f, linha)) {
        if (linha.empty()) continue;
        std::istringstream ss(linha);
        Cliente c;
        std::string tmp;

        std::getline(ss, tmp, ','); c.id = std::stoi(tmp);
        c.nome      = lerCampo(ss);
        c.cpf       = lerCampo(ss);
        c.telefone  = lerCampo(ss);
        c.email     = lerCampo(ss);
        std::getline(ss, tmp); c.ativo = (std::stoi(tmp) == 1);

        clientes.inserirFim(c);
        if (c.id >= proximoId) proximoId = c.id + 1;
    }
    return true;
}

// ─── Locações ────────────────────────────────────────────────────────────────
inline bool salvarLocacoes(ListaEncadeada<Locacao>& locacoes) {
    criarDiretorio();
    std::ofstream f(DIR + "locacoes.csv");
    if (!f.is_open()) return false;

    f << "id,idCliente,idCarro,dataInicio,dataFimPrevista,dataFimReal,valorTotal,status\n";
    locacoes.paraCada([&](Locacao& l) {
        f << l.id << ","
          << l.idCliente << ","
          << l.idCarro   << ","
          << l.dataInicio << ","
          << l.dataFimPrevista << ","
          << l.dataFimReal << ","
          << l.valorTotal << ","
          << (int)l.status << "\n";
    });
    return true;
}

inline bool carregarLocacoes(ListaEncadeada<Locacao>& locacoes, int& proximoId) {
    std::ifstream f(DIR + "locacoes.csv");
    if (!f.is_open()) return false;

    std::string linha;
    std::getline(f, linha); // cabeçalho

    while (std::getline(f, linha)) {
        if (linha.empty()) continue;
        std::istringstream ss(linha);
        Locacao l;
        std::string tmp;

        std::getline(ss, tmp, ','); l.id = std::stoi(tmp);
        std::getline(ss, tmp, ','); l.idCliente = std::stoi(tmp);
        std::getline(ss, tmp, ','); l.idCarro   = std::stoi(tmp);
        std::getline(ss, tmp, ','); l.dataInicio = (time_t)std::stoll(tmp);
        std::getline(ss, tmp, ','); l.dataFimPrevista = (time_t)std::stoll(tmp);
        std::getline(ss, tmp, ','); l.dataFimReal     = (time_t)std::stoll(tmp);
        std::getline(ss, tmp, ','); l.valorTotal = std::stod(tmp);
        std::getline(ss, tmp);     l.status = (StatusLocacao)std::stoi(tmp);

        locacoes.inserirFim(l);
        if (l.id >= proximoId) proximoId = l.id + 1;
    }
    return true;
}

// ─── Catalogo: Marcas ────────────────────────────────────────────────────────
inline bool salvarMarcas(ListaEncadeada<Marca>& marcas) {
    criarDiretorio();
    std::ofstream f(DIR + "marcas.csv");
    if (!f.is_open()) return false;

    f << "id,nome\n";
    marcas.paraCada([&](Marca& m) {
        f << m.id << "," << campo(m.nome) << "\n";
    });
    return true;
}

inline bool carregarMarcas(ListaEncadeada<Marca>& marcas) {
    std::ifstream f(DIR + "marcas.csv");
    if (!f.is_open()) return false;

    std::string linha;
    std::getline(f, linha); // cabeçalho
    while (std::getline(f, linha)) {
        if (linha.empty()) continue;
        std::istringstream ss(linha);
        Marca m;
        std::string tmp;
        std::getline(ss, tmp, ','); m.id = std::stoi(tmp);
        m.nome = lerCampo(ss);
        marcas.inserirFim(m);
    }
    return true;
}

// ─── Catalogo: Modelos ───────────────────────────────────────────────────────
inline bool salvarModelos(ListaEncadeada<Modelo>& modelos) {
    criarDiretorio();
    std::ofstream f(DIR + "modelos.csv");
    if (!f.is_open()) return false;

    f << "id,marcaId,nome\n";
    modelos.paraCada([&](Modelo& m) {
        f << m.id << "," << m.marcaId << "," << campo(m.nome) << "\n";
    });
    return true;
}

inline bool carregarModelos(ListaEncadeada<Modelo>& modelos) {
    std::ifstream f(DIR + "modelos.csv");
    if (!f.is_open()) return false;

    std::string linha;
    std::getline(f, linha); // cabeçalho
    while (std::getline(f, linha)) {
        if (linha.empty()) continue;
        std::istringstream ss(linha);
        Modelo m;
        std::string tmp;
        std::getline(ss, tmp, ','); m.id = std::stoi(tmp);
        std::getline(ss, tmp, ','); m.marcaId = std::stoi(tmp);
        m.nome = lerCampo(ss);
        modelos.inserirFim(m);
    }
    return true;
}

} // namespace Persistencia
