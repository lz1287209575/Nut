#include "NString.h"
#include "NArray.h"
#include "NHashMap.h"
#include "NLogger.h"

class NLibTest : public NObject
{
public:
    static void RunAllTests()
    {
        NLogger::GetLogger().Info("开始 NLib 容器测试...");
        
        TestNString();
        TestNArray(); 
        TestNHashMap();
        
        NLogger::GetLogger().Info("NLib 容器测试完成!");
    }
    
private:
    static void TestNString()
    {
        NLogger::GetLogger().Info("=== NString 测试 ===");
        
        // 基础构造和赋值
        NString str1;
        NString str2("Hello");
        NString str3("World", 5);
        NString str4(str2);
        NString str5 = std::move(str3);
        
        NLogger::GetLogger().Info("str1: '{}'", str1.CStr());
        NLogger::GetLogger().Info("str2: '{}'", str2.CStr());
        NLogger::GetLogger().Info("str4: '{}'", str4.CStr());
        NLogger::GetLogger().Info("str5: '{}'", str5.CStr());
        
        // 字符串操作
        str1 = str2 + " " + str5 + "!";
        NLogger::GetLogger().Info("连接后: '{}'", str1.CStr());
        
        // 查找和替换
        size_t pos = str1.Find("World");
        NLogger::GetLogger().Info("'World' 位置: {}", pos);
        
        // 子字符串
        NString sub = str1.Substring(0, 5);
        NLogger::GetLogger().Info("子字符串: '{}'", sub.CStr());
        
        // 格式化
        NString formatted = NString::Format("数字: %d, 浮点: %.2f", 42, 3.14);
        NLogger::GetLogger().Info("格式化: '{}'", formatted.CStr());
        
        // 类型转换
        NString numStr = "123";
        int32_t num = numStr.ToInt32();
        NLogger::GetLogger().Info("字符串 '{}' 转整数: {}", numStr.CStr(), num);
        
        // 哈希和比较
        NLogger::GetLogger().Info("str1 哈希: {}", str1.GetHashCode());
        NLogger::GetLogger().Info("str2 == str4: {}", str2 == str4);
    }
    
    static void TestNArray()
    {
        NLogger::GetLogger().Info("=== NArray 测试 ===");
        
        // 基础构造
        NArray<int> arr1;
        NArray<int> arr2(5, 42);
        NArray<int> arr3{1, 2, 3, 4, 5};
        
        NLogger::GetLogger().Info("arr1 大小: {}", arr1.GetSize());
        NLogger::GetLogger().Info("arr2 大小: {}, 第一个元素: {}", arr2.GetSize(), arr2[0]);
        NLogger::GetLogger().Info("arr3 大小: {}, 内容: [", arr3.GetSize());
        
        for (size_t i = 0; i < arr3.GetSize(); ++i)
        {
            NLogger::GetLogger().Info("  [{}] = {}", i, arr3[i]);
        }
        
        // 动态操作
        arr1.PushBack(10);
        arr1.PushBack(20);
        arr1.PushBack(30);
        
        NLogger::GetLogger().Info("arr1 添加元素后大小: {}", arr1.GetSize());
        
        // 查找
        auto it = arr3.Find(3);
        if (it != arr3.End())
        {
            NLogger::GetLogger().Info("找到元素 3 在位置: {}", it - arr3.Begin());
        }
        
        // 排序
        NArray<int> arr4{5, 2, 8, 1, 9, 3};
        NLogger::GetLogger().Info("排序前: [");
        for (size_t i = 0; i < arr4.GetSize(); ++i)
        {
            NLogger::GetLogger().Info("  {}", arr4[i]);
        }
        
        arr4.Sort();
        NLogger::GetLogger().Info("排序后: [");
        for (size_t i = 0; i < arr4.GetSize(); ++i)
        {
            NLogger::GetLogger().Info("  {}", arr4[i]);
        }
        
        // 迭代器
        NLogger::GetLogger().Info("使用迭代器遍历:");
        for (auto it = arr3.Begin(); it != arr3.End(); ++it)
        {
            NLogger::GetLogger().Info("  {}", *it);
        }
    }
    
    static void TestNHashMap()
    {
        NLogger::GetLogger().Info("=== NHashMap 测试 ===");
        
        // 基础构造
        NHashMap<NString, int> map1;
        NHashMap<int, NString> map2{{1, NString("one")}, {2, NString("two")}, {3, NString("three")}};
        
        // 插入操作
        map1["hello"] = 1;
        map1["world"] = 2;
        map1.Insert(NString("test"), 3);
        
        NLogger::GetLogger().Info("map1 大小: {}", map1.GetSize());
        NLogger::GetLogger().Info("map2 大小: {}", map2.GetSize());
        
        // 查找
        auto it1 = map1.Find(NString("hello"));
        if (it1 != map1.End())
        {
            NLogger::GetLogger().Info("找到 'hello': {}", it1->Value);
        }
        
        auto it2 = map2.Find(2);
        if (it2 != map2.End())
        {
            NLogger::GetLogger().Info("找到键 2: '{}'", it2->Value.CStr());
        }
        
        // 包含检查
        bool contains = map1.Contains(NString("world"));
        NLogger::GetLogger().Info("map1 包含 'world': {}", contains);
        
        // 遍历
        NLogger::GetLogger().Info("map1 内容:");
        for (auto it = map1.Begin(); it != map1.End(); ++it)
        {
            NLogger::GetLogger().Info("  '{}' -> {}", it->Key.CStr(), it->Value);
        }
        
        NLogger::GetLogger().Info("map2 内容:");
        for (auto it = map2.Begin(); it != map2.End(); ++it)
        {
            NLogger::GetLogger().Info("  {} -> '{}'", it->Key, it->Value.CStr());
        }
        
        // 删除
        bool erased = map1.Erase(NString("hello"));
        NLogger::GetLogger().Info("删除 'hello': {}, 新大小: {}", erased, map1.GetSize());
        
        // 负载因子
        NLogger::GetLogger().Info("map1 负载因子: {:.2f}", map1.GetLoadFactor());
        NLogger::GetLogger().Info("map2 负载因子: {:.2f}", map2.GetLoadFactor());
    }
};

// 测试入口函数
void RunNLibTests()
{
    NLibTest::RunAllTests();
}