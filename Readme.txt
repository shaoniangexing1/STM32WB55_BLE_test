1.实现蓝牙的不同功耗的测试
	通过main.c头部的不同宏定义实现不同的时钟源配置选择，stm32wb55不支持LSI作为RF唤醒时钟，所以不做测试
2.低功耗模式测试，
	通过主函数下MX_APPE_Process()==》 app_entry.c ==》void UTIL_SEQ_Idle(void)开启和关闭CFG_LPM_SUPPORTED宏定义实现是否使用低功耗，
3.低功耗的模式配置
	1. if （StopModeDisable == 1） 
		进入sleep模式
	1. else （ StopModeDisable == 0） 
			if （OffModeDisable == 1）
				进入停止模式：PWR_EnterStopMode（）通过改变这个函数中的LL_PWR_SetPowerMode(LL_PWR_MODE_STOP0);参数进入不同的停止模式
																					 LL_PWR_MODE_STOP1
																					 LL_PWR_MODE_STOP2
					
			else(OffModeDisable == 0)
				进入待机模式
4. 在蓝牙模块进入低功耗的时候是在空闲模式下进入的
5. 关闭debug调试的宏定义
	// 在app_conf.h中修改：
#define CFG_DEBUGGER_SUPPORTED    0  // 生产环境下关闭调试器
#define CFG_DEBUG_BLE_TRACE       0  // 关闭BLE跟踪
#define CFG_DEBUG_APP_TRACE 
6.关闭hal_delay 进入睡眠模式 
7. 修改app_ble的代码，使得可以修改连接间隔，更改了连接间隔和广播间隔是得功耗降低