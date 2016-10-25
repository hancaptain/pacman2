// Pacman2本地服务器
// by wd
// 将四个玩家的程序命名为player0~3.exe
// 与server.exe，input.txt，output.txt，debug.txt放在同一文件夹下
// 初始input.txt为空，将defaultInput.txt复制到input.txt，清空debug.txt

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
    ifstream fin("input.txt");
    stringstream ss;
    ss << fin.rdbuf();
    fin.close();
    string str = ss.str();
    Json::Value input;
    int id;
    if (str == "")
    {
        ifstream fDefaultIn("defaultInput.txt", ifstream::binary);
        fDefaultIn >> input;
        input["requests"][0]["id"] = id = 0;
        ofstream fout("debug.txt");
        fout.close();
    }
    else
    {
        Json::Reader reader;
        reader.parse(str, input);
        id = input["requests"][0]["id"].asInt();
        input["requests"][0]["id"] = id = (id + 1) % MAX_PLAYER_COUNT;
        int len = input["requests"].size();
        Json::Value input2;
        ifstream fin2("output.txt", ifstream::binary);
        fin2 >> input2;
        input["requests"][len] = input2;
    }
    Json::FastWriter writer;
    ofstream fout("input.txt");
    fout << writer.write(input);
    // system("player0.exe");
    return 0;
}
