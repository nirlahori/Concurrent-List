#include "concurrent_list.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <tuple>

template<template<class T> class C, typename V>
void add_front(C<V>& cont){
    int i=0;
    for(;;){
        std::cout << "pushing value at front -> " << std::this_thread::get_id() << std::endl;
        cont.add_node_front(i);
        i++;
    }
}

template<template<class T> class C, typename V>
void del_front(C<V>& cont){
    int i=0;
    for(;;){
        std::cout << "removing value at front: " << i << " by id " << std::this_thread::get_id() << std::endl;
        cont.delete_node_front();
        i++;
    }
}

template<template<class T> class C, typename V>
void add_back(C<V>& cont){
    int i=0;
    for(;;){
        std::cout << "pushing value at back ->" << std::this_thread::get_id() << std::endl;
        cont.add_node_back(i);
        i++;
    }
}

template<template<class T> class C, typename V>
void del_back(C<V>& cont){
    int i=0;
    for(;;){
        std::cout << "removing value at back: " << i << std::endl;
        cont.delete_node_back();
        i++;
    }
}

template<template<class T> class C, typename V>
void insert(C<V>& cont, int start, int end, int value){
    int i=start;
    while(i<end){
        std::cout << "inserting node -> " << std::this_thread::get_id() << std::endl;
        cont.insert_after(value, i);
        i++;
    }
}

template<template<class T> class C, typename V>
void remove(C<V>& cont, int pos){
    while(true){
        std::cout << "removing node -> " << std::this_thread::get_id() << std::endl;
        cont.delete_after(pos);
    }
}

template<typename ... T, std::size_t ... In>
Concurrent_List<int> init(std::index_sequence<In...> seq, std::tuple<T...> tup){
    return Concurrent_List<int>{std::get<In>(tup)...};
}

int main(){

    Concurrent_List<int> clist(500, 78);

    std::thread th1{insert<Concurrent_List, int>, std::ref(clist), 340, 490, 13};
    std::thread th2{insert<Concurrent_List, int>, std::ref(clist), 100, 300, 64};
    std::thread th3{del_front<Concurrent_List, int>, std::ref(clist)};

    std::thread th4{add_back<Concurrent_List, int>, std::ref(clist)};
    std::thread th5{remove<Concurrent_List, int>, std::ref(clist), 450};
    std::thread th6{remove<Concurrent_List, int>, std::ref(clist), 240};

    std::thread th7{add_back<Concurrent_List, int>, std::ref(clist)};
    std::thread th8{del_front<Concurrent_List, int>, std::ref(clist)};
    std::thread th9{add_front<Concurrent_List, int>, std::ref(clist)};

    th1.join();
    th2.join();
    th3.join();

    th4.join();
    th5.join();
    th6.join();

    th7.join();
    th8.join();
    th9.join();

    return 0;
}
