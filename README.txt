idTech4A++ (Harmattan Edition)
DIII4A++, com.n0n3m4.diii4a, DOOM III/Quake IV for Android, 毁灭战士3/雷神之锤4安卓移植版

Latest version: 1.1.0harmattan15(Natasha)
Last update release: 2022-10-15
Arch: arm64 armv7-a
Platform: Android 4.0+
Update

    1.1.0harmattan15 (2022-10-15)

    Add gyroscope control support.

    Add reset onscreen buttton layout with fullscreen.

    If running Quake 4 crash on arm32 device, trying to check Use ETC1 compression for decreasing memory usage.

    Fixup some Quake 4 bugs:

            Fixup start new game in main menu, now start new game is work.
            Fixup loading zombie material in level game/waste.
            Fixup AI Singer can not move when opening the door in level game/building_b.
            Fixup jump down on broken floor in level game/putra.
            Fixup player model choice and view in Settings menu in Multiplayer game.
            Add bool cvar harm_g_flashlightOn for controling gun-lighting is open/close initial, default is 1(open).
            Add bool cvar harm_g_vehicleWalkerMoveNormalize for re-normalize vehicle walker movment if enable Smooth joystick in launcher, default is 1(re-normalize), it can fix up move left-right.

    新增陀螺仪支持.

    重置按键新增按全屏分辨率.

    如果在32位设备上运行雷神之锤4崩溃, 尝试勾选Use ETC1 compression以减少内存使用.

    修复一些雷神之锤4Bug:

            修复主菜单的开始游戏, 现在可以再主菜单开始新游戏.
            修复关卡game/waste僵尸材质加载错误.
            修复关卡game/building_b的AI Singer在打开门后不会移动.
            修复关卡game/putra的破损地板可以跳下.
            修复设置界面多人游戏玩家模型选择预览.
            新增布尔型cvar harm_g_flashlightOn, 可以控制枪灯的初始状态是否打开, 默认是1(打开).
            新增布尔型cvar harm_g_vehicleWalkerMoveNormalize, 重新规范机器人载具的移动方向, 如果启动器启用了Smooth joystick会有效果, 默认是1(启用), 这可以修复机器人载具左右移动的问题.

    1.1.0harmattan13 (2022-10-23)

    Fixup Strogg health station GUI interactive in Quake 4.

    Fixup skip cinematic in Quake 4.

    If harm_g_alwaysRun is 1, hold Walk key to walk in Quake 4.

    Fixup level map script fatal error or bug in Quake 4(All maps have not fatal errors no longer, but have some bugs yet.).

            game/mcc_landing: Player collision error on last elevator. You can jump before elevator ending or using noclip.
            game/mcc_1: Loading crash after last level ending. Using map game/mcc_1 to reload.
            game/convoy1: State error is not care no longer and ignore. But sometimes has player collision error when jumping form vehicle, using noclip.
            game/putra: Script fatal error has fixed. But can not down on broken floor, using noclip(Fixed in version 15).
            game/waste: Script fatal error has fixed.
            game/process1 first: Last elevator has ins collision cause killing player. Using god. If tower's elevator GUI not work, using teleport tgr_endlevel to next level directly.
            game/process1 second: Second elevator has incorrect collision cause killing player(same as game/process1 first level). Using god.
            game/core1: Fixup first elevator platform not go up.
            game/core2: Fixup entity rotation.

    修复雷神之锤4Strogg血站GUI交互.

    修复雷神之锤4跳过影片过场动画.

    雷神之锤4中如果harm_g_alwaysRun为1(启用自动跑), 按住Walk键行走.

    修复雷神之锤4关卡地图脚本的致命错误和bug(所有关卡地图不再有严重错误, 但是一些bug依然存在.).

            game/mcc_landing: 最后一个电梯到顶时, 玩家依然会卡住. 可以在电梯快到顶时提前跳跃, 或者使用noclip.
            game/mcc_1: 上个关卡通关时, 载入该关卡程序会崩溃. 使用map game/mcc_1重新加载.
            game/convoy1: State error不再中止地图脚本. 下载具时有时会卡主, 使用noclip.
            game/putra: 修复脚本致命错误. 但是不能跳下最后的破损的地板, 使用noclip(版本15已修复).
            game/waste: 修复脚本致命错误.
            game/process1 first: 最后的电梯有错误的碰撞, 会杀死玩家. 使用god. 如果最后的塔电梯GUI不工作, 只能使用teleport tgr_endlevel直接通关.
            game/process1 second: 第二个电梯有错误的碰撞, 会杀死玩家(和game/process1 first一样). 使用god.
            game/core1: 修复开始的电梯平台不能自动上升.
            game/core2: 修复物体旋转错误.

About Quake IV
For playing Quake 4(jmarshall 's Quake4Doom). Now can play most levels, but some levels has error.

        Putting PC Quake 4 game data file to q4base folder.
        Click START to open Quake 4 map level dialog in game launcher.
        Suggest to extract Quake 4 patch resource to q4base game data folder first.

        Quake 4 fonts with DOOM3 format by IlDucci.
        Quake 3 bot files(If you want to add bots in Multiplayer-Game, using command addbot <bot_file> or fillbots after enter map in console).

        Then Choose map level/Go main-menu/Start directly, most levels is working.
        Player is always run(can using bool cvar harm_g_alwaysRun to control), and gun-lighting default is opened.

Problems and resolutions

        Door-opening: Now using set harm_g_useSimpleTriggerClip to 1 for force to using simple collision clip to resolve(default), it work all doors well.
        Main-menu: Now can show full main menu, but without background color. But can not New-Game(need using map or devmap for starting a map game in Singleplayer-Game), Create-Server(using si_map and serverMapRestart or nextMap for starting a MP map game in Multiplayer-Game), and can not interactive in some dialog.
        Sound: It looks work well now(jmarshall's icedTech using DOOM3-BFG sound system).
        Loading-UI: It looks work well now.
        Multiplayer-Game: Now is working well with bots(jmarshall added Q3-bot engine, but need bots decl file and Multiplayer-Game map AAS file, now set cvar harm_g_autoGenAASFileInMPGame to 1 for generating a bad AAS file when loading map in Multiplayer-Game and not valid AAS file in current map, you can also put your MP map's AAS file to maps/mp folder).
        Script error: Some maps have any script errors, and some errors is fatal.
        Particle system: Now is work incompleted(Quake4 using new advanced BSE particle system, jmarshall has realized and added by decompiling ETQW's BSE binary file, also see jmarshall23/Quake4BSE).
        Entity render: Some game entities render incorrect.
        Collision: Some incorrect collision(using noclip to pass).

关于雷神之锤4
运行雷神之锤4(jmarshall 's Quake4Doom). 目前可以运行大部分关卡, 剩余部分关卡存在错误..

        将PC端雷神之锤4游戏文件放到q4base文件夹.
        在启动器中点击START打开关卡地图选择器.
        建议先解压雷神之锤4补丁资源到q4base资源目录.

        毁灭战士3格式的雷神之锤4字体, IlDucci提供.
        雷神之锤3bot文件(在多人游戏中, 进入游戏后在控制台使用命令addbot <bot_file>或fillbots添加bot).

        然后选择关卡或进入主菜单启动, 运行大部分可运行的关卡.
        Player is always run(can using bool cvar harm_g_alwaysRun to control), and gun-lighting default is opened.

问题和解决方案:

        门打不开: 目前通过设置harm_g_useSimpleTriggerClip为1, 强制使用简单碰撞检测解决, 基本没问题, 没有根本解决.
        主菜单: 目前可以正常显示, 去掉背景色. 开始新游戏不工作(可以通过map或devmap命令开始新单人游戏), 创建多人服务器不工作(可以通过si_map, serverMapRestart或nextMap命令开始多人游戏), 部分对话框无法交互.
        声音: 正常工作.
        游戏载入界面: 正常工作.
        多人游戏: 目前正常工作, 并且可以添加bot(jmarshall添加了雷神之锤3的bot支持, 但是需要先添加bot文件和多人游戏地图的AAS文件, 目前可以设置harm_g_autoGenAASFileInMPGame为1自动在多人游戏地图载入(如果没有一个有效的该地图的AAS文件)后生成一个不怎么好的AAS文件, 也可以把你自己用其他方式生成的AAS文件放到游戏数据目录的maps/mp文件夹).
        脚本错误: 部分脚本错误无关紧要, 但是有一些影响游戏进程或者崩溃.
        粒子系统: 目前工作的不完整(雷神之锤4使用了新的更高级的粒子系统BSE, jmarshall通过反编译深入敌后: 雷神战争的BSE二进制实现了, 更多详情 jmarshall23/Quake4BSE).
        物体渲染: 存在一些物体错误的渲染结果.
        碰撞: 有一些错误的碰撞结果(可以使用noclip强制穿越).

Screenshot

    Game

Classic bathroom Classic bathroom in Rivensin mod Quake IV for DOOM3

    Mod

Resurrection of EvilThe lost mission Classic DOOMHardcorps RivensinQuake IV
Changes

    1.1.0harmattan12 (2022-07-19)

    Quake 4 in DOOM3 engine support. Also see https://github.com/jmarshall23/Quake4Doom. Now can play most levels, but some levels has error.

    Quake 4 game data folder named q4base, also see https://store.steampowered.com/app/2210/Quake_4/.

    Fix Rivensin and Hardcorps mod load game from save game.

    Add console command history record.

    On-screen buttons layer's resolution always same to device screen.

    Add volume key map config(Enable Map volume keys to show it).

    雷神之锤4 for 毁灭战士3引擎支持. 详情https://github.com/jmarshall23/Quake4Doom. 目前可以运行大部分关卡, 剩余部分关卡存在错误.

    雷神之锤4游戏数据文件目录为q4base, 游戏详情https://store.steampowered.com/app/2210/Quake_4/.

    修复Rivensin和Hardcorpsmod载入存档bug.

    控制台命令记录.

    虚拟按键的分辨率不再依赖游戏分辨率.

    音量键映射设置(启用Map volume keys时显示).

    1.1.0harmattan11 (2022-06-30)

    Add Hardcorps mod library support, game path name is hardcorps, if play the mod, first suggest to close Smooth joystick in Controls tab panel, more view in https://www.moddb.com/mods/hardcorps.

    In Rivensin mod, add bool Cvar harm_pm_doubleJump to enable double-jump(From hardcorps mod source code, default disabled).

    In Rivensin mod, add bool Cvar harm_pm_autoForceThirdPerson for auto set pm_thirdPerson to 1 after level load end when play original DOOM3 maps(Default disabled).

    In Rivensin mod, add float Cvar harm_pm_preferCrouchViewHeight for view poking out some tunnel's ceil when crouch(Default 0 means disabled, and also can set pm_crouchviewheight to a smaller value).

    Add on-screen button config page, and reset some on-screen button keymap to DOOM3 default key.

    Add menu Special Cvar list in Other menu for list all new special Cvar.

    编译Hardcorps mod游戏库支持, 游戏包路径hardcorps, 建议在控制选项卡Controls关闭平滑摇杆Smooth joystick, 更多mod信息https://www.moddb.com/mods/hardcorps.

    Rivensin mod新增布尔类型Cvar harm_pm_doubleJump 启用二段跳(引用自 hardcorps mod, 默认关闭).

    Rivensin mod新增布尔类型Cvar harm_pm_autoForceThirdPerson 自动设置 pm_thirdPerson 为 1, 当加载原DOOM3游戏地图切换关卡后自动切换为第三人称(默认禁用).

    Rivensin mod新增浮点数类型Cvar harm_pm_preferCrouchViewHeight 调整避免角色下蹲通过管道时, 第三人称视角相机在管道外.(0为禁用, 也可以设置 pm_crouchviewheight 到一个小的值来解决).

    新增虚拟按键配置设置, 重置部分按键键值为DOOM3默认键值.

    在Other菜单新增Special Cvar list列出所有新增加的特殊 Cvar.

    2022-06-23 Update 1.1.0harmattan10

    Add Rivensin mod library support, game path name is rivensin, more view in https://www.moddb.com/mods/ruiner.

    The Rivensin game library support load DOOM3 base game map. But first must add include original DOOM3 all map script into doom_main.script of Rivensin mod file.

    Add weapon panel keys configure.

    Fix file access permission grant on Android 10(Sorry for I have not Android 10/11+ device to testing).

    编译RivensinMod游戏库支持, 游戏包路径为rivensin, Mod信息https://www.moddb.com/mods/ruiner.

    此Rivensin库支持加载DOOM3源基础游戏地图关卡. 但是需要添加原基础游戏的地图脚本到RivensinMod的资源文件中的doom_main.script.

    新增武器切换面板按键配置.

    新增武器切换面板按键配置.

    修复Android 10文件访问授权(由于没有设备, Android 10/11+仅模拟器测试, 实机请自测).

    2022-06-15 Update 1.1.0harmattan9

    Fix file access permission grant on Android 11+.

    Add Android 4.x apk package v1 sign.

    修复Android 11+文件访问授权.

    添加Android 4.x的apk包签名.

    2022-05-19 Update 1.1.0harmattan8

DIII4A++_harmattan.1.1.0.8.apk: include armv8-64 and armv7 32 neon library. DIII4A++_harmattan.1.1.0.8_only_armv7a.apk: only include armv7 32 neon library.

    Compile armv8-a 64 bits library, and set FPU neon is default on armv7-a, and do not compile old armv5e library and armv7-a vfp.

    Fix input event when modal MessageBox is visible in game.

    Add cURL support for downloading in multiplayer game.

    Add weapon on-screen button panel.

    编译arm 64位库支持, armv7 32位默认启用NEON, 不再编译旧的armv5版本和armv7 VFP库支持.

    修复输入事件拉取当游戏中模态消息框打开时.

    新增cURL支持, 用于多人游戏的资源下载.

    新增武器切换圆盘虚拟键.

    2022-05-05 1.1.0harmattan7

Update:

    Fix shadow clipped.

    Fix sky box.

    Fix fog and blend light.

    Fix glass reflection.

    Add texgen shader for like D3XP hell level sky.

    Fix translucent object. i.e. window glass, transclucent Demon in Classic DOOM mod.

    Fix dynamic texture interaction. i.e. rotating fans.

    Fix Berserk, Grabber, Helltime vision effect(First set cvar harm_g_skipBerserkVision, harm_g_skipWarpVision and harm_g_skipHelltimeVision to 0).

    Fix screen capture image when quick save game or mission tips.

    Fix machine gun's ammo panel.

    Add light model setting with Phong and Blinn-Phong when render interaction shader pass(string cvar harm_r_lightModel).

    Add specular exponent setting in light model(float cvar harm_r_specularExponent).

    Default using program internal OpenGL shader.

    Reset extras virtual button size, and add Console(~) key.

    Add Back key function setting, add 3-Click to exit.

    Add cvar harm_r_shadowCarmackInverse to change general Z-Fail stencil shadow or Carmack-Inverse Z-Fail stencil shadow.

    DIII4A build on Android Studio now.

    修复阴影被裁剪.

    修复天空盒.

    修复雾.

    修复镜面反射.

    加入纹理坐标生成着色器, 针对邪恶复苏地狱关卡的出生点的天空.

    修复透明物体渲染, 比如窗玻璃, 经典DOOM的透明粉红魔.

    修复动态纹理, 比如旋转风扇的影子.

    修复狂暴化, 重力枪, 地狱之心的视觉效果(需要先设置之前的 cvar harm_g_skipBerserkVision, harm_g_skipWarpVision 和 harm_g_skipHelltimeVision 为0).

    修复截屏图像, 例如快速存档或任务提示的图片.

    修复机枪的弹药量GUI.

    新增光照模型切换cvar harm_r_lightModel, Phong 和 Blinn-Phong.

    新增镜面指数cvar harm_r_specularExponent.

    着色器现在内置源码中.

    修改虚拟按键布局, 新增控制台按键.

    新增返回键功能设置, 3次点击退出.

    新增 cvar harm_r_shadowCarmackInverse 切换常用的或卡马克反转Z-Fail模板缓冲区阴影.

    DIII4A使用Android Studio打包, 代替Eclipse.

    2020-08-25 1.1.0harmattan6

    Fix video playing - 1.1.0harmattan6.

    Choose game library when load other game mod, more view in Help menu - 1.1.0harmattan6.

    Fix game audio sound playing(Testing) - 1.1.0harmattan5.

    Add launcher orientation setting on CONTROLS tab - 1.1.0harmattan5.

    修复游戏视频播放噪点(1.1.0harmattan6).

    加载自定义mod时选择游戏动态库(1.1.0harmattan6).

    修复音频播放(待测试)(1.1.0harmattan5).

    新增启动器方向是否跟踪系统(1.1.0harmattan5).

    2020-08-17 1.1.0harmattan3

    Uncheck 4 checkboxs, default value is 0(disabled).

    Hide software keyboard when open launcher activity.

    Check WRITE_EXTERNAL_STORAGE permission when start game or edit config file.

    Add game data directory chooser.

    Add Save settings menu if you only change settings but don't want to start game.

    UI editor can hide navigation bar if checked Hide navigation bar(the setting must be saved before do it).

    Add Help menu.

    默认取消选中前4个选择框(默认禁用).

    当进入程序界面时默认不打开软键盘.

    当开始游戏或编辑配置文件时检查外部存储权限.

    新增游戏文件目录选择器.

    新增保存配置菜单, 当仅想修改配置而不想运行游戏时.

    按键编辑界面可以隐藏导航栏, 当选中了隐藏导航栏时(需要先保存配置使之生效).

    新增帮助菜单, 里面有些问题解决方案.

    2020-08-16 1.1.0harmattan2

Notification:

    If you have installed other version apk(package name is com.n0n3m4.diii4a) of other sources, you first to uninstall the old version apk package named com.n0n3m4.diii4a, after install this new version apk. Because the apk package is same com.n0n3m4.diii4a, but certificate is different.

    If app running crash(white screen), first make sure to allow WRITE_EXTERNAL_STORAGE permission, alter please uncheck 4th checkbox named Use ETC1(or RGBA4444) cache or clear ETC1 texture cache file manual on resource folder(exam. /sdcard/diii4a/<base/d3xp/d3le/cdoom/or...>/dds).

    Clear vertex buffer suggest to select 3rd or 2nd for clear vertex buffer every frame! If you select 1st, it will be same as original apk, maybe flash and crash with out of graphics memory! More view in game, on DOOM3 console, cvar named harm_r_clearVertexBuffer.

    TODO: Classic DOOM some trigger can not interact, exam last door of E1M1. I don't know what reason. But you can toggle noclip with console or shortcut key to through it.

    如果你已经安装了其他作者的apk包, 并且包名为com.n0n3m4.diii4a, 你需要先卸载原来的版本, 然后才能安装这个新版本. 如果你出现安装失败的情况, 可以按此操作尝试安装. 因为我在原作者的基础上修改的, 没有重新更换包名, 但是apk证书又不一致.

    如果运行时点击开始出现白屏, 首先检查存储空间权限是否已经打开, 然后取消勾选Use ETC1(or RGBA4444) cache运行, 或者手动删除ETC1纹理缓存(缓存文件目录在/sdcard/diii4a/<base/d3xp/d3le/cdoom/取决于运行的游戏...>/dds).

    Clear vertex buffer选项建议选择第3个, 或者第2个亦可, 渲染每帧清理顶点缓冲区! 如果选择第1个, 则和原始的版本行为相同, 玩一会可能会爆显存闪屏崩溃! 对应的游戏控制台变量为harm_r_clearVertexBuffer, 可以查看该变量说明.

    经典DOOM(Classic DOOM)中有些触发器无法交互, 像第一关E1M1最后的通关大门不能打开. 但是可以通关在控制台输入noclip或绑定noclip到快捷键, 穿过不能触发的门.

    2020-08-16 1.1.0harmattan1

    Compile DOOM3:RoE game library named libd3xp, game path name is d3xp, more view in https://store.steampowered.com/app/9070/DOOM_3_Resurrection_of_Evil/.
    Compile Classic DOOM3 game library named libcdoom, game path name is cdoom, more view in https://www.moddb.com/mods/classic-doom-3.
    Compile DOOM3-BFG:The lost mission game library named libd3le, game path name is d3le, need d3xp resources(+set fs_game_base d3xp), more view in https://www.moddb.com/mods/the-lost-mission(now fix stack overflow when load model models/mapobjects/hell/hellintro.lwo of level game/le_hell map on Android).
        Clear vertex buffer for graphics memory overflow(integer cvar harm_r_clearVertexBuffer).
        Skip visual vision for Berserk Powerup on DOOM3(bool cvar harm_g_skipBerserkVision).
        Skip visual vision for Grabber on D3 RoE(bool cvar harm_g_skipWarpVision).
        Skip visual vision for Helltime Powerup on D3 RoE(bool cvar harm_g_skipHelltimeVision).
    Add support to run on background.
    Add support to hide navigation bar.
    Add RGBA4444 16-bits color.
    Add config file editor.

About:

    All changes in folder __HARAMTTAN__ on github, /doom3/neo/cdoom is Classic DOOM3 game library source, /doom3/neo/d3le is DOOM3:The Lost Mission game library source,

    Source in assets/source folder in APK file.

    需要Android 4.0(Ice Cream)以上版本.

    编译邪恶复苏游戏库(DOOM3 RoE), 默认跳过地狱之心buf(Helltime)视觉特效/重力枪(Grabber)视觉特效.

    编译经典DOOM游戏库(Classic DOOM).

    编译DOOM3-BFG:The Lost Mission资料片游戏库, 修复载入le_hell关卡地图时models/mapobjects/hell/hellintro.lwo模型的栈溢出.

    跳过狂暴(Berserk)视觉特效.

    新增选择OpenGL配置, RGBA8888 32位, RGBA4444 16位.

    新增选择后台行为, 当前Activity不活动时不调用GLSurfaceView.onPaused/onResume, 不会出现之前的游戏切换到后台, 再回来时黑屏. 也可选择后台继续播放声音.

    新增左上角渲染当前内存使用情况, Native堆内存/图形显存使用量. 不需要则不要开启, 操作耗时.

    新增额外4个虚拟按键. 原来键盘键, 1, 2, 3键默认固定到布局.

    新增选择OpenGL顶点缓冲区清理行为, 以防止打开光影后, 显存使用量一直增加而最终导致显存OOM, 以致程序闪屏崩溃. 需要使用HARMATTAN/DIII4A/libs编译的库.

    DOOM3新增harm_r_clearVertexBuffer参数(源码 renderer/VertexCache.cpp)来控制OpenGL顶点缓冲区的清理行为, 0 不清理(原来的行为), 1 每一帧绘制完释放顶点缓冲区内存(默认), 2 每一帧绘制完或关闭渲染系统时释放顶点缓冲区内存. 可以防止打开光影后, 显存使用量一直增加而最终导致显存OOM, 无法申请到显存而导致的程序闪屏崩溃.

    支持隐藏底部导航栏.

Directory:

        /DIII4A: DOOM3 frontend source(DOOM3前端启动器源码)
        /doom3: DOOM3 source(DOOM3源码)
        /HARAMTTAN: Other resources(额外资源)

                /*.apk: Latest update version(最近更新版本)
                /build: old version APK packages(旧版本的apk包)
                /screenshot: screenshot pictures(截屏)
                /cdoom: Original Classic DOOM changed source(https://www.moddb.com/mods/classic-doom-3)
                /d3le: Original The Lost Mission changed source(https://www.moddb.com/mods/the-lost-mission)

Original old n0n3m4 version source in n0n3m4_original_old_version branch.
Extras download:

    Google: https://drive.google.com/drive/folders/1qgFWFGICKjcQ5KfhiNBHn_JYhJN5XoLb
    Baidu网盘: https://pan.baidu.com/s/1hXvKmrajAACfcCj9_ThZ_w 提取码: pyyj

