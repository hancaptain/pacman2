// Pacman2本地服务器
// by wd
// 将四个玩家的程序命名为player0~3.exe
// 与server.exe，input.txt，output.txt，debug.txt放在同一文件夹下
// 初始input.txt为空，将defaultInput.txt复制到input.txt
// defaultInput.txt中没有id，由server.exe添加

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "jsoncpp/json.h"

#define MAX_PLAYER_COUNT 4

using namespace std;

int main()
{
    ifstream fin;
    stringstream ss;
    fin.open("player0\\input.txt");
    ss << fin.rdbuf();
    fin.close();
    string str = ss.str();
    ss.clear();
    Json::Value input;
    if (str == "")
    {
        fin.open("defaultInput.txt", ifstream::binary);
        fin >> input;
        fin.close();
    }
    else
    {
        Json::Reader reader;
        reader.parse(str, input);
        int len = input["requests"].size();
        Json::Value input2;
        for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
        {
            fin.open("player" + to_string(i) + "\\output.txt",
                     ifstream::binary);
            fin >> input2;
            input["requests"][len][to_string(i)] = input2["response"];
            fin.close();
        }
    }
    Json::FastWriter writer;
    ofstream fout;
    for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
    {
        input["requests"][0]["id"] = i;
        fout.open("player" + to_string(i) + "\\input.txt");
        fout << writer.write(input);
        fout.close();
        system(("cd player" + to_string(i) + " & player.exe").c_str());
    }
    return 0;
}
