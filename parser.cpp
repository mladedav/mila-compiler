#include "lexan.h"
#include "parser.h"
#include <iostream>
#include <string>
#include <map>
#include <list>
#include <unordered_set>
#include <sstream>
#include <utility>

//#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/FileSystem.h"

using namespace mila;

//=========================================================
mila::LLSymbol::LLSymbol ( const LexicalSymbol & ls )
: terminal ( ls )
{
    type = TERMINAL;
}

mila::LLSymbol::LLSymbol ( const std::string & str )
: terminal ( LexicalSymbol ( str ) )
{
    type = TERMINAL;
}

mila::LLSymbol::LLSymbol ( const char * str )
: terminal ( LexicalSymbol ( str ) )
{
    type = TERMINAL;
}

mila::LLSymbol::LLSymbol ( Nonterminal nonterm )
: nonterminal ( nonterm )
{
    type = NONTERMINAL;
}

bool mila::LLSymbol::isTerminal ( void ) const
{
    return type == TERMINAL;
}

bool mila::LLSymbol::isNonterminal ( void ) const
{
    return type == NONTERMINAL;
}

bool mila::LLSymbol::operator == ( const LLSymbol & lls ) const
{
    if ( this -> type != lls . type )
        return false;
    if ( this -> type == TERMINAL )
        return this -> terminal == lls . terminal;
    return this -> nonterminal == lls . nonterminal;
}

bool mila::LLSymbol::operator != ( const LLSymbol & lls ) const
{
    return !( *this == lls );
}

bool mila::LLSymbol::operator == ( const LexicalSymbol & ls ) const
{
    if ( this -> type != TERMINAL )
        return false;
    return this -> terminal == ls;
}

bool mila::LLSymbol::operator != ( const LexicalSymbol & ls ) const
{
    return !( *this == ls );
}

bool mila::LLSymbol::operator == ( const Nonterminal & nt ) const
{
    if ( this -> type != NONTERMINAL )
        return false;
    return this -> nonterminal == nt;
}

bool mila::LLSymbol::operator != ( const Nonterminal & nt ) const
{
    return !( *this == nt );
}

namespace mila
{
    bool operator == ( const LexicalSymbol & ls, const LLSymbol & lls )
    {
        return lls == ls;
    }

    bool operator != ( const LexicalSymbol & ls, const LLSymbol & lls )

    {
        return !( lls == ls );
    }

    bool operator == ( const Nonterminal & nt, const LLSymbol & lls )
    {
        return lls == nt;
    }

    bool operator != ( const Nonterminal & nt, const LLSymbol & lls )
    {
        return !( lls == nt );
    }

    std::ostream & operator << ( std::ostream & os, const LLSymbol & lls )
    {
        if ( lls . type == TERMINAL )
            return os << lls . terminal;
        return os << lls . nonterminal;
    }
}
//=========================================================
mila::ParserException::ParserException ( const std::string & error )
: message ( error )
{
}

namespace mila
{
    std::ostream & operator << ( std::ostream & os, const ParserException & error )
    {
        return os << error . message;
    }
}
//#########################################################
mila::Parser::Parser ( Lexan && lex )
: expected ( 1, LLSymbol ( START ) ),
parseNonterm
{
    &Parser::start,
    &Parser::ident,
    //&Parser::arrayOrParam,
    //&Parser::array,
    //&Parser::functionParams,
    //&Parser::minusNumb,
    //&Parser::numb,
    //&Parser::value,
    //&Parser::positiveValue,
    &Parser::expression,
    //&Parser::expression2,
    //&Parser::operation,
    //&Parser::type,
    //&Parser::declarations,
    &Parser::block,
    &Parser::variable,
    //&Parser::variable2,
    //&Parser::variable3,
    &Parser::constant,
    &Parser::constant2,
    &Parser::function,
    &Parser::procedure,
    //&Parser::parameters,
    //&Parser::parameters2,
    //&Parser::definition,
    //&Parser::statement,
    //&Parser::nextStatement,
    //&Parser::nextStatement2,
    &Parser::command,
    //&Parser::control,
    //&Parser::blockOrCommand,
    //&Parser::ifElseIf,
    //&Parser::ifElse,
    //&Parser::toDownto,
    //&Parser::param,
    //&Parser::param2,
    //&Parser::assign,
    //&Parser::foo
},
lex ( std::move ( lex ) )
{
}

void mila::Parser::printQue ( std::ostream & os ) const
{
    for ( auto it : expected )
        os << it << std::endl;
}

void mila::Parser::expect ( tokenList && next )
{
    expected . pop_front ();
    expected . splice ( expected . begin (), next );
std::cerr << "EXPECTING" << std::endl;
}

int mila::Parser::readNumber ( void )
{
    LexicalSymbol ls;
    lex >> ls;
    int i = 1;
    if ( ls == "-" )
    {
        i *= -1;
        lex >> ls;
    }
    if ( ls . type != INTEGER )
        parserError ( "number", ls );
    i *= ls . value;
    return i;
}

std::string mila::Parser::readIdentifier ( void )
{
    LexicalSymbol ls = getNextLS ();
    if ( ls . type != IDENTIFIER )
        parserError ( "identifier", ls );
    return ls . name;
}

void mila::Parser::parserError ( const char * expected, const LexicalSymbol & read ) const
{
            std::stringstream error;
            error << "Expected \"" << expected << "\", read \"" << read << "\".";
            throw ParserException ( error . str () );
}

void mila::Parser::parserError ( const LexicalSymbol & expected, const LexicalSymbol & read ) const
{
            std::stringstream error;
            error << "Expected \"" << expected << "\", read \"" << read << "\".";
            throw ParserException ( error . str () );
}

LexicalSymbol mila::Parser::getNextLS ( void )
{
    LexicalSymbol ls;
    lex >> ls;
    return ls;
}

LexicalSymbol mila::Parser::peekNextLS ( void )
{
    LexicalSymbol ls;
    lex >> ls;
    lex << ls;
    return ls;
}

void mila::Parser::parse ( void )
{
    start ( getNextLS () );
}

int mila::Parser::parseSymbol ( const LexicalSymbol & ls )
{
    if ( expected . empty () )
    {
        if ( ls . type != END_OF_INPUT )
            parserError ( LexicalSymbol ( END_OF_INPUT ), ls );
        return 0;
    }
    if ( expected . front () . isTerminal ()  )
        comparison ( ls );
    else
        expansion ( ls );
    return 1;
}

void mila::Parser::expansion ( const LexicalSymbol & ls )
{
    ( this ->* parseNonterm [ expected . front () . nonterminal ] ) ( ls );
}


void mila::Parser::comparison ( const LexicalSymbol & ls )
{
    if ( ls != this -> expected . front () )
    {
std::cerr<<"comparison: "<<std::endl;
        parserError ( expected . front () . terminal, ls );
    }

    this -> expected . pop_front ();
}

void mila::Parser::discard ( std::vector < LexicalSymbol > symbols )
{
    LexicalSymbol ls;
    for ( const auto & symbol : symbols )
    {
        lex >> ls;
        if ( ls != symbol )
            parserError ( symbol, ls );
    }
}

std::vector<std::unique_ptr<ExprAST>> mila::Parser::getParameters ( void )
{
    std::vector <std::unique_ptr<ExprAST>> args;
    discard ( { "(" } );
    LexicalSymbol next;
    while ( true )
    {
        auto argument = expression ();
        args . push_back ( std::move ( argument ) );
        next = getNextLS ();
        if ( next == "," )
            continue;
        else if ( next == ")" )
            break;
        else
            parserError ( "',' or ')'", peekNextLS () );
    }
    return args;
}

std::vector<std::string> mila::Parser::getParameterDefinitions ( void )
{
    std::vector<std::string> args;
    discard ( { "(" } );
    LexicalSymbol next = peekNextLS ();
    while ( true )
    {
        if ( next == ")" )
            break;
        std::string name = readIdentifier ();
        args . push_back ( name );
        discard ( { ":", "integer" } );
        next = getNextLS ();
        if ( next != ";" && next != ")" )
            parserError ( "';' or ')'", next );
    }
    return args;
}

std::unique_ptr <ExprAST> mila::Parser::createExpression ( std::vector <std::unique_ptr <ExprAST>> operands, std::vector <OperEnum> operators ) const
{
    while ( operands . size () != 1 )
    {
        for ( unsigned i = 1 ; i < operands . size () ; i ++ )
        {
            if ( i == operators . size () || getPrecedence ( operators [ i - 1 ] ) >= getPrecedence ( operators [ i ] ) )
            {
                operands [ i - 1 ] = make_unique <BinaryExprAST> ( operators [ i - 1 ], std::move ( operands [ i - 1 ] ), std::move ( operands [ i ] ) );
                operators . erase ( operators . begin () + ( i - 1 ) );
                operands . erase ( operands . begin () + i );
                break;
            }
        }
    }

    return std::move ( operands [ 0 ] );
}
//====================================================================================================================
std::unique_ptr<ExprAST> mila::Parser::start ( const LexicalSymbol & ls )
{
    if ( ls != "program" )
    {
        parserError ( LexicalSymbol ( "program" ), ls );
    }
    std::string name = readIdentifier ();
    discard ( { ";" } );

    Builder.SetInsertPoint(mainBlock);

    // Generate prototypes
    PrototypeAST("printi",{"x"}).codegen();
    PrototypeAST("writeln",{"x"}).codegen();
    PrototypeAST("dec",{"x"}).codegen();
    PrototypeAST("inc",{"x"}).codegen();
    Builder.SetInsertPoint(mainBlock);

    // My fun stuff
    try
    {
        auto decl = declarations ();
        decl -> codegen ();
        //std::cerr << "Declarations OK" << std::endl;
        auto bl = block ();
        //bl -> print ();
        Builder.SetInsertPoint(mainBlock);
        bl -> codegen ();
        //std::cerr << "Block OK" << std::endl;
        discard ( { "." } );
        //expect ({});
    }
    catch ( const char * e )
    {
        std::cerr << e << std::endl;
        exit (1);
        return nullptr;
    }


    // Create return
    Builder.CreateRet(NumberExprAST(0).codegen());

    // Write the code out
    raw_ostream *out;
    std::error_code EC;
    out = new raw_fd_ostream(std::string("binary/")+name, EC, sys::fs::F_None);
    WriteBitcodeToFile(TheModule.get(), *out);
    delete out;

    return nullptr;
}

std::unique_ptr<ExprAST> mila::Parser::ident ( void )
{
    std::string name = readIdentifier ();
    LexicalSymbol next = peekNextLS ();
    if ( next == "[" )
    {
        discard ( { "[" } );
        auto index = expression ();
        discard ( { "]" } );
        return make_unique <ArrayExprAST> ( name, std::move ( index ) );
    }
    return make_unique <VariableExprAST> ( name );
}

std::unique_ptr<ExprAST> mila::Parser::ident ( const LexicalSymbol & ls )
{
    lex << ls;
    expect ({});
    if ( ls . type == IDENTIFIER )
    {
        std::string name = readIdentifier ();
        LexicalSymbol next = peekNextLS ();
        if ( next == "[" )
        {
            discard ( { "[" } );
            auto index = expression ();
            discard ( { "]" } );
            return make_unique <ArrayExprAST> ( name, std::move ( index ) );
        }
        return make_unique <VariableExprAST> ( name );
    }
    else
        parserError ( LexicalSymbol ( IDENTIFIER ), ls );
    return nullptr;
}

std::unique_ptr<ExprAST> mila::Parser::expression ( void )
{
    std::vector <std::unique_ptr <ExprAST>> operands;
    std::vector <OperEnum> operators;
    LexicalSymbol next;
    do
    {
        next = peekNextLS ();
        if ( next . type == INTEGER || next == "-" )
        {
            auto expr = make_unique <NumberExprAST> ( readNumber () );
            operands . push_back ( std::move ( expr ) );
        }
        else if ( next . type == IDENTIFIER )
        {
            std::string name = readIdentifier ();
            next = peekNextLS ();
            if ( next == "[" )
            {
                discard ( { "[" } );
                auto index = expression ();
                discard ( { "]" } );
                auto expr = make_unique <ArrayExprAST> ( name, std::move ( index ) );
                operands . push_back ( std::move ( expr ) );
            }
            else if ( next == "(" )
            {
                std::vector <std::unique_ptr<ExprAST>> args;
                args = getParameters ();
                auto expr = make_unique <CallExprAST> ( name, std::move ( args ) );
                operands . push_back ( std::move ( expr ) );
            }
            else
            {
                auto expr = make_unique <VariableExprAST> ( name );
                operands . push_back ( std::move ( expr ) );
            }
        }
        else if ( next == "(" )
        {
            discard ( { "(" } );
            auto expr = expression ();
            discard ( { ")" } );
            operands . push_back ( std::move ( expr ) );
        }
        else
            parserError ( "Number, identifier or '('", next );

        next = peekNextLS ();
        if ( LexicalSet { "+", "-", "*", "div", "mod", "=", "<", ">", "<=", ">=", "<>", "and", "or" } . count ( next ) )
        {
            if ( next == "+" )
                operators . push_back ( ADD );
            if ( next == "-" )
                operators . push_back ( SUB );
            if ( next == "*" )
                operators . push_back ( MULT );
            if ( next == "div" )
                operators . push_back ( DIV );
            if ( next == "mod" )
                operators . push_back ( MOD );
            if ( next == "=" )
                operators . push_back ( EQ );
            if ( next == "<" )
                operators . push_back ( LT );
            if ( next == ">" )
                operators . push_back ( GT );
            if ( next == "<=" )
                operators . push_back ( LE );
            if ( next == ">=" )
                operators . push_back ( GE );
            if ( next == "<>" )
                operators . push_back ( NE );
            if ( next == "and" )
                operators . push_back ( AND );
            if ( next == "or" )
                operators . push_back ( OR );
            getNextLS ();
            continue;
        }
        else if ( LexicalSet { ";", ":=", ")", "]", "then", "do", "else", "end", ",", "to", "downto" } . count ( next ) )
            break;
    }
    while ( true );

    return createExpression ( std::move ( operands ), operators );
}

std::unique_ptr<ExprAST> mila::Parser::expression ( const LexicalSymbol & ls )
{
    std::unique_ptr<ExprAST>foo;
    lex << ls;
    LexicalSymbol next;
    do
    {
        next = peekNextLS ();
        if ( next . type == INTEGER || next == "-" )
        {
            auto expr = make_unique <NumberExprAST> ( readNumber () );
        }
        else if ( next . type == IDENTIFIER )
        {
            std::string name = readIdentifier ();
            next = peekNextLS ();
            if ( next == "[" )
            {
                discard ( { "[" } );
                auto index = expression ();
                discard ( { "]" } );
                auto expr = make_unique <ArrayExprAST> ( name, std::move ( index ) );
            }
            else if ( next == "(" )
            {
                std::vector <std::unique_ptr<ExprAST>> args;
                args = getParameters ();
                auto expr = make_unique <CallExprAST> ( name, std::move ( args ) );
            }
            else
            {
                auto expr = make_unique <VariableExprAST> ( name );
            }
        }
        else if ( next == "(" )
        {
            discard ( { "(" } );
            auto expr = expression ();
            discard ( { ")" } );
        }
        else
            parserError ( "Number, identifier or '('", next );

        next = peekNextLS ();
        if ( LexicalSet { "+", "-", "*", "div", "mod", "=", "<", ">", "<=", ">=", "<>", "and", "or" } . count ( next ) )
        {
            getNextLS ();
            continue;
        }
        else if ( LexicalSet { ";", ":=", ")", "]", "then", "do", "else", "end", ",", "to", "downto" } . count ( next ) )
        {
            expect ({});
            return nullptr;
        }
    }
    while ( true );

    return nullptr;
}

std::unique_ptr<ExprAST> mila::Parser::declarations ()
{
    LexicalSymbol next = getNextLS ();
    std::vector <std::unique_ptr <ExprAST>> decl;
    while ( true )
    {
        if ( next == "var" )
        {
            auto varDecl = variable ( next );
            decl . push_back ( std::move ( varDecl ) );
        }
        else if ( next == "const" )
        {
            auto constDecl = constant ( next );
            decl . push_back ( std::move ( constDecl ) );
        }
        else if ( next == "function" )
        {
            auto funcDecl = function ( next );
            decl . push_back ( std::move ( funcDecl ) );
        }
        else if ( next == "procedure" )
        {
            auto procDecl = procedure ( next );
            decl . push_back ( std::move ( procDecl ) );
        }
        else  if ( next == "begin" )
        {
            lex << next;
            return make_unique <ExprListAST> ( std::move ( decl ) );
        }
        else
            parserError ( "Declarations or 'begin'", next );
        lex >> next;
    }
    return nullptr;
}

std::unique_ptr<ExprAST> mila::Parser::block ()
{
    discard ( { "begin" } );
    std::vector <std::unique_ptr<ExprAST>> body;
    LexicalSymbol next;
    while ( true )
    {
        next = peekNextLS ();
        if ( LexicalSet { "readln", "writeln", "write", "exit", "dec", "inc" } . count ( next ) || next . type == IDENTIFIER )
        {
            auto comm = command ();
            body . push_back ( std::move ( comm ) );
        }
        else if ( LexicalSet { "if", "for", "while" } . count ( next ) )
        {
            auto contr = control ();
            body . push_back ( std::move ( contr ) );
        }
        else
            parserError ( "Statement or 'end'", next );

        next = getNextLS ();
        if ( next == ";" )
            next = getNextLS ();

        if ( LexicalSet { "readln", "writeln", "write", "exit", "dec", "inc", "if", "for", "while" } . count ( next ) || next . type == IDENTIFIER )
            lex << next;
        else if ( next == "end" )
            break;
        else
            parserError ( "Statement or 'end'", next );
    }
    return make_unique <ExprListAST> ( std::move ( body ) );
}

std::unique_ptr<ExprAST> mila::Parser::block ( const LexicalSymbol & ls )
{
    expect ( { "begin", STATEMENT, "end" } );
    parseSymbol ( ls );
    return nullptr;
}

std::unique_ptr<ExprAST> mila::Parser::variable ( const LexicalSymbol & ls )
{
    if ( ls != "var" )
        parserError ( LexicalSymbol ( "var" ), ls );
    std::vector<std::string> names;
    std::vector<std::unique_ptr<ExprAST>> decl;
    LexicalSymbol next;
    do
    {
        do
        {
            std::string name = readIdentifier ();
            names . push_back ( name );
            lex >> next;
            if ( next == ":" )
                break;
            if ( next != "," )
                parserError ( "':' or ','", next );
        }
        while ( true );

        lex >> next;
        if ( next == "integer" )
        {
            for ( std::string & name : names )
                decl . push_back ( make_unique <DeclareExprAST> ( name ) );
        }
        else if ( next == "array" )
        {
            discard ( { "[" } );
            int lo = readNumber ();
            discard ( { ".." } );
            int hi = readNumber ();
            discard ( { "]", "of", "integer" } );

            for ( std::string & name : names )
                decl . push_back ( make_unique <DeclareExprAST> ( name, -lo, hi - lo + 1 ) );
        }
        else
            parserError ( "'integer' or 'array'", next );

        names . clear ();

        discard ( { ";" } );
        lex >> next;

        if ( next == "var" || next == "const" || next == "function" || next == "procedure" || next == "begin" )
        {
            lex << next;
            break;
        }
        else if ( next . type == IDENTIFIER )
        {
            lex << next;
            continue;
        }
        parserError ( "'var', 'const', 'function', 'procedure', 'begin' or identifier", next );
    }
    while ( true );

    return make_unique <ExprListAST> ( std::move ( decl ) );
}

std::unique_ptr<ExprAST> mila::Parser::constant ( const LexicalSymbol & ls )
{
    if ( ls != "const" )
        parserError ( LexicalSymbol ( "const" ), ls );
    std::vector<std::unique_ptr<ExprAST>> decl;
    LexicalSymbol next;
    do
    {
        std::string name = readIdentifier ();
        discard ( { "=" } );
        int val = readNumber ();
        discard ( { ";" } );
        decl . push_back ( make_unique <ConstExprAST> ( name, val ) );

        next = peekNextLS ();
        if ( next == "var" || next == "const" || next == "function" || next == "procedure" || next == "begin" )
            break;
        else if ( next . type == IDENTIFIER )
            continue;
        parserError ( "'var', 'const', 'function', 'procedure', 'begin' or identifier", next );
    }
    while ( true );

    return make_unique <ExprListAST> ( std::move ( decl ) );
}

std::unique_ptr<ExprAST> mila::Parser::constant2 ( const LexicalSymbol & ls )
{
    return nullptr;
}

std::unique_ptr<ExprAST> mila::Parser::function ( const LexicalSymbol & ls )
{
    if ( ls != "function" )
        parserError ( LexicalSymbol ( "function" ), ls );
    std::string name = readIdentifier ();
    std::vector <std::string> args;
    args = getParameterDefinitions ();
    discard ( { ":", "integer", ";" } );
    auto proto = make_unique <PrototypeAST> ( name, std::move ( args ) );
    // add name to the mix of arguments/variables
    LexicalSymbol next = peekNextLS ();
    if ( next == "forward" )
    {
        discard ( { "forward", ";" } );
        return proto;
    }
    std::vector <std::unique_ptr<ExprAST>> body;
    while ( next == "var" )
    {
        auto decl = variable ( getNextLS () );
        body . push_back ( std::move ( decl ) );
        next = peekNextLS ();
    }
    auto def = block ();
    discard ( { ";" } );
    body . push_back ( make_unique <DeclareExprAST> ( name ) );
    body . push_back ( std::move ( def ) );
    body . push_back ( make_unique <VariableExprAST> ( name ) );
    //auto bodyAST = make_unique <ExprListAST> ( std::move ( body ) );
    return make_unique <FunctionAST> ( std::move ( proto ), std::move ( body ) );
}

std::unique_ptr<ExprAST> mila::Parser::procedure ( const LexicalSymbol & ls )
{
    if ( ls != "procedure" )
        parserError ( LexicalSymbol ( "procedure" ), ls );
    std::string name = readIdentifier ();
    std::vector <std::string> args;
    args = getParameterDefinitions ();
    discard ( { ";" } );
    auto proto = make_unique <PrototypeAST> ( name, std::move ( args ) );
    // DO NOT add name to the mix of arguments/variables
    // but return something probably either way just in case
    LexicalSymbol next = peekNextLS ();
    if ( next == "forward" )
    {
        discard ( { "forward", ";" } );
        return proto;
    }
    std::vector <std::unique_ptr<ExprAST>> body;
    while ( next == "var" )
    {
        // THE HELL IS HAPPENING HERE??
        auto decl = variable ( getNextLS () );
        body . push_back ( std::move ( decl ) );
        next = peekNextLS ();
    }
    auto def = block ();
    discard ( { ";" } );
    body . push_back ( std::move ( def ) );
    body . push_back ( make_unique <NumberExprAST> (0) );
    //auto bodyAST = make_unique <ExprListAST> ( std::move ( body ) );
    return make_unique <FunctionAST> ( std::move ( proto ), std::move ( body ) );
}

std::unique_ptr<ExprAST> mila::Parser::command ()
{
    LexicalSymbol ls = getNextLS ();
    if ( ls == "write" )
    {
        discard ( { "(", "'" } );
        while ( getNextLS () != "'" );
        discard ( { ")" } );
        return make_unique <NumberExprAST> (0);
    }
    if ( ls . type == IDENTIFIER || ( ls == "writeln" && peekNextLS () == "(" ) )
    {
        std::string name = ls . name;
        LexicalSymbol next = peekNextLS ();
        if ( next == "[" || next == ":=" )
        {
            std::unique_ptr <ExprAST> RHS;
            if ( next == "[" )
            {
                discard ( { "[" } );
                auto index = expression ();
                discard ( { "]" } ); 
                RHS = make_unique <ArrayExprAST> ( name, std::move ( index ) );
            }
            else
            {
                RHS = make_unique <VariableExprAST> ( name );
            }
            discard ( { ":=" } );
            auto expr = expression ();
            return make_unique <BinaryExprAST> ( ASSIGN, std::move ( RHS ), std::move ( expr ) );
        }
        else if ( next == "(" )
        {
            std::vector <std::unique_ptr<ExprAST>> args;
            args = getParameters ();
            return make_unique <CallExprAST> ( name, std::move ( args ) );
        }
        else
            parserError ( "'[', '(' or ':='", next );
    }
    if ( LexicalSet { "readln", "inc", "dec" } . count ( ls ) )
    {
        discard ( { "(" } );
        auto arg = readIdentifier ();
        discard ( { ")" } );
        return make_unique <LibraryExprAST> ( ls . name, arg );
    }
    else if ( ls == "exit" )
        return make_unique <ReturnExprAST> ();
    else
        parserError ( "Command", ls );
    return nullptr;
}

std::unique_ptr<ExprAST> mila::Parser::command ( const LexicalSymbol & ls )
{
    if ( ls == "write" )
    {
        expect ( { "write", "(", "'", FOO, "'", ")" } );
        parseSymbol ( ls );
        return nullptr;
    }
    expect ({});
    if ( ls . type == IDENTIFIER || ( LexicalSet ( { "readln", "writeln", "dec", "inc" } ) . count ( ls ) && peekNextLS () == "(" ) )
    {
        std::string name = ls . name;
        LexicalSymbol next = peekNextLS ();
        if ( next == "[" || next == ":=" )
        {
            std::unique_ptr <ExprAST> RHS;
            if ( next == "[" )
            {
                discard ( { "[" } );
                auto index = expression ();
                discard ( { "]" } ); 
                RHS = make_unique <ArrayExprAST> ( name, std::move ( index ) );
            }
            else
            {
                RHS = make_unique <VariableExprAST> ( name );
            }
            discard ( { ":=" } );
            auto expr = expression ();
            return make_unique <BinaryExprAST> ( ASSIGN, std::move ( RHS ), std::move ( expr ) );
        }
        else if ( next == "(" )
        {
            std::vector <std::unique_ptr<ExprAST>> args;
            args = getParameters ();
            return make_unique <CallExprAST> ( name, std::move ( args ) );
        }
        else
            parserError ( "'[', '(' or ':='", next );
    }
    else if ( ls == "exit" )
        return nullptr;
    else
        parserError ( "Command", ls );
    return nullptr;
}

std::unique_ptr<ExprAST> mila::Parser::control ()
{
    auto next = getNextLS ();
    if ( next == "if" )
    {
        auto cond = expression ();
        discard ( { "then" } );
        auto then = blockCommand ();
        next = peekNextLS ();
        std::unique_ptr <ExprAST> els;
        if ( next == "else" )
        {
            discard ( { "else" } );
            els = blockCommand ();
        }
        else
        {
            els = make_unique <NumberExprAST> ( 0 );
        }
        return make_unique <IfExprAST> ( std::move ( cond ), std::move ( then ), std::move ( els ) );
    }
    else if ( next == "for" )
    {
        std::string var = readIdentifier ();
        discard ( { ":=" } );
        auto startVal = expression ();
        next = getNextLS ();
        int step;
        if ( next == "to" )
            step = 1;
        else if ( next == "downto" )
            step = -1;
        else
            parserError ( "'to' or 'downto'", next );
        auto stepVal = make_unique <NumberExprAST> ( step );
        auto endVal = expression ();
        discard ( { "do" } );
        auto then = blockCommand ();

        return make_unique <ForExprAST> ( var, std::move ( startVal ), std::move ( endVal ), std::move ( stepVal ), std::move ( then ) );
    }
    else if ( next == "while" )
    {
        auto cond = expression ();
        discard ( { "do" } );
        auto body = blockCommand ();
        return make_unique <WhileExprAST> ( std::move ( cond ), std::move ( body ) );
    }
    else
        parserError ( "'if', 'for' or 'while'", next );
    return nullptr;
}

std::unique_ptr<ExprAST> mila::Parser::blockCommand ( void )
{
    LexicalSymbol next = peekNextLS ();
    if ( next == "begin" )
        return block ();
    return command ();
}
