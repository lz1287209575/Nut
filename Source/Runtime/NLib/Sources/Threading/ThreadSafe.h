#pragma once

#include "Core/Object.h"
#include <mutex>
#include <shared_mutex>

namespace NLib
{

/**
 * @brief 线程安全的互斥锁封装
 */
class CThreadSafeMutex : public NObject
{
public:
	CThreadSafeMutex() = default;
	~CThreadSafeMutex() = default;

	/**
	 * @brief 锁定
	 */
	void Lock()
	{
		Mutex.lock();
	}

	/**
	 * @brief 解锁
	 */
	void Unlock()
	{
		Mutex.unlock();
	}

	/**
	 * @brief 尝试锁定
	 * @return 是否成功锁定
	 */
	bool TryLock()
	{
		return Mutex.try_lock();
	}

	/**
	 * @brief 获取原生互斥锁
	 * @return std::mutex引用
	 */
	std::mutex& GetNativeMutex()
	{
		return Mutex;
	}

private:
	std::mutex Mutex;

	// 禁止拷贝和赋值
	CThreadSafeMutex(const CThreadSafeMutex&) = delete;
	CThreadSafeMutex& operator=(const CThreadSafeMutex&) = delete;
	CThreadSafeMutex(CThreadSafeMutex&&) = delete;
	CThreadSafeMutex& operator=(CThreadSafeMutex&&) = delete;
};

/**
 * @brief 线程安全的读写锁封装
 */
class CThreadSafeRWMutex : public NObject
{
public:
	CThreadSafeRWMutex() = default;
	~CThreadSafeRWMutex() = default;

	/**
	 * @brief 独占锁定（写锁）
	 */
	void Lock()
	{
		Mutex.lock();
	}

	/**
	 * @brief 解锁
	 */
	void Unlock()
	{
		Mutex.unlock();
	}

	/**
	 * @brief 共享锁定（读锁）
	 */
	void LockShared()
	{
		Mutex.lock_shared();
	}

	/**
	 * @brief 解锁共享锁
	 */
	void UnlockShared()
	{
		Mutex.unlock_shared();
	}

	/**
	 * @brief 尝试独占锁定
	 * @return 是否成功锁定
	 */
	bool TryLock()
	{
		return Mutex.try_lock();
	}

	/**
	 * @brief 尝试共享锁定
	 * @return 是否成功锁定
	 */
	bool TryLockShared()
	{
		return Mutex.try_lock_shared();
	}

	/**
	 * @brief 获取原生读写锁
	 * @return std::shared_mutex引用
	 */
	std::shared_mutex& GetNativeMutex()
	{
		return Mutex;
	}

private:
	std::shared_mutex Mutex;

	// 禁止拷贝和赋值
	CThreadSafeRWMutex(const CThreadSafeRWMutex&) = delete;
	CThreadSafeRWMutex& operator=(const CThreadSafeRWMutex&) = delete;
	CThreadSafeRWMutex(CThreadSafeRWMutex&&) = delete;
	CThreadSafeRWMutex& operator=(CThreadSafeRWMutex&&) = delete;
};

/**
 * @brief RAII锁保护类
 */
template <typename TMutex>
class TLockGuard
{
public:
	explicit TLockGuard(TMutex& InMutex)
	    : MutexRef(InMutex)
	{
		MutexRef.Lock();
	}

	~TLockGuard()
	{
		MutexRef.Unlock();
	}

private:
	TMutex& MutexRef;

	// 禁止拷贝和赋值
	TLockGuard(const TLockGuard&) = delete;
	TLockGuard& operator=(const TLockGuard&) = delete;
	TLockGuard(TLockGuard&&) = delete;
	TLockGuard& operator=(TLockGuard&&) = delete;
};

/**
 * @brief 共享锁保护类
 */
template <typename TRWMutex>
class TSharedLockGuard
{
public:
	explicit TSharedLockGuard(TRWMutex& InMutex)
	    : MutexRef(InMutex)
	{
		MutexRef.LockShared();
	}

	~TSharedLockGuard()
	{
		MutexRef.UnlockShared();
	}

private:
	TRWMutex& MutexRef;

	// 禁止拷贝和赋值
	TSharedLockGuard(const TSharedLockGuard&) = delete;
	TSharedLockGuard& operator=(const TSharedLockGuard&) = delete;
	TSharedLockGuard(TSharedLockGuard&&) = delete;
	TSharedLockGuard& operator=(TSharedLockGuard&&) = delete;
};

// 便捷类型别名
using CThreadSafeLock = TLockGuard<CThreadSafeMutex>;
using CThreadSafeReadLock = TSharedLockGuard<CThreadSafeRWMutex>;

// 便捷宏定义
#define LOCK_GUARD(mutex) TLockGuard<decltype(mutex)> LockGuard_##__LINE__(mutex)
#define SHARED_LOCK_GUARD(mutex) TSharedLockGuard<decltype(mutex)> SharedLockGuard_##__LINE__(mutex)

} // namespace NLib