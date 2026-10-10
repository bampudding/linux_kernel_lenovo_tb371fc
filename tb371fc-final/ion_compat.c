/*
 * EXPERIMENTAL ABI PROTOTYPE — NOT FOR BOOT INSTALLATION.
 * A narrowly scoped C-only bridge for the four functions referenced by
 * Qualcomm's v676.68 libgsl.  The device has /dev/ion but not /dev/dma_heap.
 * Real-world buffer allocation, cache synchronization and secure heap
 * behavior require further testing before any systemless deployment.
 * Inspired by AOSP libdmabufheap's legacy ION fallback (Apache-2.0).
 */
#include <stddef.h>
#include <stdint.h>

extern int ion_open(void);
extern int ion_close(int fd);
extern int ion_is_legacy(int fd);
extern int ion_query_heap_cnt(int fd, int *count);
extern int ion_query_get_heaps(int fd, int count, void *buffer);
extern int ion_alloc_fd(int fd, size_t length, size_t align,
                        unsigned int mask, unsigned int flags, int *outfd);
extern void *calloc(size_t, size_t);
extern void free(void *);
extern int strcmp(const char *, const char *);
extern void *memcpy(void *, const void *, size_t);

#define EINVAL 22
#define ENODEV 19
#define ENOSPC 28
#define MAX_MAPS 24
#define MAX_HEAPS 48
#define MAX_HEAP_NAME 32
#define PUBLIC __attribute__((visibility("default")))

struct heap_info {
    char name[MAX_HEAP_NAME];
    uint32_t type, heap_id, reserved0, reserved1, reserved2;
};

struct heap_mapping {
    char name[MAX_HEAP_NAME];
    uint32_t mask, flags;
};

typedef struct {
    int ion_fd;
    int legacy;
    int count;
    struct heap_mapping mappings[MAX_MAPS];
} BufferAllocator;

static void copy_name(char dst[MAX_HEAP_NAME], const char *src)
{
    int i;
    for (i = 0; i < MAX_HEAP_NAME - 1 && src && src[i]; i++)
        dst[i] = src[i];
    dst[i] = '\0';
}

static int query_heap_mask(int fd, const char *name, uint32_t *mask)
{
    int count = 0, n, i;
    struct heap_info heaps[MAX_HEAPS];
    if (!name || !name[0]) return -EINVAL;
    if (ion_query_heap_cnt(fd, &count) < 0) return -ENODEV;
    if (count <= 0 || count > MAX_HEAPS) return -ENODEV;
    n = ion_query_get_heaps(fd, count, heaps);
    if (n < 0) return n;
    for (i = 0; i < count; ++i) {
        if (heaps[i].heap_id < 32 && !strcmp(heaps[i].name, name)) {
            *mask = (uint32_t)1 << heaps[i].heap_id;
            return 0;
        }
    }
    return -ENODEV;
}

PUBLIC BufferAllocator *CreateDmabufHeapBufferAllocator(void)
{
    BufferAllocator *a = calloc(1, sizeof(*a));
    if (!a) return (void *)0;
    a->ion_fd = ion_open();
    if (a->ion_fd < 0) { free(a); return (void *)0; }
    a->legacy = ion_is_legacy(a->ion_fd);
    return a;
}

PUBLIC void FreeDmabufHeapBufferAllocator(BufferAllocator *a)
{
    if (!a) return;
    if (a->ion_fd >= 0) ion_close(a->ion_fd);
    free(a);
}

PUBLIC int MapDmabufHeapNameToIonHeap(BufferAllocator *a,
    const char *heap_name, const char *ion_heap_name,
    unsigned int ion_flags, unsigned int legacy_mask,
    unsigned int legacy_flags)
{
    struct heap_mapping *m;
    uint32_t mask = 0, flags = 0;
    int i;
    if (!a || !heap_name || !heap_name[0]) return -EINVAL;
    if (!a->legacy && ion_heap_name && ion_heap_name[0] &&
        !query_heap_mask(a->ion_fd, ion_heap_name, &mask)) {
        flags = ion_flags;
    } else {
        mask = legacy_mask;
        flags = legacy_flags;
    }
    if (!mask) return -EINVAL;
    for (i = 0; i < a->count; i++)
        if (!strcmp(a->mappings[i].name, heap_name)) break;
    if (i >= MAX_MAPS) return -ENOSPC;
    if (i == a->count) a->count++;
    m = &a->mappings[i];
    copy_name(m->name, heap_name);
    m->mask = mask;
    m->flags = flags;
    return 0;
}

PUBLIC int DmabufHeapAlloc(BufferAllocator *a,
    const char *heap_name, size_t length,
    unsigned int flags, size_t legacy_align)
{
    uint32_t mask = 0, ion_flags = flags;
    int i, fd = -1, ret;
    if (!a || !heap_name || !length) return -EINVAL;
    for (i = 0; i < a->count; i++) {
        if (!strcmp(a->mappings[i].name, heap_name)) {
            mask = a->mappings[i].mask;
            ion_flags |= a->mappings[i].flags;
            break;
        }
    }
    if (!mask && query_heap_mask(a->ion_fd, heap_name, &mask))
        return -ENODEV;
    ret = ion_alloc_fd(a->ion_fd, length, legacy_align,
                       mask, ion_flags, &fd);
    return ret < 0 ? ret : fd;
}
