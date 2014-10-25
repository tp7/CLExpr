#ifndef __Mt_Symbol_H__
#define __Mt_Symbol_H__

#include <string>
#include <deque>
#include <stdint.h>


class Symbol {
public:
   typedef enum {
      NUMBER,
      OPERATOR,
      FUNCTION,
      TERNARY,
      VARIABLE_X,
      VARIABLE_Y,
      VARIABLE_Z,

      UNDEFINED

   } Type;

public:

   Type type;
   std::string value;
   std::string value2;
   int nParameter;
   std::string code;

private:

private:
public:

   Symbol();
   Symbol(std::string value, Type type, int nParameter, std::string op);
   Symbol(std::string value, std::string value2, Type type, int nParameter, std::string op);
   Symbol(std::string value, float dValue, Type type, int nParameter, std::string op);

   static Symbol Addition;
   static Symbol Multiplication;
   static Symbol Division;
   static Symbol Substraction;
   static Symbol Power;
   static Symbol Modulo;
   static Symbol Interrogation;
   static Symbol Equal;
   static Symbol Equal2;
   static Symbol NotEqual;
   static Symbol Inferior;
   static Symbol InferiorStrict;
   static Symbol Superior;
   static Symbol SuperiorStrict;
   static Symbol And;
   static Symbol Or;
   static Symbol AndNot;
   static Symbol Xor;
   static Symbol AndUB;
   static Symbol OrUB;
   static Symbol XorUB;
   static Symbol NegateUB;
   static Symbol PosShiftUB;
   static Symbol NegShiftUB;
   static Symbol AndSB;
   static Symbol OrSB;
   static Symbol XorSB;
   static Symbol NegateSB;
   static Symbol PosShiftSB;
   static Symbol NegShiftSB;
   static Symbol Pi;
   static Symbol X;
   static Symbol Y;
   static Symbol Z;
   static Symbol Cos;
   static Symbol Sin;
   static Symbol Tan;
   static Symbol Log;
   static Symbol Abs;
   static Symbol Exp;
   static Symbol Acos;
   static Symbol Atan;
   static Symbol Asin;
   static Symbol Round;
   static Symbol Clip;
   static Symbol Min;
   static Symbol Max;
   static Symbol Ceil;
   static Symbol Floor;
   static Symbol Trunc;
};

class Context {
   Symbol *pSymbols;
   int nSymbols;
   int nPos;

   std::string rec_infix();
public:
   
   Context(const std::deque<Symbol> &expression);
   ~Context();
   std::string infix();
};



#endif
