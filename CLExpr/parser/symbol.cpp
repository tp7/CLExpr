#include "symbol.h"
#include <math.h>
#include <assert.h>

Symbol Symbol::Addition       ("+", OPERATOR, 2, "+");
Symbol Symbol::Multiplication ("*", OPERATOR, 2, "*");
Symbol Symbol::Division       ("/", OPERATOR, 2, "/");
Symbol Symbol::Substraction   ("-", OPERATOR, 2, "-");
Symbol Symbol::Power          ("^", FUNCTION, 2, "pow");

Symbol Symbol::Modulo         ("%", FUNCTION, 2, "fmod");
Symbol Symbol::Interrogation  ("?", FUNCTION, 3, "interrogation");
Symbol Symbol::Equal          ("==", FUNCTION, 2, "equal");
Symbol Symbol::Equal2         ("=", FUNCTION, 2, "equal");
Symbol Symbol::NotEqual       ("!=", FUNCTION, 2, "notEqual");
Symbol Symbol::Inferior       ("<=", FUNCTION, 2, "inferior");
Symbol Symbol::InferiorStrict ("<", FUNCTION, 2, "inferiorStrict");
Symbol Symbol::Superior       (">=", FUNCTION, 2, "superior");
Symbol Symbol::SuperiorStrict (">", FUNCTION, 2, "superiorStrict");
Symbol Symbol::And            ("&", FUNCTION, 2, "mt_and");
Symbol Symbol::Or             ("|", FUNCTION, 2, "mt_or");
Symbol Symbol::AndNot         ("&!", FUNCTION, 2, "mt_andNot");
Symbol Symbol::Xor            ("°", "@", FUNCTION, 2, "mt_xor");
Symbol Symbol::AndUB          ("&u", FUNCTION, 2, "andUB");
Symbol Symbol::OrUB           ("|u", FUNCTION, 2, "orUB");
Symbol Symbol::XorUB          ("°u", "@u", FUNCTION, 2, "xorUB");
Symbol Symbol::NegateUB       ("~u", FUNCTION, 1, "negateUB");
Symbol Symbol::PosShiftUB     ("<<", "<<u", FUNCTION, 2, "posshiftUB");
Symbol Symbol::NegShiftUB     (">>", ">>u", FUNCTION, 2, "negshiftUB");
Symbol Symbol::AndSB          ("&s", FUNCTION, 2, "andSB");
Symbol Symbol::OrSB           ("|s", FUNCTION, 2, "orSB");
Symbol Symbol::XorSB          ("°s", "@s", FUNCTION, 2, "xorSB");
Symbol Symbol::NegateSB       ("~s", FUNCTION, 1, "negateSB");
Symbol Symbol::PosShiftSB     ("<<s", FUNCTION, 2, "posshiftSB");
Symbol Symbol::NegShiftSB     (">>s", FUNCTION, 2, "negshiftSB");

Symbol Symbol::Pi             ("pi", 3.1415927f, NUMBER, 0, "");
Symbol Symbol::X              ("x", VARIABLE_X, 0, "");
Symbol Symbol::Y              ("y", VARIABLE_Y, 0, "");
Symbol Symbol::Z              ("z", VARIABLE_Z, 0, "");

Symbol Symbol::Cos            ("cos", FUNCTION, 1, "cos");
Symbol Symbol::Sin            ("sin", FUNCTION, 1, "sin");
Symbol Symbol::Tan            ("tan", FUNCTION, 1, "tan");
Symbol Symbol::Log            ("log", FUNCTION, 1, "log");
Symbol Symbol::Exp            ("exp", FUNCTION, 1, "exp");
Symbol Symbol::Abs            ("abs", FUNCTION, 1, "fabs");
Symbol Symbol::Atan           ("atan", FUNCTION, 1, "atan");
Symbol Symbol::Acos           ("acos", FUNCTION, 1, "acos");
Symbol Symbol::Asin           ("asin", FUNCTION, 1, "asin");
Symbol Symbol::Round          ("round", FUNCTION, 1, "round");
Symbol Symbol::Clip           ("clip", FUNCTION, 3, "clamp");
Symbol Symbol::Min            ("min", FUNCTION, 2, "min");
Symbol Symbol::Max            ("max", FUNCTION, 2, "max");
Symbol Symbol::Ceil           ("ceil", FUNCTION, 1, "ceil");
Symbol Symbol::Floor          ("floor", FUNCTION, 1, "floor");
Symbol Symbol::Trunc          ("trunc", FUNCTION, 1, "trunc");

Symbol::Symbol() :
type(UNDEFINED), value(""), value2("")
{
}

Symbol::Symbol(std::string value, Type type, int nParameter, std::string op) :
type(type), value(value), value2(""), nParameter(nParameter), code(op)
{
}

Symbol::Symbol(std::string value, std::string value2, Type type, int nParameter, std::string op) :
type(type), value(value), value2(value2), nParameter(nParameter), code(op)
{
}

Symbol::Symbol(std::string value, float dValue, Type type, int nParameter, std::string op) :
type(type), value(value), value2(""), nParameter(nParameter), code(op)
{
}


Context::Context(const std::deque<Symbol> &expression)
{
   nPos = -1;
   nSymbols = expression.size();
   pSymbols = new Symbol[nSymbols];

   auto it = expression.begin();

   for ( int i = 0; i < nSymbols; i++, it++ )
      pSymbols[i] = *it;
}

Context::~Context()
{
   delete[] pSymbols;
}


std::string Context::rec_infix()
{
    const Symbol &s = pSymbols[--nPos];

    switch ( s.type )
    {
    case Symbol::VARIABLE_X: 
    case Symbol::VARIABLE_Y: 
    case Symbol::VARIABLE_Z: 
        return s.value;
    case Symbol::NUMBER: 
        if (s.value.find('.') == std::string::npos)
            return s.value + ".0f";
        return s.value;
    case Symbol::FUNCTION:
        if (s.nParameter == 1) {
            return s.code + "(" + rec_infix() + ")";
        } else if (s.nParameter == 2) {
            auto op2 = rec_infix();
            return s.code + "(" + rec_infix() + "," + op2 + ")";
        } else {
            auto op3 = rec_infix();
            auto op2 = rec_infix();
            return s.code + "(" + rec_infix() + "," + op2 + "," + op3 + ")";
        }
    case Symbol::OPERATOR:
        {
            auto op2 = rec_infix();
            return "(" + rec_infix() + s.code + op2 + ")";
        }
    default:
        assert(0);
        return "";
    }
}

std::string Context::infix()
{
   nPos = nSymbols;

   return rec_infix();
}
