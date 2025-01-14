#ifndef CONCURRENT_DOUBLE_LIST_HPP
#define CONCURRENT_DOUBLE_LIST_HPP

#include <memory>
#include <iostream>
#include <mutex>
#include <string>
#include <initializer_list>
#include <shared_mutex>


template<typename T>
struct Node{
    std::shared_timed_mutex shmtx;
    T data;
    std::unique_ptr<Node<T>> next;
    std::unique_ptr<Node<T>> prev;
    Node() = default;
    Node(T d) : data{d}, next{}, prev{} {}
};


template<typename T>
class Concurrent_List
{

public:
    using value_type = T;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = value_type*;
    using const_iterator = const value_type*;

private:

    std::unique_ptr<Node<T>> head = nullptr;
    std::unique_ptr<Node<T>> tail = nullptr;
    mutable std::shared_timed_mutex headmtx;
    mutable std::shared_timed_mutex tailmtx;

	bool is_tail(Node<T>* ptr){
        return ptr == tail.get();
	}

	void insert_at_back(std::unique_ptr<Node<value_type>>  new_node){
		tail->next.reset(new_node.get());
		new_node->prev = std::move(tail);
		new_node->next = nullptr;
		tail = std::move(new_node);
	}

	void insert_at_front(std::unique_ptr<Node<value_type>>  new_node){
		new_node->prev = nullptr;
		head->prev.reset(new_node.get());
		new_node->next = std::move(head);
		head = std::move(new_node);
	}

	void remove_at_front(){
		auto old_head = std::move(head);
		head = std::move(old_head->next);
		head->prev.release();
	}

	void remove_at_back(){
		auto old_tail = std::move(tail);
		tail = std::move(old_tail->prev);
		tail->next.release();
	}

    void insert_between(std::unique_ptr<Node<value_type>>  new_node, Node<value_type>* first, Node<value_type>* second){
        new_node->prev.reset(second->prev.release());
        new_node->next.reset(first->next.release());
        first->next.reset(new_node.get());
        second->prev = std::move(new_node);
    }


    // Utility function for testing purpose
    void print(){

        std::unique_lock headlk {headmtx};
        std::lock_guard taillk {tailmtx};

        Node<value_type>* ptr = head.get();
        for(Node<value_type>* curr=ptr; curr != nullptr; curr=curr->next.get()){
            std::cout << curr->data << "\t";
            if(headlk)
                headlk.unlock();
        }
        std::cout << std::endl;
    }



public:

    Concurrent_List() = default;

    Concurrent_List(const std::initializer_list<value_type>& _ilist){
        for(auto elem : _ilist){
            add_node_back(elem);
        }
    }

    Concurrent_List(std::size_t count, const value_type& value){
        for(std::size_t i{0}; i<count; i++){
            add_node_back(value);
        }
    }

    Concurrent_List(std::size_t count) : Concurrent_List(count, value_type()){}

    Concurrent_List(const Concurrent_List& list){
        std::unique_lock srclk_head {list.headmtx};
        std::unique_lock srclk_tail {list.tailmtx};

        auto node{list.head.get()};
        for(Node<value_type>* curr {node}; curr != nullptr; curr = curr->next.get())
           add_node_back(curr->data);
    }


    void add_node_front(const value_type& data){
        std::unique_ptr<Node<value_type>> node_ptr {std::make_unique<Node<value_type>>(data)};
        std::unique_lock headlk {headmtx};
        std::unique_lock taillk {tailmtx};
        if(!head){
            head = std::move(node_ptr);
            tail.reset(head.get());
            head->next = nullptr;
            head->prev = nullptr;
        }
        else if(!is_tail(head.get()) && !is_tail(head->next.get())){
            taillk.unlock();
            insert_at_front(std::move(node_ptr));
        }
        else{
            insert_at_front(std::move(node_ptr));
        }
    }

    void add_node_back(const value_type& data){
        std::unique_ptr<Node<value_type>> node_ptr {std::make_unique<Node<value_type>>(data)};
        std::unique_lock headlk {headmtx};
        std::unique_lock taillk {tailmtx};

        if(!head){
            head = std::move(node_ptr);
            tail.reset(head.get());
            head->next = nullptr;
            head->prev = nullptr;
        }
        else if(!is_tail(head.get()) && !is_tail(head->next.get())){
            headlk.unlock();
			insert_at_back(std::move(node_ptr));
        }
        else{
            insert_at_back(std::move(node_ptr));
        }
    }

    void insert_after(const value_type& data, std::size_t pos){
        std::unique_ptr<Node<value_type>> node_ptr {std::make_unique<Node<value_type>>(data)};
        std::unique_lock lock {headmtx};

        if(!head){
            return; // Implement the appropriate functionality
        }
        auto curr_ptr = head.get();
        std::size_t curr_pos {0};
        std::unique_lock taillk {tailmtx};

        if(is_tail(head.get())){
            if(pos == 0)
                return insert_at_back(std::move(node_ptr));
            else
                return;
        }

        while(curr_pos < pos){
            std::unique_lock nxlock {curr_ptr->next->shmtx};
            if(is_tail(curr_ptr->next.get())){
                if(pos == (curr_pos + 1)){
                    return insert_at_back(std::move(node_ptr));
            	}
                else if(curr_pos == (pos - 1)){
                    return insert_between(std::move(node_ptr), curr_ptr, tail.get());
            	}
                else{
                    return;
            	}
            }
            curr_ptr = curr_ptr->next.get();
            lock = std::move(nxlock);
            curr_pos++;
        }
        taillk.unlock();
        std::unique_lock nxlock {curr_ptr->next->shmtx};
        insert_between(std::move(node_ptr), curr_ptr, curr_ptr->next.get());
    }

    void delete_after(std::size_t pos){
        std::unique_lock lock {headmtx};

        if(!head){
            return;
        }
        auto curr_ptr {head.get()};
        std::size_t curr_pos {0};
        std::unique_lock taillk {tailmtx};

        if(is_tail(head.get())){
            if(pos == 0)
                return remove_at_back();
            else
                return;
        }

        while(curr_pos < pos){
            std::unique_lock nxlock {curr_ptr->next->shmtx};
            if(is_tail(curr_ptr->next.get())){
                return;
            }
            curr_ptr = curr_ptr->next.get();
            lock = std::move(nxlock);
            curr_pos++;
        }
        if(is_tail(curr_ptr->next.get())){
            return remove_at_back();
        }
        std::unique_lock<std::shared_timed_mutex> fwd_lock;
        {
            std::lock_guard old_lock {curr_ptr->next->shmtx};
            fwd_lock = std::move(std::unique_lock(curr_ptr->next->next->shmtx));
            if(!is_tail(curr_ptr->next->next.get()))
                taillk.unlock();
        }
        curr_ptr->next->next->prev.release();
        auto old_node = std::move(curr_ptr->next);
        curr_ptr->next = std::move(old_node->next);
        curr_ptr->next->prev = std::move(old_node->prev);
    }


    void delete_node_front(){
        std::unique_lock headlock {headmtx};
        std::unique_lock taillock {tailmtx};
        if(!head)
            return;
        else if(!is_tail(head.get()) && !is_tail(head->next.get())){
            taillock.unlock();
            std::unique_lock nxlock {head->next->shmtx};
            remove_at_front();
        }
        else{
            if(is_tail(head.get())){
                tail.release();
                head.reset(nullptr);
            }
            else{
                remove_at_front();
            }
        }
    }

    void delete_node_back(){
        std::unique_lock headlock {headmtx};
        std::unique_lock taillock {tailmtx};
        
        if(!head)
            return;
        else if(!is_tail(head.get()) && !is_tail(head->next.get())){
            headlock.unlock();
            std::unique_lock nxlock {tail->prev->shmtx};
            remove_at_back();
		}
        else{
            if(is_tail(head.get())){
                head.release();
                tail.reset(nullptr);
            }
            else{
                remove_at_back();
            }
        }
    }

    std::size_t size(){
        std::size_t count {0};
        std::unique_lock headlk {headmtx};
        std::unique_lock taillk {tailmtx};

        Node<value_type>* ptr {head.get()};
        for(Node<value_type>* curr {ptr}; curr != nullptr; curr = curr->next.get())
            ++count;
        return count;
    }

    bool empty(){
        return !size();
    }



    value_type front() const{
        std::lock_guard headlk {headmtx};
        if(!head)
            throw std::runtime_error("front() called on an empty list");
        return head.get()->data;
    }

    value_type back() const{
        std::lock_guard taillk {tailmtx};
        if(!tail)
            throw std::runtime_error("back() called on an empty list");
        return tail->data;
    }


    ~Concurrent_List(){
        std::size_t index {0};
        std::size_t sz {size()};
        for(index=0; index<sz; ++index){
    		delete_node_back();
    	}
    }
};



#endif // CONCURRENT_DOUBLE_LIST_HPP
