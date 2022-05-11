// Copyright (C) 2022  ilobilo

#pragma once

template<typename func>
class function;

template<typename retval, typename ...Args>
class function<retval(Args...)>
{
    public:
    function() { };

    template<typename func>
    function(func t) : callable_(new callableFunc<func>(t)) { }

    ~function()
    {
        this->clear();
    }

    void clear()
    {
        if (this->callable_ == nullptr) return;
        delete this->callable_;
    }

    template<typename func>
    function &operator=(func t)
    {
        if (this->callable_ != nullptr) delete this->callable_;
        this->callable_ = new callableFunc<func>(t);
        return *this;
    }

    retval operator()(Args ...args) const
    {
        if (this->callable_ == nullptr) return retval();
        return this->callable_->invoke(args...);
    }

    bool operator==(bool set)
    {
        return (this->callable_ != nullptr) == set;
    }

    private:
    class callable
    {
        public:
        virtual ~callable() = default;
        virtual retval invoke(Args...) = 0;
    };

    template<typename func>
    class callableFunc : public callable
    {
        public:
        callableFunc(const func &t) : t_(t) { }
        ~callableFunc() override = default;

        retval invoke(Args ...args) override
        {
            return t_(args...);
        }

        private:
        func t_;
    };

    callable *callable_;
};

namespace std
{
    template<typename func>
    using function = ::function<func>;
}