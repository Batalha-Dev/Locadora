#include <iostream>
#include "servicos/gerenciador_frota.h"
#include "servicos/gerenciador_clientes.h"
#include "servicos/gerenciador_locacoes.h"
#include "servicos/persistencia.h"
#include "servicos/catalogo.h"
#include "ui/console.h"
#include "ui/telas.h"

#ifdef _WIN32
#include <windows.h>
inline void habilitarCoresWindows() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}
#else
inline void habilitarCoresWindows() {}
#endif

// ─── Persistência ────────────────────────────────────────────────────────────

void salvarTudo(GerenciadorFrota& frota,
                GerenciadorClientes& clientes,
                GerenciadorLocacoes& locacoes,
                Catalogo& catalogo) {
    bool ok = Persistencia::salvarCarros(frota.getFrota())
           && Persistencia::salvarClientes(clientes.getClientes())
           && Persistencia::salvarLocacoes(locacoes.getLocacoes())
           && Persistencia::salvarMarcas(catalogo.getMarcas())
           && Persistencia::salvarModelos(catalogo.getModelos());
    ok ? Console::sucesso("Dados salvos em dados/*.csv")
       : Console::erro("Falha ao salvar dados.");
    Console::pausar();
}

void carregarTudo(GerenciadorFrota& frota,
                  GerenciadorClientes& clientes,
                  GerenciadorLocacoes& locacoes,
                  Catalogo& catalogo) {
    // Ordem importa: frota primeiro (locações referenciam carros)
    int idC = 1, idCl = 1, idL = 1;

    ListaEncadeada<Carro>   carrosTemp;
    ListaEncadeada<Cliente> clientesTemp;
    ListaEncadeada<Locacao> locacoesTemp;

    if (Persistencia::carregarCarros(carrosTemp, idC)) {
        carrosTemp.paraCada([&](Carro& c){ frota.inserirExistente(c); });
        Console::info("Frota carregada.");
    }
    if (Persistencia::carregarClientes(clientesTemp, idCl)) {
        clientesTemp.paraCada([&](Cliente& c){ clientes.inserirExistente(c); });
        Console::info("Clientes carregados.");
    }
    if (Persistencia::carregarLocacoes(locacoesTemp, idL)) {
        locacoesTemp.paraCada([&](Locacao& l){ locacoes.inserirExistente(l); });
        Console::info("Locacoes carregadas.");
    }

    // Catalogo de marcas/modelos
    ListaEncadeada<Marca>  marcasTemp;
    ListaEncadeada<Modelo> modelosTemp;
    if (Persistencia::carregarMarcas(marcasTemp)) {
        marcasTemp.paraCada([&](Marca& m){ catalogo.inserirMarcaExistente(m); });
    }
    if (Persistencia::carregarModelos(modelosTemp)) {
        modelosTemp.paraCada([&](Modelo& m){ catalogo.inserirModeloExistente(m); });
        Console::info("Catalogo carregado.");
    }

    // Garante consistencia entre o status da frota e as locacoes ativas
    int corrigidas = locacoes.reconciliarFrota();
    if (corrigidas > 0)
        Console::aviso(std::to_string(corrigidas) +
                       " inconsistencia(s) de status corrigida(s) na carga.");
}

// ─── Submenus ────────────────────────────────────────────────────────────────

void menuFrota(GerenciadorFrota& frota, Catalogo& catalogo) {
    int op;
    do {
        Console::cabecalho("GERENCIAR FROTA");
        std::cout << "  [1] Listar todos os veiculos\n";
        std::cout << "  [2] Cadastrar veiculo\n";
        std::cout << "  [3] Buscar veiculo (ID ou placa)\n";
        std::cout << "  [4] Disponibilidade por categoria\n";
        std::cout << "  [5] Manutencao\n";
        std::cout << "  [0] Voltar\n\n";
        op = Console::lerInteiro("Opcao");

        switch (op) {
            case 1: telaListarCarros(frota);        break;
            case 2: telaCadastrarCarro(frota, catalogo); break;
            case 3: telaBuscarVeiculo(frota);       break;
            case 4: telaDisponibilidade(frota);     break;
            case 5: telaManutencao(frota);          break;
        }
    } while (op != 0);
}

void menuClientes(GerenciadorClientes& clientes, GerenciadorLocacoes& locacoes) {
    int op;
    do {
        Console::cabecalho("GERENCIAR CLIENTES");
        std::cout << "  [1] Listar clientes\n";
        std::cout << "  [2] Cadastrar cliente\n";
        std::cout << "  [3] Buscar cliente (ID ou CPF)\n";
        std::cout << "  [4] Remover cliente\n";
        std::cout << "  [0] Voltar\n\n";
        op = Console::lerInteiro("Opcao");

        switch (op) {
            case 1: telaListarClientes(clientes);              break;
            case 2: telaCadastrarCliente(clientes);            break;
            case 3: telaBuscarCliente(clientes);               break;
            case 4: telaRemoverCliente(clientes, locacoes);    break;
        }
    } while (op != 0);
}

void menuLocacoes(GerenciadorLocacoes& locacoes,
                  GerenciadorClientes& clientes,
                  GerenciadorFrota&    frota) {
    int op;
    do {
        Console::cabecalho("LOCACOES");
        std::cout << "  [1] Todas as locacoes\n";
        std::cout << "  [2] Locacoes ativas (ord. por data)\n";
        std::cout << "  [3] Locacoes em atraso\n";
        std::cout << "  [4] Nova locacao\n";
        std::cout << "  [5] Registrar devolucao\n";
        std::cout << "  [6] Desfazer ultima operacao\n";
        std::cout << "  [0] Voltar\n\n";
        op = Console::lerInteiro("Opcao");

        switch (op) {
            case 1: telaListarLocacoes(locacoes, clientes, frota);  break;
            case 2: telaLocacoesAtivas(locacoes, clientes, frota);  break;
            case 3: telaLocacoesEmAtraso(locacoes, clientes, frota);break;
            case 4: telaNovaLocacao(locacoes, clientes, frota);     break;
            case 5: telaDevolucao(locacoes, clientes, frota);       break;
            case 6: {
                std::string desc = locacoes.desfazerUltima();
                desc.empty() ? Console::aviso("Nada a desfazer.")
                             : Console::sucesso("Desfeito: " + desc);
                Console::pausar();
                break;
            }
        }
    } while (op != 0);
}

void menuCatalogo(Catalogo& catalogo) {
    int op;
    do {
        Console::cabecalho("CATALOGO DE VEICULOS (MARCAS / MODELOS)");
        std::cout << "  [1] Listar marcas e modelos\n";
        std::cout << "  [2] Cadastrar marca\n";
        std::cout << "  [3] Cadastrar modelo\n";
        std::cout << "  [0] Voltar\n\n";
        op = Console::lerInteiro("Opcao");

        switch (op) {
            case 1: telaListarCatalogo(catalogo);   break;
            case 2: telaCadastrarMarca(catalogo);   break;
            case 3: telaCadastrarModelo(catalogo);  break;
        }
    } while (op != 0);
}

void menuConsultas(GerenciadorLocacoes& locacoes,
                   GerenciadorClientes& clientes,
                   GerenciadorFrota&    frota) {
    int op;
    do {
        Console::cabecalho("CONSULTAS E HISTORICO");
        std::cout << "  [1] Historico por cliente\n";
        std::cout << "  [2] Historico por veiculo\n";
        std::cout << "  [3] Busca binaria de locacao por ID\n";
        std::cout << "  [0] Voltar\n\n";
        op = Console::lerInteiro("Opcao");

        switch (op) {
            case 1: telaHistoricoCliente(locacoes, clientes, frota); break;
            case 2: telaHistoricoVeiculo(locacoes, clientes, frota); break;
            case 3: telaBuscaBinaria(locacoes, clientes, frota);     break;
        }
    } while (op != 0);
}

// ─── Main ────────────────────────────────────────────────────────────────────

int main() {
    habilitarCoresWindows();

    GerenciadorFrota     frota;
    GerenciadorClientes  clientes;
    GerenciadorLocacoes  locacoes(frota, clientes);
    Catalogo             catalogo;

    // Tenta carregar dados salvos; se não houver, usa dados de demonstração
    Console::limpar();
    Console::info("Carregando dados...");
    bool dadosExistem = false;
    {
        std::ifstream teste("dados/carros.csv");
        dadosExistem = teste.is_open();
    }

    if (dadosExistem) {
        carregarTudo(frota, clientes, locacoes, catalogo);
    } else {
        Console::info("Sem dados salvos - carregando demonstracao.");
        frota.cadastrar("Volkswagen", "Gol",        "ABC-1234", 2022, 0);
        frota.cadastrar("Chevrolet",  "Onix",       "DEF-5678", 2023, 0);
        frota.cadastrar("Toyota",     "Corolla",    "GHI-9012", 2021, 1);
        frota.cadastrar("Jeep",       "Renegade",   "JKL-3456", 2023, 2);
        frota.cadastrar("BMW",        "320i",       "MNO-7890", 2022, 3);
        frota.cadastrar("Ford",       "Ranger",     "PQR-1122", 2023, 4);

        clientes.cadastrar("Ana Silva",    "111.222.333-44", "(11) 99999-0001", "ana@email.com");
        clientes.cadastrar("Bruno Costa",  "555.666.777-88", "(21) 99999-0002", "bruno@email.com");
        clientes.cadastrar("Carla Mendes", "999.000.111-22", "(31) 99999-0003", "carla@email.com");
    }

    // Catalogo de marcas/modelos: usa o salvo; se vazio, carrega o padrao
    if (catalogo.vazio()) {
        catalogo.carregarPadrao();
        Console::info("Catalogo padrao de marcas/modelos carregado.");
    }

    int op;
    do {
        Console::cabecalho("MENU PRINCIPAL");
        std::cout << "  [1] Dashboard\n";
        std::cout << "  [2] Frota de veiculos\n";
        std::cout << "  [3] Clientes\n";
        std::cout << "  [4] Locacoes\n";
        std::cout << "  [5] Consultas e historico\n";
        std::cout << "  [6] Catalogo (marcas/modelos)\n";
        std::cout << "  [7] Salvar dados\n";
        std::cout << "  [0] Sair\n\n";
        op = Console::lerInteiro("Opcao");

        switch (op) {
            case 1: telaDashboard(frota, clientes, locacoes);            break;
            case 2: menuFrota(frota, catalogo);                          break;
            case 3: menuClientes(clientes, locacoes);                    break;
            case 4: menuLocacoes(locacoes, clientes, frota);             break;
            case 5: menuConsultas(locacoes, clientes, frota);            break;
            case 6: menuCatalogo(catalogo);                              break;
            case 7: salvarTudo(frota, clientes, locacoes, catalogo);     break;
        }
    } while (op != 0);

    // Salva automaticamente ao sair
    Persistencia::salvarCarros(frota.getFrota());
    Persistencia::salvarClientes(clientes.getClientes());
    Persistencia::salvarLocacoes(locacoes.getLocacoes());
    Persistencia::salvarMarcas(catalogo.getMarcas());
    Persistencia::salvarModelos(catalogo.getModelos());

    Console::limpar();
    std::cout << Cor::CIANO << "\n  Dados salvos. Ate logo!\n\n" << Cor::RESET;
    return 0;
}
