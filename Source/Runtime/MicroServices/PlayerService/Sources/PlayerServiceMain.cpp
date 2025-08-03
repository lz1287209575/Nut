#include <iostream>

// 包含LibNut核心功能
#include "../../../../LibNut/Sources/LibNut.h"

using namespace Nut;

int main()
{
	std::cout << "🚀 PlayerService 启动中..." << std::endl;

	try
	{
		// 初始化LibNut框架
		LibNutConfig Config;
		Config.bEnableMemoryProfiling = true;
		Config.GCMode = GarbageCollector::EGCMode::Manual; // 手动模式便于测试
		Config.LogLevel = NutLogger::LogLevel::Debug;

		if (!LibNut::Initialize(Config))
		{
			std::cout << "❌ LibNut初始化失败" << std::endl;
			return -1;
		}

		std::cout << "✅ LibNut框架初始化成功" << std::endl;

		// 打印框架状态
		LibNut::PrintStatus();

		// 运行GC测试套件
		std::cout << "\n🧪 开始运行GC测试..." << std::endl;
		LibNut::RunGCTests();

		// 最终状态
		std::cout << "\n📊 最终状态:" << std::endl;
		LibNut::PrintStatus();

		std::cout << "\n🎉 PlayerService 运行完成" << std::endl;

		// 关闭框架
		LibNut::Shutdown();
	}
	catch (const std::exception& e)
	{
		std::cout << "❌ 错误: " << e.what() << std::endl;
		return -1;
	}

	return 0;
}
