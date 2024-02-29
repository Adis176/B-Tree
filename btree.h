#pragma once

#include <cstddef>
#include <cstring>
#include <functional>
#include <iostream>
#include <optional>

#include "buffer/buffer_manager.h"
#include "common/defer.h"
#include "common/macros.h"
#include "storage/segment.h"

#define UNUSED(p)  ((void)(p))

using namespace std;

namespace buzzdb {

template<typename KeyT, typename ValueT, typename ComparatorT, size_t PageSize>
struct BTree : public Segment {
    struct Node {

        /// The level in the tree.
        uint16_t level;

        /// The number of children.
        uint16_t count;

        // Constructor
        Node(uint16_t level, uint16_t count)
            : level(level), count(count) {}

        /// Is the node a leaf node?
        bool is_leaf() const { return level == 0; }
        optional<uint64_t> parent;
    };

    struct InnerNode: public Node {
        /// The capacity of a node.
        /// TODO think about the capacity that the nodes have.
        static constexpr uint32_t kCapacity = (PageSize / (sizeof(KeyT) + sizeof(ValueT))) - 2;

        /// The keys.
        KeyT keys[kCapacity];

        /// The children. Increase size by 1 as comapred to keys, as children between the keys, and can have values greater than/less than too.
        uint64_t children[kCapacity+1];

        /// Constructor.
        InnerNode() : Node(0, 0) {}

        /// Get the index of the first key that is not less than than a provided key.
        /// @param[in] key          The key that should be searched.
        std::pair<uint32_t, bool> lower_bound(const KeyT &key) {
            pair<uint32_t , bool> ans;
            int low=0;
            int high=this->count - 1;
            optional<uint32_t> pos;
            if(high==1 && !ComparatorT()(this->keys[0], key)){
                ans.first = 0;
                ans.second = true;
                return ans;
            }
            while (low<=high) {
                int m=((high-low)/2) + low;
                if(key==this->keys[m]) {
                    ans.first = static_cast<uint32_t>(m);
                    ans.second = true;
                    return ans;
                } 
                else if(key <= this->keys[m]) {
                    pos = m;
                    high = m-1;
                } 
                else{
                    low=m+1;
                }
            }
            
            if(pos){
                ans.first = *pos;
                ans.second = true;
                return ans;
            }
            ans.first = 0;
            ans.second=false;
            return ans;
        }

        /// Insert a key.
        /// @param[in] key          The separator that should be inserted.
        /// @param[in] split_page   The id of the split page that should be inserted.
        void insert(const KeyT &key, uint64_t split_page, const optional<uint64_t> &insertNew) {
 	    // TODO: remove the below lines of code 
    	// and add your implementation here
            vector<KeyT> keysNew;
            vector<uint64_t> childNew;
            optional<int> pos;
            for (int i=0; i<this->count-1; i++) {
                keysNew.push_back(this->keys[i]);
                childNew.push_back(this->children[i]);
                if(!ComparatorT()(this->keys[i], key) && !pos) pos = i;
                this->keys[i] = 0;
                this->children[i] = 0;
            }


            childNew.push_back(this->children[this->count - 1]);
            this->children[this->count - 1] = 0;
            if(!pos) {
                keysNew.insert(keysNew.begin() + (this->count - 1), key);
                childNew.insert(childNew.begin() + this->count, split_page);
            } 
            else{
                keysNew.insert(keysNew.begin() + *pos, key);
                if(insertNew) pos = *pos + 1;
                childNew.insert(childNew.begin() + *pos, split_page);
            }

            for(int i=0;i < static_cast<int>(keysNew.size()); i++) this->keys[i] = keysNew[i];
            for(int i=0;i < static_cast<int>(childNew.size()); i++) this->children[i] = childNew[i];

            this->count++;

        }

        /// Split the node.
        /// @param[in] buffer       The buffer for the new page.
        /// @return                 The separator key.
        KeyT split(std::byte* buffer) {
        // TODO: remove the below lines of code 
    	// and add your implementation here
            auto middle = (this->kCapacity + 1) / 2;
            auto addInner = reinterpret_cast<InnerNode*>(buffer);
            int start = 0;

            auto sep = static_cast<int>((this->keys[middle] + this->keys[middle-1])/2);
            for(int i=middle ;i<static_cast<int>(this->kCapacity)+1; i++){
                if(start == 0){
                    addInner->keys[start] = this->keys[i];
                    addInner->children[start] = this->children[i];
                    this->keys[i] = 0;
                    this->children[i] = 0;
                    this->count--;
                    start++;
                    i++;
                    addInner->children[start] = this->children[i];
                    this->children[i] = 0;
                    this->count--;
                    start++;
                } 
                else{
                    addInner->keys[start - 1] = this->keys[i - 1];
                    addInner->children[start] = this->children[i];
                    this->keys[i - 1] = 0;
                    this->children[i] = 0;
                    this->count--;
                    start++;
                }
            }
            addInner->count = start;
            return sep;
		//UNUSED(buffer);
        }

        /// Returns the keys.
        /// Can be implemented inefficiently as it's only used in the tests.
        std::vector<KeyT> get_key_vector() {
        // TODO
            vector<KeyT> kv;
            for(size_t i=0; i<kCapacity; i++){
                kv.push_back(keys[i]);
            }
            return kv;
        }

        /// Returns the child page ids.
        /// Can be implemented inefficiently as it's only used in the tests.
        std::vector<uint64_t> get_child_vector() {
        // TODO
            vector<uint64_t> cv;
            for(int i=0; i<=kCapacity; i++){
                cv.push_back(children[i]);
            }
            return cv;
        }
    };

    struct LeafNode: public Node {
        /// The capacity of a node.
        /// TODO think about the capacity that the nodes have.
        static constexpr uint32_t kCapacity = (PageSize / (sizeof(KeyT) + sizeof(ValueT))) - 2;;

        /// The keys.
        KeyT keys[kCapacity];

        /// The values.
        ValueT values[kCapacity];

        /// Constructor.
        LeafNode() : Node(0, 0) {}

        /// Insert a key.
        /// @param[in] key          The key that should be inserted.
        /// @param[in] value        The value that should be inserted.
        void insert(const KeyT &key, const ValueT &value) {
        //TODO
            if(this->count == 0){
                this->keys[this->count] = key;
                this->values[this->count] = value;
                this->count++;
            } 
            else{
                /// sort keys and shift keys (including values) wrt incoming key
                vector<KeyT> keysNew;
                vector<KeyT> tempValues;
                optional<int> pos;
                for(int i=0; i<this->count; i++){
                    keysNew.push_back(this->keys[i]);
                    tempValues.push_back(this->values[i]);
                    if(!ComparatorT()(this->keys[i], key) && !pos)  pos = i;
                    this->keys[i] = 0;
                    this->values[i] = 0;
                }

                long unsigned int temp;
                if(!pos){
                    pos = this->count;
                    temp = *pos;
                    temp--;
                }
                else temp=*pos;

                if(keysNew[temp]== key){
                    tempValues[temp]=value;
                }
                else{
                    keysNew.insert(keysNew.begin()+*pos, key);
                    tempValues.insert(tempValues.begin() +*pos, value);
                    this->count++;
                }

                for(int i=0; i<static_cast<int>(keysNew.size()); i++){
                    this->values[i] = tempValues[i];
                    this->keys[i] = keysNew[i];
                }
            }
        }


        void erase(int &pos) {
            // delete, we do that by making curr element val = next element val, and like making last element null, deleting it
            for(int i=pos; i<this->count; i++){
                this->values[i] = this->values[i + 1];
                this->keys[i] = this->keys[i + 1];
            }
            this->values[this->count]=0;
            this->keys[this->count]=0;
            this->count--;
        }

        /// Split the node.
        /// @param[in] buffer       The buffer for the new page.
        /// @return                 The separator key.
        KeyT split(std::byte* buffer) {
        // TODO
            auto middle = (this->kCapacity + 1) / 2;
            auto addLeaf = reinterpret_cast<LeafNode*>(buffer);
            int start = 0;
            for(int i=middle; i < static_cast<int>(this->kCapacity);i++){
                addLeaf->keys[start] = this->keys[i];
                addLeaf->values[start] = this->values[i];
                this->keys[i] = 0;
                this->values[i] = 0;
                this->count--;
                start++;
            }
            addLeaf->count =start;
            return this->keys[start-1];
        }

        std::vector<KeyT> get_key_vector() {
        // TODO
            vector<KeyT> kv;
            for(size_t i=0; i<kCapacity; i++){
                kv.push_back(keys[i]);
            }
            return kv;
        }


        std::vector<ValueT> get_value_vector() {
        // TODO
            vector<ValueT> vv;
            for(size_t i=0; i<kCapacity; i++){
                vv.push_back(values[i]);
            }
            return vv;
        }
    };

    optional<uint64_t> root;
    bool Occupied;
    uint16_t levelTree = 0;
    unordered_map<KeyT, bool> removed;

    uint64_t nextID;

    BTree(uint16_t segment_id, BufferManager &buffer_manager) : Segment(segment_id, buffer_manager) {
        this->Occupied = false;
    }

    /// Lookup an entry in the tree.
    /// @param[in] key      The key that should be searched.
    optional<ValueT> lookup(const KeyT &key){
        // if found in deleted keys, or tree doesn exist, just return
        optional<ValueT> found;
        if (this->removed.find(key) != this->removed.end() || !this->root){
            return found;
        }
        auto ID = *this->root;
        int next = 0;
        uint64_t previousID = ID;
        while(true){
            auto& curr = this->buffer_manager.fix_page(ID, false);
            auto trav = reinterpret_cast<Node*>(curr.get_data());
            if(trav->is_leaf()){
                auto leafNow = reinterpret_cast<LeafNode*>(trav);
                long unsigned int l1=0, h1=leafNow->count-1, m1=0;
                while(l1 <= h1){
                    m1 = (h1-l1)/2 + l1;
                    if(leafNow->keys[m1]==key){
                        found = leafNow->values[m1];
                        return found;
                    }
                    else if(leafNow->keys[m1]>key){
                        h1=m1-1;
                    }
                    else{
                        l1=m1+1;
                    }
                }
                ID = previousID;
                next++;
            } 
            else{
                auto innerNode = reinterpret_cast<InnerNode*>(trav);
                if (trav->parent){
                    previousID = *trav->parent;
                }
                pair<uint32_t, bool> lower_bound = innerNode->lower_bound(key);
                if(lower_bound.second == true){
                    if(innerNode->count <next) return found;
                    else{
                        ID = innerNode->children[lower_bound.first+next];
                        next = 0;
                    }
                } 
                else{
                    ID = innerNode->children[innerNode->count-1];
                }
            }
            this->buffer_manager.unfix_page(curr, false);
        }
        return found;

	//UNUSED(key);
	//return optional<ValueT>();
    }

    /// Erase an entry in the tree.
    /// @param[in] key      The key that should be searched.
    void erase(const KeyT &key) {
    // TODO
        if(this->root) {
            auto ID=*this->root;
            int next =0;
            uint64_t previousID = ID;
            while (true){
                auto& curr = this->buffer_manager.fix_page(ID, true);
                auto trav = reinterpret_cast<Node*>(curr.get_data());
                if (trav->is_leaf()){
                    auto leafNow = reinterpret_cast<LeafNode*>(trav);
                    for (int i=0; i<leafNow->count; i++){
                        if (leafNow->keys[i]==key){
                            leafNow->erase(i);
                            this->removed.insert({key,true});
                            this->buffer_manager.unfix_page(curr,true);
                            return;
                        }
                    }
                    ID = previousID;
                    next++;
                    this->buffer_manager.unfix_page(curr, true);
                } 
                else{
                    auto innerNode=reinterpret_cast<InnerNode*>(trav);
                    if(trav->parent) previousID = *trav->parent;
                    

                    pair<uint32_t, bool> lower_bound = innerNode->lower_bound(key);
                    if(lower_bound.second == true){
                        if(innerNode->count<next){
                            this->buffer_manager.unfix_page(curr, true);
                            return;
                        } 
                        else{
                            ID = innerNode->children[lower_bound.first+next];
                            next = 0;
                            this->buffer_manager.unfix_page(curr, true);
                        }
                    } 
                    else{
                        ID = innerNode->children[innerNode->count - 1];
                        this->buffer_manager.unfix_page(curr, true);
                    }
                }
            }
        }
    }

    /// Inserts a new entry into the tree.
    /// @param[in] key      The key that should be inserted.
    /// @param[in] value    The value that should be inserted.
    void insert(const KeyT &key, const ValueT &value) {
        if(!this->Occupied) {
            this->root = 0;
            this->nextID = 1;
            this->Occupied = true;
        }

        auto temp2 = *this->root;

        while (true) {
            auto& curr = this->buffer_manager.fix_page(temp2,true);
            auto trav = reinterpret_cast<Node*>(curr.get_data());
            
            /// When just one node - root becomes leaf, do oprations wrt that
            if(trav->is_leaf() ){
                auto leafNow = static_cast<LeafNode*>(trav);
                
                // Now two scenarios 
                // 1. space present - just push
                // 2. Else, we create new node (new leaf), have to create separator, increase level by one 
                if(leafNow->count < leafNow->kCapacity){
                    
                    leafNow->insert(key, value);
                    this->buffer_manager.unfix_page(curr, true);
                    break;
                } 
                else{

                    /// create new leaf node
                    auto leafPageID = this->nextID;
                    this->nextID++;
                    auto& addLeaf_page = this->buffer_manager.fix_page(leafPageID, true);
                    KeyT sep = leafNow->split(reinterpret_cast<byte *>(addLeaf_page.get_data()));
                    auto new_node = reinterpret_cast<Node*>(addLeaf_page.get_data());
                    auto addLeaf = static_cast<LeafNode*>(new_node);

                    if(!ComparatorT()(sep, key)) leafNow->insert(key, value);
                    else addLeaf->insert(key, value);

                    /// check if current node has a parent or not
                    // If no parent
                    if(leafNow->parent){
                        auto& parPage = this->buffer_manager.fix_page(*leafNow->parent, true);
                        auto par = reinterpret_cast<Node *>(parPage.get_data());
                        auto parInner = static_cast<InnerNode *>(par);
                        
                        optional<uint64_t> insertNew;
                        if(leafNow->values[leafNow->count -1] <addLeaf->values[0]) insertNew = temp2;
                        parInner->insert(sep, leafPageID, insertNew);
                        addLeaf->parent = *leafNow->parent;
                        this->buffer_manager.unfix_page(parPage, true);
                    } 
                    else{
                        this->root = this->nextID;
                        this->nextID++;
                        auto& parPageNew = this->buffer_manager.fix_page(*this->root, true);
                        auto parNodeNew = static_cast<InnerNode *>(reinterpret_cast<Node *>(parPageNew.get_data()));

                        parNodeNew->level = ++levelTree;
                        parNodeNew->keys[0] = sep;
                        parNodeNew->children[0] = temp2;
                        parNodeNew->children[1] = leafPageID;
                        parNodeNew->count = 2+parNodeNew->count;
                        leafNow->parent = *this->root;
                        addLeaf->parent = *this->root;
                        this->buffer_manager.unfix_page(parPageNew, true);
                        this->buffer_manager.unfix_page(addLeaf_page, true);
                    }
                    this->buffer_manager.unfix_page(curr, true);
                    break;
                }
            } 

            // when current node is not leaf - we are at some inner node
            else{
                auto innerNode = static_cast<InnerNode*>(trav);
                /// creating new node if capacity overflows
                if(innerNode->count ==innerNode->kCapacity + 1){
                    auto innerPageID = this->nextID;
                    this->nextID++;
                    auto& addInner_page = this->buffer_manager.fix_page(innerPageID, true);
                    KeyT sep = innerNode->split(reinterpret_cast<std::byte *>(addInner_page.get_data()));
                    auto new_node = reinterpret_cast<Node*>(addInner_page.get_data());
                    auto addInner = static_cast<InnerNode*>(new_node);
                    addInner->level = innerNode->level;

                    for (int i=0; i<addInner->count; i++){
                        auto& child = this->buffer_manager.fix_page(addInner->children[i], true);
                        auto child_node = reinterpret_cast<Node*>(child.get_data());
                        auto child_innerNode = static_cast<InnerNode*>(child_node);
                        child_innerNode->parent = innerPageID;
                    }

                    /// same as root-leaf, check if parent present or not, create or pass separator accordingly.
                    // 1. if parent not present, new node -> increases level, 
                    if(innerNode->parent){
                        auto& parPage = this->buffer_manager.fix_page(*innerNode->parent, true);
                        auto par = reinterpret_cast<Node *>(parPage.get_data());
                        auto parInner = static_cast<InnerNode *>(par);
                        optional<uint64_t> insertNew;
                        parInner->insert(sep, innerPageID, insertNew);
                        addInner->parent = *innerNode->parent;
                        pair<uint32_t, bool> lower_bound = parInner->lower_bound(key);
                        if(lower_bound.second == true) temp2 = parInner->children[lower_bound.first];
                        else temp2 = parInner->children[parInner->count - 1];
                        this->buffer_manager.unfix_page(parPage, true);
                        
                    } 
                    else{
                        this->root = this->nextID;
                        this->nextID++;
                        auto& parPageNew = this->buffer_manager.fix_page(*this->root, true);
                        auto parNodeNew = reinterpret_cast<Node *>(parPageNew.get_data());
                        auto parInnerNew = static_cast<InnerNode *>(parNodeNew);
                        parInnerNew->level = ++levelTree;
                        parInnerNew->keys[0] = sep;
                        parInnerNew->children[0] = temp2;
                        parInnerNew->children[1] = innerPageID;
                        parInnerNew->count += 2;
                        innerNode->parent = *this->root;
                        addInner->parent = *this->root;

                        this->buffer_manager.unfix_page(parPageNew, true);

                        pair<uint32_t, bool> lower_bound = parInnerNew->lower_bound(key);
                        if(lower_bound.second == true) temp2 = parInnerNew->children[lower_bound.first];
                        else temp2 = parInnerNew->children[parInnerNew->count - 1];
                    }
                    this->buffer_manager.unfix_page(addInner_page, true);
                } 
                else{

                    /// if we are not at correct node, use previously defined lower bound function - in order to find correct node
                    // lower bound - fi returns true, then we find a plausible value, move to that value.
                    // else if we didnt find any particular value, means no value present in greater than curr value
                    // so we beyond the last value - going to last node present.
                    pair<uint32_t, bool> lower_bound = innerNode->lower_bound(key);
                    if(lower_bound.second ==true) temp2 = innerNode->children[lower_bound.first];
                    else temp2 = innerNode->children[innerNode->count - 1];
                }
                this->buffer_manager.unfix_page(curr, true);
            }
        }
    }
};

}  
