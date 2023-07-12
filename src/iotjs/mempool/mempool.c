#include <iotjs/mempool/mempool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define IOTJS_MEP_OUT(p) ((uint8_t *)p + sizeof(iotjs_mep_ele_t))
#define IOTJS_MEP_IN(p) (void *)((uint8_t *)p - sizeof(iotjs_mep_ele_t))

void iotjs_mep_init(iotjs_mep_t *mep, iotjs_mep_alloctor_t *alloctors, size_t len)
{
    mep->alloctors = alloctors;
    mep->len = len;
}
static iotjs_mep_ele_t *iotjs_mep_new_block(size_t block, size_t len)
{
    iotjs_mep_ele_t *p = malloc(block + sizeof(iotjs_mep_ele_t));
    if (!p)
    {
        return 0;
    }
    p->value.len = len;
    p->value.block = block;
    return p;
}
static void _iotjs_mep_alloctor_clear(iotjs_mep_alloctor_t *alloctor)
{
    iotjs_mep_list_t *l = &alloctor->idle;
    if (l->len)
    {
        iotjs_mep_ele_t *next = l->root.next;
        iotjs_mep_ele_t *current;
        while (next && next != &l->root)
        {
            current = next;
            next = next->next;

            free(current);
        }
        iotjs_mep_list_init(l);
    }
    l = &alloctor->used;
    if (l->len)
    {
        iotjs_mep_ele_t *next = l->root.next;
        iotjs_mep_ele_t *current;
        while (next && next != &l->root)
        {
            current = next;
            next = next->next;

            free(current);
        }
        iotjs_mep_list_init(l);
    }
}

void iotjs_mep_destroy(iotjs_mep_t *mep)
{
    for (size_t i = 0; i < mep->len; i++)
    {
        _iotjs_mep_alloctor_clear(mep->alloctors + i);
    }
}
static int iotjs_mep_find_alloctor(iotjs_mep_t *mep, size_t sz)
{
    int i = 0;
    int n = mep->len;
    int j = n;
    while (i < j)
    {
        int h = (i + j) / 2; // avoid overflow when computing h
        // i ≤ h < j
        if (mep->alloctors[h].block < sz)
        {
            i = h + 1; // preserves cmp(i-1) > 0
        }
        else
        {
            j = h; // preserves cmp(j) <= 0
        }
    }

    if (i < n && mep->alloctors[i].block >= sz)
    {
        return i;
    }
    return -1;
}
static iotjs_mep_ele_t *iotjs_mep_alloctor_malloc(iotjs_mep_t *mep, int i, size_t sz)
{
    iotjs_mep_alloctor_t *alloctor = mep->alloctors + i;
    iotjs_mep_ele_t *front;
    if (alloctor->idle.len)
    {
        front = iotjs_mep_list_front(&alloctor->idle);
        iotjs_mep_list_remove(&alloctor->idle, front);
        iotjs_mep_list_push_back(&alloctor->used, front);
        front->value.len = sz;
    }
    else
    {
        front = iotjs_mep_new_block(alloctor->block, sz);
        if (front)
        {
            iotjs_mep_list_push_back(&alloctor->used, front);
        }
    }
    return front;
}
void *iotjs_mep_malloc(iotjs_mep_t *mep, size_t sz)
{

    iotjs_mep_ele_t *p;
    // 超過 最大塊的大內存 直接交給系統處理
    if (sz > mep->alloctors[mep->len - 1].block)
    {
        p = iotjs_mep_new_block(sz, sz);
    }
    else
    {
        p = iotjs_mep_alloctor_malloc(mep, iotjs_mep_find_alloctor(mep, sz), sz);
    }
    return p ? IOTJS_MEP_OUT(p) : NULL;
}

static void iotjs_mep_alloctor_free(iotjs_mep_t *mep, int i, iotjs_mep_ele_t *ele)
{
    iotjs_mep_alloctor_t *alloctor = mep->alloctors + i;
    if (ele->list != &alloctor->used)
    {
        puts("iotjs_mep_free not by iotjs_mep_malloc");
        exit(1);
    }
    else
    {
        iotjs_mep_list_remove(&alloctor->used, ele);
        if (alloctor->idle.len >= alloctor->cache)
        {
            free(ele);
            return;
        }
        iotjs_mep_list_push_back(&alloctor->idle, ele);
    }
}
void iotjs_mep_free(iotjs_mep_t *mep, void *ptr)
{
    if (ptr)
    {
        iotjs_mep_ele_t *ele = IOTJS_MEP_IN(ptr);
        if (ele->value.block > mep->alloctors[mep->len - 1].block)
        {
            free(ele);
            return;
        }
        int i = iotjs_mep_find_alloctor(mep, ele->value.block);
        iotjs_mep_alloctor_free(mep, i, ele);
    }
}
void *iotjs_mep_realloc(iotjs_mep_t *mep, void *ptr, size_t sz)
{
    if (!ptr)
    {
        return iotjs_mep_malloc(mep, sz);
    }
    else if (!sz)
    {
        iotjs_mep_free(mep, ptr);
        return NULL;
    }
    iotjs_mep_ele_t *ele = IOTJS_MEP_IN(ptr);
    if (sz <= ele->value.block)
    {
        ele->value.len = sz;
        return ptr;
    }

    size_t old_size = ele->value.len;
    void *dst = iotjs_mep_malloc(mep, sz);
    if (dst)
    {
        memcpy(dst, ptr, old_size);
    }
    iotjs_mep_free(mep, ptr);
    return dst;
}