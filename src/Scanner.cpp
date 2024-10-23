#include "Scanner.hpp"
#include <regex>
#include <string>
#include <vector>
#include <fstream>
using namespace std;

vector<string> Scanner::read_config(string config_file)
{
    vector <string> retorno;

    regex reg("([ ]\\d+\\.?\\d*)");
    smatch matches;

    fstream file(config_file);
    string line;

    while (getline(file, line))
    {
        regex_search(line, matches, reg);

        retorno.push_back(matches.str(1));
    }
    file.close();

    return retorno;
}