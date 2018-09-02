#include <iostream>
#include <string>
#include <set>
#include <queue>

#ifndef MILA_LEXAN_H
#define MILA_LEXAN_H

namespace mila
{
    extern std::set < std::string > keywords;

    enum GraphemType
    {
        LETTER,
        NUMBER,
        WHITE_SPACE,
        EOI,
        FAIL,
        OTHER,
    };
    struct InputCharacter
    {
        GraphemType type;
        char value;
    };

    enum SymbolType
    {
        IDENTIFIER,
        INTEGER,
        OPERATOR,
        KEYWORD,
        END_OF_INPUT,
        ERROR,
    };

    struct LexicalSymbol
    {
        LexicalSymbol ( void );
        LexicalSymbol ( SymbolType );
        LexicalSymbol ( const std::string & );
        LexicalSymbol ( const char * );
        SymbolType type;
        std::string name;
        int value;
        bool operator == ( const LexicalSymbol & ) const;
        bool operator != ( const LexicalSymbol & ) const;
        bool operator == ( const char * ) const;
        bool operator != ( const char * ) const;
        friend std::ostream & operator << ( std::ostream &, const LexicalSymbol & );
    };

    class Lexan
    {
        public:
            Lexan ( std::istream && );
            Lexan & operator << ( const LexicalSymbol & );
            Lexan & operator >> ( LexicalSymbol & );
            bool eof ( void ) const;

        private:
            void clearSpace ( void );
            void readNumber ( LexicalSymbol &, int );
            int checkDigit ( char, int, int & );
            void getNext ( InputCharacter & );
            void checkKeyword ( LexicalSymbol & );
            std::istream && input;
            std::queue < LexicalSymbol > que; 
    };
}

namespace std
{
    template <> struct hash < mila::LexicalSymbol > 
    {
        std::size_t operator () ( const mila::LexicalSymbol & ) const;
    };
}

#endif
