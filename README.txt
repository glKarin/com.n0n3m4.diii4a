DIII4A++ (Harmattan)
com.n0n3m4.diii4a, DOOM III for Android, 毁灭战士3安卓移植版
Latest version: 1.1.0harmattan9(Natasha)
Last update release: 2022-06-15
arm64 armv7-a
Android 4.0+

----------------------------------------------------------------------------------

2022-06-15 Update 1.1.0harmattan9

Update:
    * Fix file access permission grant on Android 10/11+.
    * Add Android 4.x apk package v1 sign.

更新:
    * 修复Android 10/11+文件访问授权.
    * 添加Android 4.x的apk包签名.

----------------------------------------------------------------------------------

2022-05-19 Update 1.1.0harmattan8

DIII4A++_harmattan.1.1.0.8.apk: include armv8-64 and armv7 32 neon library.
DIII4A++_harmattan.1.1.0.8_only_armv7a.apk: only include armv7 32 neon library.

Update:
    * Compile armv8-a 64 bits library, and set FPU neon is default on armv7-a, and do not compile old armv5e library and armv7-a vfp.
    * Fix input event when modal MessageBox is visible in game.
    * Add cURL support for downloading in multiplayer game.
    * Add weapon on-screen button panel.

更新:
    * 编译arm 64位库支持, armv7 32位默认启用NEON, 不再编译旧的armv5版本和armv7 VFP库支持.
    * 修复输入事件拉取当游戏中模态消息框打开时.
    * 新增cURL支持, 用于多人游戏的资源下载.
    * 新增武器切换圆盘虚拟键.

----------------------------------------------------------------------------------

Directory(目录):
	/DIII4A: DOOM3 frontend source(DOOM3前端启动器源码)
	/doom3: DOOM3 source(DOOM3源码)
	/__HARAMTTAN__: Other resources(额外资源)
	    /*.apk: Latest update version(最近更新版本)
	    /build: old version APK packages(旧版本的apk包)
	    /screenshot: screenshot pictures(截屏)
	    /cdoom: Original `Classic DOOM` changed source(https://www.moddb.com/mods/classic-doom-3)
	    /d3le: Original `The Lost Mission` changed source(https://www.moddb.com/mods/the-lost-mission)

Original old n0n3m4 version source in `n0n3m4_original_old_version` branch.

----------------------------------------------------------------------------------
Extras download:
    Google: https://drive.google.com/drive/folders/1qgFWFGICKjcQ5KfhiNBHn_JYhJN5XoLb
    Baidu: https://pan.baidu.com/s/1hXvKmrajAACfcCj9_ThZ_w 提取码: pyyj
----------------------------------------------------------------------------------
Changed history:
----------------------------------------------------------------------------------

2022-05-05 1.1.0harmattan7

Update:
    * Fix shadow clipped.
    * Fix sky box.
    * Fix fog and blend light.
    * Fix glass reflection.
    * Add texgen shader for like `D3XP` hell level sky.
    * Fix translucent object. i.e. window glass, transclucent Demon in `Classic DOOM` mod.
    * Fix dynamic texture interaction. i.e. rotating fans.
    * Fix `Berserk`, `Grabber`, `Helltime` vision effect(First set cvar `harm_g_skipBerserkVision`, `harm_g_skipWarpVision` and `harm_g_skipHelltimeVision` to 0).
    * Fix screen capture image when quick save game or mission tips.
    * Fix machine gun's ammo panel.
    * Add light model setting with `Phong` and `Blinn-Phong` when render interaction shader pass(string cvar `harm_r_lightModel`).
    * Add specular exponent setting in light model(float cvar `harm_r_specularExponent`).
    * Default using program internal OpenGL shader.
    * Reset extras virtual button size, and add Console(~) key.
    * Add `Back` key function setting, add 3-Click to exit.
    * Add cvar `harm_r_shadowCarmackInverse` to change general Z-Fail stencil shadow or `Carmack-Inverse` Z-Fail stencil shadow.
    * DIII4A build on Android Studio now.

更新:
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

2020-08-25 1.1.0harmattan6

Update:
	* Fix video playing - 1.1.0harmattan6.
	* Choose game library when load other game mod, more view in `Help` menu - 1.1.0harmattan6.
	* Fix game audio sound playing(Testing) - 1.1.0harmattan5.
	* Add launcher orientation setting on `CONTROLS` tab - 1.1.0harmattan5.
	
更新:
	* 修复游戏视频播放噪点(1.1.0harmattan6).
	* 加载自定义mod时选择游戏动态库(1.1.0harmattan6).
	* 修复音频播放(待测试)(1.1.0harmattan5).
	* 新增启动器方向是否跟踪系统(1.1.0harmattan5).

----------------------------------------------------------------------------------

2020-08-17 1.1.0harmattan3

Update:
	* Uncheck 4 checkboxs, default value is 0(disabled).
	* Hide software keyboard when open launcher activity.
	* Check `WRITE_EXTERNAL_STORAGE` permission when start game or edit config file.
	* Add game data directory chooser.
	* Add `Save settings` menu if you only change settings but don't want to start game.
	* UI editor can hide navigation bar if checked `Hide navigation bar`(the setting must be saved before do it).
	* Add `Help` menu.
	
更新:
	* 默认取消选中前4个选择框(默认禁用).
	* 当进入程序界面时默认不打开软键盘.
	* 当`开始游戏`或`编辑配置文件`时检查`外部存储`权限.
	* 新增游戏文件目录选择器.
	* 新增`保存配置`菜单, 当仅想修改配置而不想运行游戏时.
	* 按键编辑界面可以隐藏导航栏, 当选中了`隐藏导航栏`时(需要先保存配置使之生效).
	* 新增`帮助`菜单, 里面有些问题解决方案.

----------------------------------------------------------------------------------

2020-08-16 1.1.0harmattan2

Notification:
	* If you have installed other version apk(package name is `com.n0n3m4.diii4a`) of other sources, you first to uninstall the old version apk package named `com.n0n3m4.diii4a`, after install this new version apk. Because the apk package is same `com.n0n3m4.diii4a`, but certificate is different.
	* If app running crash(white screen), first make sure to allow `WRITE_EXTERNAL_STORAGE` permission, alter please uncheck 4th checkbox named `Use ETC1(or RGBA4444) cache` or clear ETC1 texture cache file manual on resource folder(exam. /sdcard/diii4a/<base/d3xp/d3le/cdoom/or...>/dds).
	* `Clear vertex buffer` suggest to select 3rd or 2nd for clear vertex buffer every frame! If you select 1st, it will be same as original apk, maybe flash and crash with out of graphics memory! More view in game, on DOOM3 console, cvar named `harm_r_clearVertexBuffer`.
	* TODO: `Classic DOOM` some trigger can not interact, exam last door of `E1M1`. I don't know what reason. But you can toggle `noclip` with console or shortcut key to through it.
	
告知:
	* 如果你已经安装了其他作者的apk包, 并且包名为`com.n0n3m4.diii4a`, 你需要先卸载原来的版本, 然后才能安装这个新版本. 如果你出现安装失败的情况, 可以按此操作尝试安装. 因为我在原作者的基础上修改的, 没有重新更换包名, 但是apk证书又不一致.
	* 如果运行时点击开始出现白屏, 首先检查`存储空间`权限是否已经打开, 然后取消勾选`Use ETC1(or RGBA4444) cache`运行, 或者手动删除ETC1纹理缓存(缓存文件目录在/sdcard/diii4a/<base/d3xp/d3le/cdoom/取决于运行的游戏...>/dds).
	* `Clear vertex buffer`选项建议选择第3个, 或者第2个亦可, 渲染每帧清理顶点缓冲区! 如果选择第1个, 则和原始的版本行为相同, 玩一会可能会爆显存闪屏崩溃! 对应的游戏控制台变量为`harm_r_clearVertexBuffer`, 可以查看该变量说明.
	* 经典DOOM(Classic DOOM)中有些触发器无法交互, 像第一关`E1M1`最后的通关大门不能打开. 但是可以通关在控制台输入`noclip`或绑定`noclip`到快捷键, 穿过不能触发的门.

----------------------------------------------------------------------------------

2020-08-16 1.1.0harmattan1

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

更新:
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
