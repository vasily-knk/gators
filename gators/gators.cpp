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
struct gator_root
    : gator<T>
{
    virtual void set_value(T const &value) = 0;
};

template<typename T>
using gator_root_ptr = shared_ptr<gator_root<T>>;

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

template<typename T>
gator_root_ptr<T> create_gator_root(T const &val)
{
    return make_shared<gator_root_impl<T>>(val);
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
    //std::function<Dst(Srcs...)> f = conv;
    //return make_shared<gator_link_impl<Dst, Srcs...>>(f, source...);
    return nullptr;
}

template<typename... Srcs>
void fuck(gator_ptr<Srcs>... source)
{}

template<typename T>
struct foo
{
    
};

template<typename T>
struct bar
    : foo<T>
{
    
};

template<typename T>
struct baz
    : bar<T>
{
    
};

template<typename... Srcs>
void aaaa(shared_ptr<foo<Srcs>>... args)
{
    
}

int main()
{
    auto b1 = make_shared<baz<string>>();
    auto b2 = make_shared<baz<int>>();

    aaaa(b1, b2);
}

int main1()
{
    auto root = make_shared<gator_root_impl<string>>();


    scoped_connection conn = root->subscribe_value_changed([root]()
    {
        cout << "Value changed: " << root->value() << endl;
    });

    root->set_value("Hello!");

    auto root2 = create_gator_root(7);


    auto conv = [](string const &str, int mult) 
    {
        return int(str.length()) * mult;
    };

    gator_ptr<string> r1(root);
    gator_ptr<int> r2(root2);
    
    auto link = create_gator_link<int>(conv, r1, r2);
    fuck(root, root2);

    scoped_connection link_conn = link->subscribe_value_changed([link]()
    {
        cout << "Multiplied value: " << link->calculate_value() << endl;
    });

    root->set_value("aa");

    return 0;
}