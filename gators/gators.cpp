#include "common/event.h"
#include "cpp_utils/default_constructors.h"
#include "cpp_utils/bind_mem.h"
#include "cpp_utils/apply_tuple.h"


template<typename T>
struct gator
{
    virtual ~gator() {};

    virtual T const &value() const = 0;

    DECLARE_EVENT(value_changed, ());
};

template<typename T>
using gator_ptr = shared_ptr<gator<T>>;

template<typename T>
gator_ptr<T> fuck(gator_ptr<T> p)
{
    return p;
}

template<typename T>
struct gator_root
    : gator<T>
{
    virtual void set_value(T const &value) = 0;
};

template<typename T>
using gator_root_ptr = shared_ptr<gator_root<T>>;

template<typename T>
gator_ptr<T> fuck(gator_root_ptr<T> p)
{
    return p;
}


template<typename T>
struct gator_root_impl
    : gator_root<T>
{
    ENABLE_NO_COPY(gator_root_impl)

    gator_root_impl() {};

    explicit gator_root_impl(T const &value)
        : value_(value)
    {
        
    }


    T const& value() const override
    {
        return value_;
    }

    void set_value(T const& value) override
    {
        value_ = value;
        value_changed_signal_();
    }
private:
    T value_;
};


template<typename T, typename... Args>
gator_root_ptr<T> create_gator_root(Args &&... args)
{
    return make_shared<gator_root_impl<T>>(std::forward<Args>(args)...);
}



template<typename T>
struct gator_link
{
    virtual ~gator_link() = default;

    virtual T calculate_value() const = 0;

    DECLARE_EVENT(value_changed, ());
};

template<typename T>
using gator_link_ptr = shared_ptr<gator_link<T>>;

template<typename T>
struct gator_source_t
{
    explicit gator_source_t(gator_ptr<T> source, std::function<void()> const &handler)
        : source(source)
        , conn(source->subscribe_value_changed(handler))
    {}

    gator_ptr<T> source;
    scoped_connection conn;
};

template<typename T>
gator_source_t<T> make_gator_source(gator_ptr<T> source, std::function<void()> const &handler)
{
    return gator_source_t<T>(source, handler);
}


template<typename T>
T const &extract_gator_source_value(gator_source_t<T> const &source)
{
    return source.source->value();
}

template<typename Dst, typename... Srcs>
using gator_conv_f = std::function<Dst(Srcs const &...)>;

template<typename Dst, typename... Srcs>
struct gator_link_impl
    : gator_link<Dst>
{
    typedef gator_conv_f<Dst, Srcs...> conv_f;
    typedef std::tuple<gator_source_t<Srcs>...> sources_t;
    
    explicit gator_link_impl(conv_f const &conv, gator_ptr<Srcs>... source)
        : conv_(conv)
        , sources_(make_gator_source(source, BIND_MEM(on_value_changed))...)
    {
        
    }

    Dst calculate_value() const override
    {
        struct executor
        {
            explicit executor(gator_link_impl const *self)
                : self(self)
            {}

            Dst operator()(gator_source_t<Srcs> const &... sss) const
            {
                return self->conv_(extract_gator_source_value(sss)...);
            }
            
            gator_link_impl const *self;
        };
        
        std::function<Dst(gator_source_t<Srcs> const &...)> f = executor(this);
        
        return cpp_utils::apply(f, sources_);
    }

private:
    void on_value_changed()
    {
        value_changed_signal_();
    }


private:
    conv_f conv_;
    sources_t sources_;
};

template<typename Dst, typename Conv, typename... Srcs>
gator_link_ptr<Dst> create_gator_link(Conv const &conv, gator_ptr<Srcs>... source)
{
    std::function<Dst(Srcs...)> f = conv;
    return make_shared<gator_link_impl<Dst, Srcs...>>(f, source...);
}

template<typename T>
struct gator_baby_impl
    : gator<T>
{
    typedef gator_link_ptr<T> link_ptr;
    
    template<typename Conv, typename... SrcGators>
    explicit gator_baby_impl(Conv const &conv, SrcGators &&... source)
        : link_(create_gator_link<T>(conv, fuck(std::forward<SrcGators>(source))...))
        , value_(link_->calculate_value())
        , conn_(link_->subscribe_value_changed(BIND_MEM(on_link_value_changed)))
    {
        
    }

    T const& value() const override
    {
        return value_;
    }

private:
    void on_link_value_changed()
    {
        value_ = link_->calculate_value();
        value_changed_signal_();
    }

private:
    link_ptr link_;
    T value_;
    scoped_connection conn_;
};


template<typename T, typename Conv, typename... SrcGators>
gator_ptr<T> create_gator_baby(Conv const &conv, SrcGators &&... src_gators)
{
    return make_shared<gator_baby_impl<T>>(conv, std::forward<SrcGators>(src_gators)...);
}


int main()
{
    auto a = create_gator_root<string>("Hello");
    auto b = create_gator_root<int>(4);

    auto c = create_gator_baby<int>([](string const &s, int i)
    {
        return int(s.length()) * i;
    }, a, b);

    auto d = create_gator_baby<string>([](int n)
    {
        std::stringstream ss;
        for (size_t i = 0; i < n; ++i)
            ss << "A";

        return ss.str();
    }, c);

    auto conn = d->subscribe_value_changed([d]()
    {
        cout << "New value: " << d->value() << endl;
    });

    cout << d->value() << endl;

    b->set_value(2);
    a->set_value("Hi!");

    return 0;
}