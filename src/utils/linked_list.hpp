#ifndef DANDELION_UTILS_LINKED_LIST_HPP
#define DANDELION_UTILS_LINKED_LIST_HPP

#include <cstddef>
#include <utility>
#include <type_traits>

/*!
 * \file utils/linked_list.hpp
 * \ingroup utils
 */

// ------------------- Declarations ----------------------

/*!
 * \~chinese
 * \brief 链表节点类型的基类。
 *
 * 若需要构造 `Node` 类型的链表，则 `Node` 类型应该继承这个类，
 * 形成 CRTP (Curiously Recurring Template Pattern) 结构，从而为 `Node`
 * 类添加侵入式链表所需的指针成员。
 *
 * \tparam Node 链表数据类型
 */
template<typename Node>
struct LinkedListNode
{
    LinkedListNode();
    Node* next_node;
    Node* prev_node;
};

/*!
 * \~chinese
 * \brief 侵入式双链表。
 *
 * 这个类实现了一个通用侵入式双链表，允许尾插入（插入到 tail 之后）
 * 和随机删除。
 *
 * \tparam Node 链表数据类型
 */
template<typename Node>
class LinkedList
{
    static_assert(std::is_base_of_v<LinkedListNode<Node>, Node>,
                  "Type Node must inherit from LinkedListNode<Node>");

public:
    LinkedList();
    ~LinkedList();
    /*! \~chinese 链表头指针（非头节点，数据有意义）。 */
    Node* head;
    /*! \~chinese 链表尾指针。 */
    Node* tail;
    /*! \~chinese 在链表末尾插入一个元素，使用 `std::forward` 转发参数原地构造，无需移动。 */
    template<typename... Args>
    Node* append(Args&&... args);
    /*! \~chinese 删除指针指向的节点。 */
    void erase(Node* node);
    /*! \~chinese 将指针指向的节点从链表中释放（删除），但不释放该节点的内存。 */
    Node* release(Node* node);
    /*! \~chinese 链表中存储的元素数量。 */
    std::size_t size;
};

// ------------------- Definitions ----------------------

template<typename Node>
LinkedListNode<Node>::LinkedListNode() : next_node(nullptr), prev_node(nullptr)
{
}

template<typename Node>
LinkedList<Node>::LinkedList() : head(nullptr), tail(nullptr), size(0)
{
}

template<typename Node>
LinkedList<Node>::~LinkedList()
{
    Node* node = this->head;
    while (node != nullptr) {
        Node* to_be_deleted = node;
        node                = node->next_node;
        delete to_be_deleted;
    }
}

template<typename Node>
template<typename... Args>
Node* LinkedList<Node>::append(Args&&... args)
{
    Node* new_node = new Node(std::forward<Args>(args)...);
    ++this->size;
    if (this->head == nullptr) {
        this->head = new_node;
        this->tail = new_node;
        return new_node;
    }
    this->tail->next_node = new_node;
    new_node->prev_node   = this->tail;
    this->tail            = new_node;
    return new_node;
}

template<typename Node>
void LinkedList<Node>::erase(Node* node)
{
    if (node == nullptr || !this->size) {
        return;
    }
    --this->size;
    // Erase the last element in the list
    if (!this->size) {
        delete this->head;
        this->head = nullptr;
        this->tail = nullptr;
        return;
    }
    Node* to_be_deleted = node;
    // Erasure for the head node
    if (node == this->head) {
        this->head            = this->head->next_node;
        this->head->prev_node = nullptr;
        delete to_be_deleted;
        return;
    }
    // Erasure for the tail node
    if (node == this->tail) {
        this->tail            = this->tail->prev_node;
        this->tail->next_node = nullptr;
        delete to_be_deleted;
        return;
    }
    // Otherwise a general erasure
    node->prev_node->next_node = node->next_node;
    node->next_node->prev_node = node->prev_node;
    delete to_be_deleted;
}

template<typename Node>
Node* LinkedList<Node>::release(Node* node)
{
    if (node == nullptr || !this->size) {
        return nullptr;
    }
    --this->size;
    if (!this->size) {
        // Erase the last element in the list
        this->head = nullptr;
        this->tail = nullptr;
    } else if (node == this->head) {
        // Erasure for the head node
        this->head            = this->head->next_node;
        this->head->prev_node = nullptr;
    } else if (node == this->tail) {
        // Erasure for the tail node
        this->tail            = this->tail->prev_node;
        this->tail->next_node = nullptr;
    } else {
        // Otherwise a general erasure
        node->prev_node->next_node = node->next_node;
        node->next_node->prev_node = node->prev_node;
    }
    return node;
}

#endif // DANDELION_UTILS_LINKED_LIST_HPP
