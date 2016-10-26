// Pacman2本地服务器
// by wd
//
// 将server.exe，defaultInput.txt放在dist文件夹下
// 每个玩家分别有一个文件夹为player0~3，放在dist文件夹下
// 里面有player.exe，input.txt，output.txt，debug.txt
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
        // 初始时每个玩家的input.txt为空，把defaultInput.txt复制过去
        fin.open("defaultInput.txt", ifstream::binary);
        fin >> input;
        fin.close();
    }
    else
    {
        // 读取之前的输入
        Json::Reader reader;
        reader.parse(str, input);

        // 读取每个玩家的输出，添加到下次的输入
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

    // 给每个玩家写入下次的输入，然后运行该玩家
    Json::FastWriter writer;
    ofstream fout;
    for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
    {
        // defaultInput.txt中没有id，由server.exe添加
        input["requests"][0]["id"] = i;

        fout.open("player" + to_string(i) + "\\input.txt");
        fout << writer.write(input);
        fout.close();

        system(("cd player" + to_string(i) + " & player.exe").c_str());
    }
    return 0;
}
