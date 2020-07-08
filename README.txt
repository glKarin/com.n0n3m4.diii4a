DIII4A, com.n0n3m4.diii4a, DOOM III for Android, 毁灭战士3安卓移植版
1.1.0harmattan2
2020-07-08

----------------------------------------------------------------------------------

Updates:
	* Compile `DOOM3:RoE` game library named `libd3xp`, game path name is `d3xp`, more view in `https://store.steampowered.com/app/9070/DOOM_3_Resurrection_of_Evil/`.
	* Compile `Classic DOOM3` game library named `libcdoom`, game path name is `cdoom`, more view in `https://www.moddb.com/mods/classic-doom-3`.
	* Compile `DOOM3-BFG:The lost mission` game library named `libd3le`, game path name is `d3le`, need `d3xp` resources(+set fs_game_base d3xp), more view in `https://www.moddb.com/mods/the-lost-mission`(now fix stack overflow when load model `models/mapobjects/hell/hellintro.lwo` of level `game/le_hell` map on Android).
	* Clear vertex buffer for graphics memory overflow(integer cvar `harm_r_clearVertexBuffer`).
	* Skip visual vision for `Berserk Powerup` on `DOOM3`(bool cvar `harm_g_skipBerserkVision`).
	* Skip visual vision for `Grabber` on `D3 RoE`(bool cvar `harm_g_skipWarpVision`).
	* Skip visual vision for `Helltime Powerup` on `D3 RoE`(bool cvar `harm_g_skipHelltimeVision`).
	* Add support to run on background.
	* Add support to hide navigation bar.
	* Add RGBA4444 16-bits color.
	* Add config file editor.
	
About:
	* All changes in folder `__HARAMTTAN__` on github, `/doom3/neo/cdoom` is Classic DOOM3 game library source, `/doom3/neo/d3le` is DOOM3:The Lost Mission game library source, 
	* Source in `assets/source` folder in APK file.
	
----------------------------------------------------------------------------------

目录:
	assets: 图标资源+OpenGL着色器源码.
	DIII4A: DOOM3前端启动器源码.
	Q3E: DOOM3前端启动器依赖, 包括虚拟按键层, Native渲染层.
	doom3: DOOM3源码, 没有直接创建Native Activity. 而是渲染到Q3E的GLSurfaceView.
	__HARAMTTAN__: 经过修改的源码
	
----------------------------------------------------------------------------------

	__HARAMTTAN__/doom3: 修改过的DOOM3源码, 可clang++编译. 默认编译arm 32位库.
	__HARAMTTAN__/DIII4A: 修改过的DOOM3前端启动器源码.
	__HARAMTTAN__/DIII4A/libs: 游戏动态库.
	__HARAMTTAN__/Q3E: 修改过的DOOM3前端启动器依赖的源码.
	__HARAMTTAN__/doom3/neo/cdoom: 经典DOOM(Classic DOOM)源码(https://www.moddb.com/mods/classic-doom-3).
	__HARAMTTAN__/doom3/neo/d3le: DOOM3-BFG:Lost Mession资料片源码(https://www.moddb.com/mods/the-lost-mission).
	
----------------------------------------------------------------------------------

更新:
	* 需要Android 4.0(Ice Cream)以上版本.
	* 编译邪恶复苏游戏库(DOOM3 RoE), 默认跳过地狱之心buf(Helltime)视觉特效/重力枪(Grabber)视觉特效.
	* 编译经典DOOM游戏库(Classic DOOM).
	* 编译DOOM3-BFG:The Lost Mession资料片游戏库, 修复载入le_hell关卡地图时models/mapobjects/hell/hellintro.lwo模型的栈溢出.
	* 跳过狂暴(Berserk)视觉特效.
	* 新增选择OpenGL配置, RGBA8888 32位, RGBA4444 16位.
	* 新增选择后台行为, 当前Activity不活动时不调用GLSurfaceView.onPaused/onResume, 不会出现之前的游戏切换到后台, 再回来时黑屏. 也可选择后台继续播放声音.
	* 新增左上角渲染当前内存使用情况, Native堆内存/图形显存使用量. 不需要则不要开启, 操作耗时.
	* 新增额外4个虚拟按键. 原来键盘键, 1, 2, 3键默认固定到布局.
	* 新增选择OpenGL顶点缓冲区清理行为, 以防止打开光影后, 显存使用量一直增加而最终导致显存OOM, 以致程序闪屏崩溃. 需要使用__HARMATTAN__/DIII4A/libs编译的库.
	* DOOM3新增harm_r_clearVertexBuffer参数(源码 renderer/VertexCache.cpp)来控制OpenGL顶点缓冲区的清理行为, 0 不清理(原来的行为), 1 每一帧绘制完释放顶点缓冲区内存(默认), 2 每一帧绘制完或关闭渲染系统时释放顶点缓冲区内存. 可以防止打开光影后, 显存使用量一直增加而最终导致显存OOM, 无法申请到显存而导致的程序闪屏崩溃.
	* 支持隐藏底部导航栏.
	
----------------------------------------------------------------------------------
