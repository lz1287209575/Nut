#include "LibNut.h"
#include <atomic>
#include <mutex>
#include <chrono>

namespace LibNut
{
    // 库状态
    static std::atomic<bool> g_initialized{false};
    static std::mutex g_init_mutex;
    
    // 统计数据
    static MemoryStats g_memory_stats;
    static PerformanceStats g_performance_stats;
    static std::mutex g_stats_mutex;
    
    // 初始化时间戳
    static std::chrono::steady_clock::time_point g_init_time;
    
    bool Initialize()
    {
        std::lock_guard<std::mutex> lock(g_init_mutex);
        
        if (g_initialized.load())
        {
            LIBNUT_LOG_WARNING("LibNut::Initialize() called multiple times");
            return true;
        }
        
        try
        {
            g_init_time = std::chrono::steady_clock::now();
            
            // 初始化内存管理器
            if (!NMemoryManager::GetInstance().Initialize())
            {
                LIBNUT_LOG_ERROR("Failed to initialize NMemoryManager");
                return false;
            }
            
            // 初始化垃圾回收器
            if (!NGarbageCollector::GetInstance().Initialize())
            {
                LIBNUT_LOG_ERROR("Failed to initialize NGarbageCollector");
                return false;
            }
            
            // 初始化日志系统
            NLogger::GetLogger().Info("LibNut {} initialized successfully", LIBNUT_VERSION);
            
            // 重置统计数据
            {
                std::lock_guard<std::mutex> stats_lock(g_stats_mutex);
                g_memory_stats = MemoryStats();
                g_performance_stats = PerformanceStats();
            }
            
            g_initialized.store(true);
            return true;
        }
        catch (const std::exception& e)
        {
            LIBNUT_LOG_ERROR("Exception during LibNut initialization: {}", e.what());
            return false;
        }
        catch (...)
        {
            LIBNUT_LOG_ERROR("Unknown exception during LibNut initialization");
            return false;
        }
    }
    
    void Shutdown()
    {
        std::lock_guard<std::mutex> lock(g_init_mutex);
        
        if (!g_initialized.load())
        {
            LIBNUT_LOG_WARNING("LibNut::Shutdown() called but library not initialized");
            return;
        }
        
        try
        {
            // 强制执行最后一次垃圾回收
            ForceGarbageCollect();
            
            // 计算运行时间
            auto shutdown_time = std::chrono::steady_clock::now();
            auto runtime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                shutdown_time - g_init_time).count();
            
            // 输出最终统计信息
            MemoryStats final_memory = GetMemoryStats();
            PerformanceStats final_perf = GetPerformanceStats();
            
            NLogger::GetLogger().Info("LibNut shutdown - Runtime: {}ms", runtime_ms);
            NLogger::GetLogger().Info("Final Memory Stats - Allocated: {} bytes, Peak: {} bytes", 
                final_memory.total_allocated, final_memory.peak_usage);
            NLogger::GetLogger().Info("Final Performance Stats - Objects: {}, GC Runs: {}", 
                final_perf.object_creations, final_perf.gc_runs);
            
            // 关闭垃圾回收器
            NGarbageCollector::GetInstance().Shutdown();
            
            // 关闭内存管理器
            NMemoryManager::GetInstance().Shutdown();
            
            g_initialized.store(false);
        }
        catch (const std::exception& e)
        {
            LIBNUT_LOG_ERROR("Exception during LibNut shutdown: {}", e.what());
        }
        catch (...)
        {
            LIBNUT_LOG_ERROR("Unknown exception during LibNut shutdown");
        }
    }
    
    bool IsInitialized()
    {
        return g_initialized.load();
    }
    
    MemoryStats GetMemoryStats()
    {
        std::lock_guard<std::mutex> lock(g_stats_mutex);
        
        // 从内存管理器获取实时数据
        const auto& memory_manager = NMemoryManager::GetInstance();
        g_memory_stats.total_allocated = memory_manager.GetTotalAllocated();
        g_memory_stats.total_used = memory_manager.GetTotalUsed();
        g_memory_stats.peak_usage = memory_manager.GetPeakUsage();
        
        // 从垃圾回收器获取对象计数
        const auto& gc = NGarbageCollector::GetInstance();
        g_memory_stats.gc_objects_count = gc.GetObjectCount();
        
        // TODO: 从容器系统获取容器计数
        g_memory_stats.container_count = 0; // 暂时为0
        
        return g_memory_stats;
    }
    
    void ForceGarbageCollect()
    {
        if (!IsInitialized())
        {
            LIBNUT_LOG_WARNING("ForceGarbageCollect() called but LibNut not initialized");
            return;
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 执行垃圾回收
        NGarbageCollector::GetInstance().CollectGarbage();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        // 更新统计数据
        {
            std::lock_guard<std::mutex> lock(g_stats_mutex);
            g_performance_stats.gc_runs++;
            
            // 更新平均GC时间
            double total_time = g_performance_stats.average_gc_time_ms * (g_performance_stats.gc_runs - 1);
            g_performance_stats.average_gc_time_ms = (total_time + duration_ms) / g_performance_stats.gc_runs;
        }
        
        LIBNUT_DEBUG_ONLY(
            LIBNUT_LOG_INFO("Garbage collection completed in {:.2f}ms", duration_ms);
        );
    }
    
    PerformanceStats GetPerformanceStats()
    {
        std::lock_guard<std::mutex> lock(g_stats_mutex);
        
        // TODO: 从各个模块收集性能数据
        // 目前返回存储的统计数据
        return g_performance_stats;
    }
    
    void ResetStats()
    {
        std::lock_guard<std::mutex> lock(g_stats_mutex);
        g_memory_stats = MemoryStats();
        g_performance_stats = PerformanceStats();
        
        LIBNUT_LOG_INFO("LibNut statistics reset");
    }
    
    // 内部函数：更新性能统计 (供其他模块调用)
    namespace Detail
    {
        void IncrementObjectCreations()
        {
            std::lock_guard<std::mutex> lock(g_stats_mutex);
            g_performance_stats.object_creations++;
        }
        
        void IncrementObjectDestructions()
        {
            std::lock_guard<std::mutex> lock(g_stats_mutex);
            g_performance_stats.object_destructions++;
        }
        
        void IncrementMemoryAllocations()
        {
            std::lock_guard<std::mutex> lock(g_stats_mutex);
            g_performance_stats.memory_allocations++;
        }
        
        void IncrementMemoryDeallocations()
        {
            std::lock_guard<std::mutex> lock(g_stats_mutex);
            g_performance_stats.memory_deallocations++;
        }
    }
}