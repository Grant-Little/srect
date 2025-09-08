#ifndef SRECT_H
#define SRECT_H

#include <limits.h> /* INT_MAX UINT_MAX ULONG_MAX */

#define SR_PRIORITY_STATIC INT_MAX

/* sr_Body flags */
#define SR_NO_COLLISION         0x0001u
#define SR_DISABLED             0x0002u

/* sr_Body_Tick_Data flags */
#define SR_COLLIDED             0x0001u
#define SR_COLLIDED_CEILING     0x0002u
#define SR_COLLIDED_RIGHT_WALL  0x0004u
#define SR_COLLIDED_FLOOR       0x0008u
#define SR_COLLIDED_LEFT_WALL   0x0010u
#define SR_COLLIDED_UP          0x0020u
#define SR_COLLIDED_RIGHT       0x0040u
#define SR_COLLIDED_DOWN        0x0080u
#define SR_COLLIDED_LEFT        0x0100u

typedef struct {
    float x, y;
} sr_Vec2;

typedef struct {
    sr_Vec2 min, max;
} sr_Rect;

typedef int sr_Body_Id;

typedef struct {
    sr_Rect r;
    sr_Vec2 offset;
    int priority;
    unsigned int flags, custom_flags;
} sr_Body;

typedef struct {
    unsigned int flags, custom_flags;
} sr_Body_Tick_Data;

typedef enum {
    SR_SWEEP_X,
    SR_SWEEP_Y
} sr_Sweep_Direction;

typedef enum {
    SR_CENTER,
    SR_TOP_CENTER,
    SR_TOP_RIGHT,
    SR_CENTER_RIGHT,
    SR_BOTTOM_RIGHT,
    SR_BOTTOM_CENTER,
    SR_BOTTOM_LEFT,
    SR_CENTER_LEFT,
    SR_TOP_LEFT
} sr_Attach_Location;

typedef struct {
    sr_Body *bodies;
    sr_Body_Id *bodies_sorted;
    sr_Body_Tick_Data *bodies_tick_data;
    int num_bodies;
    int bodies_cap;
    sr_Sweep_Direction sweep_direction;
} sr_Context;

int sr_context_init(sr_Context *ctx, int expected_num_bodies, sr_Sweep_Direction sdir);

void sr_context_deinit(sr_Context *ctx);

void sr_context_clear(sr_Context *ctx);

sr_Body_Id sr_new_body(sr_Context *ctx, float xpos, float ypos, float xdim, float ydim, sr_Attach_Location loc, int priority, unsigned int flags, unsigned int custom_flags);

sr_Body_Id sr_register_body(sr_Context *ctx, sr_Body b);

int sr_place_body(sr_Context *ctx, sr_Body_Id id, float xpos, float ypos);

int sr_translate_body(sr_Context *ctx, sr_Body_Id id, float xmove, float ymove);

int sr_get_body_pos(sr_Vec2 *pos_out, const sr_Context *ctx, sr_Body_Id id);

int sr_get_body_pos_comp(float *xpos_out, float *ypos_out, const sr_Context *ctx, sr_Body_Id id);

int sr_get_body_dim(sr_Vec2 *dim_out, const sr_Context *ctx, sr_Body_Id id);

int sr_get_body_dim_comp(float *xdim_out, float *ydim_out, const sr_Context *ctx, sr_Body_Id id);

int sr_get_body_rect(sr_Rect *rect_out, const sr_Context *ctx, sr_Body_Id id);

int sr_get_body_rect_comp(float *xmin_out, float *ymin_out, float *xmax_out, float *ymax_out, const sr_Context *ctx, sr_Body_Id id);

int sr_do_bodies_overlap(sr_Context *ctx, sr_Body_Id id1, sr_Body_Id id2);

int sr_get_body_to_body_vector(sr_Vec2 *vec_out, sr_Context *ctx, sr_Body_Id id1, sr_Body_Id id2);

int sr_get_body_to_body_vector_comp(float *xvec_out, float *yvec_out, sr_Context *ctx, sr_Body_Id id1, sr_Body_Id id2);

int sr_get_body_tick_data(sr_Body_Tick_Data *data_out, sr_Context *ctx, sr_Body_Id id);

int sr_did_body_collide(sr_Context *ctx, sr_Body_Id id);

int sr_did_body_collide_wall(sr_Context *ctx, sr_Body_Id id);

int sr_did_body_collide_ceiling(sr_Context *ctx, sr_Body_Id id);

int sr_did_body_collide_right_wall(sr_Context *ctx, sr_Body_Id id);

int sr_did_body_collide_floor(sr_Context *ctx, sr_Body_Id id);

int sr_did_body_collide_left_wall(sr_Context *ctx, sr_Body_Id id);

int sr_did_body_collide_up(sr_Context *ctx, sr_Body_Id id);

int sr_did_body_collide_right(sr_Context *ctx, sr_Body_Id id);

int sr_did_body_collide_down(sr_Context *ctx, sr_Body_Id id);

int sr_did_body_collide_left(sr_Context *ctx, sr_Body_Id id);

void sr_resolve_collisions(sr_Context *ctx);

#endif /* #ifndef SRECT_H */

#ifdef SRECT_IMPLEMENTATION

#include <stddef.h> /* NULL */
#include <string.h> /* memcpy() memset()  */
#include <float.h> /* FLT_DIG */

#if UINT_MAX == 4294967295
    #define SR_U32 unsigned int
#elif ULONG_MAX == 4294967295
    #define SR_U32 unsigned long
#else
    #error "Cannot determine acceptable unsigned 32 bit integer type"
#endif

#if FLT_DIG != 6
    #error "float does not appear to be 32 bit"
#endif

#if defined(SR_REALLOC) && !defined(SR_FREE) || !defined(SR_REALLOC) && defined(SR_FREE)
    #error "Custom allocator support requires defining both SR_REALLOC and SR_FREE"
#endif

#ifndef SR_ALLOC_CONTEXT
    #define SR_ALLOC_CONTEXT NULL
#endif

#if !defined(SR_REALLOC) && !defined(SR_FREE)
    #include <stdlib.h>
    #define SR_REALLOC(c, ptr, sz) realloc(ptr, sz)
    #define SR_FREE(c, ptr) free(ptr)
#endif

#ifndef SR_ASSERT
    #include <assert.h>
    #define SR_ASSERT(e) assert(e)
#endif

float sr_fabsf(float num) {
    SR_U32 bits;

    bits = *(SR_U32 *)&num;
    bits &= 0x7FFFFFFFul;

    return *(float *)&bits;
}

int sr_is_b1_xmin_edge_less(const sr_Context *ctx, sr_Body_Id b1, sr_Body_Id b2) {
    if (ctx->bodies[b1].r.min.x < ctx->bodies[b2].r.min.x) {
        return 1;
    } else {
        return 0;
    }
}

int sr_is_b1_ymin_edge_less(const sr_Context *ctx, sr_Body_Id b1, sr_Body_Id b2) {
    if (ctx->bodies[b1].r.min.y < ctx->bodies[b2].r.min.y) {
        return 1;
    } else {
        return 0;
    }
}

void sr_stable_sort(sr_Context *ctx) {
    sr_Body_Id temp;
    int i, j;

    if (ctx->sweep_direction == SR_SWEEP_X) {
        for (i = 1; i < ctx->num_bodies; ++i) {
            temp = ctx->bodies_sorted[i];
            j = i - 1;

            while(j >= 0 && sr_is_b1_xmin_edge_less(ctx, temp, ctx->bodies_sorted[j])) {
                ctx->bodies_sorted[j + 1] = ctx->bodies_sorted[j];
                --j;
            }

            ctx->bodies_sorted[j + 1] = temp;
        }
    } else {
        for (i = 1; i < ctx->num_bodies; ++i) {
            temp = ctx->bodies_sorted[i];
            j = i - 1;

            while(j >= 0 && sr_is_b1_ymin_edge_less(ctx, temp, ctx->bodies_sorted[j])) {
                ctx->bodies_sorted[j + 1] = ctx->bodies_sorted[j];
                --j;
            }

            ctx->bodies_sorted[j + 1] = temp;
        }
    }
}

int sr_do_rects_overlap(sr_Rect r1, sr_Rect r2) {
    return !(r1.min.x > r2.max.x || r1.max.x < r2.min.x || r1.min.y > r2.max.y || r1.max.y < r2.min.y);
}

void sr_translate_body_direct(sr_Body *b, float xmove, float ymove) {
    b->r.min.x += xmove;
    b->r.min.y += ymove;
    b->r.max.x += xmove;
    b->r.max.y += ymove;
}

void sr_resolve_bodies(sr_Context *ctx, sr_Body_Id id1, sr_Body_Id id2) {
    sr_Body *b1, *b2;
    sr_Body_Tick_Data *b1t, *b2t;
    sr_Vec2 b2_to_b1;
    float b1_extent, b2_extent, x_overlap, y_overlap;

    b1 = &(ctx->bodies[id1]);
    b2 = &(ctx->bodies[id2]);

    b1t = &(ctx->bodies_tick_data[id1]);
    b2t = &(ctx->bodies_tick_data[id2]);

    if (b1->priority == SR_PRIORITY_STATIC && b2->priority == SR_PRIORITY_STATIC) {
        return;
    }

    b1t->flags |= SR_COLLIDED;
    b2t->flags |= SR_COLLIDED;

    b1t->custom_flags |= b2->custom_flags;
    b2t->custom_flags |= b1->custom_flags;

    b2_to_b1.x = (b1->r.min.x + b1->r.max.x) / 2.0f - (b2->r.min.x + b2->r.max.x) / 2.0f;
    b2_to_b1.y = (b1->r.min.y + b1->r.max.y) / 2.0f - (b2->r.min.y + b2->r.max.y) / 2.0f;

    b1_extent = (b1->r.max.x - b1->r.min.x) / 2.0f;
    b2_extent = (b2->r.max.x - b2->r.min.x) / 2.0f;

    x_overlap = b1_extent + b2_extent - sr_fabsf(b2_to_b1.x);

    b1_extent = (b1->r.max.y - b1->r.min.y) / 2.0f;
    b2_extent = (b2->r.max.y - b2->r.min.y) / 2.0f;

    y_overlap = b1_extent + b2_extent - sr_fabsf(b2_to_b1.y);

    if (x_overlap > y_overlap) {
        if (b1->priority < b2->priority) {
            if (b2_to_b1.y > 0.0f) {
                sr_translate_body_direct(b1, 0.0f, y_overlap);
                if (b2->priority == SR_PRIORITY_STATIC) {
                    b1t->flags |= SR_COLLIDED_CEILING;
                }
                b1t->flags |= SR_COLLIDED_UP;
                b2t->flags |= SR_COLLIDED_DOWN;
            } else {
                sr_translate_body_direct(b1, 0.0f, -y_overlap);
                if (b2->priority == SR_PRIORITY_STATIC) {
                    b1t->flags |= SR_COLLIDED_FLOOR;
                }
                b1t->flags |= SR_COLLIDED_DOWN;
                b2t->flags |= SR_COLLIDED_UP;
            }
        } else if (b1->priority == b2->priority) {
            if (b2_to_b1.y > 0.0f) {
                sr_translate_body_direct(b1, 0.0f, y_overlap / 2.0f);
                sr_translate_body_direct(b2, 0.0f, -y_overlap / 2.0f);
                b1t->flags |= SR_COLLIDED_UP;
                b2t->flags |= SR_COLLIDED_DOWN;
            } else {
                sr_translate_body_direct(b1, 0.0f, -y_overlap / 2.0f);
                sr_translate_body_direct(b2, 0.0f, y_overlap / 2.0f);
                b1t->flags |= SR_COLLIDED_DOWN;
                b2t->flags |= SR_COLLIDED_UP;
            }
        } else {
            if (b2_to_b1.y < 0.0f) {
                sr_translate_body_direct(b2, 0.0f, y_overlap);
                if (b1->priority == SR_PRIORITY_STATIC) {
                    b2t->flags |= SR_COLLIDED_CEILING;
                }
                b1t->flags |= SR_COLLIDED_DOWN;
                b2t->flags |= SR_COLLIDED_UP;
            } else {
                sr_translate_body_direct(b2, 0.0f, -y_overlap);
                if (b1->priority == SR_PRIORITY_STATIC) {
                    b2t->flags |= SR_COLLIDED_FLOOR;
                }
                b1t->flags |= SR_COLLIDED_UP;
                b2t->flags |= SR_COLLIDED_DOWN;
            }
        }
    } else {
        if (b1->priority < b2->priority) {
            if (b2_to_b1.x > 0.0f) {
                sr_translate_body_direct(b1, x_overlap, 0.0f);
                if (b2->priority == SR_PRIORITY_STATIC) {
                    b1t->flags |= SR_COLLIDED_LEFT_WALL;
                }
                b1t->flags |= SR_COLLIDED_LEFT;
                b2t->flags |= SR_COLLIDED_RIGHT;
            } else {
                sr_translate_body_direct(b1, -x_overlap, 0.0f);
                if (b2->priority == SR_PRIORITY_STATIC) {
                    b1t->flags |= SR_COLLIDED_RIGHT_WALL;
                }
                b1t->flags |= SR_COLLIDED_RIGHT;
                b2t->flags |= SR_COLLIDED_LEFT;
            }
        } else if (b1->priority == b2->priority) {
            if (b2_to_b1.x > 0.0f) {
                sr_translate_body_direct(b1, x_overlap / 2.0f, 0.0f);
                sr_translate_body_direct(b2, -x_overlap / 2.0f, 0.0f);
                b1t->flags |= SR_COLLIDED_LEFT;
                b2t->flags |= SR_COLLIDED_RIGHT;
            } else {
                sr_translate_body_direct(b1, -x_overlap / 2.0f, 0.0f);
                sr_translate_body_direct(b2, x_overlap / 2.0f, 0.0f);
                b1t->flags |= SR_COLLIDED_RIGHT;
                b2t->flags |= SR_COLLIDED_LEFT;
            }
        } else {
            if (b2_to_b1.x < 0.0f) {
                sr_translate_body_direct(b2, x_overlap, 0.0f);
                if (b1->priority == SR_PRIORITY_STATIC) {
                    b2t->flags |= SR_COLLIDED_LEFT_WALL;
                }
                b1t->flags |= SR_COLLIDED_RIGHT;
                b2t->flags |= SR_COLLIDED_LEFT;
            } else {
                sr_translate_body_direct(b2, -x_overlap, 0.0f);
                if (b1->priority == SR_PRIORITY_STATIC) {
                    b2t->flags |= SR_COLLIDED_RIGHT_WALL;
                }
                b1t->flags |= SR_COLLIDED_LEFT;
                b2t->flags |= SR_COLLIDED_RIGHT;
            }
        }
    }
}

int sr_context_init(sr_Context *ctx, int expected_num_bodies, sr_Sweep_Direction sdir) {
    SR_ASSERT(sizeof(float) == 4 && sizeof(unsigned int) == 4 && "custom fabsf relies on float and unsigned int being 32 bit, which does not appear to be true");
    SR_ASSERT(sr_fabsf(-1.0f) == 1.0f && "custom fabsf does not appear to be working");
    SR_ASSERT(ctx != NULL && "cannot initialize null pointer");

    ctx->bodies = SR_REALLOC(SR_ALLOC_CONTEXT, NULL, sizeof(sr_Body) * expected_num_bodies + sizeof(sr_Body_Id) * expected_num_bodies + sizeof(sr_Body_Tick_Data) * expected_num_bodies);
    if (ctx->bodies == NULL) {
        return -1;
    } else {
        ctx->bodies_sorted = (sr_Body_Id *)(ctx->bodies + expected_num_bodies);
        ctx->bodies_tick_data = (sr_Body_Tick_Data *)(ctx->bodies_sorted + expected_num_bodies);
        ctx->num_bodies = 0;
        ctx->bodies_cap = expected_num_bodies;
        ctx->sweep_direction = sdir;

        return 0;
    }
}

void sr_context_deinit(sr_Context *ctx) {
    SR_FREE(SR_ALLOC_CONTEXT, ctx->bodies);
    ctx->bodies = NULL;
    ctx->bodies_sorted = NULL;
    ctx->bodies_tick_data = NULL;
    ctx->num_bodies = 0;
    ctx->bodies_cap = 0;
    ctx->sweep_direction = 0;
}

void sr_context_clear(sr_Context *ctx) {
    ctx->num_bodies = 0;
}

sr_Body_Id sr_new_body(sr_Context *ctx, float xpos, float ypos, float xdim, float ydim, sr_Attach_Location loc, int priority, unsigned int flags, unsigned int custom_flags) {
    sr_Body b;

    b.priority = priority;
    b.flags = flags;
    b.custom_flags = custom_flags;

    switch (loc) {
    case SR_CENTER:
        b.offset.x = xdim / 2.0f;
        b.offset.y = ydim / 2.0f;
        break;
    case SR_TOP_CENTER:
        b.offset.x = xdim / 2.0f;
        b.offset.y = 0.0f;
        break;
    case SR_TOP_RIGHT:
        b.offset.x = xdim;
        b.offset.y = 0.0f;
        break;
    case SR_CENTER_RIGHT:
        b.offset.x = xdim;
        b.offset.y = ydim / 2.0f;
        break;
    case SR_BOTTOM_RIGHT:
        b.offset.x = xdim;
        b.offset.y = ydim;
        break;
    case SR_BOTTOM_CENTER:
        b.offset.x = xdim / 2.0f;
        b.offset.y = ydim;
        break;
    case SR_BOTTOM_LEFT:
        b.offset.x = 0.0f;
        b.offset.y = ydim;
        break;
    case SR_CENTER_LEFT:
        b.offset.x = 0.0f;
        b.offset.y = ydim / 2.0f;
        break;
    case SR_TOP_LEFT:
        b.offset.x = 0.0f;
        b.offset.y = 0.0f;
        break;
    }

    b.r.min.x = xpos - b.offset.x;
    b.r.min.y = ypos - b.offset.y;
    b.r.max.x = b.r.min.x + xdim;
    b.r.max.y = b.r.min.y + ydim;

    return sr_register_body(ctx, b);
}

sr_Body_Id sr_register_body(sr_Context *ctx, sr_Body b) {
    void *realloc_out;
    sr_Body_Id *bodies_sorted_temp;
    sr_Body_Tick_Data *bodies_tick_data_temp;
    sr_Body_Id next_id;

    if (ctx == NULL) {
        return -1;
    }

    next_id = ctx->num_bodies;

    if (ctx->num_bodies >= ctx->bodies_cap) {
        realloc_out = SR_REALLOC(SR_ALLOC_CONTEXT, ctx->bodies, 2 * sizeof(sr_Body) * ctx->num_bodies + 2 * sizeof(sr_Body_Id) * ctx->num_bodies + 2 * sizeof(sr_Body_Tick_Data) * ctx->num_bodies);
        if (realloc_out == NULL) {
            return -1;
        } else {
            ctx->bodies = realloc_out;
            bodies_sorted_temp = (sr_Body_Id *)(ctx->bodies + ctx->num_bodies);
            bodies_tick_data_temp = (sr_Body_Tick_Data *)(bodies_sorted_temp + ctx->num_bodies);

            ctx->bodies_sorted = (sr_Body_Id *)(ctx->bodies + ctx->num_bodies * 2);
            ctx->bodies_tick_data = (sr_Body_Tick_Data *)(ctx->bodies_sorted + ctx->num_bodies * 2);
            memcpy(ctx->bodies_tick_data, bodies_tick_data_temp, ctx->num_bodies * sizeof(sr_Body_Tick_Data));
            memcpy(ctx->bodies_sorted, bodies_sorted_temp, ctx->num_bodies * sizeof(sr_Body_Id));
            ctx->bodies_cap = ctx->num_bodies * 2;
        }
    }

    ctx->bodies[next_id] = b;
    ctx->bodies_sorted[next_id] = next_id;
    ++ctx->num_bodies;

    return next_id;
}

int sr_place_body(sr_Context *ctx, sr_Body_Id id, float xpos, float ypos) {
    sr_Vec2 min_to_max;

    if (id >= ctx->num_bodies) {
        return -1;
    } else if (ctx->bodies[id].flags & SR_DISABLED) {
        return 0;
    } else {
        min_to_max.x = ctx->bodies[id].r.max.x - ctx->bodies[id].r.min.x;
        min_to_max.y = ctx->bodies[id].r.max.y - ctx->bodies[id].r.min.y;

        ctx->bodies[id].r.min.x = xpos - ctx->bodies[id].offset.x;
        ctx->bodies[id].r.min.y = ypos - ctx->bodies[id].offset.y;

        ctx->bodies[id].r.max.x = min_to_max.x + ctx->bodies[id].r.min.x;
        ctx->bodies[id].r.max.y = min_to_max.y + ctx->bodies[id].r.min.y;

        return 0;
    }
}

int sr_translate_body(sr_Context *ctx, sr_Body_Id id, float xmove, float ymove) {
    if (id >= ctx->num_bodies) {
        return -1;
    } else if (ctx->bodies[id].flags & SR_DISABLED || ctx->bodies[id].priority == SR_PRIORITY_STATIC) {
        return 0;
    } else {
        ctx->bodies[id].r.min.x += xmove;
        ctx->bodies[id].r.min.y += ymove;
        ctx->bodies[id].r.max.x += xmove;
        ctx->bodies[id].r.max.y += ymove;

        return 0;
    }
}

int sr_get_body_pos(sr_Vec2 *pos_out, const sr_Context *ctx, sr_Body_Id id) {
    if (id >= ctx->num_bodies) {
        return -1;
    } else {
        pos_out->x = ctx->bodies[id].r.min.x + ctx->bodies[id].offset.x;
        pos_out->y = ctx->bodies[id].r.min.y + ctx->bodies[id].offset.y;

        return 0;
    }
}

int sr_get_body_pos_comp(float *xpos_out, float *ypos_out, const sr_Context *ctx, sr_Body_Id id) {
    if (id >= ctx->num_bodies) {
        return -1;
    } else {
        *xpos_out = ctx->bodies[id].r.min.x + ctx->bodies[id].offset.x;
        *ypos_out = ctx->bodies[id].r.min.y + ctx->bodies[id].offset.y;

        return 0;
    }
}

int sr_get_body_dim(sr_Vec2 *dim_out, const sr_Context *ctx, sr_Body_Id id) {
    if (id >= ctx->num_bodies) {
        return -1;
    } else {
        dim_out->x = ctx->bodies[id].r.max.x - ctx->bodies[id].r.min.x;
        dim_out->y = ctx->bodies[id].r.max.y - ctx->bodies[id].r.min.y;

        return 0;
    }
}

int sr_get_body_dim_comp(float *xdim_out, float *ydim_out, const sr_Context *ctx, sr_Body_Id id) {
    if (id >= ctx->num_bodies) {
        return -1;
    } else {
        *xdim_out = ctx->bodies[id].r.max.x - ctx->bodies[id].r.min.x;
        *ydim_out = ctx->bodies[id].r.max.y - ctx->bodies[id].r.min.y;

        return 0;
    }
}

int sr_get_body_rect(sr_Rect *rect_out, const sr_Context *ctx, sr_Body_Id id) {
    if (id >= ctx->num_bodies) {
        return -1;
    } else {
        *rect_out = ctx->bodies[id].r;
        return 0;
    }
}

int sr_get_body_rect_comp(float *xmin_out, float *ymin_out, float *xmax_out, float *ymax_out, const sr_Context *ctx, sr_Body_Id id) {
    if (id >= ctx->num_bodies) {
        return -1;
    } else {
        *xmin_out = ctx->bodies[id].r.min.x;
        *ymin_out = ctx->bodies[id].r.min.y;
        *xmax_out = ctx->bodies[id].r.max.x;
        *ymax_out = ctx->bodies[id].r.max.y;

        return 0;
    }
}

int sr_do_bodies_overlap(sr_Context *ctx, sr_Body_Id id1, sr_Body_Id id2) {
    if (id1 >= ctx->num_bodies || id2 >= ctx->num_bodies) {
        return -1;
    } else {
        return sr_do_rects_overlap(ctx->bodies[id1].r, ctx->bodies[id2].r);
    }
}

int sr_get_body_to_body_vector(sr_Vec2 *vec_out, sr_Context *ctx, sr_Body_Id id1, sr_Body_Id id2) {
    if (id1 >= ctx->num_bodies || id2 >= ctx->num_bodies) {
        return -1;
    } else {
        vec_out->x = (ctx->bodies[id2].r.max.x + ctx->bodies[id2].r.min.x) / 2 - (ctx->bodies[id1].r.max.x + ctx->bodies[id1].r.min.x) / 2;
        vec_out->y = (ctx->bodies[id2].r.max.y + ctx->bodies[id2].r.min.y) / 2 - (ctx->bodies[id1].r.max.y + ctx->bodies[id1].r.min.y) / 2;

        return 0;
    }
}

int sr_get_body_to_body_vector_comp(float *xvec_out, float *yvec_out, sr_Context *ctx, sr_Body_Id id1, sr_Body_Id id2) {
    if (id1 >= ctx->num_bodies || id2 >= ctx->num_bodies) {
        return -1;
    } else {
        *xvec_out = (ctx->bodies[id2].r.max.x + ctx->bodies[id2].r.min.x) / 2 - (ctx->bodies[id1].r.max.x + ctx->bodies[id1].r.min.x) / 2;
        *yvec_out = (ctx->bodies[id2].r.max.y + ctx->bodies[id2].r.min.y) / 2 - (ctx->bodies[id1].r.max.y + ctx->bodies[id1].r.min.y) / 2;

        return 0;
    }
}

int sr_get_body_tick_data(sr_Body_Tick_Data *data_out, sr_Context *ctx, sr_Body_Id id) {
    if (id >= ctx->num_bodies) {
        return -1;
    } else {
        *data_out = ctx->bodies_tick_data[id];

        return 0;
    }
}

int sr_did_body_collide(sr_Context *ctx, sr_Body_Id id) {
    if (id >= ctx->num_bodies) {
        return -1;
    } else {
        if (ctx->bodies_tick_data[id].flags & SR_COLLIDED) {
            return 1;
        } else {
            return 0;
        }
    }
}

int sr_did_body_collide_wall(sr_Context *ctx, sr_Body_Id id) {
    if (id >= ctx->num_bodies) {
        return -1;
    } else {
        if (ctx->bodies_tick_data[id].flags & SR_COLLIDED_RIGHT_WALL || ctx->bodies_tick_data[id].flags & SR_COLLIDED_LEFT_WALL) {
            return 1;
        } else {
            return 0;
        }
    }
}

int sr_did_body_collide_ceiling(sr_Context *ctx, sr_Body_Id id) {
    if (id >= ctx->num_bodies) {
        return -1;
    } else {
        if (ctx->bodies_tick_data[id].flags & SR_COLLIDED_CEILING) {
            return 1;
        } else {
            return 0;
        }
    }
}

int sr_did_body_collide_right_wall(sr_Context *ctx, sr_Body_Id id) {
    if (id >= ctx->num_bodies) {
        return -1;
    } else {
        if (ctx->bodies_tick_data[id].flags & SR_COLLIDED_RIGHT_WALL) {
            return 1;
        } else {
            return 0;
        }
    }
}

int sr_did_body_collide_floor(sr_Context *ctx, sr_Body_Id id) {
    if (id >= ctx->num_bodies) {
        return -1;
    } else {
        if (ctx->bodies_tick_data[id].flags & SR_COLLIDED_FLOOR) {
            return 1;
        } else {
            return 0;
        }
    }
}

int sr_did_body_collide_left_wall(sr_Context *ctx, sr_Body_Id id) {
    if (id >= ctx->num_bodies) {
        return -1;
    } else {
        if (ctx->bodies_tick_data[id].flags & SR_COLLIDED_LEFT_WALL) {
            return 1;
        } else {
            return 0;
        }
    }
}

int sr_did_body_collide_up(sr_Context *ctx, sr_Body_Id id) {
    if (id >= ctx->num_bodies) {
        return -1;
    } else {
        if (ctx->bodies_tick_data[id].flags & SR_COLLIDED_UP) {
            return 1;
        } else {
            return 0;
        }
    }
}

int sr_did_body_collide_right(sr_Context *ctx, sr_Body_Id id) {
    if (id >= ctx->num_bodies) {
        return -1;
    } else {
        if (ctx->bodies_tick_data[id].flags & SR_COLLIDED_RIGHT) {
            return 1;
        } else {
            return 0;
        }
    }
}

int sr_did_body_collide_down(sr_Context *ctx, sr_Body_Id id) {
    if (id >= ctx->num_bodies) {
        return -1;
    } else {
        if (ctx->bodies_tick_data[id].flags & SR_COLLIDED_DOWN) {
            return 1;
        } else {
            return 0;
        }
    }
}

int sr_did_body_collide_left(sr_Context *ctx, sr_Body_Id id) {
    if (id >= ctx->num_bodies) {
        return -1;
    } else {
        if (ctx->bodies_tick_data[id].flags & SR_COLLIDED_LEFT) {
            return 1;
        } else {
            return 0;
        }
    }
}

void sr_resolve_collisions(sr_Context *ctx) {
    int i, j;

    memset(ctx->bodies_tick_data, 0, sizeof(sr_Body_Tick_Data) * ctx->num_bodies);

    sr_stable_sort(ctx);

#define SR_B(ind) (ctx->bodies[ctx->bodies_sorted[ind]])
    if (ctx->sweep_direction == SR_SWEEP_X) {

    for (i = 0; i < ctx->num_bodies; ++i) {
        if (SR_B(i).flags & SR_DISABLED) {
            continue;
        }
        for (j = i + 1; j < ctx->num_bodies; ++j) {
            if (SR_B(j).r.min.x > SR_B(i).r.max.x) {
                break;
            } else if (SR_B(j).flags & SR_DISABLED) {
                continue;
            } else {
                if (sr_do_rects_overlap(SR_B(i).r, SR_B(j).r)) {
                    sr_resolve_bodies(ctx, ctx->bodies_sorted[i], ctx->bodies_sorted[j]);
                }
            }
        }
    }

    } else {

    for (i = 0; i < ctx->num_bodies; ++i) {
        if (SR_B(i).flags & SR_DISABLED || SR_B(i).flags & SR_NO_COLLISION) {
            continue;
        }
        for (j = i + 1; j < ctx->num_bodies; ++j) {
            if (SR_B(j).r.min.y > SR_B(i).r.max.y) {
                break;
            } else if (SR_B(j).flags & SR_DISABLED || SR_B(j).flags & SR_NO_COLLISION) {
                continue;
            } else {
                if (sr_do_rects_overlap(SR_B(i).r, SR_B(j).r)) {
                    sr_resolve_bodies(ctx, ctx->bodies_sorted[i], ctx->bodies_sorted[j]);
                }
            }
        }
    }

    }
#undef SR_B
}

#endif /* #ifdef SRECT_IMPLEMENTATION */

/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org/>
*/
