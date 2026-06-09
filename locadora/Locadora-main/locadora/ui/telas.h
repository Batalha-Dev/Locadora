#pragma once
#include "console.h"
#include "tabela.h"
#include "validacao.h"
#include "../servicos/gerenciador_frota.h"
#include "../servicos/gerenciador_clientes.h"
#include "../servicos/gerenciador_locacoes.h"
#include "../servicos/consultas.h"
#include "../servicos/catalogo.h"
#include <sstream>
#include <iomanip>

// ─── Helpers de impressao (tabela estilizada) ────────────────────────────────

// Cria uma grade ja com as colunas padrao de locacao.
inline Tabela::Grade novaGradeLocacoes(const std::string& titulo) {
    Tabela::Grade g(titulo);
    g.colunas({"ID", "CLIENTE", "VEICULO", "INICIO", "PREV.FIM", "VALOR", "STATUS"});
    return g;
}

// Converte uma locacao na linha de celulas correspondente (com cor de status).
inline std::vector<Tabela::Celula> celulasLocacao(const Locacao& l,
                                                  GerenciadorClientes& clientes,
                                                  GerenciadorFrota& frota) {
    Cliente* cli = clientes.buscarPorId(l.idCliente);
    Carro*   car = frota.buscarPorId(l.idCarro);

    std::string nomeCli = cli ? cli->nome : "?";
    std::string nomeVei = car ? (car->marca + " " + car->modelo) : "?";
    if (nomeCli.size() > 18) nomeCli = nomeCli.substr(0, 17) + ".";
    if (nomeVei.size() > 16) nomeVei = nomeVei.substr(0, 15) + ".";

    time_t agora = std::time(nullptr);
    bool emAtraso = (l.status == StatusLocacao::ATIVA && agora > l.dataFimPrevista);

    std::string corStatus = Tabela::COR_TEXTO;
    if (l.status == StatusLocacao::ATIVA)     corStatus = emAtraso ? Cor::VERMELHO : Cor::VERDE;
    if (l.status == StatusLocacao::CANCELADA) corStatus = Cor::CINZA;

    return {
        Tabela::Celula(std::to_string(l.id)),
        Tabela::Celula(nomeCli),
        Tabela::Celula(nomeVei),
        Tabela::Celula(Console::formatarData(l.dataInicio)),
        Tabela::Celula(Console::formatarData(l.dataFimPrevista)),
        Tabela::Celula(Console::formatarMoeda(l.valorTotal), Cor::VERDE),
        Tabela::Celula(emAtraso ? "ATRASO" : l.statusStr(), corStatus)
    };
}

// ─── FROTA ───────────────────────────────────────────────────────────────────

inline void telaListarCarros(GerenciadorFrota& frota) {
    Console::limpar();

    if (frota.getFrota().vazia()) {
        Console::cabecalho("FROTA DE VEICULOS");
        Console::aviso("Nenhum veiculo cadastrado.");
        Console::pausar();
        return;
    }

    Tabela::Grade g("FROTA DE VEICULOS");
    g.colunas({"ID", "PLACA", "MARCA", "MODELO", "ANO", "DIARIA", "CATEGORIA", "STATUS"});

    frota.getFrota().paraCada([&](Carro& c) {
        Categoria* cat = frota.buscarCategoria(c.categoriaId);
        std::string nomeCat = cat ? cat->nome : "?";

        std::string statusCor = Cor::VERDE;
        if (c.status == StatusCarro::LOCADO)     statusCor = Cor::AMARELO;
        if (c.status == StatusCarro::MANUTENCAO) statusCor = Cor::VERMELHO;
        if (c.status == StatusCarro::RESERVADO)  statusCor = Cor::AZUL;

        g.adicionar({
            Tabela::Celula(std::to_string(c.id)),
            Tabela::Celula(c.placa),
            Tabela::Celula(c.marca),
            Tabela::Celula(c.modelo),
            Tabela::Celula(std::to_string(c.ano)),
            Tabela::Celula(Console::formatarMoeda(c.diaria), Cor::VERDE),
            Tabela::Celula(nomeCat),
            Tabela::Celula(c.statusStr(), statusCor)
        });
    });

    g.desenhar();
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

    // Ano permitido: somente o ano atual ou os 2 anos anteriores.
    int aAtual = Console::anoAtual();
    int anoMin = aAtual - 2;
    Console::info("Ano permitido: " + std::to_string(anoMin) + ", " +
                  std::to_string(aAtual - 1) + " ou " + std::to_string(aAtual) +
                  " (ano atual ou ate 2 anos anteriores).");
    int ano;
    while (true) {
        ano = Console::lerInteiro("Ano (" + std::to_string(anoMin) + "-" +
                                  std::to_string(aAtual) + ")");
        if (ano >= anoMin && ano <= aAtual) break;
        Console::erro("Ano invalido. Informe " + std::to_string(anoMin) + ", " +
                      std::to_string(aAtual - 1) + " ou " + std::to_string(aAtual) + ".");
    }

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
    std::cout << "  [1] Por ID\n  [2] Por Placa\n  [0] Voltar\n";

    // Aceita apenas 1 (ID) ou 2 (Placa); 0 volta. Qualquer outro valor e rejeitado.
    int op;
    while (true) {
        op = Console::lerInteiro("Opcao");
        if (op == 0) return;
        if (op == 1 || op == 2) break;
        Console::erro("Opcao invalida. Escolha 1 (ID), 2 (Placa) ou 0 (Voltar).");
    }

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

    std::string statusCor = Cor::VERDE;
    if (c->status == StatusCarro::LOCADO)     statusCor = Cor::AMARELO;
    if (c->status == StatusCarro::MANUTENCAO) statusCor = Cor::VERMELHO;
    if (c->status == StatusCarro::RESERVADO)  statusCor = Cor::AZUL;

    std::cout << "\n";
    Tabela::Grade ficha("VEICULO #" + std::to_string(c->id), false);
    ficha.colunas({"CAMPO", "VALOR"});
    ficha.adicionar({Tabela::Celula("Placa"),     Tabela::Celula(c->placa)});
    ficha.adicionar({Tabela::Celula("Marca"),     Tabela::Celula(c->marca)});
    ficha.adicionar({Tabela::Celula("Modelo"),    Tabela::Celula(c->modelo)});
    ficha.adicionar({Tabela::Celula("Ano"),       Tabela::Celula(std::to_string(c->ano))});
    ficha.adicionar({Tabela::Celula("Categoria"), Tabela::Celula(cat ? cat->nome : "?")});
    ficha.adicionar({Tabela::Celula("Diaria"),    Tabela::Celula(Console::formatarMoeda(c->diaria), Cor::VERDE)});
    ficha.adicionar({Tabela::Celula("Status"),    Tabela::Celula(c->statusStr(), statusCor)});
    ficha.desenhar();

    Console::pausar();
}

inline void telaDisponibilidade(GerenciadorFrota& frota) {
    Console::limpar();

    auto& cats = frota.getCategorias();
    Tabela::Grade resumo("DISPONIBILIDADE POR CATEGORIA");
    resumo.colunas({"CATEGORIA", "DISPONIVEIS", "NA FILA"});

    for (int i = 0; i < cats.getTamanho(); i++) {
        int disp = frota.contarDisponiveis(cats[i].id);
        int fila = frota.tamanhoFila(cats[i].id);
        resumo.adicionar({
            Tabela::Celula(cats[i].nome),
            Tabela::Celula(std::to_string(disp), disp > 0 ? Cor::VERDE : Cor::VERMELHO),
            Tabela::Celula(std::to_string(fila), Cor::AMARELO)
        });
    }
    resumo.desenhar();

    auto disponiveis = Consultas::todosDisponiveis(frota);
    if (disponiveis.vazio()) {
        Console::aviso("Nenhum veiculo disponivel no momento.");
    } else {
        std::cout << "\n";
        Tabela::Grade g("VEICULOS DISPONIVEIS");
        g.colunas({"ID", "PLACA", "MARCA", "MODELO", "ANO", "DIARIA"});
        for (int i = 0; i < disponiveis.getTamanho(); i++) {
            Carro& c = disponiveis[i];
            g.adicionar({
                Tabela::Celula(std::to_string(c.id)),
                Tabela::Celula(c.placa),
                Tabela::Celula(c.marca),
                Tabela::Celula(c.modelo),
                Tabela::Celula(std::to_string(c.ano)),
                Tabela::Celula(Console::formatarMoeda(c.diaria), Cor::VERDE)
            });
        }
        g.desenhar();
    }

    Console::pausar();
}

inline void telaManutencao(GerenciadorFrota& frota) {
    Console::cabecalho("MANUTENCAO DE VEICULO");
    int id = Console::lerInteiro("ID do veiculo");
    Carro* c = frota.buscarPorId(id);

    if (!c) { Console::erro("Veiculo nao encontrado."); Console::pausar(); return; }

    std::cout << "\n  Veiculo: " << c->marca << " " << c->modelo
              << " (" << c->placa << ")\n";

    // Le uma opcao restrita a {1, 0}: 1 confirma a acao, 0 volta.
    auto lerAcao = [&]() -> int {
        while (true) {
            int op = Console::lerInteiro("Opcao");
            if (op == 0 || op == 1) return op;
            Console::erro("Opcao invalida. Escolha 1 ou 0 (Voltar).");
        }
    };

    // A tela so oferece a acao que faz sentido para o status atual.
    if (c->status == StatusCarro::MANUTENCAO) {
        Console::aviso("Este veiculo JA esta em manutencao.");
        std::cout << "\n  [1] Retirar da manutencao (deixar disponivel)\n";
        std::cout << "  [0] Voltar\n";
        if (lerAcao() == 1) {
            frota.marcarDisponivel(id);
            Console::sucesso("Veiculo retirado da manutencao. Agora esta disponivel.");
        }
    }
    else if (c->status == StatusCarro::DISPONIVEL) {
        Console::info("Status atual: disponivel.");
        std::cout << "\n  [1] Enviar para manutencao\n";
        std::cout << "  [0] Voltar\n";
        if (lerAcao() == 1) {
            frota.marcarManutencao(id)
                ? Console::sucesso("Veiculo enviado para manutencao.")
                : Console::erro("Nao foi possivel enviar para manutencao.");
        }
    }
    else {
        // LOCADO ou RESERVADO: nao ha acao de manutencao aplicavel agora.
        Console::aviso("Veiculo esta " + c->statusStr() +
                       ". Nao e possivel alterar a manutencao neste momento.");
        Console::info("Finalize a locacao/reserva antes de enviar para manutencao.");
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

// Ficha do cliente no estilo emoldurado (mesmo visual do dashboard).
inline Tabela::Grade fichaCliente(const Cliente& c) {
    Tabela::Grade g("CLIENTE #" + std::to_string(c.id), false);
    g.colunas({"CAMPO", "VALOR"});
    g.adicionar({Tabela::Celula("Nome"),     Tabela::Celula(c.nome)});
    g.adicionar({Tabela::Celula("CPF"),      Tabela::Celula(c.cpf)});
    g.adicionar({Tabela::Celula("Telefone"), Tabela::Celula(c.telefone)});
    g.adicionar({Tabela::Celula("Email"),    Tabela::Celula(c.email)});
    return g;
}

inline void telaListarClientes(GerenciadorClientes& clientes) {
    Console::limpar();

    if (clientes.getClientes().vazia()) {
        Console::cabecalho("CLIENTES CADASTRADOS");
        Console::aviso("Nenhum cliente cadastrado.");
        Console::pausar();
        return;
    }

    Tabela::Grade g("CLIENTES CADASTRADOS");
    g.colunas({"ID", "NOME", "CPF", "TELEFONE", "EMAIL"});

    clientes.getClientes().paraCada([&](Cliente& c) {
        g.adicionar({
            Tabela::Celula(std::to_string(c.id)),
            Tabela::Celula(c.nome),
            Tabela::Celula(c.cpf),
            Tabela::Celula(c.telefone),
            Tabela::Celula(c.email)
        });
    });

    g.desenhar();
    Console::pausar();
}

inline void telaCadastrarCliente(GerenciadorClientes& clientes) {
    Console::cabecalho("CADASTRAR CLIENTE");

    // Nome e sobrenome em campos separados. Cada parte aceita letras (com
    // acentos de qualquer idioma), espaco, hifen e apostrofo - sem numeros/simbolos.
    std::string nome = Console::lerTextoValidado(
        "Nome",
        [](const std::string& v, std::string& e){ return Validacao::validarNomeParte(v, "Nome", e); }
    );
    if (nome.empty()) return;   // EOF

    std::string sobrenome = Console::lerTextoValidado(
        "Sobrenome",
        [](const std::string& v, std::string& e){ return Validacao::validarNomeParte(v, "Sobrenome", e); }
    );
    if (sobrenome.empty()) return;

    auto aparar = [](const std::string& s) {
        size_t a = s.find_first_not_of(" \t");
        size_t b = s.find_last_not_of(" \t");
        return (a == std::string::npos) ? std::string() : s.substr(a, b - a + 1);
    };
    std::string nomeCompleto = aparar(nome) + " " + aparar(sobrenome);

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

    Cliente* novo = clientes.cadastrar(nomeCompleto, cpf, telefone, email);
    if (novo)
        Console::sucesso("Cliente cadastrado! ID: " + std::to_string(novo->id));
    else
        Console::erro("CPF ja cadastrado no sistema.");

    Console::pausar();
}

inline void telaBuscarCliente(GerenciadorClientes& clientes) {
    Console::cabecalho("BUSCAR CLIENTE");
    std::cout << "  [1] Por ID\n  [2] Por CPF\n  [0] Voltar\n";

    // Aceita apenas 1 (ID) ou 2 (CPF); 0 volta. Qualquer outro valor e rejeitado.
    int op;
    while (true) {
        op = Console::lerInteiro("Opcao");
        if (op == 0) return;
        if (op == 1 || op == 2) break;
        Console::erro("Opcao invalida. Escolha 1 (ID), 2 (CPF) ou 0 (Voltar).");
    }

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
    fichaCliente(*c).desenhar();

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
    fichaCliente(*c).desenhar();
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
    Console::limpar();

    if (locacoes.getLocacoes().vazia()) {
        Console::cabecalho("TODAS AS LOCACOES");
        Console::aviso("Nenhuma locacao registrada.");
        Console::pausar();
        return;
    }

    Tabela::Grade g = novaGradeLocacoes("TODAS AS LOCACOES");
    locacoes.getLocacoes().paraCada([&](Locacao& l) {
        g.adicionar(celulasLocacao(l, clientes, frota));
    });

    g.desenhar();
    Console::pausar();
}

inline void telaLocacoesAtivas(GerenciadorLocacoes& locacoes,
                               GerenciadorClientes& clientes,
                               GerenciadorFrota& frota) {
    Console::limpar();

    auto ativas = Consultas::locacoesAtivasOrdenadas(locacoes);
    if (ativas.vazio()) {
        Console::cabecalho("LOCACOES ATIVAS");
        Console::aviso("Nenhuma locacao ativa.");
        Console::pausar();
        return;
    }

    Tabela::Grade g = novaGradeLocacoes("LOCACOES ATIVAS (ordenadas por data de inicio)");
    for (int i = 0; i < ativas.getTamanho(); i++)
        g.adicionar(celulasLocacao(ativas[i], clientes, frota));

    g.desenhar();
    Console::info("Vermelho = em atraso");
    Console::pausar();
}

inline void telaLocacoesEmAtraso(GerenciadorLocacoes& locacoes,
                                  GerenciadorClientes& clientes,
                                  GerenciadorFrota& frota) {
    Console::limpar();

    auto atrasadas = Consultas::locacoesEmAtraso(locacoes);
    if (atrasadas.vazio()) {
        Console::cabecalho("LOCACOES EM ATRASO");
        Console::sucesso("Nenhuma locacao em atraso.");
        Console::pausar();
        return;
    }

    Tabela::Grade g = novaGradeLocacoes("LOCACOES EM ATRASO");
    for (int i = 0; i < atrasadas.getTamanho(); i++)
        g.adicionar(celulasLocacao(atrasadas[i], clientes, frota));

    g.desenhar();
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

    Tabela::Grade g = novaGradeLocacoes("HISTORICO - " + cli->nome);
    double totalGasto = 0;
    for (int i = 0; i < hist.getTamanho(); i++) {
        g.adicionar(celulasLocacao(hist[i], clientes, frota));
        if (hist[i].status == StatusLocacao::CONCLUIDA)
            totalGasto += hist[i].valorTotal;
    }
    g.desenhar();

    std::cout << "  Total locacoes : " << hist.getTamanho() << "\n";
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

    Tabela::Grade g = novaGradeLocacoes("HISTORICO - " + car->marca + " " + car->modelo +
                                        " (" + car->placa + ")");
    double totalReceita = 0;
    for (int i = 0; i < hist.getTamanho(); i++) {
        g.adicionar(celulasLocacao(hist[i], clientes, frota));
        if (hist[i].status == StatusLocacao::CONCLUIDA)
            totalReceita += hist[i].valorTotal;
    }
    g.desenhar();

    std::cout << "  Total locacoes : " << hist.getTamanho() << "\n";
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
        Tabela::Grade g = novaGradeLocacoes("LOCACAO #" + std::to_string(idAlvo));
        g.adicionar(celulasLocacao(vetor[idx], clientes, frota));
        g.desenhar();
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

    std::ostringstream fc;
    fc << std::fixed << std::setprecision(2) << frota.fatorCargaHash();

    // Painel FROTA (indicador/valor, sem numeracao de linha)
    Tabela::Grade gFrota("FROTA", false);
    gFrota.colunas({"INDICADOR", "VALOR"});
    gFrota.adicionar({Tabela::Celula("Total de veiculos"), Tabela::Celula(std::to_string(totalCarros))});
    gFrota.adicionar({Tabela::Celula("Disponiveis"),       Tabela::Celula(std::to_string(disponiveis), Cor::VERDE)});
    gFrota.adicionar({Tabela::Celula("Locados"),           Tabela::Celula(std::to_string(locados), Cor::AMARELO)});
    gFrota.adicionar({Tabela::Celula("Em manutencao"),     Tabela::Celula(std::to_string(manutencao), Cor::VERMELHO)});
    gFrota.adicionar({Tabela::Celula("Hash - fator carga"),Tabela::Celula(fc.str())});

    // Painel CLIENTES
    Tabela::Grade gCli("CLIENTES", false);
    gCli.colunas({"INDICADOR", "VALOR"});
    gCli.adicionar({Tabela::Celula("Total cadastrados"), Tabela::Celula(std::to_string(clientes.total()))});

    // Painel FINANCEIRO
    Tabela::Grade gFin("FINANCEIRO", false);
    gFin.colunas({"INDICADOR", "VALOR"});
    gFin.adicionar({Tabela::Celula("Locacoes ativas"), Tabela::Celula(std::to_string(locacoes.locacoesAtivas()))});
    gFin.adicionar({Tabela::Celula("Em atraso"),
                    Tabela::Celula(std::to_string(emAtraso.getTamanho()),
                                   emAtraso.vazio() ? Cor::VERDE : Cor::VERMELHO)});
    gFin.adicionar({Tabela::Celula("Receita prevista"),
                    Tabela::Celula(Console::formatarMoeda(locacoes.receitaPrevista()), Cor::AMARELO)});
    gFin.adicionar({Tabela::Celula("Receita realizada"),
                    Tabela::Celula(Console::formatarMoeda(locacoes.receitaTotal()), Cor::VERDE)});

    // Os tres paineis na mesma faixa horizontal.
    Tabela::desenharLadoALado({gFrota, gCli, gFin});

    Console::pausar();
}
