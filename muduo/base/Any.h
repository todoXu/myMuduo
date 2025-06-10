#pragma once
#include <memory>

namespace myMuduo {
namespace base {
class Any
{
public:
    Any() = default;
    ~Any() = default;

    Any(const Any &other)
        : dataPtr_(other.dataPtr_ ? other.dataPtr_->Clone() : nullptr)
    {
    }

    Any &operator=(const Any &other)
    {
        if (this != &other)
        {
            InternalData *temp = other.dataPtr_->Clone();
            dataPtr_.reset(temp);
        }
        return *this;
    }

    Any(Any &&other)
        : dataPtr_(std::move(other.dataPtr_))
    {
        other.dataPtr_ = nullptr;
    }

    Any &operator=(Any &&other)
    {
        if (this != &other)
        {
            dataPtr_ = std::move(other.dataPtr_);
            other.dataPtr_ = nullptr;
        }
        return *this;
    }

    template <typename ValueType>
    Any(const ValueType &value)
        : dataPtr_(new InternalDataImpl<ValueType>(value))
    {
    }

    bool isEmpty() const { return !dataPtr_; }

    void reset() { dataPtr_.reset(); }

    const std::type_info &type() const { return dataPtr_ ? dataPtr_->getType() : typeid(void); }

    template <typename T>
    T *cast()
    {
        if (dataPtr_ && dataPtr_->getType() == typeid(T))
        {
            return &(static_cast<InternalDataImpl<T> *>(dataPtr_.get())->data_);
        }
        return nullptr;
    }

    template <typename T>
    const T *cast() const
    {
        if (dataPtr_ && dataPtr_->getType() == typeid(T))
        {
            return &(static_cast<const InternalDataImpl<T> *>(dataPtr_.get())->data_);
        }
        return nullptr;
    }

private:
    class InternalData
    {
    public:
        InternalData() = default;
        virtual ~InternalData() = default;
        virtual const std::type_info &getType() const = 0;
        virtual InternalData *Clone() const = 0;
    };

    //实际存储数据的类模板
    template <typename T>
    class InternalDataImpl : public InternalData
    {
    public:
        InternalDataImpl(const T &data)
            : data_(data)
        {
        }
        InternalDataImpl(T &&data)
            : data_(std::move(data))
        {
        }
        const std::type_info &getType() const override { return typeid(T); }
        InternalData *Clone() const override { return new InternalDataImpl<T>(data_); }

        T data_;
    };

    std::unique_ptr<InternalData> dataPtr_;
};
}  // namespace base

}  // namespace myMuduo
