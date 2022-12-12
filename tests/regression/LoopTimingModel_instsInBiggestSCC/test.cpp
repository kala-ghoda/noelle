#include <memory>
#include <string>
#include <unordered_set>

#include "ast.h"
#include "scanner.h"
#include "visitor.h"

namespace json_parser {

void *ASTNode::Accept(BaseVisitor &visitor) {
  return visitor.Visit(*this);
}

std::string ASTNode::ToString() const {
  return str_;
}

AST::AST(const std::string &input) : scanner_(input) {}

bool AST::Build() {
  return Parse(scanner_);
}

bool AST::Parse(Scanner &scanner) {
  Token tok;
  std::unique_ptr<Object> object = std::make_unique<Object>();
  std::unique_ptr<Array> array = std::make_unique<Array>();
  scanner.PushStatus();
  if (object->Parse(scanner)) {
    tok = scanner.GetNextToken();
    REQUIRE(tok == Token::END);

    str_ = object->ToString();
    children.push_back(std::move(object));
    scanner.PopStatus();
    return true;
  }
  scanner.RestoreStatus();
  if (array->Parse(scanner)) {
    tok = scanner.GetNextToken();
    REQUIRE(tok == Token::END);

    str_ = array->ToString();
    children.push_back(std::move(array));
    scanner.PopStatus();
    return true;
  }
fail:
  scanner.RestoreStatus();
  return false;
}

bool Object::Parse(Scanner &scanner) {
  scanner.PushStatus();
  std::string tmp;
  std::unique_ptr<Member> member;
  std::unordered_set<std::string> keys;
  auto GetMemberName = [&member]() -> auto{
    return member->children[0]->ToString();
  };

  Token tok = scanner.GetNextToken();
  REQUIRE(tok == Token::OBJ_OPEN);

  str_ += scanner.GetLastLexeme();
  member = std::make_unique<Member>();
  if (!member->Parse(scanner)) {
    tok = scanner.GetNextToken();
    goto closing;
  }

  str_ += member->ToString();
  REQUIRE(keys.find(GetMemberName()) == keys.end());

  keys.insert(GetMemberName());
  children.push_back(std::move(member));
  tok = scanner.GetNextToken();
  while (tok == Token::COMMA) {
    str_ += scanner.GetLastLexeme();
    member = std::make_unique<Member>();
    REQUIRE(member->Parse(scanner));
    REQUIRE(keys.find(GetMemberName()) == keys.end());

    keys.insert(GetMemberName());
    str_ += member->ToString();
    children.push_back(std::move(member));
    tok = scanner.GetNextToken();
  }

closing:
  REQUIRE(tok == Token::OBJ_CLOSE);
  str_ += scanner.GetLastLexeme();
  scanner.PopStatus();
  return true;

fail:
  scanner.RestoreStatus();
  scanner.PopStatus();
  return false;
}

bool Array::Parse(Scanner &scanner) {
  scanner.PushStatus();
  std::unique_ptr<Value> value;

  Token tok = scanner.GetNextToken();
  REQUIRE(tok == Token::ARRAY_OPEN);

  str_ += scanner.GetLastLexeme();
  value = std::make_unique<Value>();
  if (!value->Parse(scanner)) {
    tok = scanner.GetNextToken();
    goto closing;
  }

  str_ += value->ToString();
  children.push_back(std::move(value));
  tok = scanner.GetNextToken();
  while (tok == Token::COMMA) {
    str_ += scanner.GetLastLexeme();
    value = std::make_unique<Value>();
    REQUIRE(value->Parse(scanner));

    str_ += value->ToString();
    children.push_back(std::move(value));
    tok = scanner.GetNextToken();
  }

closing:
  REQUIRE(tok == Token::ARRAY_CLOSE);
  str_ += scanner.GetLastLexeme();
  scanner.PopStatus();
  return true;

fail:
  scanner.RestoreStatus();
  scanner.PopStatus();
  return false;
}

bool Member::Parse(Scanner &scanner) {
  scanner.PushStatus();
  Token tok;
  std::string name_lexeme;
  std::unique_ptr<Name> name = std::make_unique<Name>();
  std::unique_ptr<Value> value;

  REQUIRE(name->Parse(scanner));

  name_lexeme = scanner.GetLastLexeme();
  tok = scanner.GetNextToken();
  REQUIRE(tok == Token::COLON);

  value = std::make_unique<Value>();
  REQUIRE(value->Parse(scanner));

  str_ = name_lexeme + ": " + value->ToString();
  children.push_back(std::move(name));
  children.push_back(std::move(value));
  scanner.PopStatus();
  return true;

fail:
  scanner.RestoreStatus();
  scanner.PopStatus();
  return false;
}

bool Value::Parse(Scanner &scanner) {
  scanner.PushStatus();

  std::unique_ptr<Object> object = std::make_unique<Object>();
  if (object->Parse(scanner)) {
    str_ = object->ToString();
    children.push_back(std::move(object));
    scanner.PopStatus();
    return true;
  }
  scanner.RestoreStatus();

  std::unique_ptr<Array> array = std::make_unique<Array>();
  if (array->Parse(scanner)) {
    str_ = array->ToString();
    children.push_back(std::move(array));
    scanner.PopStatus();
    return true;
  }
  scanner.RestoreStatus();

  std::unique_ptr<Literal> literal = std::make_unique<Literal>();
  if (literal->Parse(scanner)) {
    str_ = literal->ToString();
    children.push_back(std::move(literal));
    scanner.PopStatus();
    return true;
  }

  scanner.RestoreStatus();
  scanner.PopStatus();
  return false;
}

bool Literal::Parse(Scanner &scanner) {
  scanner.PushStatus();
  Token tok = scanner.GetNextToken();
  str_ = scanner.GetLastLexeme();

  switch (tok) {
    case Token::INT:
      type_ = Type::INT;
      break;
    case Token::FLOAT:
      type_ = Type::FLOAT;
      break;
    case Token::STRING:
      type_ = Type::STRING;
      break;
    case Token::BOOL:
      type_ = Type::BOOL;
      break;
    case Token::NULLTOKEN:
      type_ = Type::NULLTYPE;
      break;
    default:
      scanner.RestoreStatus();
      scanner.PopStatus();
      return false;
  }

  scanner.PopStatus();
  return true;
}

bool Name::Parse(Scanner &scanner) {
  scanner.PushStatus();
  Token tok = scanner.GetNextToken();
  std::string name_lexeme = scanner.GetLastLexeme();
  REQUIRE(tok == Token::STRING);

  str_ = name = name_lexeme;
  scanner.PopStatus();
  return true;

fail:
  scanner.RestoreStatus();
  scanner.PopStatus();
  return false;
}

Literal::Type Literal::GetType() const {
  return type_;
}

/* 'Accept' overrides */

void *AST::Accept(BaseVisitor &visitor) {
  return visitor.Visit(*this);
}
void *Object::Accept(BaseVisitor &visitor) {
  return visitor.Visit(*this);
}
void *Array::Accept(BaseVisitor &visitor) {
  return visitor.Visit(*this);
}
void *Member::Accept(BaseVisitor &visitor) {
  return visitor.Visit(*this);
}
void *Name::Accept(BaseVisitor &visitor) {
  return visitor.Visit(*this);
}
void *Value::Accept(BaseVisitor &visitor) {
  return visitor.Visit(*this);
}
void *Literal::Accept(BaseVisitor &visitor) {
  return visitor.Visit(*this);
}

} // namespace json_parser
#include <fstream>
#include <iostream>
#include <sstream>

#include "json_parser.h"

int main(int argc, char **argv) {
  std::stringstream input;

  if (argc > 1) {
    std::ifstream file(argv[1]);
    input << file.rdbuf();
  } else {
    input << std::cin.rdbuf();
  }

  json_parser::AST ast(input.str());
  if (!ast.Build()) {
    std::cout << "\e[0;31mERROR \e[0m: input text is not in JSON format"
              << std::endl;
    return 1;
  }

  std::cout << "\e[0;32mOK \e[0m: input text is in JSON format" << std::endl;
  return 0;
}
#include <string>
#include <memory>
#include <iostream>
#include <stack>

#include "scanner.h"
#include "token.h"

#define RETURN_TOKEN(x)                                                        \
  last_token_ = (x);                                                           \
  return last_token_

namespace json_parser {

Scanner::Scanner(const std::string &inStream) {
  input_ = input_.append(inStream);
}

Token Scanner::GetNextToken() {
  while (ValidPos()) {
    if (IsWhiteSpace(Char())) {
      ++position_;
      continue;
    }

    switch (Char()) {
      case ',':
        Validate(1);
        RETURN_TOKEN(Token::COMMA);
      case ':':
        Validate(1);
        RETURN_TOKEN(Token::COLON);
      case '[':
        Validate(1);
        RETURN_TOKEN(Token::ARRAY_OPEN);
      case ']':
        Validate(1);
        RETURN_TOKEN(Token::ARRAY_CLOSE);
      case '{':
        Validate(1);
        RETURN_TOKEN(Token::OBJ_OPEN);
      case '}':
        Validate(1);
        RETURN_TOKEN(Token::OBJ_CLOSE);
      case '"':
        if (Validate(String())) {
          RETURN_TOKEN(Token::STRING);
        }
        RETURN_TOKEN(Token::UNKNOWN);
      default:
        if (IsDigit(Char()) || Char() == '.' || Char() == '-') {
          if (Validate(Integer())) {
            RETURN_TOKEN(Token::INT);
          }
          if (Validate(Float())) {
            RETURN_TOKEN(Token::FLOAT);
          }
        }
        if (Char() == 't' || Char() == 'f') {
          if (Validate(Boolean())) {
            RETURN_TOKEN(Token::BOOL);
          }
        }
        if (Char() == 'n') {
          if (Validate(Null())) {
            RETURN_TOKEN(Token::NULLTOKEN);
          }
        }
        RETURN_TOKEN(Token::UNKNOWN);
    }
  }
  RETURN_TOKEN(Token::END);
}

Token Scanner::GetLastToken() const {
  return last_token_;
}

bool Scanner::ValidPos(int offset) const {
  return position_ + offset < input_.size();
}

bool Scanner::ValidPos() const {
  return ValidPos(0);
}

void Scanner::PushStatus() {
  saved_pos_.push(position_);
  saved_lexeme_.push(last_lexeme_);
  saved_token_.push(last_token_);
}

void Scanner::RestoreStatus() {
  position_ = saved_pos_.top();
  last_lexeme_ = saved_lexeme_.top();
  last_token_ = saved_token_.top();
}

void Scanner::PopStatus() {
  saved_pos_.pop();
  saved_lexeme_.pop();
  saved_token_.pop();
}

char Scanner::Char(int offset) const {
  return input_[position_ + offset];
}

char Scanner::Char() const {
  return Char(0);
}

int Scanner::String() const {
  int offset = 0;
  if (Char() != '"') {
    return -1;
  }
  ++offset;
  bool escape = false;
  while (ValidPos(offset)) {
    switch (Char(offset)) {
      case '\\':
        ++offset;
        escape = !escape;
        break;
      case '\n':
      case '\r':
      case '\t':
        return -1;
      case '"':
        ++offset;
        if (escape) {
          escape = false;
          break;
        }
        return offset;
      default:
        if (escape) {
          REQUIRE(Char(offset) == 'b' || Char(offset) == 'f'
                  || Char(offset) == 'n' || Char(offset) == 'r'
                  || Char(offset) == 't' || Char(offset) == 'u'
                  || Char(offset) == '/');
          if (Char(offset) == 'u') {
            for (int i = 0; i < 4; i++) {
              ++offset;
              REQUIRE(ValidPos(offset));
              REQUIRE(IsHex(Char(offset)));
            }
          }
        }
        ++offset;
        escape = false;
        break;
    }
  }

fail:
  return -1;
}

int Scanner::Integer() const {
  int offset = 0;
  REQUIRE(ValidPos(offset));
  if (Char(offset) == '-') {
    ++offset;
    REQUIRE(ValidPos(offset));
    REQUIRE(IsDigit(Char(offset)));
  }
  if (Char(offset) == '0') {
    ++offset;
    if (ValidPos(offset)) {
      REQUIRE(!IsDigit(Char(offset)));
      if (Char(offset) != '.') {
        return offset;
      }
    }
  }
  while (ValidPos(offset)) {
    if (IsDigit(Char(offset))) {
      ++offset;
    } else {
      REQUIRE(Char(offset) != '.');
      REQUIRE(Char(offset) != 'e');
      REQUIRE(Char(offset) != 'E');
      return offset;
    }
  }
  return offset;

fail:
  return -1;
}

int Scanner::Float() const {
  int offset = 0;
  bool foundDot = false;
  bool foundE = false;
  REQUIRE(ValidPos(offset));
  if (Char(offset) == '-') {
    ++offset;
    REQUIRE(IsDigit(Char(offset)));
  }
  if (Char(offset) == '0') {
    ++offset;
    if (ValidPos(offset)) {
      REQUIRE(!IsDigit(Char(offset)));
      if (Char(offset) != '.') {
        return offset;
      }
    }
  }
  while (ValidPos(offset)) {
    if (IsDigit(Char(offset))) {
    } else if (Char(offset) == '.') {
      REQUIRE(!foundDot);
      foundDot = true;
      ++offset;
      REQUIRE(ValidPos(offset));
      REQUIRE(IsDigit(Char(offset)));
    } else if (Char(offset) == 'e' || Char(offset) == 'E') {
      REQUIRE(!foundE);
      foundE = true;
      ++offset;
      REQUIRE(ValidPos(offset));
      if (!IsDigit(Char(offset))) {
        REQUIRE(Char(offset) == '+' || Char(offset) == '-');
        ++offset;
        REQUIRE(ValidPos(offset));
        REQUIRE(IsDigit(Char(offset)));
      }
    } else {
      break;
    }
    ++offset;
  }
  return offset;

fail:
  return -1;
}

int Scanner::Boolean() const {
  if (input_.compare(position_, 4, "true") == 0) {
    return 4;
  }
  if (input_.compare(position_, 5, "false") == 0) {
    return 5;
  }
  return -1;
}

int Scanner::Null() const {
  if (input_.compare(position_, 4, "null") == 0) {
    return 4;
  }
  return -1;
}

bool Scanner::IsDigit(char c) const {
  return '0' <= c && c <= '9';
}

bool Scanner::IsHex(char c) const {
  return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f')
         || ('A' <= c && c <= 'F');
}

bool Scanner::IsWhiteSpace(char c) const {
  return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

bool Scanner::Validate(int length) {
  if (length <= 0) {
    last_lexeme_ = "";
    return false;
  }
  std::size_t newPos = position_ + length;
  if (newPos <= input_.size()) {
    last_lexeme_ =
        std::string(input_.begin() + position_, input_.begin() + newPos);
    position_ += length;
    return true;
  }
  last_lexeme_ = "";
  std::cerr << "ERROR 0 Scanner::Validate" << std::endl;
  return false;
}

std::string Scanner::GetLastLexeme() const {
  return last_lexeme_;
}

}; // namespace json_parser
#include <string>

#include "token.h"

namespace json_parser {

std::string TokenToString(Token t) {
  switch (t) {
    case Token::ARRAY_OPEN:
      return "[";
    case Token::ARRAY_CLOSE:
      return "]";
    case Token::OBJ_OPEN:
      return "{";
    case Token::OBJ_CLOSE:
      return "}";
    case Token::COMMA:
      return ",";
    case Token::COLON:
      return ":";
    case Token::UNKNOWN:
      return "UNKNOWN";
    case Token::FLOAT:
      return "FLOAT";
    case Token::INT:
      return "INT";
    case Token::STRING:
      return "STRING";
    case Token::END:
      return "END";
    default:
      return "";
  }
}

} // namespace json_parser
#include "visitor.h"

namespace json_parser {

void *BaseVisitor::Visit(ASTNode &) {
  return nullptr;
}
void *BaseVisitor::Visit(AST &) {
  return nullptr;
}
void *BaseVisitor::Visit(Object &) {
  return nullptr;
}
void *BaseVisitor::Visit(Array &) {
  return nullptr;
}
void *BaseVisitor::Visit(Member &) {
  return nullptr;
}
void *BaseVisitor::Visit(Name &) {
  return nullptr;
}
void *BaseVisitor::Visit(Value &) {
  return nullptr;
}
void *BaseVisitor::Visit(Literal &) {
  return nullptr;
}

} // namespace json_parser
