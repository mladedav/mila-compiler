#include "lexan.h"
#include "ast.h"
#include <iostream>
#include <string>
#include <map>
#include <list>
#include <unordered_set>

#ifndef MILA_PARSER_H
#define MILA_PARSER_H

namespace mila
{
    class ParserException
    {
        public:
            ParserException ( const std::string & );
        private:
            std::string message;
            friend std::ostream & operator << ( std::ostream &, const ParserException & );
    };

    enum LLType
    {
        TERMINAL,
        NONTERMINAL,
    };

    enum Nonterminal
    {
        START,              // 0
        IDENT,              // 1
        ARRAY_OR_FUNCTION,  // 2
        ARRAY_BOUNDS,       // 3
        FUNCTION_PARAMS,    // 4
        MINUS_NUMB,         // 5
        NUMB,               // 6
        VALUE,              // 7
        POSITIVE_VALUE,     // 8
        EXPRESSION,         // 9
        EXPRESSION2,        // 10
        OPERATION,          // 11
        TYPE,               // 12
        DECLARATIONS,       // 13
        BLOCK,              // 14
        VARIABLE,           // 15
        VARIABLE2,          // 16
        VARIABLE3,          // 17
        CONSTANT,           // 18
        CONSTANT2,          // 19
        FUNCTION,           // 20
        PROCEDURE,          // 21
        PARAMETERS,         // 22
        PARAMETERS2,        // 23
        DEFINITION,         // 24
        STATEMENT,          // 25
        NEXT_STATEMENT,     // 26
        NEXT_STATEMENT2,    // 27
        COMMAND,            // 28
        CONTROL,            // 29
        BLOCK_OR_COMMAND,   // 30
        IF_ELSE_IF,         // 31
        IF_ELSE,            // 32
        TO_DOWNTO,          // 33
        PARAM,              // 34
        PARAM2,             // 35
        ASSIGN_OR_CALL,     // 36
        FOO,                // 37
        NONTERM_CNT
    };

    struct LLSymbol
    {
        LLSymbol ( const std::string & );
        LLSymbol ( const char * );
        LLSymbol ( const LexicalSymbol & );
        LLSymbol ( Nonterminal );
        bool isTerminal ( void ) const;
        bool isNonterminal ( void ) const;
        bool operator == ( const LLSymbol & ) const;
        bool operator != ( const LLSymbol & ) const;
        bool operator == ( const LexicalSymbol & ) const;
        bool operator != ( const LexicalSymbol & ) const;
        bool operator == ( const Nonterminal & ) const;
        bool operator != ( const Nonterminal & ) const;
        friend bool operator == ( const LexicalSymbol &, const LLSymbol & );
        friend bool operator != ( const LexicalSymbol &, const LLSymbol & );
        friend bool operator == ( const Nonterminal &, const LLSymbol & );
        friend bool operator != ( const Nonterminal &, const LLSymbol & );
        friend std::ostream & operator << ( std::ostream &, const LLSymbol & );
        LLType type;
        LexicalSymbol terminal;
        Nonterminal nonterminal;
    };

    class Parser;
    typedef std::unique_ptr<ExprAST> ( expandType ) ( const LexicalSymbol & );
    typedef std::unique_ptr<ExprAST> ( Parser::*expandPointer ) ( const LexicalSymbol & );
    typedef std::unordered_set < LexicalSymbol > LexicalSet;
    typedef std::list < LLSymbol > tokenList;
    class Parser
    {
        public:
            Parser ( Lexan && lex );
            int parseSymbol ( const LexicalSymbol & );
            void printQue ( std::ostream & os = std::cerr ) const;
            void parse ( void );
        private:
            void discard ( std::vector < LexicalSymbol > symbols );
            int readNumber ( void );
            std::string readIdentifier ( void );
            std::vector <std::unique_ptr <ExprAST>> getParameters ( void );
            std::vector <std::string> getParameterDefinitions ( void );
            std::unique_ptr <ExprAST> expression ( void );
            std::unique_ptr <ExprAST> ident ( void );
            std::unique_ptr <ExprAST> declarations ( void );
            std::unique_ptr <ExprAST> command ( void );
            std::unique_ptr <ExprAST> control ( void );
            std::unique_ptr <ExprAST> block ( void );
            std::unique_ptr <ExprAST> blockCommand ( void );
            std::unique_ptr <ExprAST> createExpression ( std::vector <std::unique_ptr <ExprAST>> operands, std::vector <OperEnum> operators ) const;
            void expansion ( const LexicalSymbol & );
            void comparison ( const LexicalSymbol & );
            LexicalSymbol getNextLS ( void );
            LexicalSymbol peekNextLS ( void );
            
            expandType start;
            expandType ident;
            expandType expression;
            expandType block;
            expandType variable;
            expandType constant;
            expandType constant2;
            expandType function;
            expandType procedure;
            expandType command;
            
            void expect ( tokenList && );
            void parserError ( const char *, const LexicalSymbol & ) const;
            void parserError ( const LexicalSymbol &, const LexicalSymbol & ) const;
            std::map < std::string, int > constants;
            std::map < std::string, int > variables;
            tokenList expected;
            expandPointer parseNonterm [ NONTERM_CNT ];
            Lexan lex;
    };
}

#endif
