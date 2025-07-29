#pragma once

#include "NContainer.h"
#include "NArray.h"

template<typename K, typename V>
struct NKeyValuePair
{
    K Key;
    V Value;
    
    NKeyValuePair() = default;
    NKeyValuePair(const K& key, const V& value) : Key(key), Value(value) {}
    NKeyValuePair(K&& key, V&& value) : Key(std::move(key)), Value(std::move(value)) {}
    
    bool operator==(const NKeyValuePair& other) const
    {
        return NEqual<K>{}(Key, other.Key) && NEqual<V>{}(Value, other.Value);
    }
};

template<typename K, typename V>
NCLASS()
class LIBNLIB_API NHashMap : public NContainer
{
    GENERATED_BODY()

public:
    using KeyType = K;
    using ValueType = V;
    using PairType = NKeyValuePair<K, V>;
    using Reference = V&;
    using ConstReference = const V&;
    using KeyReference = const K&;
    
    // Robin Hood 哈希表的桶结构
    struct Bucket
    {
        PairType pair;
        size_t distance; // 距离理想位置的距离
        bool occupied;   // 是否被占用
        
        Bucket() : distance(0), occupied(false) {}
    };

    class Iterator
    {
    public:
        Iterator(Bucket* buckets, size_t index, size_t capacity)
            : buckets_(buckets), index_(index), capacity_(capacity)
        {
            // 跳过空桶
            while (index_ < capacity_ && !buckets_[index_].occupied)
            {
                ++index_;
            }
        }
        
        PairType& operator*() { return buckets_[index_].pair; }
        const PairType& operator*() const { return buckets_[index_].pair; }
        PairType* operator->() { return &buckets_[index_].pair; }
        const PairType* operator->() const { return &buckets_[index_].pair; }
        
        Iterator& operator++()
        {
            ++index_;
            while (index_ < capacity_ && !buckets_[index_].occupied)
            {
                ++index_;
            }
            return *this;
        }
        
        Iterator operator++(int)
        {
            Iterator temp(*this);
            ++(*this);
            return temp;
        }
        
        bool operator==(const Iterator& other) const
        {
            return index_ == other.index_;
        }
        
        bool operator!=(const Iterator& other) const
        {
            return index_ != other.index_;
        }

    private:
        Bucket* buckets_;
        size_t index_;
        size_t capacity_;
    };

    class ConstIterator
    {
    public:
        ConstIterator(const Bucket* buckets, size_t index, size_t capacity)
            : buckets_(buckets), index_(index), capacity_(capacity)
        {
            while (index_ < capacity_ && !buckets_[index_].occupied)
            {
                ++index_;
            }
        }
        
        const PairType& operator*() const { return buckets_[index_].pair; }
        const PairType* operator->() const { return &buckets_[index_].pair; }
        
        ConstIterator& operator++()
        {
            ++index_;
            while (index_ < capacity_ && !buckets_[index_].occupied)
            {
                ++index_;
            }
            return *this;
        }
        
        ConstIterator operator++(int)
        {
            ConstIterator temp(*this);
            ++(*this);
            return temp;
        }
        
        bool operator==(const ConstIterator& other) const
        {
            return index_ == other.index_;
        }
        
        bool operator!=(const ConstIterator& other) const
        {
            return index_ != other.index_;
        }

    private:
        const Bucket* buckets_;
        size_t index_;
        size_t capacity_;
    };

    // 构造函数
    NHashMap();
    explicit NHashMap(size_t initial_capacity);
    NHashMap(std::initializer_list<PairType> init_list);
    NHashMap(const NHashMap& other);
    NHashMap(NHashMap&& other) noexcept;
    
    // 析构函数
    virtual ~NHashMap();
    
    // 赋值操作符
    NHashMap& operator=(const NHashMap& other);
    NHashMap& operator=(NHashMap&& other) noexcept;
    NHashMap& operator=(std::initializer_list<PairType> init_list);
    
    // 基础访问
    Reference operator[](const K& key);
    Reference operator[](K&& key);
    ConstReference At(const K& key) const;
    Reference At(const K& key);
    
    // 容器接口实现
    size_t GetSize() const override { return size_; }
    size_t GetCapacity() const override { return capacity_; }
    bool IsEmpty() const override { return size_ == 0; }
    void Clear() override;
    
    // 容量管理
    void Reserve(size_t new_capacity);
    void Rehash(size_t bucket_count);
    float GetLoadFactor() const { return capacity_ > 0 ? static_cast<float>(size_) / capacity_ : 0.0f; }
    float GetMaxLoadFactor() const { return max_load_factor_; }
    void SetMaxLoadFactor(float factor) { max_load_factor_ = factor; }
    
    // 元素操作
    std::pair<Iterator, bool> Insert(const PairType& pair);
    std::pair<Iterator, bool> Insert(PairType&& pair);
    std::pair<Iterator, bool> Insert(const K& key, const V& value);
    std::pair<Iterator, bool> Insert(K&& key, V&& value);
    
    template<typename... Args>
    std::pair<Iterator, bool> Emplace(const K& key, Args&&... args);
    template<typename... Args>
    std::pair<Iterator, bool> Emplace(K&& key, Args&&... args);
    
    std::pair<Iterator, bool> InsertOrAssign(const K& key, const V& value);
    std::pair<Iterator, bool> InsertOrAssign(const K& key, V&& value);
    std::pair<Iterator, bool> InsertOrAssign(K&& key, const V& value);
    std::pair<Iterator, bool> InsertOrAssign(K&& key, V&& value);
    
    template<typename... Args>
    std::pair<Iterator, bool> TryEmplace(const K& key, Args&&... args);
    template<typename... Args>
    std::pair<Iterator, bool> TryEmplace(K&& key, Args&&... args);
    
    bool Erase(const K& key);
    Iterator Erase(Iterator pos);
    Iterator Erase(ConstIterator pos);
    size_t Erase(const K& key);
    
    // 查找操作
    Iterator Find(const K& key);
    ConstIterator Find(const K& key) const;
    
    bool Contains(const K& key) const;
    size_t Count(const K& key) const; // 对于HashMap总是返回0或1
    
    // 批量操作  
    void Merge(const NHashMap& other);
    void Merge(NHashMap&& other);
    
    // 键和值的访问
    NArray<K> GetKeys() const;
    NArray<V> GetValues() const;
    
    // 迭代器
    Iterator Begin() { return Iterator(buckets_, 0, capacity_); }
    Iterator End() { return Iterator(buckets_, capacity_, capacity_); }
    ConstIterator Begin() const { return ConstIterator(buckets_, 0, capacity_); }
    ConstIterator End() const { return ConstIterator(buckets_, capacity_, capacity_); }
    ConstIterator CBegin() const { return ConstIterator(buckets_, 0, capacity_); }
    ConstIterator CEnd() const { return ConstIterator(buckets_, capacity_, capacity_); }
    
    // 比较操作符
    bool operator==(const NHashMap& other) const;
    bool operator!=(const NHashMap& other) const;
    
    // NObject 接口重写
    virtual bool Equals(const NObject* other) const override;
    virtual size_t GetHashCode() const override;
    virtual NString ToString() const override;

private:
    Bucket* buckets_;
    size_t size_;
    size_t capacity_;
    float max_load_factor_;
    
    static constexpr size_t DEFAULT_CAPACITY = 16;
    static constexpr float DEFAULT_MAX_LOAD_FACTOR = 0.75f;
    
    // 内部工具函数
    size_t Hash(const K& key) const;
    size_t FindBucket(const K& key) const;
    size_t FindInsertPosition(const K& key, size_t hash_value) const;
    void InsertAtPosition(size_t pos, PairType&& pair, size_t distance);
    void RobinHoodInsert(PairType&& pair);
    void ResizeIfNeeded();
    void Resize(size_t new_capacity);
    void InitializeBuckets();
    void DeallocateBuckets();
    void MoveFrom(NHashMap&& other) noexcept;
    void CopyFrom(const NHashMap& other);
    
    // Robin Hood 相关
    size_t CalculateDistance(size_t hash_value, size_t actual_pos) const;
    void ShiftBackward(size_t pos);
};

// 模板实现
template<typename K, typename V>
NHashMap<K, V>::NHashMap()
    : buckets_(nullptr), size_(0), capacity_(0), max_load_factor_(DEFAULT_MAX_LOAD_FACTOR)
{
}

template<typename K, typename V>
NHashMap<K, V>::NHashMap(size_t initial_capacity)
    : buckets_(nullptr), size_(0), capacity_(0), max_load_factor_(DEFAULT_MAX_LOAD_FACTOR)
{
    if (initial_capacity > 0)
    {
        // 确保容量是2的幂次
        size_t actual_capacity = 1;
        while (actual_capacity < initial_capacity)
        {
            actual_capacity <<= 1;
        }
        
        capacity_ = actual_capacity;
        InitializeBuckets();
    }
}

template<typename K, typename V>
NHashMap<K, V>::NHashMap(std::initializer_list<PairType> init_list)
    : buckets_(nullptr), size_(0), capacity_(0), max_load_factor_(DEFAULT_MAX_LOAD_FACTOR)
{
    if (init_list.size() > 0)
    {
        size_t required_capacity = static_cast<size_t>(init_list.size() / max_load_factor_) + 1;
        size_t actual_capacity = DEFAULT_CAPACITY;
        while (actual_capacity < required_capacity)
        {
            actual_capacity <<= 1;
        }
        
        capacity_ = actual_capacity;
        InitializeBuckets();
        
        for (const auto& pair : init_list)
        {
            Insert(pair);
        }
    }
}

template<typename K, typename V>
NHashMap<K, V>::NHashMap(const NHashMap& other)
    : buckets_(nullptr), size_(0), capacity_(0), max_load_factor_(DEFAULT_MAX_LOAD_FACTOR)
{
    CopyFrom(other);
}

template<typename K, typename V>
NHashMap<K, V>::NHashMap(NHashMap&& other) noexcept
    : buckets_(nullptr), size_(0), capacity_(0), max_load_factor_(DEFAULT_MAX_LOAD_FACTOR)
{
    MoveFrom(std::move(other));
}

template<typename K, typename V>
NHashMap<K, V>::~NHashMap()
{
    DeallocateBuckets();
}

template<typename K, typename V>
NHashMap<K, V>& NHashMap<K, V>::operator=(const NHashMap& other)
{
    if (this != &other)
    {
        CopyFrom(other);
    }
    return *this;
}

template<typename K, typename V>
NHashMap<K, V>& NHashMap<K, V>::operator=(NHashMap&& other) noexcept
{
    if (this != &other)
    {
        DeallocateBuckets();
        MoveFrom(std::move(other));
    }
    return *this;
}

template<typename K, typename V>
typename NHashMap<K, V>::Reference NHashMap<K, V>::operator[](const K& key)
{
    auto result = TryEmplace(key);
    return result.first->Value;
}

template<typename K, typename V>
typename NHashMap<K, V>::Reference NHashMap<K, V>::operator[](K&& key)
{
    auto result = TryEmplace(std::move(key));
    return result.first->Value;
}

template<typename K, typename V>
typename NHashMap<K, V>::ConstReference NHashMap<K, V>::At(const K& key) const
{
    ConstIterator it = Find(key);
    if (it == End())
    {
        NLogger::GetLogger().Error("NHashMap::At: Key not found");
        static const V dummy{};
        return dummy;
    }
    return it->Value;
}

template<typename K, typename V>
typename NHashMap<K, V>::Reference NHashMap<K, V>::At(const K& key)
{
    Iterator it = Find(key);
    if (it == End())
    {
        NLogger::GetLogger().Error("NHashMap::At: Key not found");
        static V dummy{};
        return dummy;
    }
    return it->Value;
}

template<typename K, typename V>
void NHashMap<K, V>::Clear()
{
    if (buckets_)
    {
        for (size_t i = 0; i < capacity_; ++i)
        {
            if (buckets_[i].occupied)
            {
                if constexpr (std::is_base_of_v<NObject, K>)
                {
                    NGarbageCollector::GetInstance().UnregisterObject(&buckets_[i].pair.Key);
                }
                if constexpr (std::is_base_of_v<NObject, V>)
                {
                    NGarbageCollector::GetInstance().UnregisterObject(&buckets_[i].pair.Value);
                }
                
                buckets_[i].pair.~PairType();
                buckets_[i].occupied = false;
                buckets_[i].distance = 0;
            }
        }
        size_ = 0;
    }
}

template<typename K, typename V>
std::pair<typename NHashMap<K, V>::Iterator, bool> NHashMap<K, V>::Insert(const PairType& pair)
{
    return Insert(PairType(pair));
}

template<typename K, typename V>
std::pair<typename NHashMap<K, V>::Iterator, bool> NHashMap<K, V>::Insert(PairType&& pair)
{
    ResizeIfNeeded();
    
    size_t hash_value = Hash(pair.Key);
    size_t pos = FindBucket(pair.Key);
    
    if (pos != capacity_ && buckets_[pos].occupied && 
        NEqual<K>{}(buckets_[pos].pair.Key, pair.Key))
    {
        // 键已存在
        return std::make_pair(Iterator(buckets_, pos, capacity_), false);
    }
    
    // 插入新元素
    RobinHoodInsert(std::move(pair));
    ++size_;
    
    // 查找插入后的位置
    pos = FindBucket(pair.Key);
    return std::make_pair(Iterator(buckets_, pos, capacity_), true);
}

template<typename K, typename V>
std::pair<typename NHashMap<K, V>::Iterator, bool> NHashMap<K, V>::Insert(const K& key, const V& value)
{
    return Insert(PairType(key, value));
}

template<typename K, typename V>
template<typename... Args>
std::pair<typename NHashMap<K, V>::Iterator, bool> NHashMap<K, V>::TryEmplace(const K& key, Args&&... args)
{
    ResizeIfNeeded();
    
    size_t pos = FindBucket(key);
    
    if (pos != capacity_ && buckets_[pos].occupied && 
        NEqual<K>{}(buckets_[pos].pair.Key, key))
    {
        // 键已存在，不插入
        return std::make_pair(Iterator(buckets_, pos, capacity_), false);
    }
    
    // 插入新元素
    PairType new_pair(key, V(std::forward<Args>(args)...));
    RobinHoodInsert(std::move(new_pair));
    ++size_;
    
    // 查找插入后的位置
    pos = FindBucket(key);
    return std::make_pair(Iterator(buckets_, pos, capacity_), true);
}

template<typename K, typename V>
template<typename... Args>
std::pair<typename NHashMap<K, V>::Iterator, bool> NHashMap<K, V>::TryEmplace(K&& key, Args&&... args)
{
    ResizeIfNeeded();
    
    size_t pos = FindBucket(key);
    
    if (pos != capacity_ && buckets_[pos].occupied && 
        NEqual<K>{}(buckets_[pos].pair.Key, key))
    {
        // 键已存在，不插入
        return std::make_pair(Iterator(buckets_, pos, capacity_), false);
    }
    
    // 插入新元素
    PairType new_pair(std::move(key), V(std::forward<Args>(args)...));
    RobinHoodInsert(std::move(new_pair));
    ++size_;
    
    // 查找插入后的位置 (注意：key已被移动，需要用new_pair.Key)
    pos = FindBucket(new_pair.Key);
    return std::make_pair(Iterator(buckets_, pos, capacity_), true);
}

template<typename K, typename V>
bool NHashMap<K, V>::Erase(const K& key)
{
    size_t pos = FindBucket(key);
    
    if (pos == capacity_ || !buckets_[pos].occupied || 
        !NEqual<K>{}(buckets_[pos].pair.Key, key))
    {
        return false;
    }
    
    // 注销GC
    if constexpr (std::is_base_of_v<NObject, K>)
    {
        NGarbageCollector::GetInstance().UnregisterObject(&buckets_[pos].pair.Key);
    }
    if constexpr (std::is_base_of_v<NObject, V>)
    {
        NGarbageCollector::GetInstance().UnregisterObject(&buckets_[pos].pair.Value);
    }
    
    buckets_[pos].pair.~PairType();
    buckets_[pos].occupied = false;
    buckets_[pos].distance = 0;
    
    // Robin Hood 回溯
    ShiftBackward(pos);
    
    --size_;
    return true;
}

template<typename K, typename V>
typename NHashMap<K, V>::Iterator NHashMap<K, V>::Find(const K& key)
{
    size_t pos = FindBucket(key);
    
    if (pos != capacity_ && buckets_[pos].occupied && 
        NEqual<K>{}(buckets_[pos].pair.Key, key))
    {
        return Iterator(buckets_, pos, capacity_);
    }
    
    return End();
}

template<typename K, typename V>
typename NHashMap<K, V>::ConstIterator NHashMap<K, V>::Find(const K& key) const
{
    size_t pos = FindBucket(key);
    
    if (pos != capacity_ && buckets_[pos].occupied && 
        NEqual<K>{}(buckets_[pos].pair.Key, key))
    {
        return ConstIterator(buckets_, pos, capacity_);
    }
    
    return End();
}

template<typename K, typename V>
bool NHashMap<K, V>::Contains(const K& key) const
{
    return Find(key) != End();
}

template<typename K, typename V>
bool NHashMap<K, V>::operator==(const NHashMap& other) const
{
    if (size_ != other.size_) return false;
    
    for (auto it = Begin(); it != End(); ++it)
    {
        auto other_it = other.Find(it->Key);
        if (other_it == other.End() || !NEqual<V>{}(it->Value, other_it->Value))
        {
            return false;
        }
    }
    
    return true;
}

template<typename K, typename V>
bool NHashMap<K, V>::operator!=(const NHashMap& other) const
{
    return !(*this == other);
}

template<typename K, typename V>
bool NHashMap<K, V>::Equals(const NObject* other) const
{
    const NHashMap<K, V>* other_map = dynamic_cast<const NHashMap<K, V>*>(other);
    return other_map && (*this == *other_map);
}

template<typename K, typename V>
size_t NHashMap<K, V>::GetHashCode() const
{
    size_t hash = 0;
    for (auto it = Begin(); it != End(); ++it)
    {
        size_t pair_hash = NHash<K>{}(it->Key) ^ (NHash<V>{}(it->Value) << 1);
        hash ^= pair_hash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash;
}

template<typename K, typename V>
NString NHashMap<K, V>::ToString() const
{
    NString result = "{";
    bool first = true;
    
    for (auto it = Begin(); it != End(); ++it)
    {
        if (!first) result += ", ";
        first = false;
        
        if constexpr (std::is_base_of_v<NObject, K>)
        {
            result += it->Key.ToString();
        }
        else
        {
            result += NString::Format("{}", it->Key);
        }
        
        result += ": ";
        
        if constexpr (std::is_base_of_v<NObject, V>)
        {
            result += it->Value.ToString();
        }
        else
        {
            result += NString::Format("{}", it->Value);
        }
    }
    
    result += "}";
    return result;
}

// 内部实现
template<typename K, typename V>
size_t NHashMap<K, V>::Hash(const K& key) const
{
    return NHash<K>{}(key);
}

template<typename K, typename V>
size_t NHashMap<K, V>::FindBucket(const K& key) const
{
    if (capacity_ == 0) return capacity_;
    
    size_t hash_value = Hash(key);
    size_t pos = hash_value & (capacity_ - 1); // 假设capacity是2的幂次
    
    while (pos < capacity_ && buckets_[pos].occupied)
    {
        if (NEqual<K>{}(buckets_[pos].pair.Key, key))
        {
            return pos;
        }
        
        pos = (pos + 1) & (capacity_ - 1);
    }
    
    return pos;
}

template<typename K, typename V>
void NHashMap<K, V>::RobinHoodInsert(PairType&& pair)
{
    size_t hash_value = Hash(pair.Key);
    size_t pos = hash_value & (capacity_ - 1);
    size_t distance = 0;
    
    while (true)
    {
        if (!buckets_[pos].occupied)
        {
            // 找到空位，插入
            new (&buckets_[pos].pair) PairType(std::move(pair));
            buckets_[pos].distance = distance;
            buckets_[pos].occupied = true;
            
            if constexpr (std::is_base_of_v<NObject, K>)
            {
                NGarbageCollector::GetInstance().RegisterObject(&buckets_[pos].pair.Key);
            }
            if constexpr (std::is_base_of_v<NObject, V>)
            {
                NGarbageCollector::GetInstance().RegisterObject(&buckets_[pos].pair.Value);
            }
            
            break;
        }
        
        // Robin Hood: 如果当前元素距离理想位置更远，则替换
        if (distance > buckets_[pos].distance)
        {
            std::swap(pair, buckets_[pos].pair);
            std::swap(distance, buckets_[pos].distance);
        }
        
        pos = (pos + 1) & (capacity_ - 1);
        ++distance;
    }
}

template<typename K, typename V>
void NHashMap<K, V>::ResizeIfNeeded()
{
    if (capacity_ == 0)
    {
        capacity_ = DEFAULT_CAPACITY;
        InitializeBuckets();
    }
    else if (GetLoadFactor() >= max_load_factor_)
    {
        Resize(capacity_ * 2);
    }
}

template<typename K, typename V>
void NHashMap<K, V>::Resize(size_t new_capacity)
{
    NHashMap temp(new_capacity);
    
    for (size_t i = 0; i < capacity_; ++i)
    {
        if (buckets_[i].occupied)
        {
            temp.RobinHoodInsert(std::move(buckets_[i].pair));
            ++temp.size_;
        }
    }
    
    *this = std::move(temp);
}

template<typename K, typename V>
void NHashMap<K, V>::InitializeBuckets()
{
    if (capacity_ > 0)
    {
        buckets_ = AllocateElements<Bucket>(capacity_);
        for (size_t i = 0; i < capacity_; ++i)
        {
            new (buckets_ + i) Bucket();
        }
    }
}

template<typename K, typename V>
void NHashMap<K, V>::DeallocateBuckets()
{
    if (buckets_)
    {
        Clear();
        for (size_t i = 0; i < capacity_; ++i)
        {
            buckets_[i].~Bucket();
        }
        DeallocateElements(buckets_, capacity_);
        
        buckets_ = nullptr;
        capacity_ = 0;
        size_ = 0;
    }
}

template<typename K, typename V>
void NHashMap<K, V>::MoveFrom(NHashMap&& other) noexcept
{
    buckets_ = other.buckets_;
    size_ = other.size_;
    capacity_ = other.capacity_;
    max_load_factor_ = other.max_load_factor_;
    
    other.buckets_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
}

template<typename K, typename V>
void NHashMap<K, V>::CopyFrom(const NHashMap& other)
{
    if (other.size_ == 0)
    {
        Clear();
        return;
    }
    
    DeallocateBuckets();
    
    capacity_ = other.capacity_;
    max_load_factor_ = other.max_load_factor_;
    InitializeBuckets();
    
    for (size_t i = 0; i < other.capacity_; ++i)
    {
        if (other.buckets_[i].occupied)
        {
            Insert(other.buckets_[i].pair);
        }
    }
}

template<typename K, typename V>
void NHashMap<K, V>::ShiftBackward(size_t pos)
{
    size_t next_pos = (pos + 1) & (capacity_ - 1);
    
    while (next_pos != pos && buckets_[next_pos].occupied && buckets_[next_pos].distance > 0)
    {
        // 将下一个元素向前移动
        buckets_[pos] = std::move(buckets_[next_pos]);
        --buckets_[pos].distance;
        
        buckets_[next_pos].occupied = false;
        buckets_[next_pos].distance = 0;
        
        pos = next_pos;
        next_pos = (next_pos + 1) & (capacity_ - 1);
    }
}