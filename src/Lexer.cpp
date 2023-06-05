#include "Lexer.hpp"

std::vector<Token> Lexer::lex(std::string source, const std::shared_ptr<Supervisor> &supervisor) noexcept {
    Lexer lexer(std::move(source), supervisor);

    std::vector<Token> tokens;
    while (!lexer.eof() && !supervisor->has_errors()) {
        const auto token = lexer.next_token();
        if (!token.matches(Token::Type::END_OF_FILE)) {
            tokens.push_back(token);
        }
    }

    return tokens;
}

Lexer::Lexer(std::string &&source, const std::shared_ptr<Supervisor> &supervisor) noexcept
        : Iterator(source), m_supervisor{supervisor} {}

Token Lexer::next_token() noexcept {
    if (m_supervisor->has_errors()) { return Token::create_dumb(); }

    skip_whitespaces();

    if (const auto ch = peek(); !ch.has_value()) {
        return Token::create_dumb();
    }

    const auto ch = peek().value();
    switch (ch) {
        case '(': {
            advance(1);
            return Token::create(Token::Type::LEFT_PAREN, "(", Position::create(cursor(), cursor()));
        }
        case ')': {
            advance(1);
            return Token::create(Token::Type::RIGHT_PAREN, ")", Position::create(cursor(), cursor()));
        }
        case '-': {
            return lex_minus();
        }
        case '{': {
            advance(1);
            return Token::create(Token::Type::LEFT_BRACE, "{", Position::create(cursor(), cursor()));
        }
        case '}': {
            advance(1);
            return Token::create(Token::Type::RIGHT_BRACE, "}", Position::create(cursor(), cursor()));
        }
        case '=': {
            return lex_equal_sign();
        }
        case ';': {
            advance(1);
            return Token::create(Token::Type::SEMICOLON, ";", Position::create(cursor(), cursor()));
        }
        case '*': {
            return lex_star();
        }
        case ',': {
            advance(1);
            return Token::create(Token::Type::COMMA, ",", Position::create(cursor(), cursor()));
        }
        case '+': {
            return lex_plus();
        }
        case '<': {
            return lex_less_than();
        }
        default: {
            return lex_keyword_or_identifier();
        }
    }
}

void Lexer::skip_whitespaces() noexcept {
    while (!eof()) {
        const auto ch = peek().value();

        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            advance(1);
            continue;
        } else {
            break;
        }
    }
}

Token Lexer::lex_keyword_or_identifier() noexcept {
    const auto start = cursor();

    std::string value;
    while (!eof()) {
        const auto ch = peek().value();

        if (std::isalnum(ch) != 0 || ch == '_') {
            value += ch;
            advance(1);
        } else {
            break;
        }
    }

    if (const auto keyword = Token::is_keyword(value);
            keyword != Token::Type::IDENTIFIER && keyword != Token::Type::END_OF_FILE) {
        return Token::create(keyword, std::move(value), Position::create(start, cursor()));
    }

    return Token::create(Token::Type::IDENTIFIER, std::move(value), Position::create(start, cursor()));
}

Token Lexer::lex_minus() noexcept {
    const auto start = cursor();

    if (auto ch = peek_ahead(1); ch == '>') {
        advance(2);
        return Token::create(Token::Type::ARROW, "->", Position::create(start, cursor()));
    } else if (ch = peek_ahead(1); ch == '-') {
        advance(2);
        return Token::create(Token::Type::MINUS_MINUS, "--", Position::create(start, cursor()));
    }

    advance(1);
    return Token::create(Token::Type::MINUS, "-", Position::create(start, cursor()));
}

Token Lexer::lex_equal_sign() noexcept {
    const auto start = cursor();

    if (auto ch = peek_ahead(1); ch == '=') {
        advance(2);
        return Token::create(Token::Type::EQUAL_EQUAL, "==", Position::create(start, cursor()));
    }

    advance(1);
    return Token::create(Token::Type::EQUAL, "=", Position::create(start, cursor()));
}

Token Lexer::lex_star() noexcept {
    const auto start = cursor();
    advance(1);
    return Token::create(Token::Type::STAR, "*", Position::create(start, cursor()));
}

Token Lexer::lex_plus() noexcept {
    const auto start = cursor();

    if (auto ch = peek_ahead(1); ch == '=') {
        advance(2);
        return Token::create(Token::Type::PLUS_EQUAL, "+=", Position::create(start, cursor()));
    }

    advance(1);
    return Token::create(Token::Type::PLUS, "+", Position::create(start, cursor()));
}

Token Lexer::lex_less_than() noexcept {
    const auto start = cursor();

    if (auto ch = peek_ahead(1); ch == '=') {
        advance(2);
        return Token::create(Token::Type::LESS_EQUAL, "<=", Position::create(start, cursor()));
    }

    advance(1);
    return Token::create(Token::Type::LESS, "<", Position::create(start, cursor()));
}
