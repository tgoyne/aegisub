/*
 * Copyright (C) 2006 Evgeniy Stepanov <eugeni.stepanov@gmail.com>
 *
 * This file is part of libass.
 *
 * libass is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libass is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libass; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <inttypes.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <assert.h>

#include "ass_utils.h"
#include "ass.h"
#include "ass_fontconfig.h"
#include "ass_font.h"
#include "ass_bitmap.h"
#include "ass_cache.h"

static unsigned hashmap_hash(void *buf, size_t len)
{
    return fnv_32a_buf(buf, len, FNV1_32A_INIT);
}

static int hashmap_key_compare(void *a, void *b, size_t size)
{
    return memcmp(a, b, size) == 0;
}

static void hashmap_item_dtor(void *key, size_t key_size, void *value,
                              size_t value_size)
{
    free(key);
    free(value);
}

hashmap_t *hashmap_init(ass_library_t *library, size_t key_size,
                        size_t value_size, int nbuckets,
                        hashmap_item_dtor_t item_dtor,
                        hashmap_key_compare_t key_compare,
                        hashmap_hash_t hash)
{
    hashmap_t *map = calloc(1, sizeof(hashmap_t));
    map->library = library;
    map->nbuckets = nbuckets;
    map->key_size = key_size;
    map->value_size = value_size;
    map->root = calloc(nbuckets, sizeof(hashmap_item_p));
    map->item_dtor = item_dtor ? item_dtor : hashmap_item_dtor;
    map->key_compare = key_compare ? key_compare : hashmap_key_compare;
    map->hash = hash ? hash : hashmap_hash;
    return map;
}

void hashmap_done(hashmap_t *map)
{
    int i;
    // print stats
    if (map->count > 0 || map->hit_count + map->miss_count > 0)
        ass_msg(map->library, MSGL_V,
               "cache statistics: \n  total accesses: %d\n  hits: %d\n  "
               "misses: %d\n  object count: %d",
               map->hit_count + map->miss_count, map->hit_count,
               map->miss_count, map->count);

    for (i = 0; i < map->nbuckets; ++i) {
        hashmap_item_t *item = map->root[i];
        while (item) {
            hashmap_item_t *next = item->next;
            map->item_dtor(item->key, map->key_size, item->value,
                           map->value_size);
            free(item);
            item = next;
        }
    }
    free(map->root);
    free(map);
}

// does nothing if key already exists
void *hashmap_insert(hashmap_t *map, void *key, void *value)
{
    unsigned hash = map->hash(key, map->key_size);
    hashmap_item_t **next = map->root + (hash % map->nbuckets);
    while (*next) {
        if (map->key_compare(key, (*next)->key, map->key_size))
            return (*next)->value;
        next = &((*next)->next);
        assert(next);
    }
    (*next) = malloc(sizeof(hashmap_item_t));
    (*next)->key = malloc(map->key_size);
    (*next)->value = malloc(map->value_size);
    memcpy((*next)->key, key, map->key_size);
    memcpy((*next)->value, value, map->value_size);
    (*next)->next = 0;

    map->count++;
    return (*next)->value;
}

void *hashmap_find(hashmap_t *map, void *key)
{
    unsigned hash = map->hash(key, map->key_size);
    hashmap_item_t *item = map->root[hash % map->nbuckets];
    while (item) {
        if (map->key_compare(key, item->key, map->key_size)) {
            map->hit_count++;
            return item->value;
        }
        item = item->next;
    }
    map->miss_count++;
    return 0;
}

//---------------------------------
// font cache

static unsigned font_desc_hash(void *buf, size_t len)
{
    ass_font_desc_t *desc = buf;
    unsigned hval;
    hval = fnv_32a_str(desc->family, FNV1_32A_INIT);
    hval = fnv_32a_buf(&desc->bold, sizeof(desc->bold), hval);
    hval = fnv_32a_buf(&desc->italic, sizeof(desc->italic), hval);
    return hval;
}

static int font_compare(void *key1, void *key2, size_t key_size)
{
    ass_font_desc_t *a = key1;
    ass_font_desc_t *b = key2;
    if (strcmp(a->family, b->family) != 0)
        return 0;
    if (a->bold != b->bold)
        return 0;
    if (a->italic != b->italic)
        return 0;
    if (a->treat_family_as_pattern != b->treat_family_as_pattern)
        return 0;
    return 1;
}

static void font_hash_dtor(void *key, size_t key_size, void *value,
                           size_t value_size)
{
    ass_font_free(value);
    free(key);
}

ass_font_t *ass_font_cache_find(hashmap_t *font_cache,
                                ass_font_desc_t *desc)
{
    return hashmap_find(font_cache, desc);
}

/**
 * \brief Add a face struct to cache.
 * \param font font struct
*/
void *ass_font_cache_add(hashmap_t *font_cache, ass_font_t *font)
{
    return hashmap_insert(font_cache, &(font->desc), font);
}

hashmap_t *ass_font_cache_init(ass_library_t *library)
{
    hashmap_t *font_cache;
    font_cache = hashmap_init(library, sizeof(ass_font_desc_t),
                              sizeof(ass_font_t),
                              1000,
                              font_hash_dtor, font_compare, font_desc_hash);
    return font_cache;
}

void ass_font_cache_done(hashmap_t *font_cache)
{
    hashmap_done(font_cache);
}


// Create hash/compare functions for bitmap and glyph
#define CREATE_HASH_FUNCTIONS
#include "ass_cache_template.h"
#define CREATE_COMPARISON_FUNCTIONS
#include "ass_cache_template.h"

//---------------------------------
// bitmap cache

static void bitmap_hash_dtor(void *key, size_t key_size, void *value,
                             size_t value_size)
{
    bitmap_hash_val_t *v = value;
    if (v->bm)
        ass_free_bitmap(v->bm);
    if (v->bm_o)
        ass_free_bitmap(v->bm_o);
    if (v->bm_s)
        ass_free_bitmap(v->bm_s);
    free(key);
    free(value);
}

void *cache_add_bitmap(hashmap_t *bitmap_cache, bitmap_hash_key_t *key,
                       bitmap_hash_val_t *val)
{
    // Note: this is only an approximation
    if (val->bm_o)
        bitmap_cache->cache_size += val->bm_o->w * val->bm_o->h * 3;
    else
        bitmap_cache->cache_size += val->bm->w * val->bm->h * 3;

    return hashmap_insert(bitmap_cache, key, val);
}

/**
 * \brief Get a bitmap from bitmap cache.
 * \param key hash key
 * \return requested hash val or 0 if not found
*/
bitmap_hash_val_t *cache_find_bitmap(hashmap_t *bitmap_cache,
                                     bitmap_hash_key_t *key)
{
    return hashmap_find(bitmap_cache, key);
}

hashmap_t *ass_bitmap_cache_init(ass_library_t *library)
{
    hashmap_t *bitmap_cache;
    bitmap_cache = hashmap_init(library,
                                sizeof(bitmap_hash_key_t),
                                sizeof(bitmap_hash_val_t),
                                0xFFFF + 13,
                                bitmap_hash_dtor, bitmap_compare,
                                bitmap_hash);
    return bitmap_cache;
}

void ass_bitmap_cache_done(hashmap_t *bitmap_cache)
{
    hashmap_done(bitmap_cache);
}

hashmap_t *ass_bitmap_cache_reset(hashmap_t *bitmap_cache)
{
    ass_library_t *lib = bitmap_cache->library;

    ass_bitmap_cache_done(bitmap_cache);
    return ass_bitmap_cache_init(lib);
}

//---------------------------------
// glyph cache

static void glyph_hash_dtor(void *key, size_t key_size, void *value,
                            size_t value_size)
{
    glyph_hash_val_t *v = value;
    if (v->glyph)
        FT_Done_Glyph(v->glyph);
    if (v->outline_glyph)
        FT_Done_Glyph(v->outline_glyph);
    free(key);
    free(value);
}

void *cache_add_glyph(hashmap_t *glyph_cache, glyph_hash_key_t *key,
                      glyph_hash_val_t *val)
{
    return hashmap_insert(glyph_cache, key, val);
}

/**
 * \brief Get a glyph from glyph cache.
 * \param key hash key
 * \return requested hash val or 0 if not found
*/
glyph_hash_val_t *cache_find_glyph(hashmap_t *glyph_cache,
                                   glyph_hash_key_t *key)
{
    return hashmap_find(glyph_cache, key);
}

hashmap_t *ass_glyph_cache_init(ass_library_t *library)
{
    hashmap_t *glyph_cache;
    glyph_cache = hashmap_init(library, sizeof(glyph_hash_key_t),
                               sizeof(glyph_hash_val_t),
                               0xFFFF + 13,
                               glyph_hash_dtor, glyph_compare, glyph_hash);
    return glyph_cache;
}

void ass_glyph_cache_done(hashmap_t *glyph_cache)
{
    hashmap_done(glyph_cache);
}

hashmap_t *ass_glyph_cache_reset(hashmap_t *glyph_cache)
{
    ass_library_t *lib = glyph_cache->library;

    ass_glyph_cache_done(glyph_cache);
    return ass_glyph_cache_init(lib);
}


//---------------------------------
// composite cache

static void composite_hash_dtor(void *key, size_t key_size, void *value,
                                size_t value_size)
{
    composite_hash_val_t *v = value;
    free(v->a);
    free(v->b);
    free(key);
    free(value);
}

void *cache_add_composite(hashmap_t *composite_cache,
                          composite_hash_key_t *key,
                          composite_hash_val_t *val)
{
    return hashmap_insert(composite_cache, key, val);
}

/**
 * \brief Get a composite bitmap from composite cache.
 * \param key hash key
 * \return requested hash val or 0 if not found
*/
composite_hash_val_t *cache_find_composite(hashmap_t *composite_cache,
                                           composite_hash_key_t *key)
{
    return hashmap_find(composite_cache, key);
}

hashmap_t *ass_composite_cache_init(ass_library_t *library)
{
    hashmap_t *composite_cache;
    composite_cache = hashmap_init(library, sizeof(composite_hash_key_t),
                                   sizeof(composite_hash_val_t),
                                   0xFFFF + 13,
                                   composite_hash_dtor, composite_compare,
                                   composite_hash);
    return composite_cache;
}

void ass_composite_cache_done(hashmap_t *composite_cache)
{
    hashmap_done(composite_cache);
}

hashmap_t *ass_composite_cache_reset(hashmap_t *composite_cache)
{
    ass_library_t *lib = composite_cache->library;

    ass_composite_cache_done(composite_cache);
    return ass_composite_cache_init(lib);
}
