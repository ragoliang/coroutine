#include <iostream>
#include <coroutine>
#include <future>
#include <unistd.h>
#include <string>
using  namespace  std;

// 协程返回类型
template<typename T>
class Future {
public:
    class promise_type {
    public:
        promise_type() = default;

        ~promise_type() = default;

        auto get_return_object()
        {
            return Future<T>{handle_type::from_promise(*this)};
        }

        //! 协程初始化后调用的方法，返回值为std::suspend_always
        //! 表示协程启动后立即挂起（不执行第一行协程函数的代码），返回std::suspend_never 表示协程启动后不立即挂起。
        auto initial_suspend()
        {
            std::cout << "start coroutine" << std::endl;
            return std::suspend_never{};
            //return std::suspend_always{};
        }

        //! 调用co_return 后会调用
        void return_value(T v)
        {
            return_data_ = v;
        }

        //! 调用co_yield后会调用
        auto yield_value(T v)
        {
            std::cout << "yield_value invoked." << std::endl;
            return_data_ = v;
            return std::suspend_always{};
        }

        //! 在协程最后退出后调用的接口。
        //! 若 final_suspend 返回 std::suspend_always 则需要用户自行调用
        //! handle.destroy() 进行销毁，但注意final_suspend被调用时协程已经结束
        auto final_suspend() noexcept
        {
            std::cout << "final_suspend invoked." << std::endl;
            return std::suspend_always{};
        }

        void unhandled_exception()
        {
            std::exit(1);
        }

        T return_data_;
    };

    using handle_type = std::coroutine_handle<promise_type>;
public:
    explicit Future(handle_type h) : m_coro_handler(h)
    {

    }

    Future(const Future &t) = delete;

    Future(Future &&t) noexcept
    {
        m_coro_handler = t.coro_;
        t.m_coro_handler = nullptr;
    }

    ~Future()
    {
        if (m_coro_handler) {
            m_coro_handler.destroy();
        }
    }

    Future &operator=(const Future &t) = delete;

    Future &operator=(Future &&t) noexcept
    {
        m_coro_handler = t.m_coro_handler;
        t.m_coro_handler = nullptr;
        return *this;
    }

    bool resume()
    {
        std::cout << "resume" << std::endl;
        m_coro_handler.resume();
        std::cout << "resume done" << std::endl;
        return m_coro_handler.done();
    }

    T get()
    {
        return m_coro_handler.promise().return_data_;
    }

public:
    handle_type m_coro_handler;
};

/*
template<typename T>
class Awaiter {
public:
    Awaiter() = default;

    ~Awaiter() = default;

    // 等待体是否准备好了，没准备好（return false）就调用await_suspend
    bool await_ready()
    {
        return false;
    }

    // 等待体挂起如何操作。参数为调用协程的句柄。return true ，或者 return void 就会挂起协程
    void await_suspend(std::coroutine_handle<> awaiting)
    {

    }

    // 协程挂起后恢复时，调用的接口，同时返回其结果，作为co_await的返回值
    T await_resume()
    {
        return m_result;
    }

public:
    T m_result{};
    std::coroutine_handle<> m_awaiting;  // 协程句柄
};
 */

auto http_connect(string &data)
{
    struct awaitable{
        awaitable(string *data) : m_data(data) {}
        ~awaitable(){}
        bool await_ready() { return false; }
        void await_suspend(std::coroutine_handle<> h)
        {
            sleep(2);
            std::cout << "await_suspend" << std::endl;
        }
        void await_resume() {
            std::cout << "await_resume" << std::endl;
            *m_data = "http1.1/";
        }
        string* m_data;
    };
    return awaitable{&data};
}

// 协程函数
Future<string> start_coroutine()
{
    std::cout << "Coroutine started on thread: " << std::this_thread::get_id() << '\n';
    // 这种co_yield和resume方式，代码看起来和同步方式有一些差距
    /*
    std::cout << "call co_yield, coroutine yield" << std::endl;
    co_yield 10;
    std::cout << "coroutine wake up" << std::endl;
    co_return 11;
     */

    // co_await方式的引入,使得异步逻辑同步化，co_await expr，expr返回值需要是awaitable对象
    string data;
    co_await http_connect(data);
    std::cout << "Coroutine resumed on thread: " << std::this_thread::get_id() << '\n';
    co_return data;
}



int main()
{
    std::cout << "main thread: " << std::this_thread::get_id() << '\n';
    // 启动一个协程
    auto task = start_coroutine();
    // task被挂起后才会继续执行
    std::cout << "main" << endl;

    //std::cout << "task.get()=" << task.get() << std::endl;
    //task.resume();
    //std::cout << "after resume, task.get()=" << task.get() << std::endl;
    while (true) {
        sleep(2);
        // 恢复协程
        task.resume();
        cout << "task.get()=" << task.get() << endl;
        break;
    }
    return 0;
}
