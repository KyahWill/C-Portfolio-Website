#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

uint64_t hash_key(const char* key);
typedef struct {
    const char* key;  // key is NULL if this slot is empty
    const char* value;
} hash_table_entry;

// Hash table structure: create with hash_table_create, free with hash_table_destroy.
typedef struct {
    hash_table_entry * entries;  // hash slots
    size_t capacity;    // size of _entries array
    size_t length;      // number of items in hash table
} hash_table ;

#define INITIAL_CAPACITY 16  // must not be zero

hash_table* hash_table_create(void) {
    // Allocate space for hash table struct.
    hash_table* table = (hash_table*)calloc(sizeof(hash_table),sizeof(hash_table));
    if (table == NULL) {
        return NULL;
    }
    table->length = 0;
    table->capacity = INITIAL_CAPACITY;

    // Allocate (zero'd) space for entry buckets.
    table->entries = (hash_table_entry *)calloc(table->capacity, sizeof(hash_table_entry));
    if (table->entries == NULL) {
        free(table); // error, free table before we return!
        return NULL;
    }
    return table;
}
const char* hash_table_set_entry(hash_table_entry* entries, size_t capacity, const char* key, void* value, size_t* plength) {
    // AND hash with capacity-1 to ensure it's within entries array.
    uint64_t hash = hash_key(key);
    size_t index = (size_t)(hash & (uint64_t)(capacity - 1));

    // Loop till we find an empty entry.
    while (entries[index].key != NULL) {
        if (strcmp(key, entries[index].key) == 0) {
            // Found key (it already exists), update value.
            entries[index].value = value;
            return entries[index].key;
        }
        // Key wasn't in this slot, move to next (linear probing).
        index++;
        if (index >= capacity) {
            // At end of entries array, wrap around.
            index = 0;
        }
    }

    // Didn't find key, allocate+copy if needed, then insert it.
    if (plength != NULL) {
        key = strdup(key);
        if (key == NULL) {
            return NULL;
        }
        (*plength)++;
    }
    entries[index].key = (char*)key;
    entries[index].value = value;
    return key;
}


bool hash_table_expand(hash_table* table) {
    // Allocate new entries array.
    size_t new_capacity = table->capacity * 2;
    if (new_capacity < table->capacity) {
        return false;  // overflow (capacity would be too big)
    }
    hash_table_entry* new_entries = calloc(new_capacity, sizeof(hash_table_entry));
    if (new_entries == NULL) {
        return false;
    }

    // Iterate entries, move all non-empty ones to new table's entries.
    for (size_t i = 0; i < table->capacity; i++) {
        hash_table_entry entry = table->entries[i];
        if (entry.key != NULL) {
            hash_table_set_entry(new_entries, new_capacity, entry.key,
                         (void *)entry.value, NULL);
        }
    }

    // Free old entries array and update this table's details.
    free(table->entries);
    table->entries = new_entries;
    table->capacity = new_capacity;
    return true;
}
void hash_table_destroy(hash_table* table) {
    // First free allocated keys.
    for (size_t i = 0; i < table->capacity; i++) {
        free((void*)table->entries[i].key);
    }

    // Then free entries array and table itself.
    free(table->entries);
    free(table);
}


#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL
// Return 64-bit FNV-1a hash for key (NUL-terminated). See description:
// hash_tabletps://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
uint64_t hash_key(const char* key) {
    uint64_t hash = FNV_OFFSET;
    for (const char* p = key; *p; p++) {
        hash ^= (uint64_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}


const char* hash_table_get(hash_table* table, const char* key) {
    // AND hash with capacity-1 to ensure it's within entries array.
    uint64_t hash = hash_key(key);
    size_t index = (size_t)(hash & (uint64_t)(table->capacity - 1));
    printf("Key: %s", key);
    // Loop till we find an empty entry.
    while (table->entries[index].key != NULL) {
        if (strcmp(key, table->entries[index].key) == 0) {
            // Found key, return value.
            return table->entries[index].value;
        }
        // Key wasn't in this slot, move to next (linear probing).
        index++;
        if (index >= table->capacity) {
            // At end of entries array, wrap around.
            index = 0;
        }
    }
    return NULL;
}


const char* hash_table_set(hash_table* table, const char* key, void* value) {
    if (value == NULL) {
        return NULL;
    }

    // If length will exceed half of current capacity, expand it.
    if (table->length >= table->capacity / 2) {
        if (!hash_table_expand(table)) {
            return NULL;
        }
    }

    // Set entry and update length.
    return hash_table_set_entry(table->entries, table->capacity, key, value,
                        &table->length);
}
// Internal function to set an entry (without expanding table).
