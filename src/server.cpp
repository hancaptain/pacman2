// Pacman2本地服务器
// by wd
//
// 适用于Windows系统
// 将server.exe，defaultInput.txt放在dist文件夹下
// 每个玩家分别有一个文件夹为player0~3，放在dist文件夹下
// 里面有player.exe，input.txt，output.txt，debug.txt
// defaultInput.txt中没有玩家的id，由server.exe添加

#define INCLUDE_JSONCPP_CPP

#include "pacman.h"

int main()
{
    ifstream fin;
    ofstream fout;
    Json::Value input;
    Json::Reader reader;
    Json::FastWriter writer;
    Pacman::GameField gameField;
    gameField.DEBUG_STR = false;
    string data, globalData;  // 这是回合之间可以传递的信息

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
    for (int count = 2; count < MAX_TURN; ++count)
    {
        cout << count << endl;

        // 这里有BUG。。
        // 判断玩家死亡情况
        // gameField.ReadInput("player0\\input.txt", data, globalData);
        // cout << gameField.turnID << " " << gameField.aliveCount << " ";
        // for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
        // {
        // cout << gameField.players[i].dead;
        // }
        // cout << endl;
        // if (gameField.aliveCount <= 1) break;

        // 读取之前的输入
        fin.open("player0\\input.txt", ifstream::binary);
        fin >> input;
        fin.close();

        int len = input["requests"].size();
        // input["requests"][len]["turnID"] = gameField.turnID;
        input["requests"][len]["turnID"] = count;

        // 读取每个玩家的输出，添加到下次的输入
        Json::Value input2;
        int aliveCount = 0;
        for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
        {
            fin.open("player" + to_string(i) + "\\output.txt",
                     ifstream::binary);
            fin >> input2;
            input["requests"][len][to_string(i)] = input2["response"];
            fin.close();

            if (input["requests"][len][to_string(i)]["tauntText"] != "DEAD")
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

            fout.open("player" + to_string(i) + "\\input.txt");
            fout << writer.write(input);
            fout.close();

            // if (!gameField.players[i].dead)
            if (input["requests"][len][to_string(i)]["tauntText"] != "DEAD")
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
