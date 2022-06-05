// Copyright (C) 2022  ilobilo

#include <lib/alloc.hpp>
#include <lib/lock.hpp>
#include <cdi/lists.h>

extern "C"
{
    cdi_list_t cdi_list_create()
    {
        auto list = malloc<list_t*>(sizeof(list_t));
        list->head = nullptr;
        list->size = 0;
        list->tail = nullptr;
        return list;
    }

    void cdi_list_destroy(cdi_list_t list)
    {
        if (list == nullptr) return;

        auto curr = list->head;
        while (curr)
        {
            auto next = curr->next;
            delete curr;
            curr = next;
        }
        list->head = list->tail = nullptr;
        list->size = 0;
        delete list;
    }

    cdi_list_t cdi_list_push(cdi_list_t list, void *value)
    {
        if (list == nullptr) list = cdi_list_create();

        auto item = malloc<list_node_t*>(sizeof(list_node_t));
        item->value = value;

        if (list->head == nullptr)
        {
            list->head = item;
            item->prev = nullptr;
            item->next = nullptr;
            list->tail = item;
        }
        else
        {
            item->next = list->head;
            item->prev = nullptr;
            list->head->prev = item;
            list->head = item;
        }

        list->size++;
        return list;
    }

    void *cdi_list_pop(cdi_list_t list)
    {
        if (list == nullptr || list->size == 0 || list->head == nullptr) return nullptr;

        auto ret = list->head->value;

        if (list->head == list->tail)
        {
            delete list->head;
            list->head = list->tail = nullptr;
        }
        else
        {
            auto curr = list->head;
            list->head = list->head->next;
            delete curr;
        }

        list->size--;
        return ret;
    }

    size_t cdi_list_empty(cdi_list_t list)
    {
        if (list == nullptr) return 0;
        return list->size == 0 || list->head == nullptr;
    }

    void *cdi_list_get(cdi_list_t list, size_t index)
    {
        if (list == nullptr) return nullptr;

        auto curr = list->head;
        for (size_t i = 0; i < index && curr != nullptr; i++) curr = curr->next;

        if (curr == nullptr) return nullptr;
        return curr->value;
    }

    cdi_list_t cdi_list_insert(cdi_list_t list, size_t index, void *value)
    {
        if (list == nullptr) list = cdi_list_create();
        if (index == 0) return cdi_list_push(list, value);

        auto item = malloc<list_node_t*>(sizeof(list_node_t));
        item->value = value;

        auto curr = list->head;
        for (size_t i = 0; i < index - 1 && curr != nullptr; i++) curr = curr->next;

        if (curr == nullptr)
        {
            delete item;
            return list;
        }

        item->next = curr->next;
        item->prev = curr;
        curr->next = item;

        list->size++;

        return list;
    }

    void *cdi_list_remove(cdi_list_t list, size_t index)
    {
        if (list == nullptr) return nullptr;
        if (index == 0) return cdi_list_pop(list);

        auto curr = list->head;
        for (size_t i = 0; i < index - 1 && curr != nullptr; i++) curr = curr->next;

        if (curr == nullptr || curr->next == nullptr) return nullptr;

        auto old = curr->next;
        curr->next = old->next;
        if (old->next != nullptr) curr->next->prev = curr;

        list->size--;

        auto ret = old->value;
        delete old;
        return ret;
    }

    size_t cdi_list_size(cdi_list_t list)
    {
        if (list == nullptr) return 0;
        return list->size;
    }
} // extern "C"