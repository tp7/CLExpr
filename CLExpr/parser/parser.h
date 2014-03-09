#ifndef __Mt_Parser_H__
#define __Mt_Parser_H__

#include <string>
#include "symbol.h"
#include <deque>



class Parser
{
    std::string parsed_string;
    std::deque<Symbol> elements;
    std::deque<Symbol> symbols;
    const Symbol *findSymbol(const std::string &value) const;
    Symbol stringToSymbol(const std::string &value) const;

public:
    Parser &parse(const std::string &parsed_string, const std::string &separators);
    Parser();
    Parser &addSymbol(const Symbol &symbol);
    std::deque<Symbol> &getExpression() { return elements; }

};
Parser getDefaultParser();



#endif
