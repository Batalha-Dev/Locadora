#pragma once
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <limits>
#include <ctime>

// Codigos ANSI (funcionam no Windows 10+, Linux e macOS)
namespace Cor {
    const std::string RESET   = "\033[0m";
    const std::string NEGRITO = "\033[1m";
    const std::string VERMELHO= "\033[31m";
    const std::string VERDE   = "\033[32m";
    const std::string AMARELO = "\033[33m";
    const std::string AZUL    = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CIANO   = "\033[36m";
    const std::string BRANCO  = "\033[37m";
    const std::string CINZA   = "\033[90m";
}

namespace Console {

inline void limpar() {
    std::cout << "\033[2J\033[H";
}

inline void pausar() {
    std::cout << "\n" << Cor::CINZA << "  Pressione ENTER para continuar..." << Cor::RESET;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

inline void linha(char c = '-', int tam = 62) {
    std::cout << Cor::CINZA << std::string(tam, c) << Cor::RESET << "\n";
}

inline void cabecalho(const std::string& titulo) {
    limpar();
    linha('=');
    std::cout << Cor::CIANO << Cor::NEGRITO
              << "  LOCADORA CLAUDIO - " << titulo
              << Cor::RESET << "\n";
    linha('=');
    std::cout << "\n";
}

inline void sucesso(const std::string& msg) {
    std::cout << "\n" << Cor::VERDE << "  [OK] " << msg << Cor::RESET << "\n";
}

inline void erro(const std::string& msg) {
    std::cout << "\n" << Cor::VERMELHO << "  [ERRO] " << msg << Cor::RESET << "\n";
}

inline void aviso(const std::string& msg) {
    std::cout << "\n" << Cor::AMARELO << "  [!] " << msg << Cor::RESET << "\n";
}

inline void info(const std::string& msg) {
    std::cout << Cor::CINZA << "  " << msg << Cor::RESET << "\n";
}

inline int lerInteiro(const std::string& prompt) {
    int val;
    while (true) {
        std::cout << Cor::AMARELO << "  > " << prompt << ": " << Cor::RESET;
        if (std::cin >> val) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return val;
        }
        // stdin fechado (EOF): encerra com seguranca em vez de entrar em
        // laco infinito. Menus tratam 0 como "sair/voltar".
        if (std::cin.eof()) return 0;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        erro("Valor invalido. Tente novamente.");
    }
}

// Leitura com validacao: chama validador(texto, msgErro) ate retornar true
template <typename Validador>
inline std::string lerTextoValidado(const std::string& prompt, Validador validar) {
    while (true) {
        std::string val;
        std::cout << Cor::AMARELO << "  > " << prompt << ": " << Cor::RESET;
        if (!std::getline(std::cin, val)) return "";  // EOF: aborta a leitura
        std::string msgErro;
        if (validar(val, msgErro)) return val;
        erro(msgErro);
    }
}

inline std::string lerTexto(const std::string& prompt) {
    std::string val;
    std::cout << Cor::AMARELO << "  > " << prompt << ": " << Cor::RESET;
    std::getline(std::cin, val);
    return val;
}

inline std::string formatarData(time_t t) {
    if (t == 0) return "--/--/----";
    char buf[20];
    struct tm* info = localtime(&t);
    strftime(buf, sizeof(buf), "%d/%m/%Y", info);
    return std::string(buf);
}

inline std::string formatarMoeda(double val) {
    std::ostringstream oss;
    oss << "R$ " << std::fixed << std::setprecision(2) << val;
    return oss.str();
}

} // namespace Console
