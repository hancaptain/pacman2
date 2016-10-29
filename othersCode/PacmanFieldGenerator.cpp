bool visited[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
Pacman::Direction dirEnumerateList[][4] = {
    {Pacman::up, Pacman::right, Pacman::down, Pacman::left},
    {Pacman::up, Pacman::right, Pacman::left, Pacman::down},
    {Pacman::up, Pacman::down, Pacman::right, Pacman::left},
    {Pacman::up, Pacman::down, Pacman::left, Pacman::right},
    {Pacman::up, Pacman::left, Pacman::right, Pacman::down},
    {Pacman::up, Pacman::left, Pacman::down, Pacman::right},
    {Pacman::right, Pacman::up, Pacman::down, Pacman::left},
    {Pacman::right, Pacman::up, Pacman::left, Pacman::down},
    {Pacman::right, Pacman::down, Pacman::up, Pacman::left},
    {Pacman::right, Pacman::down, Pacman::left, Pacman::up},
    {Pacman::right, Pacman::left, Pacman::up, Pacman::down},
    {Pacman::right, Pacman::left, Pacman::down, Pacman::up},
    {Pacman::down, Pacman::up, Pacman::right, Pacman::left},
    {Pacman::down, Pacman::up, Pacman::left, Pacman::right},
    {Pacman::down, Pacman::right, Pacman::up, Pacman::left},
    {Pacman::down, Pacman::right, Pacman::left, Pacman::up},
    {Pacman::down, Pacman::left, Pacman::up, Pacman::right},
    {Pacman::down, Pacman::left, Pacman::right, Pacman::up},
    {Pacman::left, Pacman::up, Pacman::right, Pacman::down},
    {Pacman::left, Pacman::up, Pacman::down, Pacman::right},
    {Pacman::left, Pacman::right, Pacman::up, Pacman::down},
    {Pacman::left, Pacman::right, Pacman::down, Pacman::up},
    {Pacman::left, Pacman::down, Pacman::up, Pacman::right},
    {Pacman::left, Pacman::down, Pacman::right, Pacman::up}};
int unvisitedCount;
bool borderBroken[4];

bool EnsureConnected(Pacman::GameField &gameField, int r, int c, int maxH,
                     int maxW)
{
    visited[r][c] = true;
    if (--unvisitedCount == 0) return true;
    Pacman::Direction *list = dirEnumerateList[RandBetween(0, 24)];
    for (int i = 0; i < 4; i++)
    {
        int nr = r + Pacman::dy[list[i]], nc = c + Pacman::dx[list[i]];
        if (nr < maxH && nr >= 0 && nc < maxW && nc >= 0)
        {
            if (!visited[nr][nc])
            {
                if (gameField.fieldStatic[nr][nc] & Pacman::generator)
                {
                    visited[nr][nc] = true;
                    if (--unvisitedCount == 0) return true;
                    continue;
                }
                // 破！
                gameField.fieldStatic[r][c] &=
                    ~Pacman::direction2OpposingWall[list[i]];
                if (list[i] == Pacman::up)
                    gameField.fieldStatic[nr][nc] &= ~Pacman::wallSouth;
                if (list[i] == Pacman::down)
                    gameField.fieldStatic[nr][nc] &= ~Pacman::wallNorth;
                if (list[i] == Pacman::left)
                    gameField.fieldStatic[nr][nc] &= ~Pacman::wallEast;
                if (list[i] == Pacman::right)
                    gameField.fieldStatic[nr][nc] &= ~Pacman::wallWest;
                if (EnsureConnected(gameField, nr, nc, maxH, maxW)) return true;
            }
        }
        else if (!borderBroken[list[i]])  // 打破一个边界壁障
        {
            borderBroken[list[i]] = true;
            gameField.fieldStatic[r][c] &=
                ~Pacman::direction2OpposingWall[list[i]];
        }
    }
    return false;
}

void InitializeField(Pacman::GameField &gameField)
{
    int portionH = (gameField.height + 1) / 2,
        portionW = (gameField.width + 1) / 2;

    // 先将所有格子围上
    for (int r = 0; r < portionH; r++)
        for (int c = 0; c < portionW; c++)
            gameField.fieldStatic[r][c] = Pacman::wallNorth | Pacman::wallEast |
                                          Pacman::wallSouth | Pacman::wallWest;

    unvisitedCount = portionH * portionW;

    // 创建产生器（因为产生器是障碍物啊）
    {
        int r = RandBetween(0, portionH - 1), c = RandBetween(0, portionW - 1);
        gameField.fieldStatic[r][c] |= Pacman::generator;
        gameField.fieldStatic[r][gameField.width - 1 - c] |= Pacman::generator;
        gameField.fieldStatic[gameField.height - 1 - r][c] |= Pacman::generator;
        gameField
            .fieldStatic[gameField.height - 1 - r][gameField.width - 1 - c] |=
            Pacman::generator;
    }

    // 打通壁障
    memset(visited, 0, sizeof(visited));
    memset(borderBroken, 0, sizeof(borderBroken));
    EnsureConnected(gameField, RandBetween(0, portionH),
                    RandBetween(0, portionW), portionH, portionW);

    if (!borderBroken[Pacman::left])
        gameField.fieldStatic[RandBetween(0, portionH)][0] &= ~Pacman::wallWest;
    if (!borderBroken[Pacman::right])
        gameField.fieldStatic[RandBetween(0, portionH)][portionW - 1] &=
            ~Pacman::wallEast;
    if (!borderBroken[Pacman::up])
        gameField.fieldStatic[0][RandBetween(0, portionW)] &=
            ~Pacman::wallNorth;
    if (!borderBroken[Pacman::down])
        gameField.fieldStatic[portionH - 1][RandBetween(0, portionW)] &=
            ~Pacman::wallSouth;

    // 生成对称场地
    for (int r = 0; r < portionH; r++)
        for (int c = 0; c < portionW; c++)
        {
            bool n = !!(gameField.fieldStatic[r][c] & Pacman::wallNorth),
                 w = !!(gameField.fieldStatic[r][c] & Pacman::wallWest),
                 s = !!(gameField.fieldStatic[r][c] & Pacman::wallSouth),
                 e = !!(gameField.fieldStatic[r][c] & Pacman::wallEast);
            Pacman::GridStaticType hasGenerator =
                gameField.fieldStatic[r][c] & Pacman::generator;
            if ((c == 0 || c == portionW - 1) && rand() % 4 == 0)
            {
                if (c == 0)
                    w = false;
                else
                    e = false;
            }
            if ((r == 0 || r == portionH - 1) && rand() % 4 == 0)
            {
                if (r == 0)
                    n = false;
                else
                    s = false;
            }
            if (r * 2 + 1 == gameField.height) s = n;
            if (c * 2 + 1 == gameField.width) e = w;

            gameField.fieldStatic[r][c] =
                (n ? Pacman::wallNorth : Pacman::emptyWall) | hasGenerator |
                (w ? Pacman::wallWest : Pacman::emptyWall) |
                (s ? Pacman::wallSouth : Pacman::emptyWall) |
                (e ? Pacman::wallEast : Pacman::emptyWall);
            gameField.fieldStatic[r][gameField.width - 1 - c] =
                (n ? Pacman::wallNorth : Pacman::emptyWall) | hasGenerator |
                (w ? Pacman::wallEast : Pacman::emptyWall) |
                (s ? Pacman::wallSouth : Pacman::emptyWall) |
                (e ? Pacman::wallWest : Pacman::emptyWall);
            gameField.fieldStatic[gameField.height - 1 - r][c] =
                (n ? Pacman::wallSouth : Pacman::emptyWall) | hasGenerator |
                (w ? Pacman::wallWest : Pacman::emptyWall) |
                (s ? Pacman::wallNorth : Pacman::emptyWall) |
                (e ? Pacman::wallEast : Pacman::emptyWall);
            gameField.fieldStatic[gameField.height - 1 - r]
                                 [gameField.width - 1 - c] =
                (n ? Pacman::wallSouth : Pacman::emptyWall) | hasGenerator |
                (w ? Pacman::wallEast : Pacman::emptyWall) |
                (s ? Pacman::wallNorth : Pacman::emptyWall) |
                (e ? Pacman::wallWest : Pacman::emptyWall);

            gameField.fieldContent[r][c] =
                gameField.fieldContent[r][gameField.width - 1 - c] =
                    gameField.fieldContent[gameField.height - 1 - r][c] =
                        gameField.fieldContent[gameField.height - 1 - r]
                                              [gameField.width - 1 - c] =
                            Pacman::empty;
        }

    // 再次围上所有的产生器
    for (int r = 0; r < gameField.height; r++)
        for (int c = 0; c < gameField.width; c++)
            if (gameField.fieldStatic[r][c] & Pacman::generator)
            {
                gameField.fieldStatic[r][c] |=
                    Pacman::wallNorth | Pacman::wallEast | Pacman::wallSouth |
                    Pacman::wallWest;
                for (Pacman::Direction dir = Pacman::up; dir < 4; ++dir)
                {
                    Pacman::GridStaticType &s =
                        gameField.fieldStatic
                            [(r + Pacman::dy[dir] + gameField.height) %
                             gameField.height]
                            [(c + Pacman::dx[dir] + gameField.width) %
                             gameField.width];
                    if (dir == Pacman::up) s |= Pacman::wallSouth;
                    if (dir == Pacman::down) s |= Pacman::wallNorth;
                    if (dir == Pacman::left) s |= Pacman::wallEast;
                    if (dir == Pacman::right) s |= Pacman::wallWest;
                }
            }

    // 创建玩家
    while (true)
    {
        int r = RandBetween(0, portionH - 1),
            c = RandBetween(0, portionW - 1);  // 保持安全距离
        if (gameField.fieldStatic[r][c] & Pacman::generator) continue;
        gameField.fieldContent[r][c] |= Pacman::player1;
        gameField.fieldContent[r][gameField.width - 1 - c] |= Pacman::player2;
        gameField.fieldContent[gameField.height - 1 - r][c] |= Pacman::player3;
        gameField
            .fieldContent[gameField.height - 1 - r][gameField.width - 1 - c] |=
            Pacman::player4;
        break;
    }

    // 创建大豆
    while (true)
    {
        int r = RandBetween(0, portionH - 1), c = RandBetween(0, portionW - 1);
        if (gameField.fieldStatic[r][c] & Pacman::generator ||
            gameField.fieldContent[r][c] & Pacman::player1)
            continue;
        gameField.fieldContent[r][c] |= Pacman::largeFruit;
        gameField.fieldContent[r][gameField.width - 1 - c] |=
            Pacman::largeFruit;
        gameField.fieldContent[gameField.height - 1 - r][c] |=
            Pacman::largeFruit;
        gameField
            .fieldContent[gameField.height - 1 - r][gameField.width - 1 - c] |=
            Pacman::largeFruit;
        break;
    }

    // 撒豆子
    gameField.smallFruitCount = 0;
    for (int r = 0; r < portionH - 1; r++)
        for (int c = 0; c < portionW - 1; c++)
        {
            if (gameField.fieldStatic[r][c] & Pacman::generator ||
                gameField.fieldContent[r][c] &
                    (Pacman::player1 | Pacman::largeFruit) ||
                rand() % 3 > 0)
                continue;
            gameField.fieldContent[r][c] =
                gameField.fieldContent[r][gameField.width - 1 - c] =
                    gameField.fieldContent[gameField.height - 1 - r][c] =
                        gameField.fieldContent[gameField.height - 1 - r]
                                              [gameField.width - 1 - c] =
                            Pacman::smallFruit;
            gameField.smallFruitCount++;
        }

    // 暴力获取状态（怕麻烦而已……）
    int gid = 0;
    gameField.generatorCount = 0;
    gameField.aliveCount = 0;
    gameField.generatorTurnLeft = gameField.GENERATOR_INTERVAL;
    for (int r = 0; r < gameField.height; r++)
        for (int c = 0; c < gameField.width; c++)
        {
            Pacman::GridContentType &content = gameField.fieldContent[r][c];
            Pacman::GridStaticType &s = gameField.fieldStatic[r][c];
            if (s & Pacman::generator)
            {
                gameField.generators[gid].row = r;
                gameField.generators[gid++].col = c;
                gameField.generatorCount++;
            }
            for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
                if (content & Pacman::playerID2Mask[_])
                {
                    Pacman::Player &p = gameField.players[_];
                    p.col = c;
                    p.row = r;
                    p.powerUpLeft = 0;
                    p.strength = 1;
                    p.dead = false;
                    gameField.aliveCount++;
                }
        }
}
