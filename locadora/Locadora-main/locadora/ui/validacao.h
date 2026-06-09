#pragma once
#include <string>
#include <algorithm>
#include <cctype>

namespace Validacao {

// ─── Helpers ─────────────────────────────────────────────────────────────────

inline std::string apenasDigitos(const std::string& s) {
    std::string r;
    for (char c : s) if (c >= '0' && c <= '9') r += c;
    return r;
}

inline bool contemLetra(const std::string& s) {
    for (char c : s) if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return true;
    return false;
}

// ─── CPF ─────────────────────────────────────────────────────────────────────
// Aceita "12345678909" ou "123.456.789-09"
// Valida digitos verificadores reais.

inline bool validarCpf(const std::string& entrada, std::string& erro) {
    std::string d = apenasDigitos(entrada);

    if (d.size() != 11) {
        erro = "CPF deve ter 11 digitos.";
        return false;
    }

    // Todos iguais (111.111.111-11, etc.) sao invalidos
    bool todosIguais = true;
    for (int i = 1; i < 11; i++) if (d[i] != d[0]) { todosIguais = false; break; }
    if (todosIguais) { erro = "CPF invalido (digitos repetidos)."; return false; }

    // Primeiro digito verificador
    int soma = 0;
    for (int i = 0; i < 9; i++) soma += (d[i] - '0') * (10 - i);
    int resto = soma % 11;
    int dv1 = (resto < 2) ? 0 : (11 - resto);
    if ((d[9] - '0') != dv1) { erro = "CPF invalido (digito verificador incorreto)."; return false; }

    // Segundo digito verificador
    soma = 0;
    for (int i = 0; i < 10; i++) soma += (d[i] - '0') * (11 - i);
    resto = soma % 11;
    int dv2 = (resto < 2) ? 0 : (11 - resto);
    if ((d[10] - '0') != dv2) { erro = "CPF invalido (digito verificador incorreto)."; return false; }

    erro = "";
    return true;
}

// Formata CPF para exibicao: "12345678909" -> "123.456.789-09"
inline std::string formatarCpf(const std::string& entrada) {
    std::string d = apenasDigitos(entrada);
    if (d.size() != 11) return entrada;
    return d.substr(0,3) + "." + d.substr(3,3) + "." + d.substr(6,3) + "-" + d.substr(9,2);
}

// ─── Telefone ────────────────────────────────────────────────────────────────
// Regras: DD (2 digitos) + 9 (padrao movel) + 8 digitos = 11 digitos total.
// Aceita "(11) 91234-5678", "11912345678", "11 91234-5678", etc.

inline bool validarTelefone(const std::string& entrada, std::string& erro) {
    if (contemLetra(entrada)) {
        erro = "Telefone nao pode conter letras.";
        return false;
    }

    std::string d = apenasDigitos(entrada);

    if (d.size() != 11) {
        erro = "Telefone deve ter 11 digitos: DD + 9 + 8 numeros. Ex: (11) 91234-5678";
        return false;
    }

    // DD valido: 11 a 99
    int dd = std::stoi(d.substr(0, 2));
    if (dd < 11 || dd > 99) {
        erro = "DDD invalido. Use um DDD valido (ex: 11, 21, 31...).";
        return false;
    }

    // Primeiro digito apos o DD deve ser 9 (celular)
    if (d[2] != '9') {
        erro = "Celular deve comecar com 9 apos o DDD. Ex: (11) 9XXXX-XXXX";
        return false;
    }

    erro = "";
    return true;
}

// Formata telefone: "11912345678" -> "(11) 91234-5678"
inline std::string formatarTelefone(const std::string& entrada) {
    std::string d = apenasDigitos(entrada);
    if (d.size() != 11) return entrada;
    return "(" + d.substr(0,2) + ") " + d.substr(2,5) + "-" + d.substr(7,4);
}

// ─── Email ───────────────────────────────────────────────────────────────────
// Regras minimas: tem @, tem algo antes, tem ponto depois do @, sem espacos.

inline bool validarEmail(const std::string& email, std::string& erro) {
    if (email.empty()) { erro = "Email nao pode ser vazio."; return false; }

    // Sem espacos
    if (email.find(' ') != std::string::npos) {
        erro = "Email nao pode conter espacos.";
        return false;
    }

    size_t at = email.find('@');
    if (at == std::string::npos) {
        erro = "Email deve conter '@'.";
        return false;
    }
    if (at == 0) {
        erro = "Email deve ter caracteres antes do '@'.";
        return false;
    }

    // So um @
    if (email.find('@', at + 1) != std::string::npos) {
        erro = "Email nao pode ter mais de um '@'.";
        return false;
    }

    std::string dominio = email.substr(at + 1);
    if (dominio.size() < 3) {
        erro = "Dominio do email invalido.";
        return false;
    }

    size_t ponto = dominio.find('.');
    if (ponto == std::string::npos || ponto == 0 || ponto == dominio.size() - 1) {
        erro = "Email deve ter um dominio valido. Ex: nome@email.com";
        return false;
    }

    erro = "";
    return true;
}

// ─── Nome ────────────────────────────────────────────────────────────────────

inline bool validarNome(const std::string& nome, std::string& erro) {
    if (nome.size() < 3) {
        erro = "Nome deve ter pelo menos 3 caracteres.";
        return false;
    }
    // Deve ter pelo menos um espaco (nome + sobrenome)
    if (nome.find(' ') == std::string::npos) {
        erro = "Informe nome e sobrenome.";
        return false;
    }
    // Nao pode ser so espacos
    bool temLetra = false;
    for (char c : nome) if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) { temLetra = true; break; }
    if (!temLetra) { erro = "Nome invalido."; return false; }

    erro = "";
    return true;
}

// Valida UMA parte do nome (nome OU sobrenome).
// Regra: aceita letras — inclusive acentuadas de QUALQUER idioma (bytes UTF-8,
// ex.: nomes eslavos: Dvořák, Łukasz, Nováková) — alem de espaco, hifen e
// apostrofo (D'Avila, Anne-Marie). Rejeita numeros e simbolos ($ % @ # ! ...).
inline bool validarNomeParte(const std::string& parte, const std::string& rotulo,
                             std::string& erro) {
    // remove espacos das pontas para medir o tamanho real
    size_t ini = parte.find_first_not_of(" \t");
    size_t fim = parte.find_last_not_of(" \t");
    std::string t = (ini == std::string::npos) ? "" : parte.substr(ini, fim - ini + 1);

    if (t.size() < 2) {
        erro = rotulo + " deve ter ao menos 2 caracteres.";
        return false;
    }

    bool temLetra = false;
    for (unsigned char c : t) {
        if (c >= 0x80) { temLetra = true; continue; }   // byte de letra acentuada (UTF-8)
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) { temLetra = true; continue; }
        if (c == ' ' || c == '-' || c == '\'') continue; // separadores usuais em nomes
        if (c >= '0' && c <= '9') {
            erro = rotulo + " nao pode conter numeros.";
            return false;
        }
        erro = rotulo + " nao pode conter o caractere '" + (char)c + "'.";
        return false;
    }
    if (!temLetra) {
        erro = rotulo + " deve conter letras.";
        return false;
    }
    erro = "";
    return true;
}

// ─── Placa (padrao brasileiro) ────────────────────────────────────────────────
// Sao 7 caracteres alfanumericos. Dois formatos validos:
//   • Antigo   LLLNNNN  -> 3 letras + 4 numeros            (ex: ABC1234 / ABC-1234)
//   • Mercosul LLLNLNN  -> 3 letras + 1 num + 1 letra + 2 num (ex: ABC1D23)
// L = letra (A-Z), N = numero (0-9). Hifen e espacos sao ignorados na validacao.

inline bool ehLetraPlaca(char c)  { return c >= 'A' && c <= 'Z'; }
inline bool ehDigitoPlaca(char c) { return c >= '0' && c <= '9'; }

// Remove hifen/espacos e coloca em maiusculas.
inline std::string normalizarPlaca(const std::string& s) {
    std::string r;
    for (char c : s) {
        if (c == '-' || c == ' ') continue;
        r += (char)std::toupper((unsigned char)c);
    }
    return r;
}

inline bool placaFormatoAntigo(const std::string& p) {     // LLLNNNN
    if (p.size() != 7) return false;
    return ehLetraPlaca(p[0]) && ehLetraPlaca(p[1]) && ehLetraPlaca(p[2]) &&
           ehDigitoPlaca(p[3]) && ehDigitoPlaca(p[4]) &&
           ehDigitoPlaca(p[5]) && ehDigitoPlaca(p[6]);
}

inline bool placaFormatoMercosul(const std::string& p) {   // LLLNLNN
    if (p.size() != 7) return false;
    return ehLetraPlaca(p[0]) && ehLetraPlaca(p[1]) && ehLetraPlaca(p[2]) &&
           ehDigitoPlaca(p[3]) && ehLetraPlaca(p[4]) &&
           ehDigitoPlaca(p[5]) && ehDigitoPlaca(p[6]);
}

inline bool validarPlaca(const std::string& entrada, std::string& erro) {
    std::string p = normalizarPlaca(entrada);

    if (p.size() != 7) {
        erro = "Placa deve ter 7 caracteres. Ex: ABC1D23 (Mercosul) ou ABC-1234 (antiga).";
        return false;
    }
    for (char c : p) {
        if (!ehLetraPlaca(c) && !ehDigitoPlaca(c)) {
            erro = "Placa so pode conter letras e numeros.";
            return false;
        }
    }
    if (placaFormatoAntigo(p) || placaFormatoMercosul(p)) { erro = ""; return true; }

    erro = "Formato invalido. Use LLLNNNN (antiga, ex ABC1234) ou LLLNLNN (Mercosul, ex ABC1D23).";
    return false;
}

// Padroniza para exibicao/armazenamento: antiga com hifen (ABC-1234),
// Mercosul sem hifen (ABC1D23). Pressupoe que ja passou por validarPlaca.
inline std::string formatarPlaca(const std::string& entrada) {
    std::string p = normalizarPlaca(entrada);
    if (placaFormatoAntigo(p)) return p.substr(0, 3) + "-" + p.substr(3, 4);
    return p;
}

} // namespace Validacao
