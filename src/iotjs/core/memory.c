
#include <event2/event.h>
#include <iotjs/core/memory.h>
#include <stdlib.h>
#include <stdio.h>
typedef struct vm_libevent_element
{
    size_t sz;
    struct vm_libevent_element *next;
    struct vm_libevent_element *prev;
} vm_libevent_element_t;
vm_libevent_element_t _vm_libevent_root = {.sz = 0};

#define IOTJS__LIST_LEN() (_vm_libevent_root.sz)
#define IOTJS__LIST_NEXT(e) (e->sz && e->next != &_vm_libevent_root ? e->next : NULL)
#define IOTJS__LIST_PREV(e) (e->sz && e->prev != &_vm_libevent_root ? e->prev : NULL)
#define IOTJS__LIST_PUSH_BACK(e) vm_libevent_insert(e, _vm_libevent_root.prev)
#define IOTJS__LIST_BACK() (_vm_libevent_root.sz ? _vm_libevent_root.prev : 0)

#define IOTJS__LIST_OUTPTR(ptr) (void *)(((char *)ptr) + sizeof(vm_libevent_element_t))
#define IOTJS__LIST_INPTR(ptr) (vm_libevent_element_t *)(((char *)ptr) - sizeof(vm_libevent_element_t))
void vm_libevent_insert(vm_libevent_element_t *e, vm_libevent_element_t *at)
{
    e->prev = at;
    e->next = at->next;
    e->prev->next = e;
    e->next->prev = e;
    _vm_libevent_root.sz++;
}
void vm_libevent_remove(vm_libevent_element_t *e)
{
    e->prev->next = e->next;
    e->next->prev = e->prev;
    // e->next = 0; // avoid memory leaks
    // e->prev = 0; // avoid memory leaks

    _vm_libevent_root.sz--;
}
size_t _m_event_size = 0;
size_t _m_event_count = 200;
void vm_memory_init(int events)
{
    _m_event_size = (size_t)event_get_struct_event_size();
    _m_event_count = events;

    _vm_libevent_root.next = &_vm_libevent_root;
    _vm_libevent_root.prev = &_vm_libevent_root;
}
void *vm_libevent_malloc(size_t sz)
{
    if (!_m_event_size || !_m_event_count)
    {
        return malloc(sz);
    }

    // if (sz == 0)
    // {
    //     return 0;
    // }
    vm_libevent_element_t *e;
    if (sz == _m_event_size)
    {
        // 從 緩存返回 event
        e = IOTJS__LIST_BACK();
        if (e)
        {
            vm_libevent_remove(e);
            return IOTJS__LIST_OUTPTR(e);
        }
        // 沒有緩存
    }

    e = malloc(sizeof(vm_libevent_element_t) + sz);
    if (!e)
    {
        return 0;
    }
    e->sz = sz;
    return IOTJS__LIST_OUTPTR(e);
}
void *vm_libevent_realloc(void *ptr, size_t sz)
{
    if (!_m_event_size || !_m_event_count)
    {
        return realloc(ptr, sz);
    }

    if (!ptr)
    {
        return vm_libevent_malloc(sz);
    }
    else if (!sz)
    {
        vm_libevent_free(ptr);
        return 0;
    }
    vm_libevent_element_t *e = IOTJS__LIST_INPTR(ptr);
    ptr = realloc(e, sizeof(vm_libevent_element_t) + sz);
    if (!ptr)
    {
        return ptr;
    }
    return IOTJS__LIST_OUTPTR(ptr);
}
void vm_libevent_free(void *ptr)
{
    // if (!ptr)
    // {
    //     return;
    // }
    if (!_m_event_size || !_m_event_count)
    {
        free(ptr);
        return;
    }
    vm_libevent_element_t *e = IOTJS__LIST_INPTR(ptr);
    if (e->sz == _m_event_size && _vm_libevent_root.sz < _m_event_count)
    {
        // 加入緩存
        IOTJS__LIST_PUSH_BACK(e);
        return;
    }
    free(e);
}