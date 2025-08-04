#pragma once

#include "Memory/MemoryManager.h"
#include "TArray.h"

#include <atomic>
#include <mutex>

namespace NLib
{
/**
 * @brief 线程安全的队列容器
 *
 * 提供高性能的FIFO队列功能：
 * - 线程安全操作
 * - 动态扩容
 * - 高效的入队出队操作
 * - 支持自定义内存分配器
 */
template <typename TElementType, typename TAllocator = CMemoryManager>
class TQueue
{
public:
	using ElementType = TElementType;
	using SizeType = int32_t;
	using AllocatorType = TAllocator;

public:
	// === 构造函数 ===

	/**
	 * @brief 默认构造函数
	 */
	TQueue()
	    : Data(),
	      Head(0),
	      Tail(0),
	      Count(0)
	{}

	/**
	 * @brief 预分配容量构造函数
	 */
	explicit TQueue(SizeType InitialCapacity)
	    : Data(),
	      Head(0),
	      Tail(0),
	      Count(0)
	{
		Reserve(InitialCapacity);
	}

	/**
	 * @brief 拷贝构造函数
	 */
	TQueue(const TQueue& Other)
	    : Data(),
	      Head(0),
	      Tail(0),
	      Count(0)
	{
		std::lock_guard<std::mutex> Lock(Other.Mutex);
		CopyFrom(Other);
	}

	/**
	 * @brief 移动构造函数
	 */
	TQueue(TQueue&& Other) noexcept
	    : Data(std::move(Other.Data)),
	      Head(Other.Head.load()),
	      Tail(Other.Tail.load()),
	      Count(Other.Count.load())
	{
		Other.Head = 0;
		Other.Tail = 0;
		Other.Count = 0;
	}

	/**
	 * @brief 析构函数
	 */
	~TQueue()
	{
		Empty();
	}

public:
	// === 赋值操作符 ===

	/**
	 * @brief 拷贝赋值操作符
	 */
	TQueue& operator=(const TQueue& Other)
	{
		if (this != &Other)
		{
			std::lock(Mutex, Other.Mutex);
			std::lock_guard<std::mutex> Lock1(Mutex, std::adopt_lock);
			std::lock_guard<std::mutex> Lock2(Other.Mutex, std::adopt_lock);

			Empty();
			CopyFrom(Other);
		}
		return *this;
	}

	/**
	 * @brief 移动赋值操作符
	 */
	TQueue& operator=(TQueue&& Other) noexcept
	{
		if (this != &Other)
		{
			std::lock(Mutex, Other.Mutex);
			std::lock_guard<std::mutex> Lock1(Mutex, std::adopt_lock);
			std::lock_guard<std::mutex> Lock2(Other.Mutex, std::adopt_lock);

			Empty();

			Data = std::move(Other.Data);
			Head = Other.Head.load();
			Tail = Other.Tail.load();
			Count = Other.Count.load();

			Other.Head = 0;
			Other.Tail = 0;
			Other.Count = 0;
		}
		return *this;
	}

public:
	// === 队列操作 ===

	/**
	 * @brief 入队元素
	 */
	void Enqueue(const ElementType& Element)
	{
		std::lock_guard<std::mutex> Lock(Mutex);

		// 检查是否需要扩容
		if (Count.load() >= Data.Size())
		{
			Resize();
		}

		// 添加元素到尾部
		Data[Tail] = Element;
		Tail = (Tail + 1) % Data.Size();
		Count.fetch_add(1);
	}

	/**
	 * @brief 入队元素（移动语义）
	 */
	void Enqueue(ElementType&& Element)
	{
		std::lock_guard<std::mutex> Lock(Mutex);

		// 检查是否需要扩容
		if (Count.load() >= Data.Size())
		{
			Resize();
		}

		// 移动元素到尾部
		Data[Tail] = std::move(Element);
		Tail = (Tail + 1) % Data.Size();
		Count.fetch_add(1);
	}

	/**
	 * @brief 原地构造入队
	 */
	template <typename... TArgs>
	void Emplace(TArgs&&... Args)
	{
		std::lock_guard<std::mutex> Lock(Mutex);

		// 检查是否需要扩容
		if (Count.load() >= Data.Size())
		{
			Resize();
		}

		// 原地构造元素
		Data[Tail] = ElementType(std::forward<TArgs>(Args)...);
		Tail = (Tail + 1) % Data.Size();
		Count.fetch_add(1);
	}

	/**
	 * @brief 出队元素
	 */
	ElementType Dequeue()
	{
		std::lock_guard<std::mutex> Lock(Mutex);

		if (Count.load() == 0)
		{
			throw std::runtime_error("Queue is empty");
		}

		// 获取头部元素
		ElementType Result = std::move(Data[Head]);
		Head = (Head + 1) % Data.Size();
		Count.fetch_sub(1);

		return Result;
	}

	/**
	 * @brief 尝试出队元素
	 */
	bool TryDequeue(ElementType& OutElement)
	{
		std::lock_guard<std::mutex> Lock(Mutex);

		if (Count.load() == 0)
		{
			return false;
		}

		// 获取头部元素
		OutElement = std::move(Data[Head]);
		Head = (Head + 1) % Data.Size();
		Count.fetch_sub(1);

		return true;
	}

	/**
	 * @brief 查看队首元素（不移除）
	 */
	const ElementType& Front() const
	{
		std::lock_guard<std::mutex> Lock(Mutex);

		if (Count.load() == 0)
		{
			throw std::runtime_error("Queue is empty");
		}

		return Data[Head];
	}

	/**
	 * @brief 尝试查看队首元素
	 */
	bool TryFront(ElementType& OutElement) const
	{
		std::lock_guard<std::mutex> Lock(Mutex);

		if (Count.load() == 0)
		{
			return false;
		}

		OutElement = Data[Head];
		return true;
	}

public:
	// === 容量和状态 ===

	/**
	 * @brief 获取队列大小
	 */
	SizeType Size() const
	{
		return Count.load();
	}

	/**
	 * @brief 检查队列是否为空
	 */
	bool IsEmpty() const
	{
		return Count.load() == 0;
	}

	/**
	 * @brief 获取队列容量
	 */
	SizeType Capacity() const
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		return Data.Size();
	}

	/**
	 * @brief 预分配容量
	 */
	void Reserve(SizeType NewCapacity)
	{
		std::lock_guard<std::mutex> Lock(Mutex);

		if (NewCapacity > Data.Size())
		{
			ResizeToCapacity(NewCapacity);
		}
	}

	/**
	 * @brief 清空队列
	 */
	void Empty()
	{
		std::lock_guard<std::mutex> Lock(Mutex);

		// 调用所有元素的析构函数
		while (Count.load() > 0)
		{
			Head = (Head + 1) % Data.Size();
			Count.fetch_sub(1);
		}

		Head = 0;
		Tail = 0;
		Count = 0;
	}

	/**
	 * @brief 收缩容量到实际大小
	 */
	void Shrink()
	{
		std::lock_guard<std::mutex> Lock(Mutex);

		SizeType CurrentCount = Count.load();
		if (CurrentCount == 0)
		{
			Data.Empty();
			Head = 0;
			Tail = 0;
			return;
		}

		// 重新排列元素
		TArray<ElementType, TAllocator> NewData;
		NewData.Reserve(CurrentCount);

		SizeType Current = Head;
		for (SizeType i = 0; i < CurrentCount; ++i)
		{
			NewData.Add(std::move(Data[Current]));
			Current = (Current + 1) % Data.Size();
		}

		Data = std::move(NewData);
		Head = 0;
		Tail = CurrentCount;
	}

public:
	// === 迭代器支持（只读） ===

	/**
	 * @brief 将队列内容复制到数组
	 */
	TArray<ElementType, TAllocator> ToArray() const
	{
		std::lock_guard<std::mutex> Lock(Mutex);

		TArray<ElementType, TAllocator> Result;
		SizeType CurrentCount = Count.load();
		Result.Reserve(CurrentCount);

		SizeType Current = Head;
		for (SizeType i = 0; i < CurrentCount; ++i)
		{
			Result.Add(Data[Current]);
			Current = (Current + 1) % Data.Size();
		}

		return Result;
	}

	/**
	 * @brief 遍历队列元素
	 */
	template <typename TFunc>
	void ForEach(TFunc&& Function) const
	{
		std::lock_guard<std::mutex> Lock(Mutex);

		SizeType CurrentCount = Count.load();
		SizeType Current = Head;

		for (SizeType i = 0; i < CurrentCount; ++i)
		{
			Function(Data[Current]);
			Current = (Current + 1) % Data.Size();
		}
	}

public:
	// === 调试功能 ===

	/**
	 * @brief 验证队列内部状态
	 */
	bool IsValid() const
	{
		std::lock_guard<std::mutex> Lock(Mutex);

		SizeType CurrentCount = Count.load();
		SizeType DataSize = Data.Size();

		// 基本检查
		if (CurrentCount < 0 || CurrentCount > DataSize)
		{
			return false;
		}

		if (Head >= DataSize || Tail >= DataSize)
		{
			return false;
		}

		// 空队列检查
		if (CurrentCount == 0)
		{
			return Head == Tail;
		}

		// 计算实际元素数量
		SizeType ActualCount;
		if (Tail >= Head)
		{
			ActualCount = Tail - Head;
		}
		else
		{
			ActualCount = DataSize - Head + Tail;
		}

		return ActualCount == CurrentCount;
	}

	/**
	 * @brief 获取调试信息
	 */
	struct SDebugInfo
	{
		SizeType Size;
		SizeType Capacity;
		SizeType Head;
		SizeType Tail;
		bool IsValid;
	};

	SDebugInfo GetDebugInfo() const
	{
		std::lock_guard<std::mutex> Lock(Mutex);

		return SDebugInfo{Count.load(), Data.Size(), Head.load(), Tail.load(), IsValid()};
	}

private:
	// === 内部实现 ===

	/**
	 * @brief 扩容队列
	 */
	void Resize()
	{
		SizeType NewCapacity = Data.Size() == 0 ? 4 : Data.Size() * 2;
		ResizeToCapacity(NewCapacity);
	}

	/**
	 * @brief 扩容到指定大小
	 */
	void ResizeToCapacity(SizeType NewCapacity)
	{
		TArray<ElementType, TAllocator> NewData;
		NewData.Reserve(NewCapacity);

		// 复制现有元素
		SizeType CurrentCount = Count.load();
		SizeType Current = Head;

		for (SizeType i = 0; i < CurrentCount; ++i)
		{
			NewData.Add(std::move(Data[Current]));
			Current = (Current + 1) % Data.Size();
		}

		// 更新数据
		Data = std::move(NewData);
		Head = 0;
		Tail = CurrentCount;
	}

	/**
	 * @brief 从另一个队列复制数据
	 */
	void CopyFrom(const TQueue& Other)
	{
		SizeType OtherCount = Other.Count.load();
		Data.Reserve(OtherCount);

		SizeType Current = Other.Head;
		for (SizeType i = 0; i < OtherCount; ++i)
		{
			Data.Add(Other.Data[Current]);
			Current = (Current + 1) % Other.Data.Size();
		}

		Head = 0;
		Tail = OtherCount;
		Count = OtherCount;
	}

private:
	// === 成员变量 ===

	TArray<ElementType, TAllocator> Data; // 底层数据存储
	std::atomic<SizeType> Head;           // 队首索引
	std::atomic<SizeType> Tail;           // 队尾索引
	std::atomic<SizeType> Count;          // 元素数量
	mutable std::mutex Mutex;             // 线程安全互斥锁
};

// === 类型别名 ===
template <typename T>
using TThreadSafeQueue = TQueue<T, CMemoryManager>;

template <typename T>
using FQueue = TQueue<T, CMemoryManager>;

} // namespace NLib