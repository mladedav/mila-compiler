#include <iostream>
#include <fstream>
#include "parser.h"
#include "lexan.h"

using namespace mila;
using namespace std;

int main ( void )
{
    try
    {
        Parser parser ( Lexan ( move ( cin ) ) );
        parser . parse ();
        cerr << "Evertying parsed." << endl;
    }
    catch ( ParserException & e )
    {
        cerr << e << endl;
        return 1;
    }
    return 0;
}

