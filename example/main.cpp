#include "../src/PluginManager.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

vector<string> split_command(const string& command_line)
{
    vector<string> tokens;
    string token = "";
    for (char c : command_line) {
        if (c == ' ') {
            if (token != "") {
                tokens.push_back(token);
                token = "";
            }
        } else {
            token += c;
        }
    }
    if (token != "") {
        tokens.push_back(token);
    }
    return tokens;
}

void load_plugin(const vector<string>& args)
{
    if (args.size() < 2) {
        cout << "usage: load <path> <name>" << endl;
        return;
    }
    cout << "load " << args[0] << " " << args[1] << endl;
}

void exit_program(const vector<string>& args)
{
    cout << "bye!" << endl;
    exit(0);
}

unordered_map<string, std::function<void(const vector<string>&)>> command_map = {
    { "load", load_plugin },
    { "exit", exit_program },
};

int main()
{
    string input;
    for (;;) {
        cout << ">> " << flush;
        if (!getline(cin, input)) {
            break;
        }
        vector<string> tokens = split_command(input);
        if (tokens.empty()) {
            continue;
        }
        auto it = command_map.find(tokens[0]);
        if (it == command_map.end()) {
            cout << "Unknown command: " << tokens[0] << endl;
            continue;
        }
        it->second({ tokens.begin() + 1, tokens.end() });
    }
}
