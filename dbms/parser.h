#pragma once
#include "query.h"
#include <string>
#include <vector>

using namespace std;

class Parser {
public:
    static Query parse(const string& sql);
private:
    static string trim(const string& str);
    static vector<string> split(const string& str, char delimiter);
    static SearchAlgorithm parseAlgorithm(const string& value);
};
