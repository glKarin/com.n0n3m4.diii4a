## 更新日志

----------------------------------------------------------------------------------

> 1.1.0harmattan50 (2024-05-31)

* 新增`毁灭战士 3 BFG`(RBDOOM-3-BFG ver1.4.0)支持, 游戏数据文件夹名为`doom3bfg/base`. 详情[RBDOOM-3-BFG](https://github.com/RobertBeckebans/RBDOOM-3-BFG)和[DOOM-3-BFG](https://store.steampowered.com/agecheck/app/208200/).
* 新增`雷神之锤 I`(Darkplaces)支持, 游戏数据文件夹名为`darkplaces/id1`. 详情[DarkPlaces](https://github.com/DarkPlacesEngine/darkplaces)和[Quake I](https://store.steampowered.com/app/2310/Quake/).
* 修复The Dark Mod(v2.12)在Mali GPU的着色器错误.
* 更新雷神之锤2(Yamagi Quake II)版本.
* 毁灭战士3/雷神之锤4/掠食(2006)支持在多线程下启用调试渲染工具(除r_showSurfaceInfo).
* 毁灭战士3/雷神之锤4/掠食(2006)支持使用cvar r_noLight 0和2, 允许在游戏中切换是否禁用光照渲染.

----------------------------------------------------------------------------------

> 1.1.0harmattan50 (2024-04-30)

* 支持新渲染通道: 热浪(例如 BFG9000飞行物的扭曲, 火箭炮的爆炸), colorProcess(例如 marscity2镜子前的血色影片).
* 雷神之锤4支持新GLSL渲染通道(例如 机枪的瞄准镜特效和弹孔).
* `Control`选项卡新增控制虚拟摇杆的显示模式(总是显示; 隐藏; 仅按下显示).
* 改进Phong/Blinn-Phong光照模型着色器使用高精度.
* The Dark Mod中强制禁用压缩纹理.
* 设置中可以启用每个游戏的数据文件夹独立放置: 毁灭战士3 -> doom3/; 雷神之锤4 -> quake4/; 掠食(2006) -> prey/; 雷神之锤1 -> quake1/; 雷神之锤2 -> quake2/; 雷神之锤3 -> quake3/; 重返德军总部 -> rtcw/; The Dark Mod -> darkmod/ (总是独立); 毁灭战士3 BFG -> doom3bfg/ (总是独立).

----------------------------------------------------------------------------------

> 1.1.0harmattan39 (2024-04-10)

* 阴影映射支持镂空图层阴影(cvar `r_forceShadowMapsOnAlphaTestedSurfaces`, 默认0).
* 新增毁灭战士3 mod `LibreCoop`支持, 游戏数据文件夹命名为`librecoop`. 详情[LibreCoop](https://www.moddb.com/mods/librecoop-dhewm3-coop).
* 新增`雷神之锤2`支持, 游戏数据文件夹命名为`baseq2`. 详情[Yamagi Quake II](https://github.com/yquake2/yquake2)和[Quake II](https://store.steampowered.com/app/2320/Quake_II/).
* 新增`雷神之锤3竞技场`支持, 游戏数据文件夹命名为`baseq3`; 新增`雷神之锤3团队竞技场`支持, 游戏数据文件夹命名为`missionpack`. 详情[ioquake3](https://github.com/ioquake/ioq3)和[Quake III Arena](https://store.steampowered.com/app/2200/Quake_III_Arena/).
* 新增`重返德军总部`支持, 游戏数据文件夹命名为`main`. 详情[iortcw](https://github.com/iortcw/iortcw)和[Return to Castle Wolfenstein](https://www.moddb.com/games/return-to-castle-wolfenstein).
* 新增`The Dark Mod`v2.12支持, 游戏数据文件夹命名为`darkmod`. 详情[The Dark Mod](https://www.thedarkmod.com).
* 新增一个虚拟按键主题.

----------------------------------------------------------------------------------

> 1.1.0harmattan38 (2024-02-05)

* 修复非Adreno GPU的阴影映射阴影.
* 支持雷神之锤4关卡加载完成暂停等待(cvar `com_skipLevelLoadPause`).

----------------------------------------------------------------------------------

> 1.1.0harmattan37 (2024-01-06)

* 修复屏幕按键初始键码配置.
* 滑动按键支持设置为点击触发.
* 新增dds格式屏幕截图.
* 新增cvar `r_scaleMenusTo43`启用4:3比例菜单.

----------------------------------------------------------------------------------

> 1.1.0harmattan36 (2023-12-31)

* 修复预烘培阴影图软阴影渲染.
* 雷神之锤4修复EFX混响.
* 添加半透明模板阴影支持(bool型cvar `harm_r_stencilShadowTranslucent`(默认 0); 浮点型cvar `harm_r_stencilShadowAlpha`设置透明度).
* 掠食(2006)添加浮点型cvar `harm_ui_subtitlesTextScale`控制字幕字体大小.
* 支持cvar `r_brightness`.
* 掠食(2006)修复武器发射爆炸贴花渲染Z-Fighting.
* 数据文件目录选择器支持安卓SAF框架.
* 新的默认屏幕按键布局.
* 添加毁灭战士3 mod `Stupid Angry Bot`(a7x)(需要邪恶复苏数据包), 游戏数据文件夹名为`sabot`. 详情 [SABot(a7x)](https://www.moddb.com/downloads/sabot-alpha-7x).
* 添加毁灭战士3 mod `Overthinked DooM^3`, 游戏数据文件夹名为`overthinked`. 详情 [Overthinked DooM^3](https://www.moddb.com/mods/overthinked-doom3).
* 添加毁灭战士3 mod `Fragging Free`(需要邪恶复苏数据包), 游戏数据文件夹名为`fraggingfree`. 详情 [Fragging Free](https://www.moddb.com/mods/fragging-free).
* 添加毁灭战士3 mod `HeXen:Edge of Chaos`, 游戏数据文件夹名为`hexeneoc`. 详情 [HeXen:Edge of Chaos](https://www.moddb.com/mods/hexen-edge-of-chaos).

----------------------------------------------------------------------------------

> 1.1.0harmattan35 (2023-10-29)

* 优化Shadow mapping软阴影. OpenGLES2.0阴影图使用深度纹理.
* 新增OpenALA(soft)和EFX混响支持.",
* 掠食2006 Beam模型渲染优化(by [lvonasek/PreyVR](https://github.com/lvonasek/PreyVR)).
* 掠食2006新增字幕支持.
* 修复反横屏下的陀螺仪.
* 雷神之锤4多人游戏修复Bot头部模型, 新增Bot等级控制支持(cvar `harm_si_botLevel`, 需要重新解压新的`sabot_a9.pk4`资源).

----------------------------------------------------------------------------------

> 1.1.0harmattan33 (2023-10-01)

* 新增shadow mapping软阴影支持(测试, 存在一些错误渲染), 使用`r_useShadowMapping`切换`shadow mapping`或`stencil shadow`.
* 雷神之锤4移除多人游戏Bot伪客户端, 使用SABot-a9 mod替换多人游戏的bot(需要先解压资源文件).
* 修复掠食2006的设置页面选项卡.
* 雷神之锤4新增`full-body awareness` mod. 设置布尔型cvar `harm_pm_fullBodyAwareness`为1开启, 并且可以使用`harm_pm_fullBodyAwarenessOffset`设置视角偏移(可以调整为第三人称视角), 使用`harm_pm_fullBodyAwarenessHeadJoint`设置自定义头部关节名称(视角位置).
* 支持限制最大(cvar `harm_r_maxFps`).
* 支持obj/dae格式静态模型, 修复png格式图片加载.
* 新增跳过启动动画支持.
* 新增简易CVar编辑器.
* OpenGL顶点索引使用4字节以加载大模型.
* 新增GLES3.0支持, 在`图形`选项卡切换.

----------------------------------------------------------------------------------

> 1.1.0harmattan32 (2023-06-30)

* 添加 `中文`, `俄语`(by [ALord7](https://4pda.ru/forum/index.php?showuser=5043340)).
* 一些虚拟按键设置移植`Configure on-screen controls`页面.
* 毁灭战士3新增`full-body awareness` mod. 通过设置cvar`harm_pm_fullBodyAwareness`为1开启, 并且可以使用cvar `harm_pm_fullBodyAwarenessOffset`来设置偏移.
* 在选项卡`General`下的`GameLib`支持添加外部的游戏动态库(测试. 由于系统安全方案不确定是否对所有设备/安卓版本有效. 允许你通过DIII4A项目编译你自己的游戏mod动态库(armv7/armv8)然后运行在原版的idTech4A++).
* 如果启用`Find game library in game data directory`, 则支持加载外部的位于`Game working directory`/`fs_game`文件夹下的游戏动态库来代替apk内置的游戏动态库(测试. 由于系统安全方案不确定是否对所有设备/安卓版本有效. 允许你通过DIII4A项目编译你自己的游戏mod动态库(armv7/armv8), 然后命名为`gameaarch64.so`或`libgameaarch64.so`(arm64设备)或命名为`gamearm.so`或`libgamearm.so`(arm32设备), 然后放入`Game working directory`/`fs_game`文件夹下, 将优先加载该mod文件夹下的游戏动态库).
* 支持jpg/png图像纹理文件.

----------------------------------------------------------------------------------

> 1.1.0harmattan31 (2023-06-10)

* 在`CONTROLS`选项卡下的`Reset on-screen controls`, 新增重置所有虚拟按键的缩放和透明度.
* 在`CONTROLS`选项卡新增统一设置所有虚拟按键大小功能.
* 在`CONTROLS`选项卡下的`Configure on-screen controls`, 新增网格辅助, 设置中`On-screen buttons position unit`大于0启用.
* 支持不固定的虚拟摇杆和内部响应死区.
* 支持自定义虚拟按键的图片. 如果`/sdcard/Android/data/com.karin.idTech4Amm/files/assets`路径下存在同名的虚拟按键图片, 将有限使用外部的代替自带的. 或者把虚拟按键图片放入一个命名好的文件夹中, 然后将文件夹放在`/sdcard/Android/data/com.karin.idTech4Amm/files/assets/controls_theme/`, 然后在`CONTROLS`选项卡下`Setup on-screen button theme`选择该文件夹名称.
* 新的鼠标支持实现.

----------------------------------------------------------------------------------

> 1.1.0harmattan30 (2023-05-23)

* 新增功能键工具条当输入法打开时(默认禁用, 在`Settings`菜单中).
* 新增摇杆释放范围设置, 在`CONTROLS`选项卡下. 值为摇杆半径的倍数, 设为0禁用.
* 修复雷神之锤4第一关动画播放完后游戏崩溃.
* 修复雷神之锤4删除存档菜单功能.

----------------------------------------------------------------------------------

> 1.1.0harmattan29 (2023-05-01)

* 修复游戏关卡载入时, 游戏切换到后台时崩溃.
* 修复雷神之锤4特效噪音和其他特效显示.
* 优化雷神之锤4天空盒渲染.
* 移除雷神之锤4cvar `harm_g_flashlightOn`.
* 修复部分设备上虚拟按键层与游戏层重叠渲染.

----------------------------------------------------------------------------------

> 1.1.0harmattan28 (2023-04-13)

* 雷神之锤4新增布尔型cvar `harm_g_mutePlayerFootStep`去控制是否静音玩家脚步声(默认静音).
* 雷神之锤4修复亮度依赖声音幅度的光源. 例如在大多数关卡(比如`airdefense2`), 在一些黑暗的通道应该有重复闪烁的光源.
* 移除启动雷神之锤4时的帮助对话框, 如果需要解压游戏资源, 在菜单`Other` -> `Extract resource`.
* (Bug)雷神之锤4中, 如果开启特效, 载入关卡后有杂音, 在控制台输入`bse_enabled`为0, 然后再输入`bse_enabled`变回1, 杂音将会消失.

----------------------------------------------------------------------------------

> 1.1.0harmattan27 (2023-04-05)

* 雷神之锤4修复一些线条特效. 比如怪物消失的特效, 线网.
* 雷神之锤4修复玩家HUD右上角的无线电图标.
* 雷神之锤4修复对话框交互. 比如创建多人游戏时的确认对话框.
* 雷神之锤4修复多人游戏的载入GUI和多人游戏主菜单.
* 雷神之锤4新增cvar `harm_si_autoFillBots` 多人游戏地图载入后自动添加bot(0为禁用). `fillbots` 命令支持追加bot数量参数.
* 雷神之锤4新增多人团队游戏模式下自动设置bot的阵营, bot模型随机, 修复了一些bot的bug.
* 雷神之锤4在`Quake 4 helper dialog`新增`SABot`的多人游戏地图的aas文件.

----------------------------------------------------------------------------------

> 1.1.0harmattan26 (2023-03-25)

* 使用SurfaceView代替GLSurfaceView创建OpenGL渲染环境(测试).
* 雷神之锤4中, 使用毁灭战士3的粒子特效系统来实现BSE粒子特效系统. 目前效果略差, 可以设置`bse_enabled`为0禁用粒子特效.
* 雷神之锤4移除cvar `harm_g_alwaysRun`, 使用原来的`in_alwaysRun`, 设为1自动跑.
* 掠食(2006)新增简单的线条模型渲染.
* 掠食(2006)优化天空盒渲染 by [lvonasek/PreyVR](https://github.com/lvonasek/PreyVR).

----------------------------------------------------------------------------------

> 1.1.0harmattan25 (2023-02-22)

* OpenSLES音频播放(测试).
* 新增启动器偏好设置备份和恢复.
* 掠食(2006)新增主菜单背景音乐播放.
* 掠食(2006)新增关卡载入时的背景音乐播放.
* 掠食(2006)支持灵魂行走状态时的物体可见/隐藏, 例如幽灵桥.
* 掠食(2006)使用视图距离控制是否渲染传送门.

----------------------------------------------------------------------------------

> 1.1.0harmattan23 (2023-02-16)

* 多线程支持(测试, 当前不支持cvar `r_multithread`, 不支持在游戏中切换多线程和单线程!!! 只可以在启动器中设置!!!)使用[d3es-multithread](https://github.com/emileb/d3es-multithread).
* 掠食(2006)修复开始新游戏加载第一个关卡后intro部分的声音播放 by [lvonasek/PreyVR](https://github.com/lvonasek/PreyVR).
* 掠食(2006)修复通道和天空盒渲染.
* 掠食(2006)修复关卡`game/spindlea`幽灵行走模式无法穿过玩家初始位置前的空气墙.
* 掠食(2006)修复幽灵行走模式下无法看到Tommy的躯壳本体.
* 游戏加载时不渲染虚拟按键.

----------------------------------------------------------------------------------

> 1.1.0harmattan22 (2023-01-10)

* 支持刘海屏/打孔屏的全屏.
* 掠食(2006)增加天空盒渲染(版本23修复).
* 掠食(2006)增加通道渲染(版本23修复).
* 掠食(2006)支持`deathwalk`(玩家死亡后行走空间)地图追加载, 但是现在有bug, 不要在玩家在`deathwalk`状态时保存游戏.
* 如果开始新游戏和载入第一关地图后没有声音, 可以尝试按`ESC`键返回主菜单, 然后再按次`ESC`键返回游戏, 就会有声音(版本23修复).

----------------------------------------------------------------------------------

> 1.1.0harmattan21 (2022-12-10)

* 掠食(2006) for 毁灭战士3引擎支持, 游戏数据包文件夹命名为`preybase`. 所有关卡都可以通过, 但是存在一些bug.
* 新增编辑虚拟按键时位置移动单位.
* 安卓Target SDK 级别回退到28(Android 9), 为了避免安卓10以上的`沙盒存储`.

----------------------------------------------------------------------------------

> 1.1.0harmattan20 (2022-11-18)

* 雷神之锤4中新增默认字体配置, 使用cvar `harm_gui_defaultFont`设置, 默认为`chain`.
* 雷神之锤4中实现了显示/隐藏模型层, 修复了物体渲染错误. 例如: AI的枪支外观模型, 玩家视角的枪支, 和boss关卡中的Makron.

----------------------------------------------------------------------------------

> 1.1.0harmattan19 (2022-11-16)

* 雷神之锤4中修复了关卡`game/tram1`的中部桥的门的开关UI不能交互.
* 雷神之锤4中修复了关卡`game/process2`的有怪的电梯1的启动开关UI不能交互.

----------------------------------------------------------------------------------

> 1.1.0harmattan18 (2022-11-11)

* 实现了一些用于调试的渲染函数.
* 雷神之锤4中新增玩家视角的可交互的GUI的高亮括号.
* 雷神之锤4中加载多人游戏时, 当启用cvar `harm_g_autoGenAASFileInMPGame`为1自动生成用于bot的aas文件时, 不再需要启用作弊cvar net_allowCheats.
* 雷神之锤4中修复了重新开始菜单的按键功能.
* 雷神之锤4中修复了一个引起崩溃的内存错误.

----------------------------------------------------------------------------------

> 1.1.0harmattan17 (2022-10-29)

* 支持雷神之锤4格式的字体. 支持其他语言包, D3格式字体不再需要解压.
* 雷神之锤4游戏中一些GUI无法交互的解决方案, 可以先`quicksave`, 再`quickload`, GUI将可以交互. 比如, 1. `game/tram1`关卡桥头的门的开关GUI, 2. `game/process2`有怪下来的电梯的开关GUI(版本19修复).

----------------------------------------------------------------------------------

> 1.1.0harmattan16 (2022-10-22)

* 新增启动时自动加载`快速存档`.
* 新增设置控制雷神之锤4启动帮助对话框是否显示, `其他`菜单新增`解压雷神之锤4资源`.
* 新增一次性设置全部虚拟按键的透明度.
* 新增与GitHub检查更新.
* 修复雷神之锤4bug:
> 1. 修复碰撞, 例如触发器, 载具, AI, 电梯, 血站. 因此也修复了关卡`game/mcc_landing`的最后一个电梯卡住, 修复了`game/process1 first`第一个电梯和`game/process1 second`最后一个电梯运行时卡死玩家, 修复了`game/convoy1`下载具后卡住. 移除之前的临时解决开门的cvar `harm_g_useSimpleTriggerClip`.
> 2. 修复载入关卡`game/mcc_1`和`game/tram1b`引起的崩溃. 现在所有关卡不会有崩溃发生.
	
----------------------------------------------------------------------------------

> 1.1.0harmattan15 (2022-10-15)

* 新增陀螺仪支持.
* 重置按键新增按全屏分辨率.
* 如果在32位设备上运行雷神之锤4崩溃, 尝试勾选`Use ETC1 compression`或`Disable lighting`以减少内存使用.
* 修复一些雷神之锤4Bug:
> 1. 修复主菜单的开始游戏, 现在可以再主菜单开始新游戏.
> 2. 修复关卡`game/waste`僵尸材质加载错误.
> 3. 修复关卡`game/building_b`的AI `Singer`在打开门后不会移动.
> 4. 修复关卡`game/putra`的破损地板可以跳下.
> 5. 修复`设置`界面多人游戏玩家模型选择预览.
> 6. 新增布尔型cvar `harm_g_flashlightOn`, 可以控制枪灯的初始状态是否打开, 默认是1(打开).
> 7. 新增布尔型cvar `harm_g_vehicleWalkerMoveNormalize`, 重新规范`机器人载具`的移动方向, 如果启动器启用了`Smooth joystick`会有效果, 默认是1(启用), 这可以修复机器人载具左右移动的问题.

----------------------------------------------------------------------------------

> 1.1.0harmattan13 (2022-10-23)

* 修复`雷神之锤4`Strogg血站GUI交互.
* 修复`雷神之锤4`跳过影片过场动画.
* `雷神之锤4`中如果`harm_g_alwaysRun`为1(启用自动跑), 按住`Walk`键行走(版本26移除, 使用原来的`in_alwaysRun`代替).
* 修复`雷神之锤4`关卡地图脚本的致命错误和bug(所有关卡地图不再有严重错误, 但是一些bug依然存在.).
> 1. `game/mcc_landing`: 最后一个电梯到顶时, 玩家依然会卡住. 可以在电梯快到顶时提前跳跃, 或者使用`noclip`(版本18修复).
> 2. `game/mcc_1`: 上个关卡通关时, 载入该关卡程序会崩溃. 使用`map game/mcc_1`重新加载(版本16修复).
> 3. `game/convoy1`: State error不再中止地图脚本. 下载具时有时会卡主, 使用`noclip`(版本18修复).
> 4. `game/putra`: 修复脚本致命错误. 但是不能跳下最后的破损的地板, 使用`noclip`(版本15修复).
> 5. `game/waste`: 修复脚本致命错误.
> 6. `game/process1 first`: 最后的电梯有错误的碰撞, 会杀死玩家. 使用`god`(版本18修复). 如果最后的塔电梯GUI不工作, 只能使用`teleport tgr_endlevel`直接通关.
> 7. `game/process1 second`: 第二个电梯有错误的碰撞, 会杀死玩家(和`game/process1 first`一样). 使用`god`(版本18修复).
> 8. `game/tram_1b`: 上个关卡通关时, 载入该关卡程序会崩溃. 使用`map game/tram_1b`重新加载(版本16修复).
> 9. `game/core1`: 修复开始的电梯平台不能自动上升.
> 10. `game/core2`: 修复物体旋转错误.

----------------------------------------------------------------------------------

> 1.1.0harmattan12 (2022-07-19)
 
 * 雷神之锤4 for 毁灭战士3引擎支持. 详情`https://github.com/jmarshall23/Quake4Doom`. 目前可以运行大部分关卡, 剩余部分关卡存在错误.
 * 雷神之锤4游戏数据文件目录为`q4base`, 游戏详情`https://store.steampowered.com/app/2210/Quake_4/`.
 * 修复`Rivensin`和`Hardcorps`mod载入存档bug.
 * 控制台命令记录.
 * 虚拟按键的分辨率不再依赖游戏分辨率.
 * 音量键映射设置(启用`Map volume keys`时显示).

----------------------------------------------------------------------------------

> 1.1.0harmattan11 (2022-06-30)

 * 编译`Hardcorps` mod游戏库支持, 游戏包路径`hardcorps`, 建议在控制选项卡`Controls`关闭平滑摇杆`Smooth joystick`, 更多mod信息`https://www.moddb.com/mods/hardcorps`.
 * `Rivensin` mod新增布尔类型Cvar `harm_pm_doubleJump` 启用二段跳(引用自 `hardcorps` mod, 默认关闭).
 * `Rivensin` mod新增布尔类型Cvar `harm_pm_autoForceThirdPerson` 自动设置 `pm_thirdPerson` 为 1, 当加载原DOOM3游戏地图切换关卡后自动切换为第三人称(默认禁用).
 * `Rivensin` mod新增浮点数类型Cvar `harm_pm_preferCrouchViewHeight` 调整避免角色下蹲通过管道时, 第三人称视角相机在管道外.(0为禁用, 也可以设置 `pm_crouchviewheight` 到一个小的值来解决).
 * 新增虚拟按键配置设置, 重置部分按键键值为DOOM3默认键值.
 * 在`Other`菜单新增`Special Cvar list`列出所有新增加的特殊 `Cvar`.

----------------------------------------------------------------------------------

> 2022-06-23 Update 1.1.0harmattan10

* 编译`Rivensin`Mod游戏库支持, 游戏包路径为`rivensin`, Mod信息`https://www.moddb.com/mods/ruiner`.
* 此`Rivensin`库支持加载DOOM3源基础游戏地图关卡. 但是需要添加原基础游戏的地图脚本到`Rivensin`Mod的资源文件中的`doom_main.script`.
* 新增武器切换面板按键配置.
* 新增武器切换面板按键配置.
* 修复Android 10文件访问授权(由于没有设备, Android 10/11+仅模拟器测试, 实机请自测).

----------------------------------------------------------------------------------

> 2022-06-15 Update 1.1.0harmattan9

* 修复Android 11+文件访问授权.
* 添加Android 4.x的apk包签名.

----------------------------------------------------------------------------------

> 2022-05-19 Update 1.1.0harmattan8

* 编译arm 64位库支持, armv7 32位默认启用NEON, 不再编译旧的armv5版本和armv7 VFP库支持.
* 修复输入事件拉取当游戏中模态消息框打开时.
* 新增cURL支持, 用于多人游戏的资源下载.
* 新增武器切换圆盘虚拟键.

----------------------------------------------------------------------------------

> 2022-05-05 1.1.0harmattan7

* 修复阴影被裁剪.
* 修复天空盒.
* 修复雾.
* 修复镜面反射.
* 加入纹理坐标生成着色器, 针对`邪恶复苏`地狱关卡的出生点的天空.
* 修复透明物体渲染, 比如窗玻璃, `经典DOOM`的透明粉红魔.
* 修复动态纹理, 比如旋转风扇的影子.
* 修复`狂暴化`, `重力枪`, `地狱之心`的视觉效果(需要先设置之前的 cvar `harm_g_skipBerserkVision`, `harm_g_skipWarpVision` 和 `harm_g_skipHelltimeVision` 为0).
* 修复截屏图像, 例如快速存档或任务提示的图片.
* 修复机枪的弹药量GUI.
* 新增光照模型切换cvar `harm_r_lightModel`, `Phong` 和 `Blinn-Phong`.
* 新增镜面指数cvar `harm_r_specularExponent`.
* 着色器现在内置源码中.
* 修改虚拟按键布局, 新增控制台按键.
* 新增返回键功能设置, 3次点击退出.
* 新增 cvar `harm_r_shadowCarmackInverse` 切换常用的或`卡马克反转`Z-Fail模板缓冲区阴影.
* DIII4A使用Android Studio打包, 代替Eclipse.

----------------------------------------------------------------------------------

> 2020-08-25 1.1.0harmattan6

* 修复游戏视频播放噪点(1.1.0harmattan6).
* 加载自定义mod时选择游戏动态库(1.1.0harmattan6).
* 修复音频播放(待测试)(1.1.0harmattan5).
* 新增启动器方向是否跟踪系统(1.1.0harmattan5).

----------------------------------------------------------------------------------

> 2020-08-17 1.1.0harmattan3

* 默认取消选中前4个选择框(默认禁用).
* 当进入程序界面时默认不打开软键盘.
* 当`开始游戏`或`编辑配置文件`时检查`外部存储`权限.
* 新增游戏文件目录选择器.
* 新增`保存配置`菜单, 当仅想修改配置而不想运行游戏时.
* 按键编辑界面可以隐藏导航栏, 当选中了`隐藏导航栏`时(需要先保存配置使之生效).
* 新增`帮助`菜单, 里面有些问题解决方案.

----------------------------------------------------------------------------------

> 2020-08-16 1.1.0harmattan2

* 如果你已经安装了其他作者的apk包, 并且包名为`com.n0n3m4.diii4a`, 你需要先卸载原来的版本, 然后才能安装这个新版本. 如果你出现安装失败的情况, 可以按此操作尝试安装. 因为我在原作者的基础上修改的, 没有重新更换包名, 但是apk证书又不一致.
* 如果运行时点击开始出现白屏, 首先检查`存储空间`权限是否已经打开, 然后取消勾选`Use ETC1(or RGBA4444) cache`运行, 或者手动删除ETC1纹理缓存(缓存文件目录在/sdcard/diii4a/<base/d3xp/d3le/cdoom/取决于运行的游戏...>/dds).
* `Clear vertex buffer`选项建议选择第3个, 或者第2个亦可, 渲染每帧清理顶点缓冲区! 如果选择第1个, 则和原始的版本行为相同, 玩一会可能会爆显存闪屏崩溃! 对应的游戏控制台变量为`harm_r_clearVertexBuffer`, 可以查看该变量说明.
* 经典DOOM(Classic DOOM)中有些触发器无法交互, 像第一关`E1M1`最后的通关大门不能打开. 但是可以通关在控制台输入`noclip`或绑定`noclip`到快捷键, 穿过不能触发的门.

----------------------------------------------------------------------------------

> 2020-08-16 1.1.0harmattan1

* 需要Android 4.0(Ice Cream)以上版本.
* 编译邪恶复苏游戏库(DOOM3 RoE), 默认跳过地狱之心buf(Helltime)视觉特效/重力枪(Grabber)视觉特效.
* 编译经典DOOM游戏库(Classic DOOM).
* 编译DOOM3-BFG:The Lost Mission资料片游戏库, 修复载入le_hell关卡地图时models/mapobjects/hell/hellintro.lwo模型的栈溢出.
* 跳过狂暴(Berserk)视觉特效.
* 新增选择OpenGL配置, RGBA8888 32位, RGBA4444 16位.
* 新增选择后台行为, 当前Activity不活动时不调用GLSurfaceView.onPaused/onResume, 不会出现之前的游戏切换到后台, 再回来时黑屏. 也可选择后台继续播放声音.
* 新增左上角渲染当前内存使用情况, Native堆内存/图形显存使用量. 不需要则不要开启, 操作耗时.
* 新增额外4个虚拟按键. 原来键盘键, 1, 2, 3键默认固定到布局.
* 新增选择OpenGL顶点缓冲区清理行为, 以防止打开光影后, 显存使用量一直增加而最终导致显存OOM, 以致程序闪屏崩溃. 需要使用__HARMATTAN__/DIII4A/libs编译的库.
* DOOM3新增harm_r_clearVertexBuffer参数(源码 renderer/VertexCache.cpp)来控制OpenGL顶点缓冲区的清理行为, 0 不清理(原来的行为), 1 每一帧绘制完释放顶点缓冲区内存(默认), 2 每一帧绘制完或关闭渲染系统时释放顶点缓冲区内存. 可以防止打开光影后, 显存使用量一直增加而最终导致显存OOM, 无法申请到显存而导致的程序闪屏崩溃.
* 支持隐藏底部导航栏.
	
----------------------------------------------------------------------------------
