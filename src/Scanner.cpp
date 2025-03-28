#include "Scanner.hpp"
#include <regex>
#include <string>
#include <vector>
#include <fstream>
using namespace std;

unordered_map<string, string> Scanner::read_config(string config_file)
{
    unordered_map <string, string> retorno;

    regex reg_number("(\\d+\\.?\\d*)");
    regex reg_text("(\\w+):");
    smatch matches;

    fstream file(config_file);
    string line;

    while (getline(file, line))
    {
        regex_search(line, matches, reg_text);
        
        string chave = matches.str(1);

        line = matches.suffix().str();

        regex_search(line, matches, reg_number);

        retorno[chave]= matches.str(1);
    }
    file.close();
    
    return retorno;
}