#ifndef IOTJS_CONTAINER_LIST_H
#define IOTJS_CONTAINER_LIST_H
#include <stdint.h>
#ifndef BOOL
#define BOOL uint8_t
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define IOTJS_LIST_DEFINE(listName, eleName, typeValue) \
    typedef struct _##eleName                           \
    {                                                   \
        struct _##eleName *next;                        \
        struct _##eleName *prev;                        \
        void *list;                                     \
        typeValue value;                                \
    } eleName;                                          \
    typedef struct                                      \
    {                                                   \
        eleName root;                                   \
        int len;                                        \
    } listName;

#define IOTJS_LIST_INIT(list)      \
    list->root.next = &list->root; \
    list->root.prev = &list->root; \
    list->len = 0;

#define IOTJS_LIST_FRONT(list) (list->len ? list->root.next : 0)
#define IOTJS_LIST_BACK(list) (list->len ? list->root.prev : 0)
#define IOTJS_LIST_INSERT(list, e, at) \
    e->prev = at;                      \
    e->next = at->next;                \
    e->prev->next = e;                 \
    e->next->prev = e;                 \
    e->list = list;                    \
    list->len++;

#define IOTJS_LIST_DEFINE_HEADER(tag, listName, eleName)                 \
    void tag##_init(listName *list);                                     \
    int tag##_len(listName *list);                                       \
    eleName *tag##_front(listName *list);                                \
    eleName *tag##_back(listName *list);                                 \
    BOOL tag##_remove(listName *list, eleName *e);                       \
    void tag##_push_front(listName *list, eleName *e);                   \
    void tag##_push_back(listName *list, eleName *e);                    \
    BOOL tag##_insert_before(listName *list, eleName *e, eleName *mark); \
    BOOL tag##_insert_after(listName *list, eleName *e, eleName *mark);
#define IOTJS_LIST_DEFINE_FUNCTION(tag, listName, eleName)                  \
    void tag##_init(listName *list) { IOTJS_LIST_INIT(list) }               \
    int tag##_len(listName *list) { return list->len; }                     \
    eleName *tag##_front(listName *list) { return IOTJS_LIST_FRONT(list); } \
    eleName *tag##_back(listName *list) { return IOTJS_LIST_BACK(list); }   \
    BOOL tag##_remove(listName *list, eleName *e)                           \
    {                                                                       \
        if (e->list == list)                                                \
        {                                                                   \
            e->prev->next = e->next;                                        \
            e->next->prev = e->prev;                                        \
            e->next = 0; /* avoid memory leaks */                           \
            e->prev = 0; /* avoid memory leaks */                           \
            e->list = 0;                                                    \
            list->len--;                                                    \
            return TRUE;                                                    \
        }                                                                   \
        return FALSE;                                                       \
    }                                                                       \
    void tag##_push_front(listName *list, eleName *e)                       \
    {                                                                       \
        eleName *at = &list->root;                                          \
        IOTJS_LIST_INSERT(list, e, at)                                      \
    }                                                                       \
    void tag##_push_back(listName *list, eleName *e)                        \
    {                                                                       \
        eleName *at = list->root.prev;                                      \
        IOTJS_LIST_INSERT(list, e, at)                                      \
    }                                                                       \
    BOOL tag##_insert_before(listName *list, eleName *e, eleName *mark)     \
    {                                                                       \
        if (mark->list != list)                                             \
        {                                                                   \
            return FALSE;                                                   \
        }                                                                   \
        IOTJS_LIST_INSERT(list, e, mark->prev)                              \
        return TRUE;                                                        \
    }                                                                       \
    BOOL tag##_insert_after(listName *list, eleName *e, eleName *mark)      \
    {                                                                       \
        if (mark->list != list)                                             \
        {                                                                   \
            return FALSE;                                                   \
        }                                                                   \
        IOTJS_LIST_INSERT(list, e, mark)                                    \
        return TRUE;                                                        \
    }
#endif