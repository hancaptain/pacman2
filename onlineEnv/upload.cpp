// by wd

// Pacman2 ��������
// ���ߣ�zhouhy
// ʱ�䣺2016/10/12 12:54
//
// ������������
// r/R/y/Y��Row���У�������
// c/C/x/X��Column���У�������
// ������±궼��[y][x]��[r][c]��˳��
// ��ұ��0123
//
// ������ϵ��
//   0 1 2 3 4 5 6 7 8
// 0 +----------------> x
// 1 |
// 2 |
// 3 |
// 4 |
// 5 |
// 6 |
// 7 |
// 8 |
//   v y
//
// ����ʾ�������ʹ��
// #ifndef _BOTZONE_ONLINE
// ������Ԥ����ָ����������������ͱ�������
//
// ����ʾ��һ����ı��༭������֧�ֽ�������۵�����
// ���������Դ�����̫���߳������Կ��ǽ�����namespace�۵�
//
// edit by wd
// �޸���DebugPrintʹ֮�����debug.txt

#ifndef PACMAN_H
#define PACMAN_H

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stack>
#include <stdexcept>
#include <string>
#include "jsoncpp/json.h"

#define FIELD_MAX_HEIGHT 20
#define FIELD_MAX_WIDTH 20
#define MAX_GENERATOR_COUNT 4  // ÿ������1
#define MAX_PLAYER_COUNT 4
#define MAX_TURN 100
#define MAX_DIRECTION 9

using namespace std;
// using std::string;
// using std::swap;
// using std::cin;
// using std::cout;
// using std::endl;
// using std::getline;
// using std::runtime_error;
// using std::setw;

// ƽ̨�ṩ�ĳԶ�������߼��������
namespace Pacman
{
	const time_t seed = time(0);
	const int dx[] = { 0, 1, 0, -1, 1, 1, -1, -1 },
		dy[] = { -1, 0, 1, 0, -1, 1, 1, -1 };

	// ��ö��Ҳ��������Щ�����ˣ����ӻ�������
	template <typename T>
	inline T operator|=(T &a, const T &b)
	{
		return a = static_cast<T>(static_cast<int>(a) | static_cast<int>(b));
	}
	template <typename T>
	inline T operator|(const T &a, const T &b)
	{
		return static_cast<T>(static_cast<int>(a) | static_cast<int>(b));
	}
	template <typename T>
	inline T operator&=(T &a, const T &b)
	{
		return a = static_cast<T>(static_cast<int>(a) & static_cast<int>(b));
	}
	template <typename T>
	inline T operator&(const T &a, const T &b)
	{
		return static_cast<T>(static_cast<int>(a) & static_cast<int>(b));
	}
	template <typename T>
	inline T operator-(const T &a, const T &b)
	{
		return static_cast<T>(static_cast<int>(a) - static_cast<int>(b));
	}
	template <typename T>
	inline T operator++(T &a)
	{
		return a = static_cast<T>(static_cast<int>(a) + 1);
	}
	template <typename T>
	inline T operator~(const T &a)
	{
		return static_cast<T>(~static_cast<int>(a));
	}

	// ö�ٶ��壻ʹ��ö����Ȼ���˷ѿռ䣨sizeof(GridContentType) == 4����
	// ���Ǽ��������32λ������Ч�ʸ���

	// ÿ�����ӿ��ܱ仯�����ݣ�����á����߼��������
	enum GridContentType
	{
		empty = 0,                   // ��ʵ�����õ�
		player1 = 1,                 // 1�����
		player2 = 2,                 // 2�����
		player3 = 4,                 // 3�����
		player4 = 8,                 // 4�����
		playerMask = 1 | 2 | 4 | 8,  // ���ڼ����û����ҵ�
		smallFruit = 16,             // С����
		largeFruit = 32              // ����
	};

	// �����ID��ȡ��������ҵĶ�����λ
	GridContentType playerID2Mask[] = { player1, player2, player3, player4 };
	string playerID2str[] = { "0", "1", "2", "3" };

	// ÿ�����ӹ̶��Ķ���������á����߼��������
	enum GridStaticType
	{
		emptyWall = 0,  // ��ʵ�����õ�
		wallNorth = 1,  // ��ǽ����������ٵķ���
		wallEast = 2,   // ��ǽ�����������ӵķ���
		wallSouth = 4,  // ��ǽ�����������ӵķ���
		wallWest = 8,   // ��ǽ����������ٵķ���
		generator = 16  // ���Ӳ�����
	};

	// ���ƶ�����ȡ����������赲�ŵ�ǽ�Ķ�����λ
	GridStaticType direction2OpposingWall[] = { wallNorth, wallEast, wallSouth,
		wallWest };

	// ���򣬿��Դ���dx��dy���飬ͬʱҲ������Ϊ��ҵĶ���
	enum Direction
	{
		stay = -1,
		up = 0,
		right = 1,
		down = 2,
		left = 3,
		shootUp = 4,     // ���Ϸ�����
		shootRight = 5,  // ���ҷ�����
		shootDown = 6,   // ���·�����
		shootLeft = 7    // ��������
	};

	// �����ϴ�����������
	struct FieldProp
	{
		int row, col;
	};

	// �����ϵ����
	struct Player : FieldProp
	{
		int strength;
		int powerUpLeft;
		bool dead;
	};

	// �غ��²����Ķ��ӵ�����
	struct NewFruits
	{
		FieldProp newFruits[MAX_GENERATOR_COUNT * 8];
		int newFruitCount;
	} newFruits[MAX_TURN];
	int newFruitsCount = 0;

	// ״̬ת�Ƽ�¼�ṹ
	struct TurnStateTransfer
	{
		enum StatusChange  // �����
		{
			none = 0,
			ateSmall = 1,
			ateLarge = 2,
			powerUpDrop = 4,
			die = 8,
			error = 16
		};

		// ���ѡ���Ķ���
		Direction actions[MAX_PLAYER_COUNT];

		// �˻غϸ���ҵ�״̬�仯
		StatusChange change[MAX_PLAYER_COUNT];

		// �˻غϸ���ҵ������仯
		int strengthDelta[MAX_PLAYER_COUNT];
	};

	// ��Ϸ��Ҫ�߼������࣬��������������غ����㡢״̬ת�ƣ�ȫ��Ψһ
	class GameField
	{
	private:
		// Ϊ�˷��㣬��������Զ�����private��

		// ��¼ÿ�غϵı仯��ջ��
		TurnStateTransfer backtrack[MAX_TURN];

		// ��������Ƿ��Ѿ�����
		static bool constructed;

	public:
#define DEFINE_DEBUG_STR
		// ��ΪtrueʱDebugStr����Ч
		bool DEBUG_STR;

		// ���صĳ��Ϳ�
		int height, width;
		int generatorCount;
		int GENERATOR_INTERVAL, LARGE_FRUIT_DURATION, LARGE_FRUIT_ENHANCEMENT,
			SKILL_COST;
		int myID;

		// ���ظ��ӹ̶�������
		GridStaticType fieldStatic[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];

		// ���ظ��ӻ�仯������
		GridContentType fieldContent[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];

		int generatorTurnLeft;  // ���ٻغϺ��������
		int aliveCount;         // �ж�����Ҵ��
		int smallFruitCount;
		int turnID;

		FieldProp generators[MAX_GENERATOR_COUNT];  // ����Щ���Ӳ�����
		Player players[MAX_PLAYER_COUNT];           // ����Щ���

													// ���ѡ���Ķ���
		Direction actions[MAX_PLAYER_COUNT];

		// �ָ����ϴγ���״̬������һ·�ָ����ʼ��
		// �ָ�ʧ�ܣ�û��״̬�ɻָ�������false
		bool PopState()
		{
			if (turnID <= 0) return false;

			const TurnStateTransfer &bt = backtrack[--turnID];
			int i, _;

			// �������ָ�״̬

			for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
			{
				Player &_p = players[_];
				GridContentType &content = fieldContent[_p.row][_p.col];
				TurnStateTransfer::StatusChange change = bt.change[_];

				// 5. �󶹻غϻָ�
				if (change & TurnStateTransfer::powerUpDrop) _p.powerUpLeft++;

				// 4. �³�����
				if (change & TurnStateTransfer::ateSmall)
				{
					content |= smallFruit;
					smallFruitCount++;
				}
				else if (change & TurnStateTransfer::ateLarge)
				{
					content |= largeFruit;
					_p.powerUpLeft -= LARGE_FRUIT_DURATION;
				}

				// 2. �������
				if (change & TurnStateTransfer::die)
				{
					_p.dead = false;
					aliveCount++;
					content |= playerID2Mask[_];
				}

				// 1. ���λ�Ӱ
				if (!_p.dead && bt.actions[_] != stay &&
					bt.actions[_] < shootUp)
				{
					fieldContent[_p.row][_p.col] &= ~playerID2Mask[_];
					_p.row = (_p.row - dy[bt.actions[_]] + height) % height;
					_p.col = (_p.col - dx[bt.actions[_]] + width) % width;
					fieldContent[_p.row][_p.col] |= playerID2Mask[_];
				}

				// 0. ���겻�Ϸ������
				if (change & TurnStateTransfer::error)
				{
					_p.dead = false;
					aliveCount++;
					content |= playerID2Mask[_];
				}

				// *. �ָ�����
				_p.strength -= bt.strengthDelta[_];
			}

			// 3. �ջض���
			if (generatorTurnLeft == GENERATOR_INTERVAL)
			{
				generatorTurnLeft = 1;
				NewFruits &fruits = newFruits[--newFruitsCount];
				for (i = 0; i < fruits.newFruitCount; i++)
				{
					fieldContent[fruits.newFruits[i].row]
						[fruits.newFruits[i].col] &= ~smallFruit;
					smallFruitCount--;
				}
			}
			else
				generatorTurnLeft++;

			return true;
		}

		// �ж�ָ�������ָ�������ƶ�/ʩ�ż����ǲ��ǺϷ��ģ�û��ײǽ��û�вȵ����Ӳ������������㹻��
		inline bool ActionValid(int playerID, Direction dir) const
		{
			if (dir == stay) return true;
			const Player &p = players[playerID];
			if (dir >= up && dir <= left &&
				!(fieldStatic[p.row][p.col] & direction2OpposingWall[dir]))
				return true;
			if (dir >= shootUp && dir <= shootLeft && p.strength > SKILL_COST)
				return true;
			return false;
		}

		// ����actionsд����Ҷ�����������һ�غϾ��棬����¼֮ǰ���еĳ���״̬���ɹ��պ�ָ���
		// ���վֵĻ��ͷ���false
		bool NextTurn()
		{
			int _, i, j;

			TurnStateTransfer &bt = backtrack[turnID];
			memset(&bt, 0, sizeof(bt));

			// 0. ɱ�����Ϸ�����
			for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
			{
				Player &p = players[_];
				if (!p.dead)
				{
					Direction &action = actions[_];
					if (action == stay) continue;

					if (!ActionValid(_, action))
					{
						bt.strengthDelta[_] += -p.strength;
						bt.change[_] = TurnStateTransfer::error;
						fieldContent[p.row][p.col] &= ~playerID2Mask[_];
						p.strength = 0;
						p.dead = true;
						aliveCount--;
					}
					else if (action < shootUp)
					{
						// �������Լ�ǿ��׳������ǲ���ǰ����
						GridContentType target =
							fieldContent[(p.row + dy[action] + height) % height]
							[(p.col + dx[action] + width) % width];
						if (target & playerMask)
							for (i = 0; i < MAX_PLAYER_COUNT; i++)
								if (target & playerID2Mask[i] &&
									players[i].strength > p.strength)
									action = stay;
					}
				}
			}

			// 1. λ�ñ仯
			for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
			{
				Player &_p = players[_];

				bt.actions[_] = actions[_];

				if (_p.dead || actions[_] == stay || actions[_] >= shootUp)
					continue;

				// �ƶ�
				fieldContent[_p.row][_p.col] &= ~playerID2Mask[_];
				_p.row = (_p.row + dy[actions[_]] + height) % height;
				_p.col = (_p.col + dx[actions[_]] + width) % width;
				fieldContent[_p.row][_p.col] |= playerID2Mask[_];
			}

			// 2. ��һ�Ź
			for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
			{
				Player &_p = players[_];
				if (_p.dead) continue;

				// �ж��Ƿ��������һ��
				int player, containedCount = 0;
				int containedPlayers[MAX_PLAYER_COUNT];
				for (player = 0; player < MAX_PLAYER_COUNT; player++)
					if (fieldContent[_p.row][_p.col] & playerID2Mask[player])
						containedPlayers[containedCount++] = player;

				if (containedCount > 1)
				{
					// NAIVE
					for (i = 0; i < containedCount; i++)
						for (j = 0; j < containedCount - i - 1; j++)
							if (players[containedPlayers[j]].strength <
								players[containedPlayers[j + 1]].strength)
								swap(containedPlayers[j],
									containedPlayers[j + 1]);

					int begin;
					for (begin = 1; begin < containedCount; begin++)
						if (players[containedPlayers[begin - 1]].strength >
							players[containedPlayers[begin]].strength)
							break;

					// ��Щ��ҽ��ᱻɱ��
					int lootedStrength = 0;
					for (i = begin; i < containedCount; i++)
					{
						int id = containedPlayers[i];
						Player &p = players[id];

						// �Ӹ���������
						fieldContent[p.row][p.col] &= ~playerID2Mask[id];
						p.dead = true;
						int drop = p.strength * 0.5;
						bt.strengthDelta[id] += -drop;
						bt.change[id] |= TurnStateTransfer::die;
						lootedStrength += drop;
						p.strength -= drop;
						aliveCount--;
					}

					// ������������
					int inc = lootedStrength / begin;
					for (i = 0; i < begin; i++)
					{
						int id = containedPlayers[i];
						Player &p = players[id];
						bt.strengthDelta[id] += inc;
						p.strength += inc;
					}
				}
			}

			// 2.5 ��ⷨ��
			for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
			{
				Player &_p = players[_];
				if (_p.dead || actions[_] < shootUp) continue;

				_p.strength -= SKILL_COST;
				bt.strengthDelta[_] -= SKILL_COST;

				int r = _p.row, c = _p.col, player;
				Direction dir = actions[_] - shootUp;

				// ��ָ���������⣨ɨ�����ֱ��������
				while (!(fieldStatic[r][c] & direction2OpposingWall[dir]))
				{
					r = (r + dy[dir] + height) % height;
					c = (c + dx[dir] + width) % width;

					// ���ת��һȦ��������
					if (r == _p.row && c == _p.col) break;

					if (fieldContent[r][c] & playerMask)
						for (player = 0; player < MAX_PLAYER_COUNT; player++)
							if (fieldContent[r][c] & playerID2Mask[player])
							{
								players[player].strength -= SKILL_COST * 1.5;
								bt.strengthDelta[player] -= SKILL_COST * 1.5;
								_p.strength += SKILL_COST * 1.5;
								bt.strengthDelta[_] += SKILL_COST * 1.5;
							}
				}
			}

			// *. ���һ�������������
			for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
			{
				Player &_p = players[_];
				if (_p.dead || _p.strength > 0) continue;

				// �Ӹ���������
				fieldContent[_p.row][_p.col] &= ~playerID2Mask[_];
				_p.dead = true;

				// ʹ��������Ϊ0
				bt.strengthDelta[_] += -_p.strength;
				bt.change[_] |= TurnStateTransfer::die;
				_p.strength = 0;
				aliveCount--;
			}

			// 3. ��������
			if (--generatorTurnLeft == 0)
			{
				generatorTurnLeft = GENERATOR_INTERVAL;
				NewFruits &fruits = newFruits[newFruitsCount++];
				fruits.newFruitCount = 0;
				for (i = 0; i < generatorCount; i++)
					for (int d = 0; d < 8; ++d)
					{
						// ȡ�࣬�������ر߽�
						int r = (generators[i].row + dy[d] + height) % height,
							c = (generators[i].col + dx[d] + width) % width;
						if (fieldStatic[r][c] & generator ||
							fieldContent[r][c] & (smallFruit | largeFruit))
							continue;
						fieldContent[r][c] |= smallFruit;
						fruits.newFruits[fruits.newFruitCount].row = r;
						fruits.newFruits[fruits.newFruitCount++].col = c;
						smallFruitCount++;
					}
			}

			// 4. �Ե�����
			for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
			{
				Player &_p = players[_];
				if (_p.dead) continue;

				GridContentType &content = fieldContent[_p.row][_p.col];

				// ֻ���ڸ�����ֻ���Լ���ʱ����ܳԵ�����
				if (content & playerMask & ~playerID2Mask[_]) continue;

				if (content & smallFruit)
				{
					content &= ~smallFruit;
					_p.strength++;
					bt.strengthDelta[_]++;
					smallFruitCount--;
					bt.change[_] |= TurnStateTransfer::ateSmall;
				}
				else if (content & largeFruit)
				{
					content &= ~largeFruit;
					if (_p.powerUpLeft == 0)
					{
						_p.strength += LARGE_FRUIT_ENHANCEMENT;
						bt.strengthDelta[_] += LARGE_FRUIT_ENHANCEMENT;
					}
					_p.powerUpLeft += LARGE_FRUIT_DURATION;
					bt.change[_] |= TurnStateTransfer::ateLarge;
				}
			}

			// 5. �󶹻غϼ���
			for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
			{
				Player &_p = players[_];
				if (_p.dead) continue;

				if (_p.powerUpLeft > 0)
				{
					bt.change[_] |= TurnStateTransfer::powerUpDrop;
					if (--_p.powerUpLeft == 0)
					{
						_p.strength -= LARGE_FRUIT_ENHANCEMENT;
						bt.strengthDelta[_] += -LARGE_FRUIT_ENHANCEMENT;
					}
				}
			}

			// *. ���һ�������������
			for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
			{
				Player &_p = players[_];
				if (_p.dead || _p.strength > 0) continue;

				// �Ӹ���������
				fieldContent[_p.row][_p.col] &= ~playerID2Mask[_];
				_p.dead = true;

				// ʹ��������Ϊ0
				bt.strengthDelta[_] += -_p.strength;
				bt.change[_] |= TurnStateTransfer::die;
				_p.strength = 0;
				aliveCount--;
			}

			++turnID;

			// �Ƿ�ֻʣһ�ˣ�
			if (aliveCount <= 1)
			{
				for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
					if (!players[_].dead)
					{
						bt.strengthDelta[_] += smallFruitCount;
						players[_].strength += smallFruitCount;
					}
				return false;
			}

			// �Ƿ�غϳ��ޣ�
			if (turnID >= MAX_TURN) return false;

			return true;
		}

		// ��ȡ�������������룬���ص��Ի��ύƽ̨ʹ�ö����ԡ�
		// ����ڱ��ص��ԣ�����������Ŷ�ȡ������ָ�����ļ���Ϊ�����ļ���
		// ʧ�ܺ���ѡ��ȴ��û�ֱ�����롣
		// ���ص���ʱ���Խ��ܶ����Ա������
		// Windows�¿�����Ctrl-Z��һ��������+�س�����ʾ���������
		// ������������ֻ����ܵ��м��ɡ�
		// localFileName����ΪNULL
		// obtainedData������Լ��ϻغϴ洢�����غ�ʹ�õ�����
		// obtainedGlobalData������Լ���Bot����ǰ�洢������
		// ����ֵ���Լ���playerID
		int ReadInput(const char *localFileName, string &obtainedData,
			string &obtainedGlobalData)
		{
			string str, chunk;
#ifdef _BOTZONE_ONLINE
			ios::sync_with_stdio(false);  //��\\)
			getline(cin, str);
#else
			if (localFileName)
			{
				ifstream fin(localFileName);
				if (1)
					while (getline(fin, chunk) && chunk != "") str += chunk;
				else
					while (getline(cin, chunk) && chunk != "") str += chunk;
			}
			else
				while (getline(cin, chunk) && chunk != "") str += chunk;
#endif
			Json::Reader reader;
			Json::Value input;
			reader.parse(str, input);

			int len = input["requests"].size();

			// ��ȡ���ؾ�̬״��
			Json::Value field = input["requests"][(Json::Value::UInt)0],
				staticField = field["static"],  // ǽ��Ͳ�����
				contentField = field["content"];        // ���Ӻ����
			height = field["height"].asInt();
			width = field["width"].asInt();
			LARGE_FRUIT_DURATION = field["LARGE_FRUIT_DURATION"].asInt();
			LARGE_FRUIT_ENHANCEMENT = field["LARGE_FRUIT_ENHANCEMENT"].asInt();
			SKILL_COST = field["SKILL_COST"].asInt();
			generatorTurnLeft = GENERATOR_INTERVAL =
				field["GENERATOR_INTERVAL"].asInt();

			PrepareInitialField(staticField, contentField);

			// ������ʷ�ָ�����
			for (int i = 1; i < len; i++)
			{
				Json::Value req = input["requests"][i];
				for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
					if (!players[_].dead)
						actions[_] =
						(Direction)req[playerID2str[_]]["action"].asInt();
				NextTurn();
			}

			obtainedData = input["data"].asString();
			obtainedGlobalData = input["globaldata"].asString();

			myID = field["id"].asInt();
			return myID;
		}

		// ���� static �� content ����׼�����صĳ�ʼ״��
		void PrepareInitialField(const Json::Value &staticField,
			const Json::Value &contentField)
		{
			int r, c, gid = 0;
			turnID = 0;
			generatorCount = 0;
			aliveCount = 0;
			smallFruitCount = 0;
			generatorTurnLeft = GENERATOR_INTERVAL;
			for (r = 0; r < height; r++)
				for (c = 0; c < width; c++)
				{
					GridContentType &content = fieldContent[r][c] =
						(GridContentType)contentField[r][c].asInt();
					GridStaticType &s = fieldStatic[r][c] =
						(GridStaticType)staticField[r][c].asInt();
					if (s & generator)
					{
						generators[gid].row = r;
						generators[gid++].col = c;
						generatorCount++;
					}
					if (content & smallFruit) smallFruitCount++;
					for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
						if (content & playerID2Mask[_])
						{
							Player &p = players[_];
							p.col = c;
							p.row = r;
							p.powerUpLeft = 0;
							p.strength = 1;
							p.dead = false;
							aliveCount++;
						}
				}
		}

		// ��ɾ��ߣ���������
		// action��ʾ���غϵ��ƶ�����stayΪ���ƶ���
		// shoot��ͷ�Ķ�����ʾ��ָ������ʩ�ż���
		// tauntText��ʾ��Ҫ���������
		// �����������ַ�����������ʾ����Ļ�ϲ������κ����ã����ձ�ʾ������
		// data��ʾ�Լ���洢����һ�غ�ʹ�õ����ݣ����ձ�ʾɾ��
		// globalData��ʾ�Լ���洢���Ժ�ʹ�õ����ݣ��滻����
		// ������ݿ��Կ�Ծ�ʹ�ã���һֱ�������Bot�ϣ����ձ�ʾɾ��
		void WriteOutput(Direction action, string tauntText = "",
			string data = "", string globalData = "") const
		{
			Json::Value ret;
			ret["response"]["action"] = action;
			ret["response"]["tauntText"] = tauntText;
			ret["data"] = data;
			ret["globaldata"] = globalData;
			ret["debug"] = (Json::Int)seed;

#ifdef _BOTZONE_ONLINE
			Json::FastWriter writer;
			cout << writer.write(ret) << endl;
#else
			// ����Ҳ��FastWriter����
			// Json::StyledWriter writer;
			Json::FastWriter writer;
			ofstream fout(".\\output.txt");
			fout << writer.write(ret);
#endif
		}

		// ������ʾ��ǰ��Ϸ״̬�������á�
		// �ύ��ƽ̨��ᱻ�Ż�����
		inline void DebugPrint() const
		{
#ifndef _BOTZONE_ONLINE
			ofstream fdebug(".\\debug.txt", fstream::app);
			fdebug
				<< "�غϺ�[" << turnID << "]�������[" << aliveCount
				<< "]| ͼ�� ������[G] �����[0/1/2/3] ������[*] ��[o] С��[.]\n";
			for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
			{
				const Player &p = players[_];
				fdebug << "[���" << _ << "(" << p.row << ", " << p.col
					<< ")|����" << p.strength << "|�ӳ�ʣ��غ�"
					<< p.powerUpLeft << "|" << (p.dead ? "����" : "���")
					<< "]\n";
			}
			fdebug << "  ";
			for (int c = 0; c < width; c++)
				fdebug << " " << setw(2) << c << " ";
			fdebug << "\n";
			for (int r = 0; r < height; r++)
			{
				fdebug << "  ";
				for (int c = 0; c < width; c++)
				{
					fdebug << " ";
					fdebug << ((fieldStatic[r][c] & wallNorth) ? "---" : "   ");
				}
				fdebug << "\n";
				fdebug << setw(2) << r;
				for (int c = 0; c < width; c++)
				{
					fdebug << ((fieldStatic[r][c] & wallWest) ? "|" : " ")
						<< " ";
					int hasPlayer = -1;
					for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
						if (fieldContent[r][c] & playerID2Mask[_])
							if (hasPlayer == -1)
								hasPlayer = _;
							else
								hasPlayer = 4;
					if (hasPlayer == 4)
						fdebug << "*";
					else if (hasPlayer != -1)
						fdebug << (char)('0' + hasPlayer);
					else if (fieldStatic[r][c] & generator)
						fdebug << "G";
					else if (fieldContent[r][c] & playerMask)
						fdebug << "*";
					else if (fieldContent[r][c] & smallFruit)
						fdebug << ".";
					else if (fieldContent[r][c] & largeFruit)
						fdebug << "o";
					else
						fdebug << " ";
					fdebug << " ";
				}
				fdebug << ((fieldStatic[r][width - 1] & wallEast) ? '|' : ' ')
					<< "\n";
			}
			fdebug << "  ";
			for (int c = 0; c < width; c++)
			{
				fdebug << " ";
				fdebug << ((fieldStatic[height - 1][c] & wallSouth) ? "---"
					: "   ");
			}
			fdebug << "\n\n";
#endif
		}

		// ����Զ���ĵ�����Ϣ
		inline void DebugStr(const string &s) const
		{
#ifndef _BOTZONE_ONLINE
			if (DEBUG_STR)
			{
				cout << s << endl;
				cout << "turnID " << turnID << endl;
			}
#endif
		}

		Json::Value SerializeCurrentTurnChange()
		{
			Json::Value result;
			TurnStateTransfer &bt = backtrack[turnID - 1];
			for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
			{
				result["actions"][_] = bt.actions[_];
				result["strengthDelta"][_] = bt.strengthDelta[_];
				result["change"][_] = bt.change[_];
			}
			return result;
		}

		// ��ʼ����Ϸ������
		GameField()
		{
			if (constructed)
				throw runtime_error(
					"One GameField Only");
			constructed = true;
			DEBUG_STR = false;
		}

		GameField(const GameField &b) : GameField()
		{
		}
	};

	bool GameField::constructed = false;
}

#endif

#ifndef HELPERS_H
#define HELPERS_H


namespace Helpers
{
	double actionScore[MAX_DIRECTION];

	inline int RandBetween(int a, int b)
	{
		if (a > b) swap(a, b);
		return rand() % (b - a) + a;
	}

	void RandomInit()
	{
		memset(&actionScore, 0, sizeof(actionScore));
	}

	void RandomPlay(Pacman::GameField &gameField, int myID)
	{
		int count = 0, myAct = -1;
		while (true)
		{
			// ��ÿ�������������ĺϷ�����
			for (int i = 0; i < MAX_PLAYER_COUNT; i++)
			{
				if (gameField.players[i].dead) continue;
				Pacman::Direction valid[MAX_DIRECTION];
				int vCount = 0;
				for (Pacman::Direction d = Pacman::stay; d < Pacman::shootLeft;
					++d)
					if (gameField.ActionValid(i, d)) valid[vCount++] = d;
				gameField.actions[i] = valid[RandBetween(0, vCount)];
			}

			if (count == 0) myAct = gameField.actions[myID];

			// ����һ������仯
			// NextTurn����true��ʾ��Ϸû�н���
			bool hasNext = gameField.NextTurn();
			count++;

			if (!hasNext) break;
		}

		// �������
		int total = 0;
		for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
			total += gameField.players[_].strength;
		if (total != 0)
			actionScore[myAct + 1] +=
			1.0 * gameField.players[myID].strength / total;

		// �ָ���Ϸ״̬����������Ǳ��غϣ�
		while (count-- > 0) gameField.PopState();
	}
}

#endif



#ifndef UTILS_H
#define UTILS_H

#include <deque>

using namespace Pacman;

///////////////////////////////////////////////////////////////////////////////
// module distance
// by cp

#define DISTANCE_NEAR_ENEMY 2
#define INFINITY_DISTANCE 1000

#define FIELD(a, r, c) *((a) + (r)*width + (c))
#define DISTANCE(a, r1, c1, r2, c2) \
    *((a) + ((r1)*width + (c1)) * width * height + (r2)*width + (c2))

// array is width * height * width * height
// array[r1][c1][r2][c2] means the shortest distance from (r1, c1) to (r2, c2)
// add considerEnemy by wd
void floyd(GameField& gameField, int* array, bool considerEnemy = true)
{
	int height = gameField.height;  // r
	int width = gameField.width;    // c
	int r1, c1, r2, c2, rt, ct, r, c;
	memset(array, 0x33,
		sizeof(int) * height * height * width * width);  // set to infinity
	for (r = 0; r < height; r++)
		for (c = 0; c < width; c++)
		{
			GridStaticType& type = gameField.fieldStatic[r][c];
			DISTANCE(array, r, c, r, c) = 0;
			for (int d = 0; d < 4; d++)
				if (!(type & direction2OpposingWall[d]))
					DISTANCE(array, r, c, (r + dy[d] + height) % height,
					(c + dx[d] + width) % width) = 1;
		}

	if (considerEnemy)
	{
		for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
		{
			if (i == gameField.myID) continue;
			Player& _p = gameField.players[i];
			if (_p.dead) continue;
			if (_p.strength < gameField.players[gameField.myID].strength)
				continue;
			int r = _p.row;
			int c = _p.col;
			GridStaticType& type = gameField.fieldStatic[r][c];
			for (int d = 0; d < 4; ++d)
				if (!(type & direction2OpposingWall[d]))
					DISTANCE(array, r, c, (r + dy[d] + height) % height,
					(c + dx[d] + width) % width) +=
					DISTANCE_NEAR_ENEMY;
		}
	}

	for (rt = 0; rt < height; rt++)
		for (ct = 0; ct < width; ct++)
			for (r1 = 0; r1 < height; r1++)
				for (c1 = 0; c1 < width; c1++)
					for (r2 = 0; r2 < height; r2++)
						for (c2 = 0; c2 < width; c2++)
						{
							int temp = DISTANCE(array, rt, ct, r1, c1) +
								DISTANCE(array, rt, ct, r2, c2);
							if (temp < DISTANCE(array, r1, c1, r2, c2))
								DISTANCE(array, r1, c1, r2, c2) = temp;
						}
}

// DEPRECATED
// return the first move from (r1, c1) to (r2, c2)
// return stay when an error occurs
// Direction dijkstra(GameField& gameField, int r1, int c1, int r2, int c2)
// {
// int height = gameField.height;
// int width = gameField.width;
// if (gameField.fieldStatic[r1][c1] & generator ||
// gameField.fieldStatic[r2][c2] & generator || (r1 == c1 && r2 == c2))
// return stay;
// int predirection[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
// int dis[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
// memset(dis, 0x33, sizeof(dis));
// dis[r1][c1] = 0;
// deque<int> row = {r1}, col = {c1};
// while (row.size())
// {
// int r = row.front(), c = col.front();
// row.pop_front();
// col.pop_front();
// for (int d = 0; d < 4; d++)
// if (!(gameField.fieldStatic[r][c] & direction2OpposingWall[d]))
// {
// int nr = (r + dy[d] + height) % height,
// nc = (c + dx[d] + width) % width;
// if (dis[nr][nc] > 1000)
// {
// dis[nr][nc] = dis[r][c] + 1;
// predirection[nr][nc] = d;
// row.push_back(nr);
// col.push_back(nc);
// if (nr == r2 && nc == c2) goto done;
// }
// }
// }
// return stay;
// done:
// int r = r2, c = c2;
// while (dis[r][c] > 1)
// {
// int rr = (r - dy[predirection[r][c]] + height) % height;
// int cc = (c - dx[predirection[r][c]] + width) % width;
// r = rr;
// c = cc;
// }
// return (Direction)predirection[r][c];
// }

// return the first move, make use of the result of floyd
// in O(1) time
Direction routineFloyd(GameField& gameField, int r1, int c1, int r2, int c2,
	int* array)
{
	if (gameField.fieldStatic[r1][c1] & generator ||
		gameField.fieldStatic[r2][c2] & generator || (r1 == r2 && c1 == c2))
		return stay;
	int height = gameField.height;  // r
	int width = gameField.width;    // c
	int d;
	for (d = 0; d < 4; d++)
		if (!(gameField.fieldStatic[r1][c1] & direction2OpposingWall[d]))
		{
			int nr = (r1 + dy[d] + height) % height,
				nc = (c1 + dx[d] + width) % width;
			if (DISTANCE(array, r2, c2, r1, c1) ==
				DISTANCE(array, r2, c2, nr, nc) + 1)
				break;
		}
	return Direction(d);
}

///////////////////////////////////////////////////////////////////////////////
// module dead road
// by wd

bool isDeadRoad[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
bool scannedDeadRoad[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
int wallCount[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];

void scanDeadRoad(GameField& gameField, int row, int col, int level)
{
	if (scannedDeadRoad[row][col]) return;
	// for (int i = 0; i < level; ++i) cout << " ";
	// cout << row << " " << col << " "
	// << gameField.fieldStatic[row][col] << endl;
	scannedDeadRoad[row][col] = true;

	int nowWallCount = 0;  // �Աߵ�ǽ����·
	for (int d = 0; d < 4; ++d)
	{
		if (gameField.fieldStatic[row][col] & direction2OpposingWall[d])
			++nowWallCount;
		else
		{
			int newRow = (row + dy[d] + gameField.height) % gameField.height;
			int newCol = (col + dx[d] + gameField.width) % gameField.width;
			scanDeadRoad(gameField, newRow, newCol, level + 1);
			if (isDeadRoad[newRow][newCol]) ++nowWallCount;
		}
	}
	if (nowWallCount >= 3) isDeadRoad[row][col] = true;
	wallCount[row][col] = nowWallCount;
}

void scanAllDeadRoad(GameField& gameField)
{
	memset(&isDeadRoad, 0, sizeof(isDeadRoad));
	// �����ϽǺ����½Ǹ�floodfillһ�Σ����ֻ��һ���ڳ�ʼλ�ø����������
	memset(&scannedDeadRoad, 0, sizeof(scannedDeadRoad));
	for (int i = 0; i < gameField.height; ++i)
		for (int j = 0; j < gameField.width; ++j)
			scanDeadRoad(gameField, i, j, 0);
	memset(&scannedDeadRoad, 0, sizeof(scannedDeadRoad));
	for (int i = 0; i < gameField.height; ++i)
		for (int j = 0; j < gameField.width; ++j)
			scanDeadRoad(gameField, gameField.height - i, gameField.height - j,
				0);
}

///////////////////////////////////////////////////////////////////////////////
// module shoot
// by wd

// ���������ж�����ķ���
// ��������䵽�����stay
// f������ߣ�t�Ǳ�����
// TODO������һ�������
Direction shootDirection(GameField& gameField, int fr, int fc, int tr, int tc)
{
	bool canShoot;
	int height = gameField.height;
	int width = gameField.width;
	if (fc == tc)
	{
		canShoot = true;
		for (int i = fr; i != tr; i = (i - 1 + height) % height)
		{
			if (gameField.fieldStatic[i][fc] & wallNorth)
			{
				canShoot = false;
				break;
			}
		}
		if (canShoot) return shootUp;

		canShoot = true;
		for (int i = fr; i != tr; i = (i + 1 + height) % height)
		{
			if (gameField.fieldStatic[i][fc] & wallSouth)
			{
				canShoot = false;
				break;
			}
		}
		if (canShoot) return shootDown;
	}
	if (fr == tr)
	{
		canShoot = true;
		for (int i = fc; i != tc; i = (i - 1 + width) % width)
		{
			if (gameField.fieldStatic[fr][i] & wallWest)
			{
				canShoot = false;
				break;
			}
		}
		if (canShoot) return shootLeft;

		canShoot = true;
		for (int i = fc; i != tc; i = (i + 1 + width) % width)
		{
			if (gameField.fieldStatic[fr][i] & wallEast)
			{
				canShoot = false;
				break;
			}
		}
		if (canShoot) return shootRight;
	}
	return stay;
}

Direction shootMustHit(GameField& gameField, int fr, int fc, int tr, int tc)
{
	Direction d = shootDirection(gameField, fr, fc, tr, tc);
	if (d == stay)
		return stay;
	else if (d == shootUp || d == shootDown)
	{
		if (gameField.fieldStatic[tr][tc] & wallWest &&
			gameField.fieldStatic[tr][tc] & wallEast)
			return d;
		else
			return stay;
	}
	else if (d == shootLeft | d == shootRight)
	{
		if (gameField.fieldStatic[tr][tc] & wallNorth &&
			gameField.fieldStatic[tr][tc] & wallSouth)
			return d;
		else
			return stay;
	}
}

#endif


#define NO_CHOICE (Direction)(-2)
#define RUN_AWAY_DISTANCE 2

int main()
{
	GameField gameField;
#ifdef DEFINE_DEBUG_STR
	gameField.DEBUG_STR = false;
#endif
	string data, globalData;  // ���ǻغ�֮����Դ��ݵ���Ϣ
	string tauntText = "";

	int myID = gameField.ReadInput("input.txt", data, globalData);

	// �Լ�����
	if (gameField.players[myID].dead)
	{
		gameField.WriteOutput((Direction)(-1), "DEAD", data, globalData);
		return 0;
	}

	srand(seed + myID + gameField.turnID);

	int height = gameField.height;
	int width = gameField.width;
	int* a = (int*)malloc(sizeof(int) * height * height * width * width);
	floyd(gameField, a, true);  // �����ƿ�������С���Լ�����֮��ľ���
	scanAllDeadRoad(gameField);

	Direction choice = NO_CHOICE;

	// ������Լ����������
	if (choice == NO_CHOICE)
	{
		tauntText = "run away";
		int minDis = INFINITY_DISTANCE;
		int r1, c1;
		for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
		{
			if (i == myID) continue;
			Player& _p = gameField.players[i];
			if (_p.dead) continue;
			if (_p.strength <= gameField.players[myID].strength) continue;
			int r = _p.row;
			int c = _p.col;
			int nowDis = DISTANCE(a, r, c, gameField.players[myID].row,
				gameField.players[myID].col);
			if (nowDis < minDis && nowDis <= RUN_AWAY_DISTANCE)
			{
				minDis = nowDis;
				r1 = r;
				c1 = c;
			}
		}

		if (minDis < INFINITY_DISTANCE)
		{
			//tauntText += " from " + to_string(r1) + " " + to_string(c1);
			// Ѱ������Ĳ�����·�ĸ���
			minDis = INFINITY_DISTANCE;
			for (int r = 0; r < height; r++)
				for (int c = 0; c < width; c++)
					if (!isDeadRoad[r][c])
					{
						int nowDis =
							DISTANCE(a, r, c, gameField.players[myID].row,
								gameField.players[myID].col);
						if (nowDis < minDis)
						{
							minDis = nowDis;
							r1 = r;
							c1 = c;
						}
					}
		}

		if (minDis < INFINITY_DISTANCE)
		{
			//tauntText += " to " + to_string(r1) + " " + to_string(c1);
			choice = routineFloyd(gameField, gameField.players[myID].row,
				gameField.players[myID].col, r1, c1, a);
		}
	}

	// ̰�ģ����ܱ��������ȳԵ��Ĺ����е������
	if (choice == NO_CHOICE)
	{
		tauntText = "eat fruit";
		int minDis = INFINITY_DISTANCE;
		int r1, c1;
		for (int r = 0; r < height; r++)
			for (int c = 0; c < width; c++)
				if (gameField.fieldContent[r][c] & (smallFruit | largeFruit))
				{
					int dis = INFINITY_DISTANCE;
					int id;
					for (int i = 0; i < MAX_PLAYER_COUNT; i++)
					{
						Player& _p = gameField.players[i];
						if (_p.dead) continue;
						if (DISTANCE(a, r, c, _p.row, _p.col) < dis)
						{
							dis = DISTANCE(a, r, c, _p.row, _p.col);
							id = i;
						}
					}
					if (id == myID && dis < minDis)
					{
						minDis = dis;
						r1 = r;
						c1 = c;
					}
				}
		if (minDis < INFINITY_DISTANCE)
		{
			//tauntText += " to " + to_string(r1) + " " + to_string(c1);
			choice = routineFloyd(gameField, gameField.players[myID].row,
				gameField.players[myID].col, r1, c1, a);
		}
	}

	// ̰�ģ��Ա��Լ�����С�����е������
	if (choice == NO_CHOICE)
	{
		tauntText = "eat small enemy";
		int minDis = INFINITY_DISTANCE;
		int r1, c1;
		for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
		{
			if (i == myID) continue;
			Player& _p = gameField.players[i];
			if (_p.dead) continue;
			if (_p.strength >= gameField.players[myID].strength) continue;
			int r = _p.row;
			int c = _p.col;
			int nowDis = DISTANCE(a, r, c, gameField.players[myID].row,
				gameField.players[myID].col);
			if (nowDis < minDis)
			{
				minDis = nowDis;
				r1 = r;
				c1 = c;
			}
		}
		if (minDis < INFINITY_DISTANCE)
		{
			//tauntText += " to " + to_string(r1) + " " + to_string(c1);
			choice = routineFloyd(gameField, gameField.players[myID].row,
				gameField.players[myID].col, r1, c1, a);
		}
	}

	// ���ģ��
	if (choice == NO_CHOICE)
	{
		tauntText = "random play";
		Helpers::RandomInit();
		for (int i = 0; i < 1000; i++) Helpers::RandomPlay(gameField, myID);
		for (int i = 0; i < MAX_DIRECTION; i++)
		{
			if (choice == NO_CHOICE ||
				Helpers::actionScore[i] > Helpers::actionScore[choice])
				choice = (Direction)i;
		}
		choice = (Direction)((int)choice - 1);
	}

	gameField.DebugPrint();
	gameField.WriteOutput(choice, tauntText, data, globalData);
	return 0;
}
