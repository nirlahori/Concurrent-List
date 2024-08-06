#ifndef CONCURRENT_LIST2_HPP
#define CONCURRENT_LIST2_HPP
#include <memory>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <initializer_list>
#include <forward_list>
#include <thread>

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
            add_node_back(elem);
        }
    }

    Concurrent_List(std::size_t count, const value_type& value){
        for(int i=0; i<count; i++){
            add_node_back(value);
        }
    }

    Concurrent_List(std::size_t count) : Concurrent_List(count, value_type()){}

    Concurrent_List(const Concurrent_List& list){
        std::unique_lock srclk_head{list.headmtx};
        std::unique_lock srclk_tail{list.tailmtx};

        auto node = list.head.get();
        for(Node<value_type>* curr = node; curr != nullptr; curr = curr->next.get())
            add_node_back(curr->data);

    }


    void add_node_front(const value_type& data){
        std::unique_ptr<Node<value_type>> node_ptr = std::make_unique<Node<value_type>>(data);
        std::unique_lock headlk {head->shmtx};
        node_ptr->next = std::move(head);
        head = std::move(node_ptr);

        /*if(head.get() != tail){
            taillk.unlock();
            node_ptr->next = std::move(head);
            head = std::move(node_ptr);
        }
        else if(!head){
            head = std::move(node_ptr);
            tail = head.get();
        }
        else{
            node_ptr->next = std::move(head);
            head = std::move(node_ptr);
        }*/
    }

    void add_node_back(const value_type& data){
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


        /*if(head.get() != tail){
            headlk.unlock();
            tail->next = std::move(node_ptr);
            tail = tail->next.get();
        }
        else if(!head){
            head = std::move(node_ptr);
            tail = head.get();
        }
        else{
            tail->next = std::move(node_ptr);
            tail = tail->next.get();
        }*/
    }

    void add_node_after(int pos, const value_type& data){
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

    void delete_node_front(){
        std::unique_lock head_lk{head->shmtx};
        if(head->next == nullptr)
            return;
        std::unique_lock head_next_lk{head->next->shmtx};
        head_lk.unlock();
        std::unique_ptr<Node<value_type>> old_head = std::move(head);
        head = std::move(old_head->next);
        old_head.reset(nullptr);
    }

    /*void delete_node_back(){
        std::unique_lock headlk{head->shmtx};
        if(head->next == nullptr)
            return;

        std::unique_lock prev_lk{head->next->shmtx};
        Node<value_type>* curr_ptr = head->next.get();
        if(curr_ptr->next == nullptr){
            //std::unique_ptr<Node<value_type>> n = std::move(curr_ptr);
            prev_lk.unlock();
            head->next.reset(nullptr);
        }
        else{
            headlk.unlock();
            std::unique_lock curr_lock{curr_ptr->next->shmtx, std::defer_lock};
            while(curr_ptr->next->next != nullptr){
                curr_lock.lock();
                prev_lk.unlock();
                curr_ptr = curr_ptr->next.get();
                prev_lk = std::move(curr_lock);
                curr_lock = std::move(std::unique_lock(curr_ptr->next->shmtx, std::defer_lock));
            }
            //std::lock_guard lock_next{curr_ptr->next->shmtx};
            //std::unique_ptr<Node<value_type>> n = std::move(curr_ptr->next);
            curr_ptr->data = value_type();
            curr_ptr->next.reset(nullptr);
            //curr_ptr->next = nullptr;

        }

        // while(curr_ptr->next->next != nullptr){
        //     curr_ptr = curr_ptr->next;
        // }
        // std::unique_lock curr_lock{curr_ptr->shmtx};
        // std::unique_lock tail_lock{curr_ptr->next->shmtx};

        // std::unique_ptr<Node<value_type>> n = std::move(curr_ptr->next);
        // curr_ptr->next = nullptr;

        // else if(ptr == tail){
        //     head.reset(nullptr);
        //     tail = nullptr;
        // }
        // else{
        //     while(ptr->next.get() != tail){
        //         ptr = ptr->next.get();
        //         if(headlk)
        //             headlk.unlock();
        //     }
        //     ptr->next.reset(nullptr);
        //     tail = ptr;
        // }
    }*/


    void delete_node_back(){
        std::unique_lock prev_lk{head->shmtx};
        if(head->next == nullptr)
            return;
        Node<value_type>* curr_ptr = head.get();
        //std::unique_lock curr_lock{curr_ptr->next->shmtx, std::defer_lock};
        while(curr_ptr->next->next != nullptr){
            std::unique_lock curr_lock{curr_ptr->next->shmtx, std::defer_lock};
            curr_lock.lock();
            prev_lk.unlock();
            curr_ptr = curr_ptr->next.get();
            prev_lk = std::move(curr_lock);
            //curr_lock = std::move(std::unique_lock(curr_ptr->next->shmtx, std::defer_lock));
        }
        curr_ptr->data = value_type();
        curr_ptr->next.reset(nullptr);
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
        for(Node<value_type>* curr=ptr; curr != nullptr; curr=curr->next.get()){
            std::cout << curr->data << "\t";
            if(headlk)
                headlk.unlock();
        }
        std::cout << std::endl;
    }

    value_type front() const{
        std::lock_guard headlk{headmtx};
        if(!head)
            throw std::runtime_error("front() called on an empty list");
        return head.get()->data;
    }

  /*  value_type back() const{
        std::lock_guard taillk{tailmtx};
        if(!tail)
            throw std::runtime_error("back() called on an empty list");
        return tail->data;
    }*/

    ~Concurrent_List(){
    }
};






#endif // CONCURRENT_LIST2_HPP
