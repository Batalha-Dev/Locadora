#pragma once
#include "console.h"
#include "validacao.h"
#include "../servicos/gerenciador_frota.h"
#include "../servicos/gerenciador_clientes.h"
#include "../servicos/gerenciador_locacoes.h"
#include "../servicos/consultas.h"
#include "../servicos/catalogo.h"
#include <sstream>
#include <iomanip>

// ─── Helpers de impressao ────────────────────────────────────────────────────

inline void imprimirCabecalhoLocacao() {
    std::cout << Cor::CIANO << std::left
              << std::setw(5)  << "ID"
              << std::setw(20) << "CLIENTE"
              << std::setw(16) << "VEICULO"
              << std::setw(12) << "INICIO"
              << std::setw(12) << "PREV.FIM"
              << std::setw(12) << "VALOR"
              << std::setw(10) << "STATUS"
              << Cor::RESET << "\n";
    Console::linha();
}

inline void imprimirLinhaLocacao(const Locacao& l,
                                  GerenciadorClientes& clientes,
                                  GerenciadorFrota& frota) {
    Cliente* cli = clientes.buscarPorId(l.idCliente);
    Carro*   car = frota.buscarPorId(l.idCarro);

    std::string nomeCli = cli ? cli->nome : "?";
    std::string nomeVei = car ? (car->marca + " " + car->modelo) : "?";
    if (nomeCli.size() > 18) nomeCli = nomeCli.substr(0, 17) + ".";
    if (nomeVei.size() > 14) nomeVei = nomeVei.substr(0, 13) + ".";

    time_t agora = std::time(nullptr);
    bool emAtraso = (l.status == StatusLocacao::ATIVA && agora > l.dataFimPrevista);

    std::string corStatus = Cor::RESET;
    if (l.status == StatusLocacao::ATIVA)
        corStatus = emAtraso ? Cor::VERMELHO : Cor::VERDE;
    if (l.status == StatusLocacao::CANCELADA) corStatus = Cor::CINZA;

    std::cout << std::left
              << std::setw(5)  << l.id
              << std::setw(20) << nomeCli
              << std::setw(16) << nomeVei
              << std::setw(12) << Console::formatarData(l.dataInicio)
              << std::setw(12) << Console::formatarData(l.dataFimPrevista)
              << std::setw(12) << Console::formatarMoeda(l.valorTotal)
              << corStatus << std::setw(10) << (emAtraso ? "ATRASO" : l.statusStr())
              << Cor::RESET << "\n";
}

// ─── FROTA ───────────────────────────────────────────────────────────────────

inline void telaListarCarros(GerenciadorFrota& frota) {
    Console::cabecalho("FROTA DE VEICULOS");

    if (frota.getFrota().vazia()) {
        Console::aviso("Nenhum veiculo cadastrado.");
        Console::pausar();
        return;
    }

    std::cout << Cor::CIANO << std::left
              << std::setw(5)  << "ID"
              << std::setw(8)  << "PLACA"
              << std::setw(12) << "MARCA"
              << std::setw(18) << "MODELO"
              << std::setw(6)  << "ANO"
              << std::setw(12) << "DIARIA"
              << std::setw(14) << "CATEGORIA"
              << std::setw(12) << "STATUS"
              << Cor::RESET << "\n";
    Console::linha();

    frota.getFrota().paraCada([&](Carro& c) {
        Categoria* cat = frota.buscarCategoria(c.categoriaId);
        std::string nomeCat = cat ? cat->nome : "?";

        std::string statusCor = Cor::VERDE;
        if (c.status == StatusCarro::LOCADO)     statusCor = Cor::AMARELO;
        if (c.status == StatusCarro::MANUTENCAO) statusCor = Cor::VERMELHO;
        if (c.status == StatusCarro::RESERVADO)  statusCor = Cor::AZUL;

        std::cout << std::left
                  << std::setw(5)  << c.id
                  << std::setw(8)  << c.placa
                  << std::setw(12) << c.marca
                  << std::setw(18) << c.modelo
                  << std::setw(6)  << c.ano
                  << std::setw(12) << Console::formatarMoeda(c.diaria)
                  << std::setw(14) << nomeCat
                  << statusCor << std::setw(12) << c.statusStr() << Cor::RESET << "\n";
    });

    Console::pausar();
}

inline void telaCadastrarCarro(GerenciadorFrota& frota, Catalogo& catalogo) {
    Console::cabecalho("CADASTRAR VEICULO");
    Console::info("O ID e gerado automaticamente (pilha) - voce nao digita.");
    std::cout << "\n";

    // 1) Marca: selecionada da lista do catalogo (sem digitar texto livre)
    if (catalogo.getMarcas().vazia()) {
        Console::erro("Nenhuma marca no catalogo. Cadastre em [Catalogo de veiculos].");
        Console::pausar();
        return;
    }
    std::cout << Cor::CIANO << "  Marcas disponiveis:\n" << Cor::RESET;
    catalogo.getMarcas().paraCada([](Marca& m){
        std::cout << "  [" << m.id << "] " << m.nome << "\n";
    });
    int marcaId  = Console::lerInteiro("Marca (ID)");
    Marca* marca = catalogo.buscarMarca(marcaId);
    if (!marca) { Console::erro("Marca invalida."); Console::pausar(); return; }

    // 2) Modelo: apenas os da marca escolhida
    auto modelos = catalogo.modelosDaMarca(marcaId);
    if (modelos.vazio()) {
        Console::erro("Marca sem modelos. Cadastre um modelo em [Catalogo de veiculos].");
        Console::pausar();
        return;
    }
    std::cout << Cor::CIANO << "\n  Modelos de " << marca->nome << ":\n" << Cor::RESET;
    for (int i = 0; i < modelos.getTamanho(); i++)
        std::cout << "  [" << modelos[i].id << "] " << modelos[i].nome << "\n";
    int modeloId   = Console::lerInteiro("Modelo (ID)");
    Modelo* modelo = catalogo.buscarModelo(modeloId);
    if (!modelo || modelo->marcaId != marcaId) {
        Console::erro("Modelo invalido para a marca escolhida.");
        Console::pausar();
        return;
    }

    // 3) Placa: validada no padrao brasileiro (antiga ou Mercosul)
    std::string placa = Console::lerTextoValidado(
        "Placa (ABC1D23 Mercosul ou ABC-1234 antiga)",
        [](const std::string& v, std::string& e){ return Validacao::validarPlaca(v, e); }
    );
    placa = Validacao::formatarPlaca(placa);

    int ano = Console::lerInteiro("Ano");

    // 4) Categoria: define a diaria
    auto& cats = frota.getCategorias();
    std::cout << Cor::CIANO << "\n  Categorias:\n" << Cor::RESET;
    for (int i = 0; i < cats.getTamanho(); i++)
        std::cout << "  [" << cats[i].id << "] " << std::left << std::setw(16) << cats[i].nome
                  << Console::formatarMoeda(cats[i].diariaPadrao) << "/dia\n";
    int catId = Console::lerInteiro("Categoria (ID)");

    Carro* novo = frota.cadastrar(marca->nome, modelo->nome, placa, ano, catId);
    if (novo)
        Console::sucesso("Veiculo cadastrado! ID automatico: " + std::to_string(novo->id) +
                         " | Placa: " + novo->placa);
    else
        Console::erro("Falha. Verifique a categoria ou se a placa ja existe.");

    Console::pausar();
}

inline void telaBuscarVeiculo(GerenciadorFrota& frota) {
    Console::cabecalho("BUSCAR VEICULO");
    std::cout << "  [1] Por ID\n  [2] Por Placa\n";
    int op = Console::lerInteiro("Opcao");

    Carro* c = nullptr;
    if (op == 1) {
        int id = Console::lerInteiro("ID do veiculo");
        c = frota.buscarPorId(id);
    } else {
        std::string placa = Console::lerTexto("Placa");
        c = frota.buscarPorPlaca(placa);
    }

    if (!c) { Console::erro("Veiculo nao encontrado."); Console::pausar(); return; }

    Categoria* cat = frota.buscarCategoria(c->categoriaId);
    std::cout << "\n";
    Console::linha();
    std::cout << "  ID       : " << c->id                        << "\n";
    std::cout << "  Placa    : " << c->placa                     << "\n";
    std::cout << "  Marca    : " << c->marca                     << "\n";
    std::cout << "  Modelo   : " << c->modelo                    << "\n";
    std::cout << "  Ano      : " << c->ano                       << "\n";
    std::cout << "  Categoria: " << (cat ? cat->nome : "?")      << "\n";
    std::cout << "  Diaria   : " << Console::formatarMoeda(c->diaria) << "\n";
    std::cout << "  Status   : ";
    if (c->status == StatusCarro::DISPONIVEL) std::cout << Cor::VERDE;
    else std::cout << Cor::AMARELO;
    std::cout << c->statusStr() << Cor::RESET << "\n";
    Console::linha();

    Console::pausar();
}

inline void telaDisponibilidade(GerenciadorFrota& frota) {
    Console::cabecalho("DISPONIBILIDADE POR CATEGORIA");

    auto& cats = frota.getCategorias();
    std::cout << Cor::CIANO << std::left
              << std::setw(18) << "CATEGORIA"
              << std::setw(12) << "DISPONIVEIS"
              << std::setw(10) << "NA FILA"
              << Cor::RESET << "\n";
    Console::linha();

    for (int i = 0; i < cats.getTamanho(); i++) {
        int disp = frota.contarDisponiveis(cats[i].id);
        int fila  = frota.tamanhoFila(cats[i].id);
        std::string cor = disp > 0 ? Cor::VERDE : Cor::VERMELHO;

        std::cout << std::left
                  << std::setw(18) << cats[i].nome
                  << cor << std::setw(12) << disp << Cor::RESET
                  << Cor::AMARELO << std::setw(10) << fila << Cor::RESET << "\n";
    }

    auto disponiveis = Consultas::todosDisponiveis(frota);
    if (disponiveis.vazio()) {
        Console::aviso("Nenhum veiculo disponivel no momento.");
    } else {
        std::cout << Cor::CIANO << "\n  Veiculos disponiveis:\n" << Cor::RESET;
        Console::linha();
        for (int i = 0; i < disponiveis.getTamanho(); i++) {
            Carro& c = disponiveis[i];
            std::cout << "  [" << c.id << "] " << c.placa << "  "
                      << c.marca << " " << c.modelo << " (" << c.ano << ")"
                      << "  - " << Console::formatarMoeda(c.diaria) << "/dia\n";
        }
    }

    Console::pausar();
}

inline void telaManutencao(GerenciadorFrota& frota) {
    Console::cabecalho("MANUTENCAO DE VEICULO");
    int id = Console::lerInteiro("ID do veiculo");
    Carro* c = frota.buscarPorId(id);

    if (!c) { Console::erro("Veiculo nao encontrado."); Console::pausar(); return; }

    std::cout << "\n  Veiculo: " << c->marca << " " << c->modelo
              << " (" << c->placa << ") - " << c->statusStr() << "\n\n";
    std::cout << "  [1] Enviar para manutencao\n";
    std::cout << "  [2] Marcar como disponivel\n";
    std::cout << "  [0] Voltar\n";

    int op = Console::lerInteiro("Opcao");
    if (op == 1) {
        frota.marcarManutencao(id)
            ? Console::sucesso("Veiculo enviado para manutencao.")
            : Console::erro("Carro pode estar locado.");
    } else if (op == 2) {
        frota.marcarDisponivel(id);
        Console::sucesso("Veiculo marcado como disponivel.");
    }
    Console::pausar();
}

// ─── CATALOGO (marcas / modelos) ──────────────────────────────────────────────

inline void telaListarCatalogo(Catalogo& catalogo) {
    Console::cabecalho("CATALOGO DE VEICULOS - MARCAS E MODELOS");

    if (catalogo.getMarcas().vazia()) {
        Console::aviso("Catalogo vazio. Cadastre uma marca.");
        Console::pausar();
        return;
    }

    catalogo.getMarcas().paraCada([&](Marca& m){
        std::cout << Cor::CIANO << "  [" << m.id << "] " << m.nome << Cor::RESET << "\n";
        auto mods = catalogo.modelosDaMarca(m.id);
        if (mods.vazio()) {
            std::cout << Cor::CINZA << "        (sem modelos)\n" << Cor::RESET;
        } else {
            for (int i = 0; i < mods.getTamanho(); i++)
                std::cout << "        [" << mods[i].id << "] " << mods[i].nome << "\n";
        }
    });

    std::cout << "\n";
    Console::info("Marcas: " + std::to_string(catalogo.totalMarcas()) +
                  " | Modelos: " + std::to_string(catalogo.totalModelos()));
    Console::pausar();
}

inline void telaCadastrarMarca(Catalogo& catalogo) {
    Console::cabecalho("CADASTRAR MARCA");
    std::string nome = Console::lerTexto("Nome da marca");
    if (nome.empty()) { Console::erro("Nome vazio."); Console::pausar(); return; }

    Marca* m = catalogo.adicionarMarca(nome);
    if (m) Console::sucesso("Marca cadastrada! ID automatico: " + std::to_string(m->id));
    else   Console::erro("Marca ja existe ou nome invalido.");
    Console::pausar();
}

inline void telaCadastrarModelo(Catalogo& catalogo) {
    Console::cabecalho("CADASTRAR MODELO");

    if (catalogo.getMarcas().vazia()) {
        Console::erro("Cadastre uma marca antes.");
        Console::pausar();
        return;
    }
    std::cout << Cor::CIANO << "  Marcas:\n" << Cor::RESET;
    catalogo.getMarcas().paraCada([](Marca& m){
        std::cout << "  [" << m.id << "] " << m.nome << "\n";
    });
    int marcaId  = Console::lerInteiro("Marca (ID)");
    Marca* marca = catalogo.buscarMarca(marcaId);
    if (!marca) { Console::erro("Marca invalida."); Console::pausar(); return; }

    std::string nome = Console::lerTexto("Nome do modelo");
    if (nome.empty()) { Console::erro("Nome vazio."); Console::pausar(); return; }

    Modelo* mod = catalogo.adicionarModelo(marcaId, nome);
    if (mod) Console::sucesso("Modelo cadastrado em " + marca->nome +
                              "! ID automatico: " + std::to_string(mod->id));
    else     Console::erro("Modelo ja existe nessa marca ou dados invalidos.");
    Console::pausar();
}

// ─── CLIENTES ─────────────────────────────────────────────────────────────────

inline void telaListarClientes(GerenciadorClientes& clientes) {
    Console::cabecalho("CLIENTES CADASTRADOS");

    if (clientes.getClientes().vazia()) {
        Console::aviso("Nenhum cliente cadastrado.");
        Console::pausar();
        return;
    }

    std::cout << Cor::CIANO << std::left
              << std::setw(5)  << "ID"
              << std::setw(25) << "NOME"
              << std::setw(16) << "CPF"
              << std::setw(16) << "TELEFONE"
              << std::setw(28) << "EMAIL"
              << Cor::RESET << "\n";
    Console::linha();

    clientes.getClientes().paraCada([](Cliente& c) {
        std::cout << std::left
                  << std::setw(5)  << c.id
                  << std::setw(25) << c.nome
                  << std::setw(16) << c.cpf
                  << std::setw(16) << c.telefone
                  << std::setw(28) << c.email << "\n";
    });

    Console::pausar();
}

inline void telaCadastrarCliente(GerenciadorClientes& clientes) {
    Console::cabecalho("CADASTRAR CLIENTE");

    std::string nome = Console::lerTextoValidado(
        "Nome completo (nome e sobrenome)",
        [](const std::string& v, std::string& e){ return Validacao::validarNome(v, e); }
    );

    std::string cpf = Console::lerTextoValidado(
        "CPF (somente numeros ou 000.000.000-00)",
        [](const std::string& v, std::string& e){ return Validacao::validarCpf(v, e); }
    );
    cpf = Validacao::formatarCpf(cpf);

    std::string telefone = Console::lerTextoValidado(
        "Telefone (somente numeros ou (DD) 9XXXX-XXXX)",
        [](const std::string& v, std::string& e){ return Validacao::validarTelefone(v, e); }
    );
    telefone = Validacao::formatarTelefone(telefone);

    std::string email = Console::lerTextoValidado(
        "Email (ex: nome@dominio.com)",
        [](const std::string& v, std::string& e){ return Validacao::validarEmail(v, e); }
    );

    Cliente* novo = clientes.cadastrar(nome, cpf, telefone, email);
    if (novo)
        Console::sucesso("Cliente cadastrado! ID: " + std::to_string(novo->id));
    else
        Console::erro("CPF ja cadastrado no sistema.");

    Console::pausar();
}

inline void telaBuscarCliente(GerenciadorClientes& clientes) {
    Console::cabecalho("BUSCAR CLIENTE");
    std::cout << "  [1] Por ID\n  [2] Por CPF\n";
    int op = Console::lerInteiro("Opcao");

    Cliente* c = nullptr;
    if (op == 1) {
        int id = Console::lerInteiro("ID do cliente");
        c = clientes.buscarPorId(id);
    } else {
        std::string cpf = Console::lerTexto("CPF");
        cpf = Validacao::formatarCpf(cpf);
        c = clientes.buscarPorCpf(cpf);
    }

    if (!c) { Console::erro("Cliente nao encontrado."); Console::pausar(); return; }

    std::cout << "\n";
    Console::linha();
    std::cout << "  ID      : " << c->id       << "\n";
    std::cout << "  Nome    : " << c->nome      << "\n";
    std::cout << "  CPF     : " << c->cpf       << "\n";
    std::cout << "  Telefone: " << c->telefone  << "\n";
    std::cout << "  Email   : " << c->email     << "\n";
    Console::linha();

    Console::pausar();
}

inline void telaRemoverCliente(GerenciadorClientes& clientes,
                                GerenciadorLocacoes& locacoes) {
    Console::cabecalho("REMOVER CLIENTE");

    int id = Console::lerInteiro("ID do cliente a remover");
    Cliente* c = clientes.buscarPorId(id);

    if (!c) {
        Console::erro("Cliente nao encontrado.");
        Console::pausar();
        return;
    }

    // Impede remocao se o cliente tiver locacao ativa.
    // temHistorico controla se o id pode ser reaproveitado (so se nao houver
    // nenhuma locacao apontando para ele, para nao corromper o historico).
    bool temAtiva = false, temHistorico = false;
    locacoes.getLocacoes().paraCada([&](Locacao& l) {
        if (l.idCliente == id) {
            temHistorico = true;
            if (l.status == StatusLocacao::ATIVA) temAtiva = true;
        }
    });

    if (temAtiva) {
        Console::erro("Nao e possivel remover: cliente possui locacao ativa.");
        Console::pausar();
        return;
    }

    std::cout << "\n";
    Console::linha();
    std::cout << "  ID      : " << c->id       << "\n";
    std::cout << "  Nome    : " << c->nome      << "\n";
    std::cout << "  CPF     : " << c->cpf       << "\n";
    std::cout << "  Telefone: " << c->telefone  << "\n";
    std::cout << "  Email   : " << c->email     << "\n";
    Console::linha();
    Console::aviso("Esta acao nao pode ser desfeita.");

    int confirmar = Console::lerInteiro("Confirmar remocao? [1=Sim / 0=Nao]");
    if (confirmar == 1) {
        clientes.remover(id, !temHistorico)
            ? Console::sucesso("Cliente removido com sucesso.")
            : Console::erro("Falha ao remover cliente.");
    } else {
        Console::aviso("Operacao cancelada.");
    }

    Console::pausar();
}

// ─── LOCACOES ─────────────────────────────────────────────────────────────────

inline void telaListarLocacoes(GerenciadorLocacoes& locacoes,
                               GerenciadorClientes& clientes,
                               GerenciadorFrota& frota) {
    Console::cabecalho("TODAS AS LOCACOES");

    if (locacoes.getLocacoes().vazia()) {
        Console::aviso("Nenhuma locacao registrada.");
        Console::pausar();
        return;
    }

    imprimirCabecalhoLocacao();
    locacoes.getLocacoes().paraCada([&](Locacao& l) {
        imprimirLinhaLocacao(l, clientes, frota);
    });

    Console::pausar();
}

inline void telaLocacoesAtivas(GerenciadorLocacoes& locacoes,
                               GerenciadorClientes& clientes,
                               GerenciadorFrota& frota) {
    Console::cabecalho("LOCACOES ATIVAS - ordenadas por data de inicio");

    auto ativas = Consultas::locacoesAtivasOrdenadas(locacoes);
    if (ativas.vazio()) {
        Console::aviso("Nenhuma locacao ativa.");
        Console::pausar();
        return;
    }

    imprimirCabecalhoLocacao();
    for (int i = 0; i < ativas.getTamanho(); i++)
        imprimirLinhaLocacao(ativas[i], clientes, frota);

    Console::info("\n  Vermelho = em atraso");
    Console::pausar();
}

inline void telaLocacoesEmAtraso(GerenciadorLocacoes& locacoes,
                                  GerenciadorClientes& clientes,
                                  GerenciadorFrota& frota) {
    Console::cabecalho("LOCACOES EM ATRASO");

    auto atrasadas = Consultas::locacoesEmAtraso(locacoes);
    if (atrasadas.vazio()) {
        Console::sucesso("Nenhuma locacao em atraso.");
        Console::pausar();
        return;
    }

    imprimirCabecalhoLocacao();
    for (int i = 0; i < atrasadas.getTamanho(); i++)
        imprimirLinhaLocacao(atrasadas[i], clientes, frota);

    Console::pausar();
}

inline void telaNovaLocacao(GerenciadorLocacoes& locacoes,
                            GerenciadorClientes& clientes,
                            GerenciadorFrota& frota) {
    Console::cabecalho("NOVA LOCACAO");

    int idCliente = Console::lerInteiro("ID do cliente");
    Cliente* cli  = clientes.buscarPorId(idCliente);
    if (!cli) { Console::erro("Cliente nao encontrado."); Console::pausar(); return; }
    Console::info("Cliente: " + cli->nome + " | CPF: " + cli->cpf);

    int idCarro  = Console::lerInteiro("ID do veiculo");
    Carro* car   = frota.buscarPorId(idCarro);
    if (!car) { Console::erro("Veiculo nao encontrado."); Console::pausar(); return; }

    if (car->status != StatusCarro::DISPONIVEL) {
        Console::erro("Veiculo nao disponivel (" + car->statusStr() + ").");
        int entrar = Console::lerInteiro("Entrar na fila de espera? [1=Sim / 0=Nao]");
        if (entrar == 1) {
            frota.entrarFilaEspera(idCliente, car->categoriaId);
            Console::sucesso("Cliente adicionado a fila de espera da categoria.");
        }
        Console::pausar();
        return;
    }
    Console::info("Veiculo: " + car->marca + " " + car->modelo +
                  " | Placa: " + car->placa +
                  " | " + Console::formatarMoeda(car->diaria) + "/dia");

    int dias = Console::lerInteiro("Quantidade de dias");
    if (dias <= 0) { Console::erro("Numero de dias invalido."); Console::pausar(); return; }

    double valorEst  = dias * car->diaria;
    time_t agora     = std::time(nullptr);
    time_t devolPrev = agora + dias * 86400;

    std::cout << "\n";
    Console::linha();
    std::cout << "  Retirada prevista : " << Console::formatarData(agora)     << "\n";
    std::cout << "  Devolucao prevista: " << Console::formatarData(devolPrev) << "\n";
    std::cout << "  Valor estimado    : " << Cor::VERDE
              << Console::formatarMoeda(valorEst) << Cor::RESET << "\n";
    Console::linha();

    int confirmar = Console::lerInteiro("Confirmar locacao? [1=Sim / 0=Nao]");
    if (confirmar == 1) {
        Locacao* loc = locacoes.registrarLocacao(idCliente, idCarro, dias);
        if (loc)
            Console::sucesso("Locacao #" + std::to_string(loc->id) + " registrada.");
        else
            Console::erro("Falha. Cliente pode ja ter uma locacao ativa.");
    } else {
        Console::aviso("Operacao cancelada.");
    }

    Console::pausar();
}

inline void telaDevolucao(GerenciadorLocacoes& locacoes,
                          GerenciadorClientes& clientes,
                          GerenciadorFrota& frota) {
    Console::cabecalho("REGISTRAR DEVOLUCAO");

    int id = Console::lerInteiro("ID da locacao");
    Locacao* loc = locacoes.buscarPorId(id);

    if (!loc) { Console::erro("Locacao nao encontrada."); Console::pausar(); return; }
    if (loc->status != StatusLocacao::ATIVA) {
        Console::erro("Locacao nao esta ativa (status: " + loc->statusStr() + ").");
        Console::pausar();
        return;
    }

    Cliente* cli = clientes.buscarPorId(loc->idCliente);
    Carro*   car = frota.buscarPorId(loc->idCarro);
    time_t agora = std::time(nullptr);
    int diasReais = loc->diasCobrados(agora);
    double valorFinal = car ? loc->valorCobrado(car->diaria, agora) : loc->valorTotal;

    std::cout << "\n";
    Console::linha();
    if (cli) std::cout << "  Cliente   : " << cli->nome << "\n";
    if (car) std::cout << "  Veiculo   : " << car->marca << " " << car->modelo
                       << " (" << car->placa << ")\n";
    std::cout << "  Inicio    : " << Console::formatarData(loc->dataInicio)      << "\n";
    std::cout << "  Prev. fim : " << Console::formatarData(loc->dataFimPrevista) << "\n";
    std::cout << "  Dias reais: " << diasReais << "\n";

    if (agora > loc->dataFimPrevista)
        std::cout << Cor::VERMELHO << "  ATENCAO: devolucao em atraso!\n" << Cor::RESET;

    std::cout << "  Valor final: " << Cor::VERDE
              << Console::formatarMoeda(valorFinal) << Cor::RESET << "\n";
    Console::linha();

    int confirmar = Console::lerInteiro("Confirmar devolucao? [1=Sim / 0=Nao]");
    if (confirmar == 1) {
        locacoes.registrarDevolucao(id)
            ? Console::sucesso("Devolucao registrada com sucesso.")
            : Console::erro("Erro ao registrar devolucao.");
    }

    Console::pausar();
}

// ─── HISTORICO / CONSULTAS ────────────────────────────────────────────────────

inline void telaHistoricoCliente(GerenciadorLocacoes& locacoes,
                                  GerenciadorClientes& clientes,
                                  GerenciadorFrota& frota) {
    Console::cabecalho("HISTORICO POR CLIENTE");

    int idCliente = Console::lerInteiro("ID do cliente");
    Cliente* cli  = clientes.buscarPorId(idCliente);
    if (!cli) { Console::erro("Cliente nao encontrado."); Console::pausar(); return; }

    std::cout << "\n  Cliente: " << Cor::CIANO << cli->nome << Cor::RESET
              << " | CPF: " << cli->cpf << "\n\n";

    auto hist = Consultas::historicoPorCliente(idCliente, locacoes);
    if (hist.vazio()) {
        Console::aviso("Nenhuma locacao encontrada para este cliente.");
        Console::pausar();
        return;
    }

    imprimirCabecalhoLocacao();
    double totalGasto = 0;
    for (int i = 0; i < hist.getTamanho(); i++) {
        imprimirLinhaLocacao(hist[i], clientes, frota);
        if (hist[i].status == StatusLocacao::CONCLUIDA)
            totalGasto += hist[i].valorTotal;
    }

    std::cout << "\n  Total locacoes : " << hist.getTamanho() << "\n";
    std::cout << "  Total gasto    : " << Cor::VERDE
              << Console::formatarMoeda(totalGasto) << Cor::RESET << "\n";

    Console::pausar();
}

inline void telaHistoricoVeiculo(GerenciadorLocacoes& locacoes,
                                  GerenciadorClientes& clientes,
                                  GerenciadorFrota& frota) {
    Console::cabecalho("HISTORICO POR VEICULO");
    std::cout << "  [1] Buscar por ID\n  [2] Buscar por Placa\n";
    int op = Console::lerInteiro("Opcao");

    Carro* car = nullptr;
    if (op == 1) {
        int id = Console::lerInteiro("ID do veiculo");
        car = frota.buscarPorId(id);
    } else {
        std::string placa = Console::lerTexto("Placa");
        car = frota.buscarPorPlaca(placa);
    }

    if (!car) { Console::erro("Veiculo nao encontrado."); Console::pausar(); return; }

    std::cout << "\n  Veiculo: " << Cor::CIANO
              << car->marca << " " << car->modelo << Cor::RESET
              << " | Placa: " << car->placa << "\n\n";

    auto hist = Consultas::historicoPorVeiculo(car->id, locacoes);
    if (hist.vazio()) {
        Console::aviso("Nenhuma locacao para este veiculo.");
        Console::pausar();
        return;
    }

    imprimirCabecalhoLocacao();
    double totalReceita = 0;
    for (int i = 0; i < hist.getTamanho(); i++) {
        imprimirLinhaLocacao(hist[i], clientes, frota);
        if (hist[i].status == StatusLocacao::CONCLUIDA)
            totalReceita += hist[i].valorTotal;
    }

    std::cout << "\n  Total locacoes : " << hist.getTamanho() << "\n";
    std::cout << "  Receita gerada : " << Cor::VERDE
              << Console::formatarMoeda(totalReceita) << Cor::RESET << "\n";

    Console::pausar();
}

inline void telaBuscaBinaria(GerenciadorLocacoes& locacoes,
                              GerenciadorClientes& clientes,
                              GerenciadorFrota& frota) {
    Console::cabecalho("BUSCA BINARIA - LOCACAO POR ID");
    Console::info("(Demonstra busca binaria em vetor ordenado por ID)");

    VetorDinamico<Locacao> vetor;
    locacoes.getLocacoes().paraCada([&](Locacao& l) { vetor.adicionar(l); });

    if (vetor.vazio()) {
        Console::aviso("Nenhuma locacao cadastrada.");
        Console::pausar();
        return;
    }

    int idAlvo = Console::lerInteiro("ID da locacao a buscar");
    int idx    = Consultas::buscarLocacaoPorId(vetor, idAlvo);

    if (idx == -1) {
        Console::erro("Locacao #" + std::to_string(idAlvo) + " nao encontrada.");
    } else {
        Console::sucesso("Encontrado na posicao " + std::to_string(idx) +
                         " do vetor ordenado:");
        std::cout << "\n";
        imprimirCabecalhoLocacao();
        imprimirLinhaLocacao(vetor[idx], clientes, frota);
    }

    Console::pausar();
}

// ─── DASHBOARD ────────────────────────────────────────────────────────────────

inline void telaDashboard(GerenciadorFrota& frota,
                          GerenciadorClientes& clientes,
                          GerenciadorLocacoes& locacoes) {
    Console::cabecalho("DASHBOARD");

    int totalCarros = 0, disponiveis = 0, locados = 0, manutencao = 0;
    frota.getFrota().paraCada([&](Carro& c) {
        totalCarros++;
        if (c.status == StatusCarro::DISPONIVEL)  disponiveis++;
        if (c.status == StatusCarro::LOCADO)      locados++;
        if (c.status == StatusCarro::MANUTENCAO)  manutencao++;
    });

    auto emAtraso = Consultas::locacoesEmAtraso(locacoes);

    std::cout << Cor::CIANO << "  [ FROTA ]\n" << Cor::RESET;
    std::cout << "  Total de veiculos : " << totalCarros  << "\n";
    std::cout << Cor::VERDE   << "  Disponiveis       : " << disponiveis << Cor::RESET << "\n";
    std::cout << Cor::AMARELO << "  Locados           : " << locados     << Cor::RESET << "\n";
    std::cout << Cor::VERMELHO<< "  Em manutencao     : " << manutencao  << Cor::RESET << "\n";
    std::cout << "  Hash fator carga  : "
              << std::fixed << std::setprecision(2) << frota.fatorCargaHash() << "\n\n";

    std::cout << Cor::CIANO << "  [ CLIENTES ]\n" << Cor::RESET;
    std::cout << "  Total cadastrados : " << clientes.total() << "\n\n";

    std::cout << Cor::CIANO << "  [ FINANCEIRO ]\n" << Cor::RESET;
    std::cout << "  Locacoes ativas   : " << locacoes.locacoesAtivas() << "\n";
    if (!emAtraso.vazio())
        std::cout << Cor::VERMELHO << "  Em atraso         : "
                  << emAtraso.getTamanho() << Cor::RESET << "\n";
    std::cout << Cor::AMARELO << "  Receita prevista  : "
              << Console::formatarMoeda(locacoes.receitaPrevista()) << Cor::RESET << "\n";
    std::cout << Cor::VERDE   << "  Receita realizada : "
              << Console::formatarMoeda(locacoes.receitaTotal())    << Cor::RESET << "\n";

    Console::pausar();
}
