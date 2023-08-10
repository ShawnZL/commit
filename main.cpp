//#include <chrono>
//#include <thread>
//#include <iostream>
//
//using namespace std;
//void download1()
//{
//    cout << "开始下载第一个视频..." << endl;
//    for (int i = 0; i < 100; ++i) {
//        std::this_thread::sleep_for(std::chrono::milliseconds(50));
//        cout << "d1下载进度:" << i << endl;
//    }
//    cout << "第一个视频下载完成..." << endl;
//}
//
//void download2()
//{
//    cout << "开始下载第二个视频..." << endl;
//    for (int i = 0; i < 100; ++i) {
//        std::this_thread::sleep_for(std::chrono::milliseconds(30));
//        cout << "d2下载进度:" << i << endl;
//    }
//    cout << "第二个视频下载完成..." << endl;
//}
//
//void threadCompute(int* res)
//{
//    std::this_thread::sleep_for(std::chrono::seconds(1));
//    *res = 100;
//}
//
//
//int main() {
//
//    cout << "主线程开始运行\n";
//    std::thread d2(download2);
//    download1();
//    d2.join();
//
//    int res;
//    std::thread th1(threadCompute, &res);
//    th1.join();
//    std::cout << res << std::endl;
//    return 0;
//}

//

#include <iostream>
#include <future>
#include <thread>
#include <chrono>
#include <list>
//int main() {
//    std::cout << "Test 1 start" << std::endl;
//    auto fut1 = std::async(std::launch::async, [] { std::this_thread::sleep_for(std::chrono::milliseconds(5000)); std::cout << "work done 1!\n";
//        return 1;}); // 这一步没有阻塞，因为async的返回的future对象用于move构造了fut1，没有析构
//
//    std::cout << "Work done - implicit join on fut1 associated thread just ended\n\n";
//
//    std::cout << "Test 2 start" << std::endl;
//    std::async(std::launch::async, [] { std::this_thread::sleep_for(std::chrono::milliseconds(5000)); std::cout << "work done 2!" << std::endl; });// 这一步竟然阻塞了！因为async返回future对象是右值，将要析构，而析构会阻塞
//    std::cout << "This shold show before work done 2!?" << std::endl;
//    return 0;
//}

template<typename T>
std::list<T> sequential_quick_sort(std::list<T> input) {
    if (input.empty()) {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    // splice 把input.begin()元素剪切到result.begin()位置
    T const& pivot = *result.begin(); // 常量引用 这样可以在不复制元素的情况下使用 pivot，并在比较操作中进行引用传递。这对于排序操作来说是有效的，因为在排序过程中只是进行比较，而不是修改元素的值。
    // 返回一个迭代器
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
    result.splice(result.begin(), new_lower.get());  // 合并lower，不能忘记.get，同时你期待的类型就是放在std::future之后
    return result;
}

int main() {
    std::list<int> l = {1, 3, 6, 2, 7, 4, 5};
    std::list<int> ans = sequential_quick_sort(l);
    return 0;
}
