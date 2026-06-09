// ─────────────────────────────────────────────────────────────────────────────
//  Testes de consistência — Locadora Claudio
//
//  Programa independente (main próprio) que exercita os SERVIÇOS diretamente,
//  sem interface/stdin, e verifica os invariantes do sistema:
//    • unicidade via tabela hash (placa / CPF)
//    • transições de status na locação e na devolução
//    • regras de consistência (veículo indisponível, cliente com locação ativa)
//    • cobrança mínima de 1 diária (regressão do bug de valor R$ 0,00)
//    • desfazer (pilha) de locação e devolução
//    • merge sort + busca binária
//    • persistência (salvar → carregar → comparar)
//    • reconciliação frota ↔ locações
//    • invariante global: nº de veículos LOCADO == nº de locações ATIVAS
//
//  Compilar:  g++ -std=c++17 -Wall -Wextra -O2 -o testes_locadora testes/testes.cpp
//  Retorna 0 se todos passam; 1 caso contrário (amigável para CTest/CI).
// ─────────────────────────────────────────────────────────────────────────────
#include <iostream>
#include <string>
#include "../servicos/gerenciador_frota.h"
#include "../servicos/gerenciador_clientes.h"
#include "../servicos/gerenciador_locacoes.h"
#include "../servicos/consultas.h"
#include "../servicos/persistencia.h"
#include "../estruturas/vetor_dinamico.h"
#include "../estruturas/ordenacao.h"
#include "../estruturas/gerador_id.h"
#include "../servicos/catalogo.h"
#include "../ui/validacao.h"

static int g_total = 0;
static int g_falhas = 0;

#define CHECK(cond, msg)                                            \
    do {                                                            \
        g_total++;                                                  \
        if (cond) {                                                 \
            std::cout << "  [OK]    " << (msg) << "\n";             \
        } else {                                                    \
            g_falhas++;                                             \
            std::cout << "  [FALHA] " << (msg) << "\n";             \
        }                                                           \
    } while (0)

static void secao(const std::string& titulo) {
    std::cout << "\n--- " << titulo << " ---\n";
}

// Conta veículos com determinado status.
static int contarCarros(GerenciadorFrota& frota, StatusCarro st) {
    int n = 0;
    frota.getFrota().paraCada([&](Carro& c){ if (c.status == st) n++; });
    return n;
}

// Conta locações com determinado status.
static int contarLocacoes(GerenciadorLocacoes& loc, StatusLocacao st) {
    int n = 0;
    loc.getLocacoes().paraCada([&](Locacao& l){ if (l.status == st) n++; });
    return n;
}

// ─── Bloco 1: cadastro e unicidade via hash ──────────────────────────────────
static void testeCadastroHash() {
    secao("Cadastro e unicidade (tabela hash)");

    GerenciadorFrota frota;
    Carro* a = frota.cadastrar("Volkswagen", "Gol", "ABC-1234", 2020, 0);
    CHECK(a != nullptr, "cadastra veiculo valido");
    Carro* dup = frota.cadastrar("Fiat", "Uno", "ABC-1234", 2021, 0);
    CHECK(dup == nullptr, "rejeita placa duplicada (hash por placa)");
    CHECK(frota.buscarPorPlaca("ABC-1234") != nullptr, "busca O(1) por placa encontra");
    CHECK(frota.buscarPorPlaca("ZZZ-0000") == nullptr, "busca por placa inexistente -> nullptr");
    CHECK(a && frota.buscarPorId(a->id) == a, "busca O(1) por id retorna o mesmo registro");

    GerenciadorClientes cli;
    Cliente* c = cli.cadastrar("Ana Silva", "111.222.333-44", "(11) 90000-0001", "ana@x.com");
    CHECK(c != nullptr, "cadastra cliente valido");
    Cliente* cdup = cli.cadastrar("Outro Nome", "111.222.333-44", "(11) 90000-0002", "o@x.com");
    CHECK(cdup == nullptr, "rejeita CPF duplicado (hash por CPF)");
    CHECK(cli.buscarPorCpf("111.222.333-44") != nullptr, "busca O(1) por CPF encontra");
}

// ─── Bloco 2: locação, devolução e disponibilidade ───────────────────────────
static void testeLocacaoDevolucao() {
    secao("Locacao, devolucao e disponibilidade");

    GerenciadorFrota frota;
    GerenciadorClientes cli;
    GerenciadorLocacoes loc(frota, cli);

    Carro* car  = frota.cadastrar("Volkswagen", "Gol", "AAA-0001", 2020, 0); // diaria 80
    Carro* car2 = frota.cadastrar("Fiat", "Uno", "BBB-0002", 2019, 0);
    Cliente* c1 = cli.cadastrar("Ana Silva",   "111.222.333-44", "(11) 90000-0001", "a@x.com");
    Cliente* c2 = cli.cadastrar("Bruno Costa",  "555.666.777-88", "(21) 90000-0002", "b@x.com");

    CHECK(car->status == StatusCarro::DISPONIVEL, "veiculo novo nasce DISPONIVEL");

    Locacao* l = loc.registrarLocacao(c1->id, car->id, 3);
    CHECK(l != nullptr, "registra locacao valida");
    CHECK(car->status == StatusCarro::LOCADO, "veiculo fica LOCADO apos a locacao");
    CHECK(l && l->valorTotal == 3 * 80.0, "valor contratado = dias * diaria");

    Locacao* l2 = loc.registrarLocacao(c2->id, car->id, 2);
    CHECK(l2 == nullptr, "rejeita locacao de veiculo indisponivel (consistencia)");

    Locacao* l3 = loc.registrarLocacao(c1->id, car2->id, 1);
    CHECK(l3 == nullptr, "rejeita 2a locacao ATIVA do mesmo cliente (consistencia)");

    bool dev = l && loc.registrarDevolucao(l->id);
    CHECK(dev, "registra devolucao da locacao ativa");
    CHECK(car->status == StatusCarro::DISPONIVEL, "veiculo volta DISPONIVEL apos devolucao");
    CHECK(l && l->status == StatusLocacao::CONCLUIDA, "locacao fica CONCLUIDA");

    Locacao* l4 = loc.registrarLocacao(c1->id, car2->id, 1);
    CHECK(l4 != nullptr, "cliente pode locar de novo apos concluir a anterior");

    CHECK(!loc.registrarDevolucao(99999), "devolver locacao inexistente falha");
    CHECK(l && !loc.registrarDevolucao(l->id), "devolver locacao ja concluida falha");
}

// ─── Bloco 3: regressão do bug de cobrança R$ 0,00 ───────────────────────────
static void testeCobrancaMinima() {
    secao("Cobranca minima de 1 diaria (regressao do valor R$ 0,00)");

    GerenciadorFrota frota;
    GerenciadorClientes cli;
    GerenciadorLocacoes loc(frota, cli);

    Carro* car = frota.cadastrar("BMW", "320i", "CCC-0003", 2022, 3); // diaria 350
    Cliente* c = cli.cadastrar("Ana Silva", "111.222.333-44", "(11) 90000-0001", "a@x.com");

    Locacao* l = loc.registrarLocacao(c->id, car->id, 1);
    bool ok = l && loc.registrarDevolucao(l->id); // devolucao quase imediata
    CHECK(ok, "devolucao imediata registrada");
    CHECK(l && l->valorTotal == 350.0,
          "devolucao no mesmo dia cobra 1 diaria (R$ 350), nao R$ 0,00");
    CHECK(l && l->valorTotal > 0.0, "valor cobrado nunca e zero");
}

// ─── Bloco 4: desfazer (pilha) ───────────────────────────────────────────────
static void testeDesfazer() {
    secao("Desfazer ultima operacao (pilha)");

    GerenciadorFrota frota;
    GerenciadorClientes cli;
    GerenciadorLocacoes loc(frota, cli);

    Carro* car = frota.cadastrar("Volkswagen", "Gol", "DDD-0004", 2020, 0);
    Cliente* c = cli.cadastrar("Ana Silva", "111.222.333-44", "(11) 90000-0001", "a@x.com");

    Locacao* l = loc.registrarLocacao(c->id, car->id, 2);
    CHECK(car->status == StatusCarro::LOCADO, "pre-undo: veiculo LOCADO");
    std::string desc = loc.desfazerUltima();
    CHECK(!desc.empty(), "undo retorna a descricao da operacao desfeita");
    CHECK(car->status == StatusCarro::DISPONIVEL, "undo da locacao libera o veiculo");
    CHECK(l && l->status == StatusLocacao::CANCELADA, "undo da locacao marca CANCELADA");

    Locacao* l2 = loc.registrarLocacao(c->id, car->id, 1);
    bool dev = l2 && loc.registrarDevolucao(l2->id);
    CHECK(dev && car->status == StatusCarro::DISPONIVEL, "pos-devolucao: veiculo DISPONIVEL");
    loc.desfazerUltima(); // desfaz a devolucao
    CHECK(l2 && l2->status == StatusLocacao::ATIVA, "undo da devolucao reativa a locacao");
    CHECK(car->status == StatusCarro::LOCADO, "undo da devolucao re-loca o veiculo");

    GerenciadorFrota f2; GerenciadorClientes cl2; GerenciadorLocacoes vazio(f2, cl2);
    CHECK(vazio.desfazerUltima().empty(), "undo sem historico retorna vazio");
}

// ─── Bloco 5: merge sort + busca binária ─────────────────────────────────────
static void testeOrdenacaoBusca() {
    secao("Merge sort e busca binaria");

    // 5a) inteiros
    VetorDinamico<int> v;
    int entrada[] = {5, 3, 9, 1, 7, 2, 8, 4, 6};
    for (int x : entrada) v.adicionar(x);
    Ordenacao::ordenar(v, [](const int& a, const int& b){ return a < b; });

    bool ordenado = true;
    for (int i = 1; i < v.getTamanho(); i++)
        if (v[i - 1] > v[i]) ordenado = false;
    CHECK(ordenado, "merge sort ordena em ordem crescente");

    int idx = Ordenacao::buscaBinaria(v, 7, [](const int& x){ return x; });
    CHECK(idx != -1 && v[idx] == 7, "busca binaria encontra elemento existente");
    CHECK(Ordenacao::buscaBinaria(v, 42, [](const int& x){ return x; }) == -1,
          "busca binaria retorna -1 para ausente");

    // 5b) locações por data (ordenação por dataInicio)
    VetorDinamico<Locacao> vl;
    Locacao a; a.id = 1; a.dataInicio = 300;
    Locacao b; b.id = 2; b.dataInicio = 100;
    Locacao c; c.id = 3; c.dataInicio = 200;
    vl.adicionar(a); vl.adicionar(b); vl.adicionar(c);
    Ordenacao::ordenar(vl, [](const Locacao& x, const Locacao& y){
        return x.dataInicio < y.dataInicio;
    });
    CHECK(vl[0].dataInicio == 100 && vl[1].dataInicio == 200 && vl[2].dataInicio == 300,
          "ordenacao por data (crescente) reposiciona corretamente");

    int pos = Consultas::buscarLocacaoPorId(vl, 2);
    CHECK(pos != -1 && vl[pos].id == 2, "busca binaria de locacao por id encontra");
    CHECK(Consultas::buscarLocacaoPorId(vl, 777) == -1, "busca binaria de locacao ausente -> -1");
}

// ─── Bloco 6: persistência (round-trip) ──────────────────────────────────────
static void testePersistencia() {
    secao("Persistencia em arquivo (salvar -> carregar -> comparar)");

    // Cria e salva
    GerenciadorFrota f1; GerenciadorClientes c1; GerenciadorLocacoes l1(f1, c1);
    Carro* car   = f1.cadastrar("Volkswagen", "Gol", "EEE-0005", 2020, 0);
    Cliente* cli = c1.cadastrar("Ana Silva", "111.222.333-44", "(11) 90000-0001", "a@x.com");
    Locacao* lo  = l1.registrarLocacao(cli->id, car->id, 4);

    bool okSave = Persistencia::salvarCarros(f1.getFrota())
               && Persistencia::salvarClientes(c1.getClientes())
               && Persistencia::salvarLocacoes(l1.getLocacoes());
    CHECK(okSave, "persistencia: salvar retorna sucesso");

    // Recarrega em gerenciadores novos
    GerenciadorFrota f2; GerenciadorClientes c2; GerenciadorLocacoes l2(f2, c2);
    ListaEncadeada<Carro> tc;   int i1 = 1;
    ListaEncadeada<Cliente> tcl; int i2 = 1;
    ListaEncadeada<Locacao> tl;  int i3 = 1;
    Persistencia::carregarCarros(tc, i1);   tc.paraCada([&](Carro& x){ f2.inserirExistente(x); });
    Persistencia::carregarClientes(tcl, i2);tcl.paraCada([&](Cliente& x){ c2.inserirExistente(x); });
    Persistencia::carregarLocacoes(tl, i3); tl.paraCada([&](Locacao& x){ l2.inserirExistente(x); });

    Carro* car2 = f2.buscarPorPlaca("EEE-0005");
    CHECK(car2 != nullptr, "persistencia: veiculo recarregado e localizado por placa");
    CHECK(car2 && car2->diaria == car->diaria, "persistencia: diaria preservada");
    Cliente* cli2 = c2.buscarPorCpf("111.222.333-44");
    CHECK(cli2 != nullptr, "persistencia: cliente recarregado e localizado por CPF");
    Locacao* lo2 = l2.buscarPorId(lo->id);
    CHECK(lo2 && lo2->valorTotal == lo->valorTotal, "persistencia: valor da locacao preservado");
    CHECK(car2 && car2->status == StatusCarro::LOCADO,
          "persistencia: status LOCADO reconstruido a partir da locacao ativa");
}

// ─── Bloco 7: reconciliação e invariante global ──────────────────────────────
static void testeReconciliacaoInvariante() {
    secao("Reconciliacao e invariante global frota <-> locacoes");

    GerenciadorFrota frota; GerenciadorClientes cli; GerenciadorLocacoes loc(frota, cli);
    Carro* car  = frota.cadastrar("Volkswagen", "Gol", "FFF-0006", 2020, 0);
    Carro* car2 = frota.cadastrar("Fiat", "Uno", "GGG-0007", 2019, 0);
    Cliente* c  = cli.cadastrar("Ana Silva", "111.222.333-44", "(11) 90000-0001", "a@x.com");
    loc.registrarLocacao(c->id, car->id, 2);

    // Inconsistencia 1: carro com locacao ativa marcado como DISPONIVEL
    car->status = StatusCarro::DISPONIVEL;
    // Inconsistencia 2: carro sem locacao ativa marcado como LOCADO
    car2->status = StatusCarro::LOCADO;

    int corr = loc.reconciliarFrota();
    CHECK(corr == 2, "reconciliacao detecta e corrige as 2 inconsistencias");
    CHECK(car->status == StatusCarro::LOCADO, "reconciliacao: carro com locacao ativa -> LOCADO");
    CHECK(car2->status == StatusCarro::DISPONIVEL, "reconciliacao: carro sem locacao ativa -> DISPONIVEL");

    // Invariante global: nº de carros LOCADO == nº de locacoes ATIVAS
    int locados = contarCarros(frota, StatusCarro::LOCADO);
    int ativas  = contarLocacoes(loc, StatusLocacao::ATIVA);
    CHECK(locados == ativas, "invariante: #veiculos LOCADO == #locacoes ATIVAS");
}

// ─── Bloco 8: validação de placa (padrão brasileiro) ──────────────────────────
static void testePlaca() {
    secao("Validacao de placa (antiga e Mercosul)");
    std::string e;
    CHECK(Validacao::validarPlaca("ABC-1234", e), "aceita placa antiga ABC-1234");
    CHECK(Validacao::validarPlaca("ABC1234", e),  "aceita placa antiga sem hifen");
    CHECK(Validacao::validarPlaca("ABC1D23", e),  "aceita placa Mercosul ABC1D23");
    CHECK(Validacao::validarPlaca("bra2e19", e),  "aceita minusculas (normaliza p/ maiuscula)");
    CHECK(!Validacao::validarPlaca("AVCFR45", e),  "rejeita formato invalido (LLLLLNN)");
    CHECK(!Validacao::validarPlaca("AB12345", e),  "rejeita 2 letras no inicio");
    CHECK(!Validacao::validarPlaca("ABC123", e),   "rejeita comprimento diferente de 7");
    CHECK(!Validacao::validarPlaca("ABC12D3", e),  "rejeita posicoes trocadas (LLLNNLN)");
    CHECK(Validacao::formatarPlaca("abc1234") == "ABC-1234", "formata antiga: maiuscula + hifen");
    CHECK(Validacao::formatarPlaca("abc1d23") == "ABC1D23",  "formata Mercosul: maiuscula sem hifen");
}

// ─── Bloco 9: gerador de id automático (pilha) ────────────────────────────────
static void testeGeradorId() {
    secao("Gerador de ID automatico via pilha");
    GeradorId g;
    int a = g.obter(), b = g.obter(), c = g.obter();
    CHECK(a == 1 && b == 2 && c == 3, "ids saem em sequencia 1, 2, 3");
    g.liberar(b);                       // devolve o id 2 para a pilha
    CHECK(g.obter() == 2, "id liberado (2) e reaproveitado no proximo obter");
    CHECK(g.obter() == 4, "apos o reuso, a sequencia continua (4)");

    GeradorId g2;
    g2.reservar(10);                    // simula carga de arquivo com id maximo 10
    CHECK(g2.obter() == 11, "reservar(10) faz o proximo id ser 11 (sem colisao)");
}

// ─── Bloco 10: catálogo de marcas/modelos ─────────────────────────────────────
static void testeCatalogo() {
    secao("Catalogo de marcas e modelos");
    Catalogo cat;
    Marca* vw = cat.adicionarMarca("Volkswagen");
    CHECK(vw != nullptr, "adiciona marca");
    CHECK(cat.adicionarMarca("Volkswagen") == nullptr, "rejeita marca duplicada");
    CHECK(cat.adicionarMarca("volkswagen") == nullptr, "duplicada ignora maiusc/minusc");

    Modelo* gol = cat.adicionarModelo(vw->id, "Gol");
    CHECK(gol != nullptr, "adiciona modelo a uma marca");
    CHECK(cat.adicionarModelo(vw->id, "Gol") == nullptr, "rejeita modelo duplicado na marca");
    CHECK(cat.adicionarModelo(999, "X") == nullptr, "rejeita modelo de marca inexistente");

    Marca* fiat = cat.adicionarMarca("Fiat");
    cat.adicionarModelo(fiat->id, "Gol"); // mesmo nome, outra marca: permitido
    CHECK(cat.buscarModeloNaMarca(fiat->id, "Gol") != nullptr,
          "mesmo nome de modelo em marca diferente e permitido");
    CHECK(cat.modelosDaMarca(vw->id).getTamanho() == 1,
          "modelosDaMarca retorna apenas os modelos da marca");
    CHECK(vw->id != fiat->id, "marcas recebem ids distintos gerados automaticamente");
}

int main() {
    std::cout << "==================================================\n";
    std::cout << "  TESTES DE CONSISTENCIA - LOCADORA CLAUDIO\n";
    std::cout << "==================================================\n";

    testeCadastroHash();
    testeLocacaoDevolucao();
    testeCobrancaMinima();
    testeDesfazer();
    testeOrdenacaoBusca();
    testePersistencia();
    testeReconciliacaoInvariante();
    testePlaca();
    testeGeradorId();
    testeCatalogo();

    std::cout << "\n==================================================\n";
    std::cout << "  RESUMO: " << (g_total - g_falhas) << "/" << g_total << " testes passaram";
    if (g_falhas == 0) std::cout << "  -> TUDO OK\n";
    else               std::cout << "  -> " << g_falhas << " FALHA(S)\n";
    std::cout << "==================================================\n";

    return g_falhas == 0 ? 0 : 1;
}
