#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
//  Componente de tabela estilizada para terminal
//  Moldura arredondada (box-drawing), titulo centralizado, cabecalhos
//  coloridos por coluna, numeracao de linhas e separadores verticais.
//  Usa cores ANSI de 256 niveis (suportadas no Terminal do macOS, iTerm,
//  Windows Terminal e a maioria dos emuladores Linux).
// ─────────────────────────────────────────────────────────────────────────────
namespace Tabela {

inline std::string fg(int n) { return "\033[38;5;" + std::to_string(n) + "m"; }

const std::string RESET = "\033[0m";
const std::string BOLD  = "\033[1m";

// Paleta
const std::string COR_BORDA  = fg(60);    // azul-acinzentado (moldura)
const std::string COR_TITULO = fg(39);    // azul (titulo)
const std::string COR_NUM    = fg(244);   // cinza (numero da linha)
const std::string COR_TEXTO  = fg(252);   // texto padrao das celulas

// Cores ciclicas dos cabecalhos (uma cor por coluna)
const std::vector<std::string> PALETA_CAB = {
    fg(203), fg(215), fg(179), fg(114), fg(80),
    fg(75),  fg(212), fg(214), fg(186), fg(150)
};

// Celula: texto + cor opcional (vazia => cor padrao).
struct Celula {
    std::string texto;
    std::string cor;
    Celula(std::string t = "", std::string c = "")
        : texto(std::move(t)), cor(std::move(c)) {}
};

// Largura visivel em colunas de terminal (conta code points UTF-8, nao bytes;
// trata acentos em nomes sem desalinhar a tabela).
inline int larguraVisivel(const std::string& s) {
    int n = 0;
    for (unsigned char c : s)
        if ((c & 0xC0) != 0x80) ++n;   // ignora bytes de continuacao
    return n;
}

class Grade {
    std::string titulo;
    std::vector<std::string> cabecalhos;
    std::vector<std::vector<Celula>> linhas;
    bool numerar;

    static std::string repetir(const std::string& u, int n) {
        std::string r;
        for (int i = 0; i < n; ++i) r += u;
        return r;
    }

public:
    explicit Grade(std::string titulo_ = "", bool numerar_ = true)
        : titulo(std::move(titulo_)), numerar(numerar_) {}

    void colunas(std::vector<std::string> cabs) { cabecalhos = std::move(cabs); }
    void adicionar(std::vector<Celula> linha)   { linhas.push_back(std::move(linha)); }
    bool vazia() const { return linhas.empty(); }

    // Resultado da renderizacao: linhas prontas (com cor) + largura visivel.
    struct Render {
        std::vector<std::string> linhas;
        int largura = 0;   // largura visivel total, incluindo as bordas laterais
    };

    // Gera a tabela como linhas de texto (sem imprimir) — permite compor
    // varias tabelas lado a lado.
    Render renderizar() const {
        Render out;

        std::vector<std::string> cabs;
        if (numerar) cabs.push_back("#");
        for (const auto& c : cabecalhos) cabs.push_back(c);
        const size_t nc = cabs.size();
        if (nc == 0) return out;

        std::vector<std::vector<Celula>> dados;
        int idx = 1;
        for (const auto& ln : linhas) {
            std::vector<Celula> nova;
            if (numerar) nova.emplace_back(std::to_string(idx++), COR_NUM);
            for (const auto& cel : ln) nova.push_back(cel);
            while (nova.size() < nc) nova.emplace_back("");
            dados.push_back(std::move(nova));
        }

        std::vector<int> larg(nc, 0);
        for (size_t i = 0; i < nc; ++i) larg[i] = larguraVisivel(cabs[i]);
        for (const auto& ln : dados)
            for (size_t i = 0; i < nc; ++i)
                larg[i] = std::max(larg[i], larguraVisivel(ln[i].texto));

        const int pad = 1;
        int total = 0;
        for (size_t i = 0; i < nc; ++i) total += larg[i] + pad * 2;
        total += (int)nc - 1;

        auto borda = [&](const std::string& esq, const std::string& meio,
                         const std::string& dir) {
            std::string s = COR_BORDA + esq;
            for (size_t i = 0; i < nc; ++i) {
                s += repetir("─", larg[i] + pad * 2);
                if (i + 1 < nc) s += meio;
            }
            return s + dir + RESET;
        };
        auto conteudo = [&](const std::vector<Celula>& cels, bool ehCab) {
            std::string s = COR_BORDA + "│" + RESET;
            for (size_t i = 0; i < nc; ++i) {
                const std::string& txt = (i < cels.size() ? cels[i].texto : std::string());
                int preencher = larg[i] - larguraVisivel(txt);
                std::string cor = ehCab
                    ? (PALETA_CAB[i % PALETA_CAB.size()] + BOLD)
                    : (i < cels.size() && !cels[i].cor.empty() ? cels[i].cor : COR_TEXTO);
                s += " " + cor + txt + RESET + repetir(" ", std::max(0, preencher)) + " ";
                s += COR_BORDA + "│" + RESET;
            }
            return s;
        };

        if (!titulo.empty()) {
            out.linhas.push_back(borda("╭", "─", "╮"));
            int espaco = std::max(0, total - larguraVisivel(titulo));
            int e = espaco / 2, d = espaco - e;
            out.linhas.push_back(COR_BORDA + "│" + RESET +
                repetir(" ", e) + COR_TITULO + BOLD + titulo + RESET + repetir(" ", d) +
                COR_BORDA + "│" + RESET);
            out.linhas.push_back(borda("├", "┬", "┤"));
        } else {
            out.linhas.push_back(borda("╭", "┬", "╮"));
        }

        std::vector<Celula> celCab;
        for (const auto& c : cabs) celCab.emplace_back(c);
        out.linhas.push_back(conteudo(celCab, true));
        out.linhas.push_back(borda("├", "┼", "┤"));
        for (const auto& ln : dados) out.linhas.push_back(conteudo(ln, false));
        out.linhas.push_back(borda("╰", "┴", "╯"));

        out.largura = total + 2;
        return out;
    }

    void desenhar(std::ostream& os = std::cout) const {
        Render r = renderizar();
        for (const auto& l : r.linhas) os << l << "\n";
    }
};

// Desenha varias grades na mesma faixa horizontal (lado a lado).
// Grades mais baixas sao completadas com espacos para manter o alinhamento.
inline void desenharLadoALado(const std::vector<Grade>& grades,
                              std::ostream& os = std::cout,
                              const std::string& espaco = "  ") {
    std::vector<Grade::Render> rs;
    size_t maxLin = 0;
    for (const auto& g : grades) {
        rs.push_back(g.renderizar());
        maxLin = std::max(maxLin, rs.back().linhas.size());
    }
    for (size_t i = 0; i < maxLin; ++i) {
        std::string linha;
        for (size_t b = 0; b < rs.size(); ++b) {
            if (b) linha += espaco;
            linha += (i < rs[b].linhas.size())
                     ? rs[b].linhas[i]
                     : std::string(rs[b].largura, ' ');
        }
        os << linha << "\n";
    }
}

} // namespace Tabela
