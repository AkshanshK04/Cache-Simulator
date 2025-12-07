#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

typedef struct {
    uint8_t valid;
    uint8_t dirty;
    uint64_t tag;
    int lru;  /* LRU counter */
} CacheBlock;

typedef struct {
    int cache_size;     /* in bytes */
    int block_size;     /* in bytes */
    int associativity;
    int num_sets;
    CacheBlock **sets;
    int misses;
    int hits;
    int accesses;
} Cache;

Cache* init_cache(int cache_size, int block_size, int associativity) {
    Cache* cache = (Cache*)malloc(sizeof(Cache));
    cache->cache_size = cache_size;
    cache->block_size = block_size;
    cache->associativity = associativity;
    cache->num_sets = cache_size / (block_size * associativity);
    cache->misses = 0;
    cache->hits = 0;
    cache->accesses = 0;

    cache->sets = (CacheBlock**)malloc(cache->num_sets * sizeof(CacheBlock*));
    for (int i = 0; i < cache->num_sets; i++) {
        cache->sets[i] = (CacheBlock*)malloc(associativity * sizeof(CacheBlock));
        for (int j = 0; j < associativity; j++) {
            cache->sets[i][j].valid = 0;
            cache->sets[i][j].dirty = 0;
            cache->sets[i][j].tag = 0;
            cache->sets[i][j].lru = j;  /* Initial LRU order */
        }
    }
    return cache;
}

void free_cache(Cache* cache) {
    for (int i = 0; i < cache->num_sets; i++) {
        free(cache->sets[i]);
    }
    free(cache->sets);
    free(cache);
}

int get_set(uint64_t addr, Cache* cache) {
    int block_bits = (int)log2(cache->block_size);
    int set_bits = (int)log2(cache->num_sets);
    return (addr >> block_bits) & ((1 << set_bits) - 1);
}

uint64_t get_tag(uint64_t addr, Cache* cache) {
    int block_bits = (int)log2(cache->block_size);
    int set_bits = (int)log2(cache->num_sets);
    return addr >> (block_bits + set_bits);
}

int find_lru(CacheBlock* set, int assoc) {
    int max_lru = -1;
    int victim = -1;
    for (int j = 0; j < assoc; j++) {
        if (set[j].lru > max_lru) {
            max_lru = set[j].lru;
            victim = j;
        }
    }
    return victim;
}

void update_lru(CacheBlock* set, int assoc, int way) {
    int old_lru = set[way].lru;
    for (int j = 0; j < assoc; j++) {
        if (set[j].lru < old_lru) {
            set[j].lru++;
        }
    }
    set[way].lru = 0;  /* Now most recent */
}
int access_cache(Cache* cache, uint64_t addr, char type) {
    cache->accesses++;
    int set_index = get_set(addr, cache);
    uint64_t tag = get_tag(addr, cache);
    CacheBlock* set = cache->sets[set_index];
    int assoc = cache->associativity;

    /* Check for hit */
    for (int j = 0; j < assoc; j++) {
        if (set[j].valid && set[j].tag == tag) {
            cache->hits++;
            update_lru(set, assoc, j);
            if (type == 'w') {
                set[j].dirty = 1;
            }
            return 1;  /* hit */
        }
    }

    /* Miss */
    cache->misses++;

    /* Find victim (empty slot or LRU) */
    int victim = -1;
    for (int j = 0; j < assoc; j++) {
        if (!set[j].valid) {
            victim = j;
            break;
        }
    }
    if (victim == -1) {
        victim = find_lru(set, assoc);
    }

    /* Evict if dirty (just simulate, no actual write) */
    if (set[victim].valid && set[victim].dirty) {
        /* Would write back to memory here */
    }

    /* Load new block */
    set[victim].valid = 1;
    set[victim].tag = tag;
    set[victim].dirty = (type == 'w') ? 1 : 0;
    update_lru(set, assoc, victim);

    return 0;  /* miss */
}

int main() {
    srand(time(NULL));  // fixed seed for reproducible results

    int cache_size = 8192;      // 8KB cache
    int block_size = 64;        // 64 bytes block
    int associativity = 4;      // 4-way set associative

    Cache* cache = init_cache(cache_size, block_size, associativity);

    // Ab realistic addresses generate karenge (locality add kar di)
    uint64_t base_addr = 0x10000000;   // sab addresses iske aas-paas
    int num_accesses = 20000;

    for (int i = 0; i < num_accesses; i++) {
        // 90% chance same 1MB region mein access (temporal + spatial locality)
        if (rand() % 100 < 90) {
            uint64_t offset = (rand() % 1024) * 64;  // same 64KB region mein reh
            uint64_t addr = base_addr + (offset % (1024*1024)); // 1MB range
            char type = (rand() % 10 < 7) ? 'r' : 'w';
            access_cache(cache, addr, type);
        } else {
            // 10% random far-away access
            uint64_t addr = ((uint64_t)rand() << 32) | rand();
            access_cache(cache, addr, 'r');
        }
    }

    printf("=== CACHE SIMULATION RESULTS ===\n");
    printf("Cache: %d KB, %d-way, Block: %d B\n", cache_size/1024, associativity, block_size);
    printf("Total Accesses : %d\n", cache->accesses);
    printf("Hits           : %d\n", cache->hits);
    printf("Misses         : %d\n", cache->misses);
    printf("Hit Rate       : %.2f%%\n", (float)cache->hits * 100 / cache->accesses);

    free_cache(cache);
    return 0;
}