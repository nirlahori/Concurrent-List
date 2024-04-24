#ifndef CONCURRENT_LIST_HPP
#define CONCURRENT_LIST_HPP

#include <memory>
#include <iostream>
#include <mutex>
#include <string>
#include <initializer_list>

struct Node{
    int data;
    std::unique_ptr<Node> next;

    Node() : data() {}
    Node(int d) : data{d}, next{} {}
};


class Concurrent_List
{
private:

    std::unique_ptr<Node> head;
    Node* tail = nullptr;
    mutable std::mutex headmtx;
    mutable std::mutex tailmtx;

    using value_type = int;
    using reference = value_type&;
    using const_reference = const value_type&;


public:

    Concurrent_List() = default;

    Concurrent_List(const std::initializer_list<int>& _ilist){
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
        for(Node* curr = node; curr != nullptr; curr = curr->next.get())
            add_node_back(curr->data);

    }


    void add_node_front(const int& data){
        std::unique_ptr<Node> node_ptr = std::make_unique<Node>(data);
        std::unique_lock headlk {headmtx};
        std::unique_lock taillk {tailmtx};
        if(head.get() != tail){
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
        }
    }

    void add_node_back(const int& data){
        std::unique_ptr<Node> node_ptr = std::make_unique<Node>(data);
        std::unique_lock headlk {headmtx};
        std::unique_lock taillk {tailmtx};

        if(head.get() != tail){
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
        }
    }

    void add_node(const int& data, int pos){
        std::unique_ptr<Node> node_ptr = std::make_unique<Node>(data);
        std::unique_lock headlk {headmtx};
    }


    void delete_node_front(){
        std::lock_guard lock{headmtx};
        if(!head)
            return;
        else{
            std::unique_ptr<Node> n = std::move(head);
            head = std::move(n->next);
        }
    }

    void delete_node_back(){
        std::unique_lock headlk{headmtx};
        std::lock_guard taillk{tailmtx};
        Node* ptr = head.get();
        if(!head)
            return;
        else if(ptr == tail){
            head.reset(nullptr);
            tail = nullptr;
        }
        else{
            while(ptr->next.get() != tail){
                ptr = ptr->next.get();
                if(headlk)
                    headlk.unlock();
            }
            ptr->next.reset(nullptr);
            tail = ptr;
        }
    }

    std::size_t size(){
        std::unique_lock headlk{headmtx};
        std::unique_lock taillk{tailmtx};
        int count = 0;
        Node* ptr = head.get();
        for(Node* curr = ptr; curr != nullptr; curr = curr->next.get())
            count++;
        return count;
    }

    bool empty(){
        return !size();
    }


    void print(){

        std::unique_lock headlk{headmtx};
        std::lock_guard taillk{tailmtx};

        Node* ptr = head.get();
        for(Node* curr=ptr; curr != nullptr; curr=curr->next.get()){
            std::cout << curr->data << "\t";
            if(headlk)
                headlk.unlock();
        }
        std::cout << std::endl;
    }

    reference front(){
        std::unique_lock headlk{headmtx};
        return head.get()->data;
    }

    const_reference front() const{
        std::unique_lock headlk{headmtx};
        return head.get()->data;
    }


    ~Concurrent_List(){
    }
};

#endif // CONCURRENT_LIST_HPP
