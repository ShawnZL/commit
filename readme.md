# join

```c++
void download1()
{
    cout << "开始下载第一个视频..." << endl;
    for (int i = 0; i < 100; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        cout << "下载进度:" << i << endl;
    }
    cout << "第一个视频下载完成..." << endl;
}

void download2()
{
    cout << "开始下载第二个视频..." << endl;
    for (int i = 0; i < 100; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        cout << "下载进度:" << i << endl;
    }
    cout << "第二个视频下载完成..." << endl;
}

int main()
{
    cout << "主线程开始运行\n";
    std::thread d2(download2);
    download1();

    return 0
}
```

这里是d2先完成，之后是d1完成。

先贴一下关于join()函数的解释：

> The function returns when the thread execution has completed.This synchronizes the moment this function returns with the completion of all the operations in the thread: This blocks the execution of the thread that calls this function until the function called on construction returns (if it hasn't yet).

总结理解一下就是两个关键点：

- **谁调用了这个函数？**调用了这个函数的线程对象，一定要等这个线程对象的方法（在构造时传入的方法）执行完毕后（或者理解为这个线程的活干完了！），这个join()函数才能得到返回。
- **在什么线程环境下调用了这个函数？**上面说了必须要等线程方法执行完毕后才能返回，那必然是阻塞调用线程的，也就是说如果一个线程对象在一个线程环境调用了这个函数，那么这个线程环境就会被阻塞，直到这个线程对象在构造时传入的**方法**执行完毕后，才能继续往下走，另外如果线程对象在调用join()函数之前，就已经做完了自己的事情（在构造时传入的方法执行完毕），那么这个函数不会阻塞线程环境，线程环境正常执行。

```c++
int main()
{
    cout << "主线程开始运行\n";
    std::thread d2(download2);
    download1();
    d2.join();
  	do_process(); // 在1，2都完成后，
}
```

因为 `download1()`一定会在主线程执行完毕，d2是另一个线程，它一定会会在自己没有完成时，阻止整个主线程，因为主线程需要12都完成才可以去执行 `do_process()` 函数。

- **谁调用了join()函数？**d2这个线程对象调用了join()函数，因此必须等待d2的下载任务结束了，d2.join()函数才能得到返回。
- **d2在哪个线程环境下调用了join()函数？**d2是在主线程的环境下调用了join()函数，因此主线程要等待d2的线程工作做完，否则主线程将一直处于block状态；这里不要搞混的是d2真正做的任务（下载）是在另一个线程做的，但是d2调用join()函数的动作是在主线程环境下做的。

**等待其他线程的线程，需要调用被等待线程对象的 `join()` 函数。**

# future

- future表示一个可能还没有实际完成的**异步任务的结果**，针对这个结果可以添加回调函数以便在任务执行成功或失败后做出对应的操作；（回调就是自己写了却不调用，给别人调用的函数）
- promise交由任务执行者，任务执行者**通过promise可以标记任务完成或者失败**；

按照我自己理解，promise就是一个信息传递机制。负责信息的传递。

传递信息的方式：

- 利用条件变量。在任务线程完成时调用`notify_one()`，在主函数中调用`wait()`；或者`notify_all()`。
- 利用flag（原子类型）。在任务完成时修改flag，在主线程中阻塞，不断轮询flag直到成功；

上面第一种上锁会带来一定开销，好处是适合长时间阻塞，第二种适合短时间阻塞。

那么c++11 future采用哪一种呢？答案是第二种，future内定义了一个原子对象，主线程通过自旋锁不断轮询，此外会进行sys_futex系统调用。futex是linux非常经典的同步机制，锁冲突时在用户态利用自旋锁，而需要挂起等待时到内核态进行睡眠与唤醒。

future/promise最强大的功能是：

- **获得结果返回值；**
- **处理异常**（如果任务线程发生异常）；
- **链式回调**（目前c++标准库不支持链式回调，不过folly支持）；

我们也可以提供拉起一个新的`std::thread`来获得结果返回值（通过返回指针），但这种写法很容易出错，举个例子：

```cpp
 #include <chrono>
 #include <thread>
 #include <iostream>
 
 void threadCompute(int* res)
 {
     std::this_thread::sleep_for(std::chrono::seconds(1));
     *res = 100;
 }
 
 int main()
 {
     int res;
     std::thread th1(threadCompute, 2, &res);
     th1.join();
     std::cout << res << std::endl;
     return 0;
 }
```

用std::thread的缺点是：

- 通过`.join`来阻塞，本文例子比较简单，但代码一长，线程一多，忘记调用`th1.join()`，就会捉襟见肘；
- 使用指针传递数据非常危险，因为互斥量不能阻止指针的访问，而且指针的方式要更改接口，比较麻烦 ；

通过future，future可以看成存储器，存储一个未来返回值。

先在主线程内创建一个promise对象，从promise对象中获得future对象；

再将promise引用传递给任务线程，在任务线程中对promise进行`set_value`，主线程可通过future获得结果。

`std::future.get()` 提供了一个重要方法就是`.get()`，这将阻塞主线程，直到future就绪。注意：`.get()`方法只能调用一次。

可以通过下面三个方式来获得`std::future`。

- `std::promise`的get_future函数
- `std::packaged_task`的get_future函数
- `std::async` 函数

## promise

```c++
 #include <iostream>
 #include <functional>
 #include <future>
 #include <thread>
 #include <chrono>
 #include <cstdlib>
 
 void thread_comute(std::promise<int>& promiseObj) {
     std::this_thread::sleep_for(std::chrono::seconds(1));
     promiseObj.set_value(100); // set_value后，future变为就绪。
 }

 int main() {
     std::promise<int> promiseObj;
     std::future<int> futureObj = promiseObj.get_future();
     std::thread t(thread_comute, std::ref(promiseObj)); 
     // 采用std::ref引用传值
     std::cout << futureObj.get() << std::endl; // 会阻塞
    
     t.join();
     return 0;
 }
```

## packaged_task

**`std::packaged_task` 本身和线程没啥关系，它只是一个关联了 `std::future` 的仿函数。**

`std::package_task`类似于`std::functional`，特殊的是，自动会把返回值可以传递给`std::future`。

`std::package_task`类似于`std::functional`，所以不会自动执行，需要显示的调用。

因为 `std::packaged_task` 对象是一个可调用对象， 可以：

- 封装在 std::function 对象中；
- 作为线程函数传递到 std::thread 对象中；
- 作为可调用对象传递另一个函数中；
- 可以直接进行调用 ；

我们经常用 std::packaged_task 打包任务， 并在它被传到别处之前的适当时机取回期望值。

```c++
#include <deque>
#include <mutex>
#include <future>
#include <thread>
#include <utility>

std::mutex m;
std::deque<std::packaged_task<void()> > tasks;

bool gui_shutdown_message_received();
void get_and_process_gui_message();

void gui_thread() // 1
{
     while(!gui_shutdown_message_received()) // 如果用户关闭界面，就退出
     {
        get_and_process_gui_message(); // get用户操作
        std::packaged_task<void()> task;
        {
            std::lock_guard<std::mutex> lk(m); // 上局部锁
            if(tasks.empty()) // 轮询直到不为空
                continue;
            task=std::move(tasks.front()); // 取FIFO任务队列第一个
            tasks.pop_front();
        }
        task(); // task是packaged_task，执行该任务，并把返回值给future对象
     }
}

std::thread gui_bg_thread(gui_thread); // 启动后台线程

template<typename Func>
std::future<void> post_task_for_gui_thread(Func f) 
{
    std::packaged_task<void()> task(f); // 作为回调函数
    std::future<void> res=task.get_future(); // 获得future对象
    std::lock_guard<std::mutex> lk(m);     
    tasks.push_back(std::move(task)); // 放入任务对列
    return res; // future对象后续将得到task的返回值
}
```

## async

`std::async`是模板函数，是C++标准更进一步的高级封装，用起来非常方便。将直接返回一个future对象。

```c++
int Sum_with_MultiThread(int from, int to, size_t thread_num) {
     int ret = 0;
     int n = thread_num ? (to - from) / thread_num : (to - from);
     std::vector<std::future<int64_t>> v;
     for (; from <= to; ++from) {
       v.push_back(std::async(Sum, from, from + n > to ? to : from + n));
       from += n;
     }
     for (auto &f : v) {
       ret += f.get();
     }
     return ret;
  }
```

此外，`std::async`比`std::thread`更安全！`std::thread`当创建太多线程时，会导致创建失败，进而程序崩溃。

而`std::async`就没有这个顾虑，为什么呢？这就要讲`std::async`的启动方式了，也就是`std::async`的第一个参数：`std::launch::deferred`【延迟调用】和`std::launch::async`【强制创建一个线程】。

1. `std::launch::deferred`:
   表示线程入口函数调用被延迟到`std::future`对象调用`wait()`或者`get()`函数 调用才执行。
   如果`wait()`和`get()`**没有调用**，则不会创建新线程，也不执行函数；
   如果调用`wait()`和`get()`，实际上**也不会创建新线程**，而是在主线程上继续执行；
2. `std::launch::async`:
   表示强制这个异步任务在 **新线程**上执行，在调用`std::async()`函数的时候就开始创建线程。
3. `std::launch::deferred|std::launch::async`:
   这里的“|”表示或者。如果没有给出launch参数，默认采用该种方式。
   操作系统会自行评估选择async or defer，如果系统资源紧张，则采用defer，就不会创建新线程。避免创建线程过长，导致崩溃。

嘶，async默认的launch方式将由操作系统决定，这样好处是不会因为开辟线程太多而崩溃，但坏处是这种不确定性会带来问题，参考[《effective modern c++》](https://zhuanlan.zhihu.com/p/349349488)：`这种不确定性会影响thread_local变量的不确定性，它隐含着任务可能不会执行，它还影响了基于超时的wait调用的程序逻辑`。

```c++
#include <iostream>
#include <future>
#include <thread>
#include <chrono>
int main() {
    std::cout << "Test 1 start" << std::endl;
    auto fut1 = std::async(std::launch::async, [] { std::this_thread::sleep_for(std::chrono::milliseconds(5000)); std::cout << "work done 1!\n";
        return 1;}); // 这一步没有阻塞，因为async的返回的future对象用于move构造了fut1，没有析构

    std::cout << "Work done - implicit join on fut1 associated thread just ended\n\n";

    std::cout << "Test 2 start" << std::endl;
    std::async(std::launch::async, [] { 
std::this_thread::sleep_for(std::chrono::milliseconds(5000)); std::cout << "work done 2!" << std::endl; });// 这一步竟然阻塞了！因为async返回future对象是右值，将要析构，而析构会阻塞
    std::cout << "This shold show before work done 2!?" << std::endl;
    return 0;
}

/*
Test 1 start
Work done - implicit join on fut1 associated thread just ended

Test 2 start
work done 2!
work done 1!
This shold show before work done 2!?
*/
```

## 异常处理

期望编程范式的一大好处是能够接住异常，这是`std::thread`不可比拟的优势

- `std::async`处理异常

future.get()可以获得async中的异常，外部套一个try/catch。至于是原始的异常对象， 还是一个拷贝，不同的编译器和库将会在这方面做出不同的选择 。

```cpp
 void foo()
 {
   std::cout << "foo()" << std::endl;
   throw std::runtime_error("Error");
 }
 
 int main()
 {
   try
   {
     std::cout << "1" << std::endl;
     auto f = std::async(std::launch::async, foo);
     f.get();
     std::cout << "2" << std::endl;
   }
   catch (const std::exception& ex)
   {
     std::cerr << ex.what() << std::endl;
   }
 }
```

- `std::packaged_task`处理异常

`std::packaged_task`与`std::async`一样，也是把异常传递给future对象，可以用上面一样的方式捕获。

- `std::promise`处理异常

`std::promise`处理异常与上面两者不同，当它存入的是一个异常而非一个数值时， 就需要调用set_exception()成员函数， 而非set_value()。这样future才能捕获异常。

```cpp
 try{
     some_promise.set_value(calculate_value());
 }
 catch(...){
     some_promise.set_exception(std::current_exception());
 } 
```

# 快排的并行计算版本

```c
template<typename T>
std::list<T> sequential_quick_sort(std::list<T> input) {
    if (input.empty()) {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    // splice 把input.begin()元素剪切到result.begin()位置
    T const& pivot = *result.begin();
    auto divide_point = std::partition(input.begin(), input.end(),
                                       [&](T const& t) { return t < pivot ; });
   // lambda 表达式 [&](T const& t) { return t < pivot ; } 是作为谓词函数传递给 std::partition 的。
   // 这个谓词函数会对容器中的每个元素进行评估，如果该元素小于 pivot，则返回 true，否则返回 false。

   std::list<T> lower_part;
   lower_part.splice(lower_part.end(), input, input.begin(), divide_point);
   //把input.begin()到divide_point，剪切到lower_part.end()处
   /* 异步运行版本
   auto new_lower(sequential_quick_sort(std::move(lower_part)));
   auto new_higher(sequential_quick_sort(std::move(input)));
   */
   std::future<std::list<T>> new_lower(
           std::async(sequential_quick_sort<T>, std::move(lower_part)));
   auto new_higher(sequential_quick_sort(std::move(input)));

    result.splice(result.end(), new_higher);  // 合并higher
    result.splice(result.begin(), new_lower.get());  // 合并lower
    return result;
}

```

# 引用常量

**其实就是不允许一个普通引用指向一个临时变量，只允许将“常量引用”指向临时对象。**

在函数参数中，使用常量引用非常重要。因为函数有可能接受临时对象，而且同时需要禁止对所引用对象的一切修改。

下面程序执行发生错误，因为不可以将一个字面值常量赋值给普通引用；函数的返回值如果是非引用类型时，实际上是作为一个临时变量返回的，经过上面的讨论，不允许一个普通引用指向临时对象。

```c
int test() {
	return 1;
}

void fun(int &x) {
    cout << x << endl;
}
 
int main()
{
	int m = 1;
	fun(m);         // ok
	fun(1);         // error
    fun(test());    // error
 
	return 0;
}
```

你的示例程序中的问题与你之前提到的问题是相关的。让我解释一下发生的情况：

在 C++ 中，普通的引用（非常量引用）需要绑定到一个具有持久性的对象。而字面值常量（如整数1）或者临时返回的值不具备持久性。因此，你无法将普通引用绑定到字面值常量或者临时对象上。

在你的代码中，`fun(1);` 和 `fun(test());` 都尝试将普通引用绑定到不具备持久性的临时对象上，因此会导致编译错误。

> 当你调用 `fun(1)` 时，发生以下步骤：
>
> 1. 编译器发现 `fun` 函数接受一个 `int` 引用作为参数。
> 2. 由于参数是一个非常量引用，它需要被绑定到一个具有持久性的对象。但是你提供了一个字面值常量 `1`，字面值常量没有持久性。
> 3. 编译器尝试将字面值常量 `1` 转换为一个可以被绑定的临时 `int` 对象。
> 4. 编译器会生成一个临时的匿名 `int` 变量，并将 `1` 的值赋给它。
> 5. 然后，编译器尝试将这个临时 `int` 对象传递给 `fun` 函数。
> 6. `fun` 函数的参数是一个非常量引用，但是它被传递的是一个临时对象，这是不允许的。
>
> 因此，调用 `fun(1)` 会导致编译错误，因为你尝试将一个非常量引用绑定到一个临时对象上。

如果你想在 `fun` 函数中使用临时对象，你可以将参数声明为 `const` 引用，这样就可以将临时对象传递给函数而不会产生错误：

```cpp
void fun(const int &x) {
    cout << x << endl;
}
```

这样的话，`fun(1);` 和 `fun(test());` 都会正常工作。
