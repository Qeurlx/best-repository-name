/*
 * Goon Module System
 * A comprehensive module for goon operations with handlers
 * Author: goon inc
 * Version: 1.0.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS AND MACROS
 * ============================================================================ */

#define GOON_VERSION "1.0.0"
#define GOON_MAX_HANDLERS 256
#define GOON_MAX_NAME_LEN 128
#define GOON_BUFFER_SIZE 4096
#define GOON_MAX_QUEUE_SIZE 1024
#define GOON_MAX_STACK_SIZE 512
#define GOON_CACHE_SIZE 64
#define GOON_POOL_SIZE 128

#define GOON_SUCCESS 0
#define GOON_ERROR -1
#define GOON_ERROR_NULL_PTR -2
#define GOON_ERROR_INVALID_PARAM -3
#define GOON_ERROR_OUT_OF_MEMORY -4
#define GOON_ERROR_NOT_FOUND -5
#define GOON_ERROR_OVERFLOW -6
#define GOON_ERROR_UNDERFLOW -7

#define GOON_LOG(level, ...) goon_log(level, __FILE__, __LINE__, __VA_ARGS__)
#define GOON_DEBUG(...) GOON_LOG(GOON_LOG_DEBUG, __VA_ARGS__)
#define GOON_INFO(...) GOON_LOG(GOON_LOG_INFO, __VA_ARGS__)
#define GOON_WARN(...) GOON_LOG(GOON_LOG_WARN, __VA_ARGS__)
#define GOON_ERROR_LOG(...) GOON_LOG(GOON_LOG_ERROR, __VA_ARGS__)

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

typedef enum {
    GOON_LOG_DEBUG,
    GOON_LOG_INFO,
    GOON_LOG_WARN,
    GOON_LOG_ERROR
} goon_log_level_t;

typedef enum {
    GOON_STATE_IDLE,
    GOON_STATE_INITIALIZING,
    GOON_STATE_RUNNING,
    GOON_STATE_PAUSED,
    GOON_STATE_STOPPING,
    GOON_STATE_ERROR,
    GOON_STATE_TERMINATED
} goon_state_t;

typedef enum {
    GOON_TYPE_INT,
    GOON_TYPE_FLOAT,
    GOON_TYPE_STRING,
    GOON_TYPE_POINTER,
    GOON_TYPE_BOOL,
    GOON_TYPE_CUSTOM
} goon_data_type_t;

typedef enum {
    GOON_PRIORITY_LOW,
    GOON_PRIORITY_NORMAL,
    GOON_PRIORITY_HIGH,
    GOON_PRIORITY_CRITICAL
} goon_priority_t;

typedef struct goon_context goon_context_t;
typedef struct goon_handler goon_handler_t;
typedef struct goon_event goon_event_t;
typedef struct goon_data goon_data_t;
typedef struct goon_queue goon_queue_t;
typedef struct goon_stack goon_stack_t;
typedef struct goon_cache goon_cache_t;
typedef struct goon_pool goon_pool_t;

typedef int (*goon_handler_func)(goon_context_t *ctx, goon_event_t *event, void *user_data);
typedef void (*goon_cleanup_func)(void *data);
typedef void* (*goon_alloc_func)(size_t size);
typedef void (*goon_free_func)(void *ptr);

struct goon_data {
    goon_data_type_t type;
    size_t size;
    void *value;
    goon_cleanup_func cleanup;
};

struct goon_event {
    uint32_t id;
    char name[GOON_MAX_NAME_LEN];
    goon_priority_t priority;
    time_t timestamp;
    goon_data_t *data;
    void *user_data;
    struct goon_event *next;
};

struct goon_handler {
    uint32_t id;
    char name[GOON_MAX_NAME_LEN];
    goon_handler_func func;
    void *user_data;
    bool enabled;
    uint64_t call_count;
    uint64_t error_count;
    double avg_exec_time;
    struct goon_handler *next;
};

struct goon_queue {
    goon_event_t *head;
    goon_event_t *tail;
    size_t size;
    size_t max_size;
};

struct goon_stack {
    void **items;
    size_t size;
    size_t capacity;
};

struct goon_cache {
    char keys[GOON_CACHE_SIZE][GOON_MAX_NAME_LEN];
    void *values[GOON_CACHE_SIZE];
    time_t timestamps[GOON_CACHE_SIZE];
    size_t sizes[GOON_CACHE_SIZE];
    size_t count;
};

struct goon_pool {
    void **objects;
    bool *in_use;
    size_t size;
    size_t capacity;
    goon_alloc_func alloc;
    goon_free_func free;
};

struct goon_context {
    uint32_t id;
    char name[GOON_MAX_NAME_LEN];
    goon_state_t state;
    goon_handler_t *handlers;
    size_t handler_count;
    goon_queue_t *event_queue;
    goon_stack_t *call_stack;
    goon_cache_t *cache;
    goon_pool_t *memory_pool;
    uint64_t event_count;
    uint64_t total_events_processed;
    time_t start_time;
    void *user_data;
    bool debug_mode;
};

/* ============================================================================
 * GLOBAL VARIABLES
 * ============================================================================ */

static goon_context_t *g_goon_ctx = NULL;
static uint32_t g_next_handler_id = 1;
static uint32_t g_next_event_id = 1;
static uint32_t g_next_context_id = 1;

/* ============================================================================
 * LOGGING FUNCTIONS
 * ============================================================================ */

void goon_log(goon_log_level_t level, const char *file, int line, const char *fmt, ...) {
    const char *level_str[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    time_t now = time(NULL);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    fprintf(stderr, "[%s] [%s] %s:%d - ", time_buf, level_str[level], file, line);
    
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    fprintf(stderr, "\n");
}

/* ============================================================================
 * DATA MANAGEMENT FUNCTIONS
 * ============================================================================ */

goon_data_t* goon_data_create(goon_data_type_t type, void *value, size_t size) {
    goon_data_t *data = (goon_data_t*)malloc(sizeof(goon_data_t));
    if (!data) {
        GOON_ERROR_LOG("Failed to allocate memory for goon_data_t");
        return NULL;
    }
    
    data->type = type;
    data->size = size;
    data->cleanup = NULL;
    
    if (value && size > 0) {
        data->value = malloc(size);
        if (!data->value) {
            GOON_ERROR_LOG("Failed to allocate memory for data value");
            free(data);
            return NULL;
        }
        memcpy(data->value, value, size);
    } else {
        data->value = value;
    }
    
    return data;
}

void goon_data_destroy(goon_data_t *data) {
    if (!data) return;
    
    if (data->cleanup && data->value) {
        data->cleanup(data->value);
    } else if (data->value && data->size > 0) {
        free(data->value);
    }
    
    free(data);
}

int goon_data_set_cleanup(goon_data_t *data, goon_cleanup_func cleanup) {
    if (!data) return GOON_ERROR_NULL_PTR;
    data->cleanup = cleanup;
    return GOON_SUCCESS;
}

void* goon_data_get_value(goon_data_t *data) {
    if (!data) return NULL;
    return data->value;
}

goon_data_type_t goon_data_get_type(goon_data_t *data) {
    if (!data) return GOON_TYPE_CUSTOM;
    return data->type;
}

/* ============================================================================
 * EVENT MANAGEMENT FUNCTIONS
 * ============================================================================ */

goon_event_t* goon_event_create(const char *name, goon_priority_t priority) {
    goon_event_t *event = (goon_event_t*)malloc(sizeof(goon_event_t));
    if (!event) {
        GOON_ERROR_LOG("Failed to allocate memory for goon_event_t");
        return NULL;
    }
    
    event->id = g_next_event_id++;
    strncpy(event->name, name, GOON_MAX_NAME_LEN - 1);
    event->name[GOON_MAX_NAME_LEN - 1] = '\0';
    event->priority = priority;
    event->timestamp = time(NULL);
    event->data = NULL;
    event->user_data = NULL;
    event->next = NULL;
    
    return event;
}

void goon_event_destroy(goon_event_t *event) {
    if (!event) return;
    
    if (event->data) {
        goon_data_destroy(event->data);
    }
    
    free(event);
}

int goon_event_set_data(goon_event_t *event, goon_data_t *data) {
    if (!event) return GOON_ERROR_NULL_PTR;
    
    if (event->data) {
        goon_data_destroy(event->data);
    }
    
    event->data = data;
    return GOON_SUCCESS;
}

goon_data_t* goon_event_get_data(goon_event_t *event) {
    if (!event) return NULL;
    return event->data;
}

/* ============================================================================
 * QUEUE MANAGEMENT FUNCTIONS
 * ============================================================================ */

goon_queue_t* goon_queue_create(size_t max_size) {
    goon_queue_t *queue = (goon_queue_t*)malloc(sizeof(goon_queue_t));
    if (!queue) {
        GOON_ERROR_LOG("Failed to allocate memory for goon_queue_t");
        return NULL;
    }
    
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    queue->max_size = max_size > 0 ? max_size : GOON_MAX_QUEUE_SIZE;
    
    return queue;
}

void goon_queue_destroy(goon_queue_t *queue) {
    if (!queue) return;
    
    goon_event_t *current = queue->head;
    while (current) {
        goon_event_t *next = current->next;
        goon_event_destroy(current);
        current = next;
    }
    
    free(queue);
}

int goon_queue_push(goon_queue_t *queue, goon_event_t *event) {
    if (!queue || !event) return GOON_ERROR_NULL_PTR;
    
    if (queue->size >= queue->max_size) {
        GOON_WARN("Queue is full, cannot push event");
        return GOON_ERROR_OVERFLOW;
    }
    
    event->next = NULL;
    
    if (queue->tail) {
        queue->tail->next = event;
    } else {
        queue->head = event;
    }
    
    queue->tail = event;
    queue->size++;
    
    return GOON_SUCCESS;
}

goon_event_t* goon_queue_pop(goon_queue_t *queue) {
    if (!queue || !queue->head) return NULL;
    
    goon_event_t *event = queue->head;
    queue->head = event->next;
    
    if (!queue->head) {
        queue->tail = NULL;
    }
    
    event->next = NULL;
    queue->size--;
    
    return event;
}

size_t goon_queue_size(goon_queue_t *queue) {
    if (!queue) return 0;
    return queue->size;
}

bool goon_queue_is_empty(goon_queue_t *queue) {
    if (!queue) return true;
    return queue->size == 0;
}

/* ============================================================================
 * STACK MANAGEMENT FUNCTIONS
 * ============================================================================ */

goon_stack_t* goon_stack_create(size_t capacity) {
    goon_stack_t *stack = (goon_stack_t*)malloc(sizeof(goon_stack_t));
    if (!stack) {
        GOON_ERROR_LOG("Failed to allocate memory for goon_stack_t");
        return NULL;
    }
    
    stack->capacity = capacity > 0 ? capacity : GOON_MAX_STACK_SIZE;
    stack->items = (void**)calloc(stack->capacity, sizeof(void*));
    if (!stack->items) {
        GOON_ERROR_LOG("Failed to allocate memory for stack items");
        free(stack);
        return NULL;
    }
    
    stack->size = 0;
    
    return stack;
}

void goon_stack_destroy(goon_stack_t *stack) {
    if (!stack) return;
    
    if (stack->items) {
        free(stack->items);
    }
    
    free(stack);
}

int goon_stack_push(goon_stack_t *stack, void *item) {
    if (!stack) return GOON_ERROR_NULL_PTR;
    
    if (stack->size >= stack->capacity) {
        GOON_WARN("Stack is full, cannot push item");
        return GOON_ERROR_OVERFLOW;
    }
    
    stack->items[stack->size++] = item;
    return GOON_SUCCESS;
}

void* goon_stack_pop(goon_stack_t *stack) {
    if (!stack || stack->size == 0) return NULL;
    
    return stack->items[--stack->size];
}

void* goon_stack_peek(goon_stack_t *stack) {
    if (!stack || stack->size == 0) return NULL;
    
    return stack->items[stack->size - 1];
}

size_t goon_stack_size(goon_stack_t *stack) {
    if (!stack) return 0;
    return stack->size;
}

bool goon_stack_is_empty(goon_stack_t *stack) {
    if (!stack) return true;
    return stack->size == 0;
}

/* ============================================================================
 * CACHE MANAGEMENT FUNCTIONS
 * ============================================================================ */

goon_cache_t* goon_cache_create(void) {
    goon_cache_t *cache = (goon_cache_t*)malloc(sizeof(goon_cache_t));
    if (!cache) {
        GOON_ERROR_LOG("Failed to allocate memory for goon_cache_t");
        return NULL;
    }
    
    memset(cache->keys, 0, sizeof(cache->keys));
    memset(cache->values, 0, sizeof(cache->values));
    memset(cache->timestamps, 0, sizeof(cache->timestamps));
    memset(cache->sizes, 0, sizeof(cache->sizes));
    cache->count = 0;
    
    return cache;
}

void goon_cache_destroy(goon_cache_t *cache) {
    if (!cache) return;
    
    for (size_t i = 0; i < cache->count; i++) {
        if (cache->values[i]) {
            free(cache->values[i]);
        }
    }
    
    free(cache);
}

int goon_cache_set(goon_cache_t *cache, const char *key, void *value, size_t size) {
    if (!cache || !key || !value) return GOON_ERROR_NULL_PTR;
    
    // Check if key exists
    for (size_t i = 0; i < cache->count; i++) {
        if (strcmp(cache->keys[i], key) == 0) {
            if (cache->values[i]) {
                free(cache->values[i]);
            }
            
            cache->values[i] = malloc(size);
            if (!cache->values[i]) {
                return GOON_ERROR_OUT_OF_MEMORY;
            }
            
            memcpy(cache->values[i], value, size);
            cache->sizes[i] = size;
            cache->timestamps[i] = time(NULL);
            return GOON_SUCCESS;
        }
    }
    
    // Add new entry
    if (cache->count >= GOON_CACHE_SIZE) {
        // Evict oldest entry
        size_t oldest_idx = 0;
        time_t oldest_time = cache->timestamps[0];
        
        for (size_t i = 1; i < GOON_CACHE_SIZE; i++) {
            if (cache->timestamps[i] < oldest_time) {
                oldest_time = cache->timestamps[i];
                oldest_idx = i;
            }
        }
        
        if (cache->values[oldest_idx]) {
            free(cache->values[oldest_idx]);
        }
        
        strncpy(cache->keys[oldest_idx], key, GOON_MAX_NAME_LEN - 1);
        cache->keys[oldest_idx][GOON_MAX_NAME_LEN - 1] = '\0';
        
        cache->values[oldest_idx] = malloc(size);
        if (!cache->values[oldest_idx]) {
            return GOON_ERROR_OUT_OF_MEMORY;
        }
        
        memcpy(cache->values[oldest_idx], value, size);
        cache->sizes[oldest_idx] = size;
        cache->timestamps[oldest_idx] = time(NULL);
    } else {
        strncpy(cache->keys[cache->count], key, GOON_MAX_NAME_LEN - 1);
        cache->keys[cache->count][GOON_MAX_NAME_LEN - 1] = '\0';
        
        cache->values[cache->count] = malloc(size);
        if (!cache->values[cache->count]) {
            return GOON_ERROR_OUT_OF_MEMORY;
        }
        
        memcpy(cache->values[cache->count], value, size);
        cache->sizes[cache->count] = size;
        cache->timestamps[cache->count] = time(NULL);
        cache->count++;
    }
    
    return GOON_SUCCESS;
}

void* goon_cache_get(goon_cache_t *cache, const char *key, size_t *size) {
    if (!cache || !key) return NULL;
    
    for (size_t i = 0; i < cache->count; i++) {
        if (strcmp(cache->keys[i], key) == 0) {
            cache->timestamps[i] = time(NULL);
            if (size) {
                *size = cache->sizes[i];
            }
            return cache->values[i];
        }
    }
    
    return NULL;
}

int goon_cache_remove(goon_cache_t *cache, const char *key) {
    if (!cache || !key) return GOON_ERROR_NULL_PTR;
    
    for (size_t i = 0; i < cache->count; i++) {
        if (strcmp(cache->keys[i], key) == 0) {
            if (cache->values[i]) {
                free(cache->values[i]);
            }
            
            // Shift remaining entries
            for (size_t j = i; j < cache->count - 1; j++) {
                strcpy(cache->keys[j], cache->keys[j + 1]);
                cache->values[j] = cache->values[j + 1];
                cache->sizes[j] = cache->sizes[j + 1];
                cache->timestamps[j] = cache->timestamps[j + 1];
            }
            
            cache->count--;
            return GOON_SUCCESS;
        }
    }
    
    return GOON_ERROR_NOT_FOUND;
}

void goon_cache_clear(goon_cache_t *cache) {
    if (!cache) return;
    
    for (size_t i = 0; i < cache->count; i++) {
        if (cache->values[i]) {
            free(cache->values[i]);
            cache->values[i] = NULL;
        }
    }
    
    cache->count = 0;
}

/* ============================================================================
 * MEMORY POOL FUNCTIONS
 * ============================================================================ */

goon_pool_t* goon_pool_create(size_t capacity, goon_alloc_func alloc, goon_free_func free_func) {
    goon_pool_t *pool = (goon_pool_t*)malloc(sizeof(goon_pool_t));
    if (!pool) {
        GOON_ERROR_LOG("Failed to allocate memory for goon_pool_t");
        return NULL;
    }
    
    pool->capacity = capacity > 0 ? capacity : GOON_POOL_SIZE;
    pool->objects = (void**)calloc(pool->capacity, sizeof(void*));
    pool->in_use = (bool*)calloc(pool->capacity, sizeof(bool));
    
    if (!pool->objects || !pool->in_use) {
        GOON_ERROR_LOG("Failed to allocate memory for pool arrays");
        if (pool->objects) free(pool->objects);
        if (pool->in_use) free(pool->in_use);
        free(pool);
        return NULL;
    }
    
    pool->size = 0;
    pool->alloc = alloc ? alloc : malloc;
    pool->free = free_func ? free_func : free;
    
    return pool;
}

void goon_pool_destroy(goon_pool_t *pool) {
    if (!pool) return;
    
    for (size_t i = 0; i < pool->size; i++) {
        if (pool->objects[i]) {
            pool->free(pool->objects[i]);
        }
    }
    
    if (pool->objects) free(pool->objects);
    if (pool->in_use) free(pool->in_use);
    free(pool);
}

void* goon_pool_acquire(goon_pool_t *pool, size_t size) {
    if (!pool) return NULL;
    
    // Check for available object
    for (size_t i = 0; i < pool->size; i++) {
        if (!pool->in_use[i] && pool->objects[i]) {
            pool->in_use[i] = true;
            return pool->objects[i];
        }
    }
    
    // Allocate new object
    if (pool->size < pool->capacity) {
        void *obj = pool->alloc(size);
        if (!obj) {
            GOON_ERROR_LOG("Failed to allocate object from pool");
            return NULL;
        }
        
        pool->objects[pool->size] = obj;
        pool->in_use[pool->size] = true;
        pool->size++;
        return obj;
    }
    
    GOON_WARN("Pool is full, cannot acquire object");
    return NULL;
}

int goon_pool_release(goon_pool_t *pool, void *obj) {
    if (!pool || !obj) return GOON_ERROR_NULL_PTR;
    
    for (size_t i = 0; i < pool->size; i++) {
        if (pool->objects[i] == obj) {
            pool->in_use[i] = false;
            return GOON_SUCCESS;
        }
    }
    
    return GOON_ERROR_NOT_FOUND;
}

/* ============================================================================
 * HANDLER MANAGEMENT FUNCTIONS
 * ============================================================================ */

goon_handler_t* goon_handler_create(const char *name, goon_handler_func func, void *user_data) {
    if (!name || !func) {
        GOON_ERROR_LOG("Invalid parameters for handler creation");
        return NULL;
    }
    
    goon_handler_t *handler = (goon_handler_t*)malloc(sizeof(goon_handler_t));
    if (!handler) {
        GOON_ERROR_LOG("Failed to allocate memory for goon_handler_t");
        return NULL;
    }
    
    handler->id = g_next_handler_id++;
    strncpy(handler->name, name, GOON_MAX_NAME_LEN - 1);
    handler->name[GOON_MAX_NAME_LEN - 1] = '\0';
    handler->func = func;
    handler->user_data = user_data;
    handler->enabled = true;
    handler->call_count = 0;
    handler->error_count = 0;
    handler->avg_exec_time = 0.0;
    handler->next = NULL;
    
    return handler;
}

void goon_handler_destroy(goon_handler_t *handler) {
    if (!handler) return;
    free(handler);
}

int goon_handler_enable(goon_handler_t *handler) {
    if (!handler) return GOON_ERROR_NULL_PTR;
    handler->enabled = true;
    return GOON_SUCCESS;
}

int goon_handler_disable(goon_handler_t *handler) {
    if (!handler) return GOON_ERROR_NULL_PTR;
    handler->enabled = false;
    return GOON_SUCCESS;
}

bool goon_handler_is_enabled(goon_handler_t *handler) {
    if (!handler) return false;
    return handler->enabled;
}

/* ============================================================================
 * CONTEXT MANAGEMENT FUNCTIONS
 * ============================================================================ */

goon_context_t* goon_context_create(const char *name) {
    goon_context_t *ctx = (goon_context_t*)malloc(sizeof(goon_context_t));
    if (!ctx) {
        GOON_ERROR_LOG("Failed to allocate memory for goon_context_t");
        return NULL;
    }
    
    ctx->id = g_next_context_id++;
    strncpy(ctx->name, name ? name : "default", GOON_MAX_NAME_LEN - 1);
    ctx->name[GOON_MAX_NAME_LEN - 1] = '\0';
    ctx->state = GOON_STATE_IDLE;
    ctx->handlers = NULL;
    ctx->handler_count = 0;
    ctx->event_queue = goon_queue_create(GOON_MAX_QUEUE_SIZE);
    ctx->call_stack = goon_stack_create(GOON_MAX_STACK_SIZE);
    ctx->cache = goon_cache_create();
    ctx->memory_pool = goon_pool_create(GOON_POOL_SIZE, NULL, NULL);
    ctx->event_count = 0;
    ctx->total_events_processed = 0;
    ctx->start_time = time(NULL);
    ctx->user_data = NULL;
    ctx->debug_mode = false;
    
    if (!ctx->event_queue || !ctx->call_stack || !ctx->cache || !ctx->memory_pool) {
        GOON_ERROR_LOG("Failed to initialize context components");
        goon_context_destroy(ctx);
        return NULL;
    }
    
    return ctx;
}

void goon_context_destroy(goon_context_t *ctx) {
    if (!ctx) return;
    
    goon_handler_t *handler = ctx->handlers;
    while (handler) {
        goon_handler_t *next = handler->next;
        goon_handler_destroy(handler);
        handler = next;
    }
    
    if (ctx->event_queue) {
        goon_queue_destroy(ctx->event_queue);
    }
    
    if (ctx->call_stack) {
        goon_stack_destroy(ctx->call_stack);
    }
    
    if (ctx->cache) {
        goon_cache_destroy(ctx->cache);
    }
    
    if (ctx->memory_pool) {
        goon_pool_destroy(ctx->memory_pool);
    }
    
    free(ctx);
}

int goon_context_register_handler(goon_context_t *ctx, goon_handler_t *handler) {
    if (!ctx || !handler) return GOON_ERROR_NULL_PTR;
    
    handler->next = ctx->handlers;
    ctx->handlers = handler;
    ctx->handler_count++;
    
    GOON_INFO("Registered handler '%s' (ID: %u)", handler->name, handler->id);
    return GOON_SUCCESS;
}

goon_handler_t* goon_context_find_handler(goon_context_t *ctx, const char *name) {
    if (!ctx || !name) return NULL;
    
    goon_handler_t *handler = ctx->handlers;
    while (handler) {
        if (strcmp(handler->name, name) == 0) {
            return handler;
        }
        handler = handler->next;
    }
    
    return NULL;
}

int goon_context_unregister_handler(goon_context_t *ctx, const char *name) {
    if (!ctx || !name) return GOON_ERROR_NULL_PTR;
    
    goon_handler_t *prev = NULL;
    goon_handler_t *handler = ctx->handlers;
    
    while (handler) {
        if (strcmp(handler->name, name) == 0) {
            if (prev) {
                prev->next = handler->next;
            } else {
                ctx->handlers = handler->next;
            }
            
            goon_handler_destroy(handler);
            ctx->handler_count--;
            GOON_INFO("Unregistered handler '%s'", name);
            return GOON_SUCCESS;
        }
        
        prev = handler;
        handler = handler->next;
    }
    
    return GOON_ERROR_NOT_FOUND;
}

int goon_context_set_state(goon_context_t *ctx, goon_state_t state) {
    if (!ctx) return GOON_ERROR_NULL_PTR;
    
    ctx->state = state;
    GOON_DEBUG("Context '%s' state changed to %d", ctx->name, state);
    return GOON_SUCCESS;
}

goon_state_t goon_context_get_state(goon_context_t *ctx) {
    if (!ctx) return GOON_STATE_ERROR;
    return ctx->state;
}

/* ============================================================================
 * EVENT PROCESSING FUNCTIONS
 * ============================================================================ */

int goon_context_emit_event(goon_context_t *ctx, goon_event_t *event) {
    if (!ctx || !event) return GOON_ERROR_NULL_PTR;
    
    int result = goon_queue_push(ctx->event_queue, event);
    if (result == GOON_SUCCESS) {
        ctx->event_count++;
        GOON_DEBUG("Event '%s' (ID: %u) emitted", event->name, event->id);
    }
    
    return result;
}

int goon_context_process_events(goon_context_t *ctx) {
    if (!ctx) return GOON_ERROR_NULL_PTR;
    
    if (ctx->state != GOON_STATE_RUNNING) {
        GOON_WARN("Context is not in RUNNING state");
        return GOON_ERROR;
    }
    
    int processed = 0;
    
    while (!goon_queue_is_empty(ctx->event_queue)) {
        goon_event_t *event = goon_queue_pop(ctx->event_queue);
        if (!event) break;
        
        GOON_DEBUG("Processing event '%s' (ID: %u)", event->name, event->id);
        
        goon_handler_t *handler = ctx->handlers;
        while (handler) {
            if (handler->enabled) {
                clock_t start = clock();
                
                int result = handler->func(ctx, event, handler->user_data);
                
                clock_t end = clock();
                double exec_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
                
                handler->call_count++;
                handler->avg_exec_time = (handler->avg_exec_time * (handler->call_count - 1) + exec_time) / handler->call_count;
                
                if (result != GOON_SUCCESS) {
                    handler->error_count++;
                    GOON_WARN("Handler '%s' returned error %d", handler->name, result);
                }
                
                if (ctx->debug_mode) {
                    GOON_DEBUG("Handler '%s' executed in %.3f ms", handler->name, exec_time);
                }
            }
            
            handler = handler->next;
        }
        
        goon_event_destroy(event);
        processed++;
        ctx->total_events_processed++;
    }
    
    return processed;
}

/* ============================================================================
 * INITIALIZATION AND CLEANUP
 * ============================================================================ */

int goon_init(const char *context_name) {
    if (g_goon_ctx) {
        GOON_WARN("Goon module already initialized");
        return GOON_ERROR;
    }
    
    g_goon_ctx = goon_context_create(context_name);
    if (!g_goon_ctx) {
        GOON_ERROR_LOG("Failed to create goon context");
        return GOON_ERROR;
    }
    
    goon_context_set_state(g_goon_ctx, GOON_STATE_INITIALIZING);
    GOON_INFO("Goon module initialized (version %s)", GOON_VERSION);
    
    return GOON_SUCCESS;
}

void goon_cleanup(void) {
    if (!g_goon_ctx) return;
    
    GOON_INFO("Cleaning up goon module");
    goon_context_destroy(g_goon_ctx);
    g_goon_ctx = NULL;
}

goon_context_t* goon_get_context(void) {
    return g_goon_ctx;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

void goon_print_stats(goon_context_t *ctx) {
    if (!ctx) return;
    
    printf("\n=== Goon Context Statistics ===\n");
    printf("Context Name: %s\n", ctx->name);
    printf("Context ID: %u\n", ctx->id);
    printf("State: %d\n", ctx->state);
    printf("Handlers Registered: %zu\n", ctx->handler_count);
    printf("Events in Queue: %zu\n", goon_queue_size(ctx->event_queue));
    printf("Total Events Processed: %llu\n", (unsigned long long)ctx->total_events_processed);
    printf("Uptime: %ld seconds\n", time(NULL) - ctx->start_time);
    printf("\n=== Handler Statistics ===\n");
    
    goon_handler_t *handler = ctx->handlers;
    while (handler) {
        printf("\nHandler: %s (ID: %u)\n", handler->name, handler->id);
        printf("  Enabled: %s\n", handler->enabled ? "Yes" : "No");
        printf("  Call Count: %llu\n", (unsigned long long)handler->call_count);
        printf("  Error Count: %llu\n", (unsigned long long)handler->error_count);
        printf("  Avg Execution Time: %.3f ms\n", handler->avg_exec_time);
        handler = handler->next;
    }
    
    printf("\n");
}

int goon_start(goon_context_t *ctx) {
    if (!ctx) return GOON_ERROR_NULL_PTR;
    
    goon_context_set_state(ctx, GOON_STATE_RUNNING);
    GOON_INFO("Goon context '%s' started", ctx->name);
    return GOON_SUCCESS;
}

int goon_stop(goon_context_t *ctx) {
    if (!ctx) return GOON_ERROR_NULL_PTR;
    
    goon_context_set_state(ctx, GOON_STATE_STOPPING);
    GOON_INFO("Goon context '%s' stopping", ctx->name);
    
    // Process remaining events
    goon_context_process_events(ctx);
    
    goon_context_set_state(ctx, GOON_STATE_TERMINATED);
    return GOON_SUCCESS;
}

int goon_pause(goon_context_t *ctx) {
    if (!ctx) return GOON_ERROR_NULL_PTR;
    
    goon_context_set_state(ctx, GOON_STATE_PAUSED);
    GOON_INFO("Goon context '%s' paused", ctx->name);
    return GOON_SUCCESS;
}

int goon_resume(goon_context_t *ctx) {
    if (!ctx) return GOON_ERROR_NULL_PTR;
    
    goon_context_set_state(ctx, GOON_STATE_RUNNING);
    GOON_INFO("Goon context '%s' resumed", ctx->name);
    return GOON_SUCCESS;
}

/* ============================================================================
 * EXAMPLE HANDLER FUNCTIONS
 * ============================================================================ */

int goon_handler_echo(goon_context_t *ctx, goon_event_t *event, void *user_data) {
    if (!ctx || !event) return GOON_ERROR_NULL_PTR;
    
    printf("[ECHO HANDLER] Event: %s (ID: %u, Priority: %d)\n", 
           event->name, event->id, event->priority);
    
    if (event->data) {
        goon_data_t *data = event->data;
        printf("[ECHO HANDLER] Data type: %d, size: %zu\n", data->type, data->size);
        
        if (data->type == GOON_TYPE_STRING && data->value) {
            printf("[ECHO HANDLER] String value: %s\n", (char*)data->value);
        } else if (data->type == GOON_TYPE_INT && data->value) {
            printf("[ECHO HANDLER] Int value: %d\n", *(int*)data->value);
        } else if (data->type == GOON_TYPE_FLOAT && data->value) {
            printf("[ECHO HANDLER] Float value: %f\n", *(float*)data->value);
        }
    }
    
    return GOON_SUCCESS;
}

int goon_handler_logger(goon_context_t *ctx, goon_event_t *event, void *user_data) {
    if (!ctx || !event) return GOON_ERROR_NULL_PTR;
    
    FILE *log_file = (FILE*)user_data;
    if (!log_file) {
        log_file = stdout;
    }
    
    fprintf(log_file, "[LOG] %s - Event: %s (ID: %u)\n", 
            ctime(&event->timestamp), event->name, event->id);
    
    return GOON_SUCCESS;
}

int goon_handler_counter(goon_context_t *ctx, goon_event_t *event, void *user_data) {
    if (!ctx || !event) return GOON_ERROR_NULL_PTR;
    
    int *counter = (int*)user_data;
    if (counter) {
        (*counter)++;
        printf("[COUNTER] Event count: %d\n", *counter);
    }
    
    return GOON_SUCCESS;
}

int goon_handler_cache_writer(goon_context_t *ctx, goon_event_t *event, void *user_data) {
    if (!ctx || !event || !ctx->cache) return GOON_ERROR_NULL_PTR;
    
    if (event->data && event->data->value) {
        int result = goon_cache_set(ctx->cache, event->name, 
                                     event->data->value, event->data->size);
        
        if (result == GOON_SUCCESS) {
            GOON_DEBUG("Cached event data for '%s'", event->name);
        }
    }
    
    return GOON_SUCCESS;
}

int goon_handler_validator(goon_context_t *ctx, goon_event_t *event, void *user_data) {
    if (!ctx || !event) return GOON_ERROR_NULL_PTR;
    
    if (strlen(event->name) == 0) {
        GOON_ERROR_LOG("Event has empty name");
        return GOON_ERROR_INVALID_PARAM;
    }
    
    if (event->priority < GOON_PRIORITY_LOW || event->priority > GOON_PRIORITY_CRITICAL) {
        GOON_ERROR_LOG("Event has invalid priority");
        return GOON_ERROR_INVALID_PARAM;
    }
    
    return GOON_SUCCESS;
}

int goon_handler_filter(goon_context_t *ctx, goon_event_t *event, void *user_data) {
    if (!ctx || !event) return GOON_ERROR_NULL_PTR;
    
    const char *filter_prefix = (const char*)user_data;
    if (!filter_prefix) return GOON_SUCCESS;
    
    if (strncmp(event->name, filter_prefix, strlen(filter_prefix)) == 0) {
        GOON_DEBUG("Event '%s' passed filter", event->name);
        return GOON_SUCCESS;
    }
    
    GOON_DEBUG("Event '%s' filtered out", event->name);
    return GOON_ERROR;
}

int goon_handler_statistics(goon_context_t *ctx, goon_event_t *event, void *user_data) {
    if (!ctx || !event) return GOON_ERROR_NULL_PTR;
    
    static uint64_t event_count_by_priority[4] = {0};
    
    if (event->priority >= GOON_PRIORITY_LOW && event->priority <= GOON_PRIORITY_CRITICAL) {
        event_count_by_priority[event->priority]++;
    }
    
    if (ctx->debug_mode) {
        printf("[STATS] Priority distribution - Low: %llu, Normal: %llu, High: %llu, Critical: %llu\n",
               (unsigned long long)event_count_by_priority[0],
               (unsigned long long)event_count_by_priority[1],
               (unsigned long long)event_count_by_priority[2],
               (unsigned long long)event_count_by_priority[3]);
    }
    
    return GOON_SUCCESS;
}

int goon_handler_transformer(goon_context_t *ctx, goon_event_t *event, void *user_data) {
    if (!ctx || !event || !event->data) return GOON_ERROR_NULL_PTR;
    
    if (event->data->type == GOON_TYPE_STRING && event->data->value) {
        char *str = (char*)event->data->value;
        size_t len = strlen(str);
        
        for (size_t i = 0; i < len; i++) {
            if (str[i] >= 'a' && str[i] <= 'z') {
                str[i] = str[i] - 'a' + 'A';
            }
        }
        
        GOON_DEBUG("Transformed string to uppercase");
    }
    
    return GOON_SUCCESS;
}

int goon_handler_duplicate_detector(goon_context_t *ctx, goon_event_t *event, void *user_data) {
    if (!ctx || !event || !ctx->cache) return GOON_ERROR_NULL_PTR;
    
    char cache_key[GOON_MAX_NAME_LEN + 16];
    snprintf(cache_key, sizeof(cache_key), "event_%s", event->name);
    
    void *cached = goon_cache_get(ctx->cache, cache_key, NULL);
    if (cached) {
        GOON_WARN("Duplicate event detected: %s", event->name);
        return GOON_ERROR;
    }
    
    int marker = 1;
    goon_cache_set(ctx->cache, cache_key, &marker, sizeof(int));
    
    return GOON_SUCCESS;
}

int goon_handler_rate_limiter(goon_context_t *ctx, goon_event_t *event, void *user_data) {
    if (!ctx || !event) return GOON_ERROR_NULL_PTR;
    
    static time_t last_event_time = 0;
    static int event_count = 0;
    
    time_t now = time(NULL);
    
    if (now != last_event_time) {
        event_count = 0;
        last_event_time = now;
    }
    
    event_count++;
    
    int *max_per_second = (int*)user_data;
    int limit = max_per_second ? *max_per_second : 10;
    
    if (event_count > limit) {
        GOON_WARN("Rate limit exceeded for event '%s'", event->name);
        return GOON_ERROR;
    }
    
    return GOON_SUCCESS;
}

/* ============================================================================
 * ADVANCED UTILITY FUNCTIONS
 * ============================================================================ */

int goon_context_enable_debug(goon_context_t *ctx) {
    if (!ctx) return GOON_ERROR_NULL_PTR;
    ctx->debug_mode = true;
    GOON_INFO("Debug mode enabled");
    return GOON_SUCCESS;
}

int goon_context_disable_debug(goon_context_t *ctx) {
    if (!ctx) return GOON_ERROR_NULL_PTR;
    ctx->debug_mode = false;
    GOON_INFO("Debug mode disabled");
    return GOON_SUCCESS;
}

int goon_context_clear_queue(goon_context_t *ctx) {
    if (!ctx || !ctx->event_queue) return GOON_ERROR_NULL_PTR;
    
    size_t cleared = 0;
    while (!goon_queue_is_empty(ctx->event_queue)) {
        goon_event_t *event = goon_queue_pop(ctx->event_queue);
        if (event) {
            goon_event_destroy(event);
            cleared++;
        }
    }
    
    GOON_INFO("Cleared %zu events from queue", cleared);
    return GOON_SUCCESS;
}

int goon_context_reset_statistics(goon_context_t *ctx) {
    if (!ctx) return GOON_ERROR_NULL_PTR;
    
    goon_handler_t *handler = ctx->handlers;
    while (handler) {
        handler->call_count = 0;
        handler->error_count = 0;
        handler->avg_exec_time = 0.0;
        handler = handler->next;
    }
    
    ctx->total_events_processed = 0;
    ctx->start_time = time(NULL);
    
    GOON_INFO("Statistics reset for context '%s'", ctx->name);
    return GOON_SUCCESS;
}

/* ============================================================================
 * SERIALIZATION FUNCTIONS
 * ============================================================================ */

int goon_event_serialize(goon_event_t *event, char *buffer, size_t buffer_size) {
    if (!event || !buffer || buffer_size == 0) return GOON_ERROR_NULL_PTR;
    
    int written = snprintf(buffer, buffer_size,
                          "EVENT{id:%u,name:%s,priority:%d,timestamp:%ld}",
                          event->id, event->name, event->priority, event->timestamp);
    
    if (written < 0 || (size_t)written >= buffer_size) {
        return GOON_ERROR_OVERFLOW;
    }
    
    return GOON_SUCCESS;
}

goon_event_t* goon_event_deserialize(const char *buffer) {
    if (!buffer) return NULL;
    
    uint32_t id;
    char name[GOON_MAX_NAME_LEN];
    int priority;
    time_t timestamp;
    
    int parsed = sscanf(buffer, "EVENT{id:%u,name:%127[^,],priority:%d,timestamp:%ld}",
                       &id, name, &priority, &timestamp);
    
    if (parsed != 4) {
        GOON_ERROR_LOG("Failed to parse event from buffer");
        return NULL;
    }
    
    goon_event_t *event = goon_event_create(name, (goon_priority_t)priority);
    if (event) {
        event->id = id;
        event->timestamp = timestamp;
    }
    
    return event;
}

/* ============================================================================
 * BATCH PROCESSING FUNCTIONS
 * ============================================================================ */

int goon_context_emit_batch(goon_context_t *ctx, goon_event_t **events, size_t count) {
    if (!ctx || !events) return GOON_ERROR_NULL_PTR;
    
    int success = 0;
    for (size_t i = 0; i < count; i++) {
        if (events[i]) {
            if (goon_context_emit_event(ctx, events[i]) == GOON_SUCCESS) {
                success++;
            }
        }
    }
    
    GOON_INFO("Emitted %d out of %zu events", success, count);
    return success;
}

int goon_context_register_batch(goon_context_t *ctx, goon_handler_t **handlers, size_t count) {
    if (!ctx || !handlers) return GOON_ERROR_NULL_PTR;
    
    int success = 0;
    for (size_t i = 0; i < count; i++) {
        if (handlers[i]) {
            if (goon_context_register_handler(ctx, handlers[i]) == GOON_SUCCESS) {
                success++;
            }
        }
    }
    
    GOON_INFO("Registered %d out of %zu handlers", success, count);
    return success;
}

/* ============================================================================
 * THREADING AND SYNCHRONIZATION UTILITIES
 * ============================================================================ */

typedef struct {
    goon_context_t *ctx;
    bool running;
    uint64_t iterations;
} goon_worker_t;

goon_worker_t* goon_worker_create(goon_context_t *ctx) {
    if (!ctx) return NULL;
    
    goon_worker_t *worker = (goon_worker_t*)malloc(sizeof(goon_worker_t));
    if (!worker) {
        GOON_ERROR_LOG("Failed to allocate worker");
        return NULL;
    }
    
    worker->ctx = ctx;
    worker->running = false;
    worker->iterations = 0;
    
    return worker;
}

void goon_worker_destroy(goon_worker_t *worker) {
    if (!worker) return;
    worker->running = false;
    free(worker);
}

int goon_worker_start(goon_worker_t *worker) {
    if (!worker) return GOON_ERROR_NULL_PTR;
    
    worker->running = true;
    goon_start(worker->ctx);
    
    GOON_INFO("Worker started for context '%s'", worker->ctx->name);
    return GOON_SUCCESS;
}

int goon_worker_stop(goon_worker_t *worker) {
    if (!worker) return GOON_ERROR_NULL_PTR;
    
    worker->running = false;
    goon_stop(worker->ctx);
    
    GOON_INFO("Worker stopped after %llu iterations", 
              (unsigned long long)worker->iterations);
    return GOON_SUCCESS;
}

int goon_worker_tick(goon_worker_t *worker) {
    if (!worker || !worker->running) return GOON_ERROR;
    
    int processed = goon_context_process_events(worker->ctx);
    worker->iterations++;
    
    return processed;
}

/* ============================================================================
 * BENCHMARK AND PROFILING FUNCTIONS
 * ============================================================================ */

typedef struct {
    char name[GOON_MAX_NAME_LEN];
    clock_t start;
    clock_t end;
    double elapsed_ms;
} goon_benchmark_t;

goon_benchmark_t* goon_benchmark_start(const char *name) {
    goon_benchmark_t *bench = (goon_benchmark_t*)malloc(sizeof(goon_benchmark_t));
    if (!bench) return NULL;
    
    strncpy(bench->name, name ? name : "benchmark", GOON_MAX_NAME_LEN - 1);
    bench->name[GOON_MAX_NAME_LEN - 1] = '\0';
    bench->start = clock();
    bench->end = 0;
    bench->elapsed_ms = 0.0;
    
    return bench;
}

double goon_benchmark_end(goon_benchmark_t *bench) {
    if (!bench) return 0.0;
    
    bench->end = clock();
    bench->elapsed_ms = ((double)(bench->end - bench->start)) / CLOCKS_PER_SEC * 1000.0;
    
    printf("[BENCHMARK] %s: %.3f ms\n", bench->name, bench->elapsed_ms);
    
    return bench->elapsed_ms;
}

void goon_benchmark_destroy(goon_benchmark_t *bench) {
    if (!bench) return;
    free(bench);
}

/* ============================================================================
 * CONFIGURATION MANAGEMENT
 * ============================================================================ */

typedef struct {
    char key[GOON_MAX_NAME_LEN];
    char value[GOON_BUFFER_SIZE];
} goon_config_entry_t;

typedef struct {
    goon_config_entry_t *entries;
    size_t count;
    size_t capacity;
} goon_config_t;

goon_config_t* goon_config_create(size_t capacity) {
    goon_config_t *config = (goon_config_t*)malloc(sizeof(goon_config_t));
    if (!config) return NULL;
    
    config->capacity = capacity > 0 ? capacity : 64;
    config->entries = (goon_config_entry_t*)calloc(config->capacity, 
                                                    sizeof(goon_config_entry_t));
    if (!config->entries) {
        free(config);
        return NULL;
    }
    
    config->count = 0;
    return config;
}

void goon_config_destroy(goon_config_t *config) {
    if (!config) return;
    
    if (config->entries) {
        free(config->entries);
    }
    
    free(config);
}

int goon_config_set(goon_config_t *config, const char *key, const char *value) {
    if (!config || !key || !value) return GOON_ERROR_NULL_PTR;
    
    // Check if key exists
    for (size_t i = 0; i < config->count; i++) {
        if (strcmp(config->entries[i].key, key) == 0) {
            strncpy(config->entries[i].value, value, GOON_BUFFER_SIZE - 1);
            config->entries[i].value[GOON_BUFFER_SIZE - 1] = '\0';
            return GOON_SUCCESS;
        }
    }
    
    // Add new entry
    if (config->count >= config->capacity) {
        return GOON_ERROR_OVERFLOW;
    }
    
    strncpy(config->entries[config->count].key, key, GOON_MAX_NAME_LEN - 1);
    config->entries[config->count].key[GOON_MAX_NAME_LEN - 1] = '\0';
    strncpy(config->entries[config->count].value, value, GOON_BUFFER_SIZE - 1);
    config->entries[config->count].value[GOON_BUFFER_SIZE - 1] = '\0';
    config->count++;
    
    return GOON_SUCCESS;
}

const char* goon_config_get(goon_config_t *config, const char *key) {
    if (!config || !key) return NULL;
    
    for (size_t i = 0; i < config->count; i++) {
        if (strcmp(config->entries[i].key, key) == 0) {
            return config->entries[i].value;
        }
    }
    
    return NULL;
}

int goon_config_remove(goon_config_t *config, const char *key) {
    if (!config || !key) return GOON_ERROR_NULL_PTR;
    
    for (size_t i = 0; i < config->count; i++) {
        if (strcmp(config->entries[i].key, key) == 0) {
            // Shift remaining entries
            for (size_t j = i; j < config->count - 1; j++) {
                config->entries[j] = config->entries[j + 1];
            }
            config->count--;
            return GOON_SUCCESS;
        }
    }
    
    return GOON_ERROR_NOT_FOUND;
}

/* ============================================================================
 * MAIN FUNCTION - EXAMPLE USAGE
 * ============================================================================ */

int main(int argc, char *argv[]) {
    printf("=== Goon Module System v%s ===\n\n", GOON_VERSION);
    
    // Initialize the goon module
    if (goon_init("main_context") != GOON_SUCCESS) {
        fprintf(stderr, "Failed to initialize goon module\n");
        return 1;
    }
    
    goon_context_t *ctx = goon_get_context();
    goon_context_enable_debug(ctx);
    
    // Create and register handlers
    goon_handler_t *echo_handler = goon_handler_create("echo", goon_handler_echo, NULL);
    goon_handler_t *logger_handler = goon_handler_create("logger", goon_handler_logger, NULL);
    goon_handler_t *counter_handler = goon_handler_create("counter", goon_handler_counter, NULL);
    goon_handler_t *cache_handler = goon_handler_create("cache", goon_handler_cache_writer, NULL);
    goon_handler_t *validator_handler = goon_handler_create("validator", goon_handler_validator, NULL);
    goon_handler_t *stats_handler = goon_handler_create("stats", goon_handler_statistics, NULL);
    
    goon_context_register_handler(ctx, echo_handler);
    goon_context_register_handler(ctx, logger_handler);
    goon_context_register_handler(ctx, counter_handler);
    goon_context_register_handler(ctx, cache_handler);
    goon_context_register_handler(ctx, validator_handler);
    goon_context_register_handler(ctx, stats_handler);
    
    // Start the context
    goon_start(ctx);
    
    // Create and emit events
    goon_benchmark_t *bench = goon_benchmark_start("event_processing");
    
    for (int i = 0; i < 10; i++) {
        char event_name[64];
        snprintf(event_name, sizeof(event_name), "test_event_%d", i);
        
        goon_event_t *event = goon_event_create(event_name, 
                                                 i % 4); // Rotate through priorities
        
        // Add some data to events
        if (i % 2 == 0) {
            int value = i * 100;
            goon_data_t *data = goon_data_create(GOON_TYPE_INT, &value, sizeof(int));
            goon_event_set_data(event, data);
        } else {
            char str_value[64];
            snprintf(str_value, sizeof(str_value), "Event number %d", i);
            goon_data_t *data = goon_data_create(GOON_TYPE_STRING, str_value, strlen(str_value) + 1);
            goon_event_set_data(event, data);
        }
        
        goon_context_emit_event(ctx, event);
    }
    
    // Process all events
    int processed = goon_context_process_events(ctx);
    printf("\nProcessed %d events\n", processed);
    
    goon_benchmark_end(bench);
    goon_benchmark_destroy(bench);
    
    // Print statistics
    goon_print_stats(ctx);
    
    // Clean up
    goon_stop(ctx);
    goon_cleanup();
    
    printf("\n=== Goon Module System Terminated ===\n");
    
    return 0;
}
