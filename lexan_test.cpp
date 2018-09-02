#include "lexan.h"
#include <iostream>
#include <fstream>
#include <utility>

using namespace mila;
using namespace std;

int main ( int argc, char ** argv )
{
    Lexan * tmp;
    fstream fs;
    if ( argc > 1 )
    {
        fs . open ( argv [ 1 ], fstream::in );
        tmp = new Lexan ( move ( fs ) );
    }
    else
    {
        tmp =  new Lexan ( move ( cin ) );
    }
    Lexan & lex = *tmp;

    LexicalSymbol ls;
    while ( !lex . eof () )
    {
        lex >> ls;

        if ( ls . type == ERROR )
        {
            cerr << ls << endl;
            return 1;
        }
        cout << ls << endl;
    }
    delete tmp;
    fs . close ();
    return 0;
}
