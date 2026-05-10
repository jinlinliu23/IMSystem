#pragma once

#include <iostream>
#include <memory>
#include <mutex>

template <typename T>
class Singleton{
    //让子类调用基类的构造函数
protected:
    Singleton()=default;
    Singleton(const Singleton<T>&)=delete;
    Singleton& operator=(const Singleton<T>& st)=delete;

    static std::shared_ptr<T> _instance;
public:
    ~Singleton(){
        std::cout<<"this is singleton destruct"<<std::endl;
    }

    //不使用锁的操作
    static std::shared_ptr<T> GetInstance(){
        //只有在第一次调用的时候才会被初始化
        static std::once_flag s_flag;
        //call_once判断s_flag有没有被设置 被初始化标记；如果被标记，则不会调用传入的可调用对象
        std::call_once(s_flag,[&](){//在内部实现上使用了锁
            _instance=std::shared_ptr<T>(new T());
        });

        return _instance;
    }

    void PrintAddress(){
        std::cout<<_instance.get()<<std::endl;
    }
};

template<typename T>//只有在实例化的时候才会被检测到
std::shared_ptr<T> Singleton<T>::_instance=nullptr;