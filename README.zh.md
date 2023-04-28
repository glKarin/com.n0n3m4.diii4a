## idTech4A++ (Harmattan Edition)
#### DIII4A++, com.n0n3m4.diii4a, DOOM III/Quake 4/Prey(2006) for Android, 毁灭战士3/雷神之锤4/掠食(2006)安卓移植版
**最新版本:**
1.1.0harmattan29(natasha)  
**最新更新日期:**
2023-05-01  
**架构支持:**
arm64 armv7-a  
**平台:**
Android 4.0+  
**许可证:**
GPLv3

----------------------------------------------------------------------------------
### 更新

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

#### 关于掠食(2006)
###### 运行掠食(2006)([jmarshall](https://github.com/jmarshall23) 's [PreyDoom](https://github.com/jmarshall23/PreyDoom)). 目前可以运行全部关卡, 部分关卡存在bug.
> 1. 将PC端掠食(2006)游戏文件放到`preybase`文件夹, 然后直接启动游戏.
> 2. 已知问题的解决方案: 例如. 使用cvar `harm_g_translateAlienFont`自动翻译GUI中的外星人文字.
> 3. 已知bugs: 例如一些错误的碰撞检测(使用`noclip`), 部分菜单的渲染, 部分GUI不工作(RoadHouse的CD播放器).
> 4. 由于选项卡窗口UI组件暂不支持, 导致设置页面不工作, 必须通过编辑`preyconfig.cfg`来绑定额外按键.
> > * bind "幽灵行走按键" "_impulse54"
> > * bind "武器第2攻击键" "_attackAlt"
> > * bind "打火机开关键" "_impulse16"
> > * bind "扔物体键" "_impulse25"

----------------------------------------------------------------------------------

#### 关于雷神之锤4
###### 运行雷神之锤4([jmarshall](https://github.com/jmarshall23) 's [Quake4Doom](https://github.com/jmarshall23/Quake4Doom)). 目前可以运行全部关卡, 部分关卡存在bug.  
> 1. 将PC端雷神之锤4游戏文件放到`q4base`文件夹, 然后直接启动游戏.
> 2. 建议先解压雷神之锤4补丁资源到`q4base`资源目录(在菜单`Other` -> `Extract resource`).
> - 毁灭战士3格式的雷神之锤4字体, [IlDucci](https://github.com/IlDucci)提供.
> - 雷神之锤3bot文件(在多人游戏中, 进入游戏后在控制台使用命令`addbot <bot_file>`或`fillbots`添加bot, 或者设置`harm_si_autoFillBots`为1自动添加bot).
> - `SABot`多人游戏地图的aas文件(多人游戏的bot寻径).

###### 问题和解决方案:    
> 1. ~~门打不开~~: 碰撞问题已经修复, 例如触发器, 载具, AI, 电梯, 血站, 所有门都可正常打开.
> 2. *主菜单*: 目前可以正常显示, 包括多人游戏菜单, 去掉背景色. 可能存在部分GUI交互有问题.
> 3. ~~声音~~: 正常工作.
> 4. ~~游戏载入界面~~: 正常工作.
> 5. *多人游戏*: 目前正常工作, 并且可以添加bot(`jmarshall`添加了雷神之锤3的bot支持, 但是需要先添加bot文件和多人游戏地图的AAS文件, 目前可以设置`harm_g_autoGenAASFileInMPGame`为1自动在多人游戏地图载入(如果没有一个有效的该地图的AAS文件)后生成一个不怎么好的AAS文件, 也可以把你自己用其他方式生成的AAS文件放到游戏数据目录的`maps/mp`文件夹).
> 6. *脚本错误*: 部分地图存在脚本错误, 虽然不会使程序崩溃, 但是可能会影响游戏进程.
> 7. *粒子系统*: 目前工作的不完整(雷神之锤4使用了新的更高级的粒子系统`BSE`, 非开源, `jmarshall`通过反编译`深入敌后: 雷神战争`的BSE二进制实现了, 更多详情 [jmarshall23/Quake4BSE](https://github.com/jmarshall23/Quake4BSE)), 但是至今不工作. 现在实现了一个基于毁灭战士3的粒子特效系统的开源BSE, 可以渲染一些, 但是效果不是很理想.
> 8. *物体渲染*: 存在一些物体错误的渲染结果.
> 9. ~~碰撞~~: 碰撞问题已经修复.

----------------------------------------------------------------------------------
### 截图
> 游戏

<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_bathroom.png" alt="Classic bathroom">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_bathroom_jill_stars.png" alt="Classic bathroom in Rivensin mod">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake4_game_2.png" alt="Quake IV on DOOM3">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_prey_girlfriend.png" alt="Prey(2006) on DOOM3">

> Mod

<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_roe.png" width="50%" alt="Resurrection of Evil"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_the_lost_mission.png" width="50%" alt="The lost mission">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_classic_doom3.png" width="50%" alt="Classic DOOM"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_hardcorps.png" width="50%" alt="Hardcorps">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_rivensin.png" width="50%" alt="Rivensin"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake4.png" width="50%" alt="Quake IV">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_prey.png" width="50%" alt="Prey(2006)">

----------------------------------------------------------------------------------

### 更新:

[更新日志](CHANGES.zh.md ':include')

----------------------------------------------------------------------------------

### 关于:

* 源码在apk里的`assets/source`目录下.
	
----------------------------------------------------------------------------------

### 分支:

> `master`:
> * /DIII4A: 前端启动器源码
> * /doom3: 游戏源码

> `package`:
> * /*.apk: 所有构建
> * /screenshot: 截图
> * /source: 引用的源码
> * /pak: 游戏资源
> * /CHECK_FOR_UPDATE.json: 检查更新的配置JSON

> `n0n3m4_original_old_version`:
> * 原来旧的`n0n3m4`的版本.

----------------------------------------------------------------------------------
### 其他下载方式:

* [Google: https://drive.google.com/drive/folders/1qgFWFGICKjcQ5KfhiNBHn_JYhJN5XoLb](https://drive.google.com/drive/folders/1qgFWFGICKjcQ5KfhiNBHn_JYhJN5XoLb)
* [Baidu网盘: https://pan.baidu.com/s/1hXvKmrajAACfcCj9_ThZ_w](https://pan.baidu.com/s/1hXvKmrajAACfcCj9_ThZ_w) 提取码: `pyyj`
* [Baidu贴吧: BEYONDK2000](https://tieba.baidu.com/p/6825594793)
----------------------------------------------------------------------------------
