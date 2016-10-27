# pacman2

wd：

defaultInputs\\000.txt是一个几乎空白的地图，其他地图是cp爬下来的数据

我打算用server判断玩家是否已死，但是有各种BUG，目前我让玩家死了之后每回合在tauntText输出DEAD，然后让server判断

玩家在tauntText输出SERVER_STOP会让服务器直接停止，用于调试

pacman.h用了using namespace std，因为我懒

我们自己写的代码都用using namespace Pacman吧

我们统一用驼峰命名法吧
