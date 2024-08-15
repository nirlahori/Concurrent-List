#ifndef CONCURRENT_LIST2_HPP
#define CONCURRENT_LIST2_HPP
#include <memory>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <initializer_list>
#include <thread>
#include <tuple>

template<typename T>
struct Node{
    std::shared_timed_mutex shmtx;
    T data;
    std::unique_ptr<Node<T>> next;

    Node() : data() {}
    Node(T d) : data{d}, next{} {}
};


template<typename T>
class Concurrent_List
{
private:

    std::unique_ptr<Node<T>> head;
    mutable std::mutex headmtx;

    using value_type = T;
    using reference = value_type&;
    using const_reference = const value_type&;


public:

    Concurrent_List() : head{std::make_unique<Node<value_type>>()} {}

    Concurrent_List(const std::initializer_list<value_type>& _ilist) : head{std::make_unique<Node<value_type>>()} {
        for(auto elem : _ilist){
            push_back(elem);
        }
    }

    Concurrent_List(std::size_t count, const value_type& value) : head{std::make_unique<Node<value_type>>()} {
        for(int i=0; i<count; i++){
            push_back(value);
        }
    }

    Concurrent_List(std::size_t count) : Concurrent_List(count, value_type()){}

    Concurrent_List(const Concurrent_List& list){
        auto node = list.head.get();
        for(Node<value_type>* curr = node; curr != nullptr; curr = curr->next.get())
            push_back(curr->data);
    }


    void push_front(const value_type& data){
        std::unique_ptr<Node<value_type>> node_ptr = std::make_unique<Node<value_type>>(data);
        std::unique_lock headlk {head->shmtx};
        node_ptr->next = std::move(head);
        head = std::move(node_ptr);
    }

    void push_back(const value_type& data){
        std::unique_ptr<Node<value_type>> node_ptr = std::make_unique<Node<value_type>>();
        std::unique_lock prevlk {head->shmtx};
        auto curr_ptr = head.get();
        while(curr_ptr->next != nullptr){
            std::unique_lock curr_lock{curr_ptr->next->shmtx};
            curr_ptr = curr_ptr->next.get();
            prevlk.unlock();
            prevlk = std::move(curr_lock);
        }
        curr_ptr->data = data;
        curr_ptr->next = std::move(node_ptr);
    }

    void insert_after(int pos, const value_type& data){
        std::unique_ptr<Node<value_type>> node_ptr = std::make_unique<Node<value_type>>(data);
        std::unique_lock lock{head->shmtx};
        auto curr_ptr = head.get();
        int curr_pos = 0;
        while(curr_pos < pos && curr_ptr->next != nullptr){
            std::unique_lock curr_lock{curr_ptr->next->shmtx};
            curr_ptr = curr_ptr->next.get();
            lock.unlock();
            lock = std::move(curr_lock);
            curr_pos++;
        }
        std::unique_lock next_lock {curr_ptr->next->shmtx};
        auto old_next = std::move(curr_ptr->next);
        node_ptr->next = std::move(old_next);
        curr_ptr->next = std::move(node_ptr);
    }

    void pop_front(){
        std::unique_lock head_lk{head->shmtx};
        if(head->next == nullptr)
            throw std::runtime_error("pop_front() called on an empty list");
        std::unique_lock head_next_lk{head->next->shmtx};
        head_lk.unlock();
        std::unique_ptr<Node<value_type>> old_head = std::move(head);
        head = std::move(old_head->next);
        old_head.reset(nullptr);
    }

    void pop_back(){
        std::unique_lock prev_lk{head->shmtx};
        if(head->next == nullptr)
            throw std::runtime_error("pop_back() called on an empty list");
        Node<value_type>* curr_ptr = head.get();
        while(curr_ptr->next->next != nullptr){
            std::unique_lock curr_lock{curr_ptr->next->shmtx};
            prev_lk.unlock();
            curr_ptr = curr_ptr->next.get();
            prev_lk = std::move(curr_lock);
        }
        curr_ptr->data = value_type();
        curr_ptr->next.reset(nullptr);
    }

    void remove_after(int pos){

        std::unique_lock prev_lk{head->shmtx};
        if(head->next == nullptr)
            throw std::runtime_error("remove_after() called on an empty list");
        int curr_pos = 0;
        auto curr_ptr = head.get();
        while(curr_pos < pos && curr_ptr->next != nullptr){
            std::unique_lock curr_lock{curr_ptr->next->shmtx};
            prev_lk.unlock();
            curr_ptr = curr_ptr->next.get();
            prev_lk = std::move(curr_lock);
            curr_pos++;
        }
        std::unique_lock next_lock{curr_ptr->next->next->shmtx};
        auto old_node = std::move(curr_ptr->next);
        curr_ptr->next = std::move(old_node->next);
    }

    std::size_t size(){
        std::unique_lock headlk{headmtx};
        //std::unique_lock taillk{tailmtx};
        int count = 0;
        Node<value_type>* ptr = head.get();
        for(Node<value_type>* curr = ptr; curr != nullptr; curr = curr->next.get())
            count++;
        return count;
    }

    bool empty(){
        return !size();
    }


    void print(){

        std::unique_lock headlk{headmtx};
//        std::lock_guard taillk{tailmtx};

        Node<value_type>* ptr = head.get();
        for(Node<value_type>* curr=ptr; curr->next != nullptr; curr=curr->next.get()){
            std::cout << curr->data << "\t";
            if(headlk)
                headlk.unlock();
        }
        std::cout << std::endl;
    }

    value_type front() const{
        std::lock_guard headlk{head->shmtx};
        if(head->next == nullptr)
            throw std::runtime_error("front() called on an empty list");
        return head->data;
    }

    value_type back() const{
        std::unique_lock lock{head->shmtx};
        if(head->next == nullptr)
            throw std::runtime_error("back() called on an empty list");
        auto curr_ptr = head.get();
        while(curr_ptr->next->next != nullptr){
            std::unique_lock curr_lock{curr_ptr->next->shmtx};
            curr_ptr = curr_ptr->next.get();
            lock.unlock();
            lock = std::move(curr_lock);
        }
        return curr_ptr->data;
    }

    ~Concurrent_List(){
    }
};


#endif // CONCURRENT_LIST2_HPP
