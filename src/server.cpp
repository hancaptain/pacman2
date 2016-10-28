// Pacman2本地服务器
// by wd
//
// 适用于Windows系统
// 运行环境见dist文件夹
// defaultInput.txt中没有玩家的id，由server.exe添加

#include "pacman.h"

int main()
{
    ifstream fin;
    ofstream fout;
    Json::Value input, input2;  // 存储输入和每个玩家的输出
    Json::Reader reader;
    Json::FastWriter writer;
    string allData[MAX_PLAYER_COUNT];
    string allGlobalData[MAX_PLAYER_COUNT];

    cout << " 0 1111" << endl;

    // 初始化每个玩家的输入，清空output.txt，debug.txt，第一次运行该玩家
    fin.open("defaultInput.txt", ifstream::binary);
    fin >> input;
    fin.close();

    for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
    {
        // 在输入中添加玩家的id
        input["requests"][0]["id"] = i;

        fout.open("player" + to_string(i) + "\\input.txt");
        fout << writer.write(input);
        fout.close();
        fout.open("player" + to_string(i) + "\\output.txt");
        fout.close();
        fout.open("player" + to_string(i) + "\\debug.txt");
        fout.close();

        system(("cd player" + to_string(i) + " & player.exe").c_str());
    }

    // 循环运行回合
    for (int count = 1; count < MAX_TURN; ++count)
    {
        cout << setw(2) << count << " ";
        input["requests"][count]["turnID"] = count;

        // 读取每个玩家的输出，添加到下次的输入
        // 第(i - 1)回合输出的动作在第i回合执行，放在input["requests"][i]
        int aliveCount = 0;
        for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
        {
            fin.open("player" + to_string(i) + "\\output.txt",
                     ifstream::binary);
            fin >> input2;
            fin.close();
            input["requests"][count][to_string(i)] = input2["response"];
            allData[i] = input2["data"].asString();
            allGlobalData[i] = input2["globaldata"].asString();

            // 玩家在tauntText输出SERVER_STOP会让服务器直接停止，用于调试
            if (input2["response"]["tauntText"] == "SERVER_STOP")
            {
                cout << endl << "SERVER_STOP" << endl;
                return 1;
            }

            if (input2["response"]["tauntText"] != "DEAD")
            {
                cout << 1;
                ++aliveCount;
            }
            else
                cout << 0;
        }
        cout << endl;
        if (aliveCount <= 1) break;

        // 给每个玩家写入下次的输入，然后运行该玩家
        for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
        {
            // 在输入中添加玩家的id
            input["requests"][0]["id"] = i;

            input["data"] = allData[i];
            input["globaldata"] = allGlobalData[i];
            // cout << allData[i] << endl;
            // cout << allGlobalData[i] << endl;

            fout.open("player" + to_string(i) + "\\input.txt");
            fout << writer.write(input);
            fout.close();

            if (input["requests"][count][to_string(i)]["tauntText"] != "DEAD")
            {
                system(("cd player" + to_string(i) + " & player.exe").c_str());
            }
            else
            {
                fout.open("player" + to_string(i) + "\\output.txt");
                fout
                    << "{\"data\":\"\",\"globaldata\":\"\",\"response\":{\"action\":-1,\"tauntText\":\"DEAD\"}}";
                fout.close();
            }
        }
    }

    return 0;
}
