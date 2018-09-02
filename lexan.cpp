#include "lexan.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <sstream>
#include <utility>

using namespace mila;

std::set < std::string > mila::keywords =
{
    "program", "var", "const", "function", "procedure",
    "begin", "end", "forward",
    "if", "then", "else", "while", "for", "do",
    "to", "downto", 
    "read", "write", "readln", "writeln", "exit",
    "dec", "inc",
    "div", "mod",
    "not", "and", "or",
    "array", "of", "integer"
};

mila::Lexan::Lexan ( std::istream && is )
: input ( move ( is ) )
{
}
//==========================================================================
Lexan & mila::Lexan::operator << ( const LexicalSymbol & ls )
{
    que . push ( ls );
    return *this;
}

Lexan & mila::Lexan::operator >> ( LexicalSymbol & ls )
{
    if ( !que . empty () )
    {
        ls = que . front ();
        que . pop ();
        return *this;
    }
    clearSpace ();
    InputCharacter next;
    getNext ( next );
    goto S;
S:
    switch ( next . type )
    {
        case NUMBER:
            ls . type = INTEGER;
            ls . value = 0;
            if ( next . value == '0' )
                goto octa;
            else
                goto deca;
        case LETTER:
            ls . name . clear ();
            goto word;
        case OTHER:
            ls . name . clear ();
            goto other;
        case WHITE_SPACE:
            ls . type = ERROR;
            ls . name = "Uncaught white space.";
            return *this;
        case EOI:
            ls . type = END_OF_INPUT;
            return *this;
        case FAIL:
            ls . type = ERROR;
            ls . name = "Error reading from stream.";
            return *this;
    }
octa:
    getNext ( next );
    switch ( next . type )
    {
        case LETTER:
            if ( next . value == 'x' || next . value == 'X' )
                goto hexa;
            ls . type = ERROR;
            ls . name = "Identifier cannot start with a number.";
            return *this;
        case NUMBER:
            input . unget ();
            readNumber ( ls, 8 );
            return *this;
        default:
            input . unget ();
        case EOI:
            ls . value = 0;
            return *this;
    }
deca:
    input . unget ();
    readNumber ( ls, 10 );
    return *this;
hexa:
    getNext ( next );
    switch ( next . type )
    {
        case NUMBER:
        case LETTER:
            input . unget ();
            readNumber ( ls, 16 );
            return *this;
        default:
            input . unget ();
        case EOI:
            ls . type = ERROR;
            ls . name = "Symbol \"0x\" is invalid.";
            return *this;
    }
word:
    ls . type = IDENTIFIER;
    ls . name += next . value;
    getNext ( next );
    switch ( next . type )
    {
        case NUMBER:
        case LETTER:
            goto word;
        case OTHER:
            if ( next . value == '_' )
                goto word;
        default:
            input . unget ();
        case EOI:
            checkKeyword ( ls );
            return *this;
    }
other:
    ls . type = OPERATOR;
    ls . name += next . value;
    switch ( next . value )
    {
        case '.':
            if ( input . peek () == '.' )
            {
                input . get ();
                ls . name += '.';
                return *this;
            }
            return *this;
        case '<':
            if ( input . peek () == '>' )
            {
                input . get ();
                ls . name += '>';
                return *this;
            }
        case ':':
        case '>':
            if ( input . peek () == '=' )
            {
                input . get ();
                ls . name += '=';
            }
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '=':
        case '(':
        case ')':
        case '[':
        case ']':
        case ',':
        case ';':
        case '\'':
            return *this;
        case '$':
            if ( isxdigit ( input . peek () ) )
            {
                ls . type = INTEGER;
                ls . value = 0;
                goto hexa;
            }
        case '&':
            if ( isdigit ( input . peek () ) )
            {
                ls . type = INTEGER;
                ls . value = 0;
                goto octa;
            }
        default:
            ls . type = ERROR;
            std::stringstream ss;
            ss << "Unrecognized operator '" << ls . name << "'.";
            ls . name = ss . str ();
            return *this;
    }
    return *this;
}
//==========================================================================
bool mila::Lexan::eof ( void ) const
{
    return input . eof ();
}

void mila::Lexan::clearSpace ( void )
{
    while ( isspace ( input . peek () ) )
        input . get ();
}

void mila::Lexan::getNext ( InputCharacter & ic )
{
    if ( !input . get ( ic . value ) )
    {
        if ( input . eof () )
            ic . type = EOI;
        else
            ic . type = FAIL;
    }
    else if ( isalpha ( ic . value ) )
        ic . type = LETTER;
    else if ( isdigit ( ic . value ) )
        ic . type = NUMBER;
    else if ( isspace ( ic . value ) )
        ic . type = WHITE_SPACE;
    else
        ic . type = OTHER;
}

void mila::Lexan::readNumber ( LexicalSymbol & ls, int base )
{
    ls . value = 0;
    char next;
    while ( isalnum (( next = input . peek () )) )
    {
        int nextVal;
        if ( !checkDigit ( next, base, nextVal ) )
        {
            ls . type = ERROR;
            std::stringstream ss;
            ss << "Number in base " << base << " cannot contain character '" << next << "'.";
            ls . name = ss . str ();
            return;
        }
        ls . value = ls . value * base + nextVal;
        if ( !input . get () )
            return;
    }
}

int mila::Lexan::checkDigit ( char num, int base, int & val )
{
    if ( isdigit ( num ) )
        val = num - '0';
    else if ( islower ( num ) )
        val = num - 'a' + 10;
    else if ( isupper ( num ) )
        val = num - 'A' + 10;
    else
        return 0;
    if ( val >= base )
        return 0;
    return 1;
}

void mila::Lexan::checkKeyword ( LexicalSymbol & ls )
{
    if ( keywords . count ( ls . name ) )
    {
        ls . type = KEYWORD;
        return;
    }
}

mila::LexicalSymbol::LexicalSymbol ( void )
: type ( ERROR ), name ( "Uninitialized lexical symbol." )
{
}

mila::LexicalSymbol::LexicalSymbol ( SymbolType st )
: type ( st ), name ( "" ), value ( -1 )
{
}

mila::LexicalSymbol::LexicalSymbol ( const char * symbol )
: LexicalSymbol ( std::string ( symbol ) )
{
}

mila::LexicalSymbol::LexicalSymbol ( const std::string & symbol )
{
    std::stringstream input ( symbol );
    Lexan lex ( move ( input ) );
    lex >> *this;
    LexicalSymbol eoi;
    lex >> eoi;
    if ( eoi . type != END_OF_INPUT )
    {
        this -> type = ERROR;
        std::stringstream error;
        error << "Cannot build single symbol from \"" << symbol << "\".";
        this -> name = error . str ();
    }
}

namespace mila
{
    std::ostream & operator << ( std::ostream & os, const LexicalSymbol & ls )
    {
        switch ( ls . type )
        {
            case IDENTIFIER:
                os << "IDENTIFIER";
                if ( !ls . name . empty () )
                    os << " - " << ls . name;
                return os;
            case INTEGER:
                os << "INTEGER";
                if ( ls . value != -1 )
                    os << " - " << ls . value;
                return os;
            case KEYWORD:
                os << "KEYWORD";
                if ( !ls . name . empty () )
                    os << " - " << ls . name;
                return os;
            case OPERATOR:
                os << "OPERATOR";
                if ( !ls . name . empty () )
                    os << " - " << ls . name;
                return os;
            case END_OF_INPUT:
                return os << "END OF INPUT";
            case ERROR:
                return os << "ERROR - " << ls . name;
        }
        os << "ERROR - Unrecognized type of lexical symbol.";
        return os;
    }
}

bool mila::LexicalSymbol::operator == ( const LexicalSymbol & ls ) const
{
    if ( type != ls . type )
        return false;
    switch ( type )
    {
        case IDENTIFIER:
        case OPERATOR:
        case KEYWORD:
        case ERROR:
            return name == ls . name;
        case INTEGER:
            return value == ls . value;
        default:;
    }
    return true;
}

bool mila::LexicalSymbol::operator != ( const LexicalSymbol & ls ) const
{
    return !( *this == ls );
}

bool mila::LexicalSymbol::operator == ( const char * symbol ) const
{
    return *this == LexicalSymbol ( symbol );
}

bool mila::LexicalSymbol::operator != ( const char * symbol ) const
{
    return !( *this == symbol );
}

std::size_t std::hash < mila::LexicalSymbol >::operator () ( const mila::LexicalSymbol & ls ) const
{
    std::size_t ret = std::hash < int > {} ( ls . type );
    switch ( ls . type )
    {
        case IDENTIFIER:
        case OPERATOR:
        case KEYWORD:
        case ERROR:
            ret += std::hash < std::string > {} ( ls . name );
            break;
        case INTEGER:
            ret += std::hash < int > {} ( ls . value );
            break;
        case END_OF_INPUT:
            break;
    }
    return ret;
}
