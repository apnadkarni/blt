/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef BLT_POOL_H
#define BLT_POOL_H

#define BLT_STRING_ITEMS                0
#define BLT_FIXED_SIZE_ITEMS            1
#define BLT_VARIABLE_SIZE_ITEMS         2

typedef struct _Blt_Pool *Blt_Pool;
typedef void *(Blt_PoolAllocProc)(Blt_Pool pool, size_t size);
typedef void (Blt_PoolFreeProc)(Blt_Pool pool, void *item);

struct _Blt_Pool {
    Blt_PoolAllocProc *allocProc;
    Blt_PoolFreeProc *freeProc;
};

BLT_EXTERN Blt_Pool Blt_Pool_Create(int type);
BLT_EXTERN void Blt_Pool_Destroy(Blt_Pool pool);

#define Blt_Pool_AllocItem(pool, size) (*((pool)->allocProc))(pool, size)
#define Blt_Pool_FreeItem(pool, item) (*((pool)->freeProc))(pool, item)

#endif /* BLT_POOL_H */
