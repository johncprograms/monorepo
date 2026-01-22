
#include <iostream>
#include <string_view>
#include <vector>
#include <deque>
#include <array>
#include <span>
#include <cassert>

using namespace std;

enum class tokentype_t {
  ident,
  slash,
  percent,
  plus,
  star,
  minus,
  paren_l,
  paren_r,
};
struct token_t {
  tokentype_t type;
  string_view sv;
};

#define isalphalower(c) ('a' <= (c) && (c) <= 'z')
#define isalphaupper(c) ('A' <= (c) && (c) <= 'Z')
#define isnum(c)        ('0' <= (c) && (c) <= '9')
#define isvarcharfirst(c) (isalphalower(c) || isalphaupper(c) || (c) == '_')
#define isvarchar(c)      (isalphalower(c) || isalphaupper(c) || (c) == '_' || isnum(c))
bool Tokenize(vector<token_t>& out, string_view in) {
  out.clear();
  const char* rgch = in.data();
  const size_t cch = in.length();
  for (size_t ich = 0; ich < cch; ) {
    const char c = rgch[ich];
    if (isspace(c)) { ++ich; continue; }

    #define Handle1CharToken(ctoken_, type_) \
      if (c == ctoken_) { \
        out.emplace_back(type_, string_view(&rgch[ich], 1)); \
        ++ich; \
        continue; \
      }
      
    Handle1CharToken('/', tokentype_t::slash);
    Handle1CharToken('%', tokentype_t::percent);
    Handle1CharToken('+', tokentype_t::plus);
    Handle1CharToken('*', tokentype_t::star);
    Handle1CharToken('-', tokentype_t::minus);
    Handle1CharToken('(', tokentype_t::paren_l);
    Handle1CharToken(')', tokentype_t::paren_r);
    
    if (isvarcharfirst(c)) {
      auto j = ich + 1;
      while (j < cch && isvarchar(rgch[j])) ++j;
      // [ich,j) is the variable name.
      out.emplace_back(tokentype_t::ident, string_view(&rgch[ich], j - ich));
      ich = j;
      continue;
    }
    printf("ERROR: Unexpected character at offset: %zu\n", ich);
    return false;
  }
  return true;
}

enum class exprtype_t {
  var,
  fncall,
};
struct expr_var_t {
  string_view name;
};
struct expr_t;
// NOTE: We don't actually need to distinguish unop/binop from fncall, it's just syntax/sugar.
// We use a pre-defined name to identify builtin unops/binops.
struct expr_fncall_t {
  string_view name;
  vector<expr_t*> args;
};
struct expr_t {
  exprtype_t type;

  // TODO: should we destructure? Both have names, for example.
  // union section:
  expr_var_t var;
  expr_fncall_t fncall;
};
void PrintExpr(string& r, expr_t* e) {
  switch (e->type) {
    default: assert(false); terminate();
    case exprtype_t::var: {
      r += e->var.name;
    } break;
    case exprtype_t::fncall: {
      const auto& fncall = e->fncall;
      r += fncall.name;
      r += "(";
      for (size_t i = 0; i < fncall.args.size(); ++i) {
        if (i) r += ",";
        PrintExpr(r, fncall.args[i]);
      }
      r += ")";
    } break;
  } // end switch (e->type)
}
bool ParseExpr(size_t& itoken, span<token_t> tokens, expr_t*& r);
bool ParseExprWithoutBinop(size_t& itoken, span<token_t> tokens, expr_t*& r) {
  if (itoken >= tokens.size()) {
    printf("ERROR: Expected a token.\n");
    return false;
  }
  const auto& tok = tokens[itoken];
  switch (tok.type) {
    default: assert(false); terminate();
    case tokentype_t::paren_l: {
      // Recurse in the case of: '(' expr ')'
      ++itoken;
      if (!ParseExpr(itoken, tokens, r)) return false;
      if (itoken >= tokens.size() || tokens[itoken].type != tokentype_t::paren_r) {
        printf("ERROR: Expected ')'.\n");
        return false;
      }
      ++itoken;
    } break;
    case tokentype_t::ident: {
      ++itoken;
      auto e = new expr_t;
      e->type = exprtype_t::var;
      e->var.name = tok.sv;
      r = move(e);
    } break;
    case tokentype_t::plus: {
      // Unop plus is a no-op, so just skip it.
      ++itoken;
      if (!ParseExprWithoutBinop(itoken, tokens, r)) return false;
    } break;
    case tokentype_t::minus: {
      ++itoken;
      expr_t* e0 = nullptr;
      if (!ParseExprWithoutBinop(itoken, tokens, e0)) return false;
      auto e = new expr_t;
      e->type = exprtype_t::fncall;
      e->fncall.name = "negate";
      e->fncall.args.emplace_back(move(e0));
      r = move(e);
    } break;
    case tokentype_t::paren_r:
    case tokentype_t::slash:
    case tokentype_t::percent:
    case tokentype_t::star: {
      printf("ERROR: Cannot start an expression with ')' or '/' or '*'.\n");
      return false;
    } break;
  }
  return true;
}
enum class binoptype_t {
  add,
  sub,
  mul,
  div,
  rem,
};
string_view NameOfBinop(binoptype_t binop) {
  switch (binop) {
    default: assert(false); terminate();
    case binoptype_t::add: return "add";
    case binoptype_t::div: return "div";
    case binoptype_t::mul: return "mul";
    case binoptype_t::sub: return "sub";
    case binoptype_t::rem: return "rem";
  }
}
// Higher precedence means they'll be evaluated first.
constexpr size_t c_precedenceMax = 1;
constexpr size_t c_precedenceMin = 0;
constexpr size_t c_cPrecedence = c_precedenceMax - c_precedenceMin + 1;
size_t Precedence(binoptype_t binop) {
  switch (binop) {
    default: assert(false); terminate();
    case binoptype_t::mul:
    case binoptype_t::div: 
    case binoptype_t::rem: return 1;
    case binoptype_t::add:
    case binoptype_t::sub: return 0;
  }
}
enum class associativity_t { l2r, r2l };
// Each precedence level has a defined associativity, aka the order of evaluation for binop chains at that level.
associativity_t Associativity(size_t precedence) {
  switch (precedence) {
    default: assert(false); terminate();
    case 1: return associativity_t::l2r;
    case 0: return associativity_t::l2r;
  }
}
bool AssociativityL2R(size_t precedence) { 
  switch (precedence) {
    default: assert(false); terminate();
    case 1: return true;
    case 0: return true;
  }
}
bool ParseExpr(size_t& itoken, span<token_t> tokens, expr_t*& r) {

  // For clarity, I'm not using operator/operand terminology.
  // Instead, I'll use op/arg which is shorter.
  vector<expr_t*> args;
  vector<binoptype_t> ops;

  {
    expr_t* e0 = nullptr;
    if (!ParseExprWithoutBinop(itoken, tokens, e0)) return false;
    assert(e0);
    args.emplace_back(move(e0));
  }

  while (itoken < tokens.size()) {
    const auto& tok = tokens[itoken];
    switch (tok.type) {
      default: assert(false); terminate();
      case tokentype_t::paren_l:
      case tokentype_t::paren_r:
      case tokentype_t::ident: {
        // This is silent, since we're speculatively looking forward for an infix operator.
        // TODO: support other infix operators (e.g. 'add' instead of '+').
        // TODO: user-defined infix operators. Lookup the ident from some user-defined table seen previously, and look for it here.
        goto Done;
      } break;

      #define HandleBinop(token_, binop_) \
        case token_: { \
          ++itoken; \
          expr_t* e1 = nullptr; \
          if (!ParseExprWithoutBinop(itoken, tokens, e1)) return false; \
          assert(e1); \
          args.emplace_back(move(e1)); \
          ops.emplace_back(binop_); \
        } break;

      HandleBinop(tokentype_t::plus, binoptype_t::add);
      HandleBinop(tokentype_t::minus, binoptype_t::sub);
      HandleBinop(tokentype_t::star, binoptype_t::mul);
      HandleBinop(tokentype_t::slash, binoptype_t::div);
      HandleBinop(tokentype_t::percent, binoptype_t::rem);

    }

  }
Done:
  assert(args.size() == ops.size() + 1);

  // now we've got a binop chain. 
  // A o0 B o1 C o2 D ...

  if (!ops.empty()) {
    const size_t cops = ops.size();
    const size_t cargs = args.size();
    // List of indices for each precedence level.
    array<deque<size_t>, c_cPrecedence> iopsAtPrecedence;
    static constexpr size_t EMPTY = numeric_limits<size_t>::max();
    vector<size_t> iopL(cops);
    vector<size_t> iopR(cops);
    vector<size_t> iargL(cops);
    vector<size_t> iargR(cops);
    for (size_t i = 0; i < cops; ++i) {
      const binoptype_t binop = ops[i];
      const size_t precedence = Precedence(binop);
      auto& iops = iopsAtPrecedence[precedence];
      if (AssociativityL2R(precedence)) {
        iops.push_back(i);
      }
      else { // R2L
        iops.push_front(i);
      }
      iopL[i] = i == 0 ? EMPTY : i - 1;
      iopR[i] = i == cops - 1 ? EMPTY : i + 1;
      iargL[i] = i;
      iargR[i] = i + 1;
    }
    // Iterate downward, from max to min.
    for (size_t precedence = c_precedenceMax + 1; precedence-- != c_precedenceMin; ) {
    #if 1
      const auto& iops = iopsAtPrecedence[precedence];
      for (size_t iop : iops) {
        const binoptype_t binop = ops[iop];
        const size_t iargLHS = iargL[iop];
        const size_t iargRHS = iargR[iop];
        auto e = new expr_t;
        e->type = exprtype_t::fncall;
        e->fncall.name = NameOfBinop(binop);
        e->fncall.args.emplace_back(move(args[iargLHS]));
        e->fncall.args.emplace_back(move(args[iargRHS]));
        const size_t iargNew = args.size();
        args.emplace_back(move(e));
        // Splice out the operator and its operands.
        if (iopL[iop] != EMPTY) {
          iopR[iopL[iop]] = iopR[iop];
          iargR[iopL[iop]] = iargNew;
        }
        if (iopR[iop] != EMPTY) {
          iopL[iopR[iop]] = iopL[iop];
          iargL[iopR[iop]] = iargNew;
        }
      }
    #else
      // WARNING: operators is modified as we iterate.
      for (size_t i = 0; i < ops.size(); ++i) {
        const binoptype_t binop = ops[i];
        if (precedence != Precedence(binop)) continue;
          
        auto e = new expr_t;
        e->type = exprtype_t::fncall;
        e->fncall.name = NameOfBinop(binop);
        e->fncall.args.emplace_back(move(args[i]));
        e->fncall.args.emplace_back(move(args[i + 1]));
        ops.erase(begin(ops)+i);
        args.erase(begin(args)+i);
        args[i] = move(e);
        --i;
      }
    #endif
    }
  }
#if 1
  assert(args.size() == 2 * ops.size() + 1);
  r = move(args.back());
#else
  assert(ops.empty());
  assert(args.size() == 1);
  r = move(args[0]);
#endif
  return true;
}

int main()
{
  string_view input { "-a/b+c*(d-e)%f*g" };
  vector<token_t> tokens;
  if (!Tokenize(tokens, input)) return 1;
  size_t itoken = 0;
  expr_t* root = 0;
  if (!ParseExpr(itoken, tokens, root)) return 1;
  string s;
  PrintExpr(s, root);
  cout << s << endl;
  return 0;
}

#if 0

QUESTION: How do we distinguish ambiguity of
  fn a + b
which could be one of:
  fn(a) + b
  fn(a + b)

source:
  a/b+c
syntax tree we want:
  add
    div
      a
      b
    c
  add(div(a,b),c)
token sequence:
  a div b add c

#endif
