#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    return PutUtils(key, value, false);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) { 
    return PutUtils(key, value, true);
}

bool SimpleLRU::PutUtils(const std::string &key, const std::string &value, bool ifAbsent){
    size_t val = key.size() + value.size();
    if(val > _max_size){
        return !ifAbsent;
    }

    lru_node* head = _lru_head.get(), *first = head->next.get();
    auto it = _lru_index.find(key);
    if(it == _lru_index.end()){
        FreeSize(val);
        
        if(tail_key == ""){
            tail_key = key;
        }
        _current_size += val;
        head->next = std::unique_ptr<lru_node>(new lru_node{key, value, head, std::move(head->next)});
        if(first){
            first->prev = head->next.get();
        }
        

        _lru_index.emplace(std::cref(head->next->key), std::ref(*(head->next.get())));
        return true;
    }

    lru_node* cur = &it->second.get();
    if(!ifAbsent) {
        size_t cur_size = cur->value.size() + cur->key.size();
        FreeSize(- cur_size + val);

        _current_size = _current_size - cur_size + val;
        cur->value = value;
    }

    if(key == tail_key){
        tail_key = cur->prev->key;
    }

    if(cur == first){
        return !ifAbsent;
    }

    std::unique_ptr<lru_node> t;
    t.swap(cur->prev->next);
    cur->prev->next.reset(cur->next.get());
    if(cur->next.get()){
        cur->next->prev = cur->prev;
    }

    t->next.release();
    t->next.reset(first);
    head->next.release();
    head->next.reset(cur);

    first->prev = cur;
    cur->prev = head;

    t.release();

    return !ifAbsent;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    size_t val = key.size() + value.size();
    if(val > _max_size){
        return false;
    }

    auto it = _lru_index.find(key);
    if(it == _lru_index.end()){
        return false;
    }
    lru_node* cur = &it->second.get();

    if(key == tail_key){
        tail_key = cur->prev->key;
    }

    size_t cur_size = cur->value.size() + cur->key.size();
    FreeSize(- cur_size + val);
    _current_size = _current_size - cur_size + val;
    cur->value = value;

    lru_node* head = _lru_head.get(), *first = head->next.get();

    if(cur == first){
        return true;
    }

    std::unique_ptr<lru_node> t;
    t.swap(cur->prev->next);
    cur->prev->next.reset(cur->next.get());
    if(cur->next.get()){
        cur->next->prev = cur->prev;
    }

    t->next.release();
    t->next.reset(first);
    head->next.release();
    head->next.reset(cur);

    first->prev = cur;
    cur->prev = head;

    t.release();
    return true;
}

void SimpleLRU::FreeSize(const size_t offset){
    while(_current_size + offset > _max_size) {
        lru_node *node = &_lru_index.find(tail_key)->second.get();
        _lru_index.erase(node->key);
        _current_size -= node->key.size() + node->value.size();
        tail_key = node->prev->key;
        node->prev->next = nullptr;
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto it = _lru_index.find(key);
    if(it == _lru_index.end()){
        return false;
    }
    lru_node* cur = &it->second.get();

    if(key == tail_key){
        tail_key = cur->prev->key;
    }
        
    _current_size -= key.size() + cur->value.size();
    _lru_index.erase(key);
    
    if(cur->next){
        cur->next->prev = cur->prev;
    }
    cur->prev->next = std::move(cur->next); 
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) { 
    auto it = _lru_index.find(key);
    if(it == _lru_index.end()){
        return false;
    }
    lru_node* cur = &it->second.get();

    value = cur->value;
    lru_node* head = _lru_head.get(), *first = head->next.get();
    if(cur == first){
        return true;
    }

    std::unique_ptr<lru_node> t;
    t.swap(cur->prev->next);
    cur->prev->next.reset(cur->next.get());
    if(cur->next.get()){
        cur->next->prev = cur->prev;
    }

    t->next.release();
    t->next.reset(first);
    head->next.release();
    head->next.reset(cur);

    first->prev = cur;
    cur->prev = head;

    t.release();
    return true; 
}

} // namespace Backend
} // namespace Afina
