// my java lib slopped to C, i did make one verion myself but for now i am lazy to update it so this will do.

#ifndef NORI_H
#define NORI_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NORI_STATIC
#define NORI_API static inline
#else
#define NORI_API
#endif

#ifndef NORI_MAX_DEPTH
#define NORI_MAX_DEPTH 128
#endif

typedef enum nori_status {
    NORI_OK = 0,
    NORI_ERR_NOMEM,
    NORI_ERR_ARG,
    NORI_ERR_SYNTAX,
    NORI_ERR_RANGE,
    NORI_ERR_REF,
    NORI_ERR_LINK,
    NORI_ERR_DEPTH,
    NORI_ERR_IO,
    NORI_ERR_UNREPRESENTABLE
} nori_status;

typedef enum nori_type {
    NORI_NUMBER,
    NORI_STRING,
    NORI_IDENTIFIER,
    NORI_BOOL,
    NORI_OBJECT,
    NORI_ARRAY,
    NORI_REF,
    NORI_LINK
} nori_type;

typedef enum nori_num_kind { NORI_INT, NORI_LONG, NORI_FLOAT, NORI_DOUBLE } nori_num_kind;

typedef struct nori_number {
    nori_num_kind kind;
    int radix;

    union {
        int32_t i;
        int64_t l;
        float f;
        double d;
    } v;
} nori_number;

typedef struct nori_doc nori_doc;
typedef struct nori_node nori_node;

typedef struct nori_prop {
    char *name;
    nori_node *value;
} nori_prop;

typedef struct nori_error {
    nori_status status;
    size_t line, column;
    char message[160];
} nori_error;

struct nori_node {
    nori_type type;
    nori_doc *owner;

    union {
        nori_number num;

        struct {
            char *data;
            size_t len;
        } str;

        int boolean;

        struct {
            char *name;
            nori_prop *props;
            size_t count, cap;
            nori_node *def;
        } obj;

        struct {
            nori_node **items;
            size_t count, cap;
            nori_node *def;
        } arr;

        struct {
            char *name;
            void *ptr;
        } ref;

        struct {
            char *path;
            size_t line, col;
            nori_node *target;
            int state;
            unsigned gen;
            nori_node *chain;
        } link;
    } as;
};

NORI_API nori_doc *nori_new(void);
NORI_API void nori_free(nori_doc *doc);
NORI_API nori_status nori_add_ref(nori_doc *doc, const char *name, void *ptr);
NORI_API nori_status nori_parse(nori_doc *doc, const char *src, size_t len);
NORI_API nori_status nori_parse_str(nori_doc *doc, const char *src);
NORI_API nori_status nori_parse_file(nori_doc *doc, const char *path);
NORI_API nori_node *nori_root(const nori_doc *doc);
NORI_API nori_status nori_set_root(nori_doc *doc, nori_node *node);
NORI_API const nori_error *nori_get_error(const nori_doc *doc);
NORI_API const char *nori_strerror(nori_status status);

NORI_API nori_node *nori_get(const nori_node *node, const char *path);
NORI_API nori_node *nori_doc_get(const nori_doc *doc, const char *path);
NORI_API nori_node *nori_follow(const nori_node *node);
NORI_API size_t nori_count(const nori_node *node);
NORI_API nori_node *nori_at(const nori_node *node, size_t index);
NORI_API const char *nori_key_at(const nori_node *obj, size_t index);
NORI_API nori_node *nori_default(const nori_node *node);
NORI_API const char *nori_object_name(const nori_node *obj);

NORI_API int32_t nori_get_int(const nori_node *node, int32_t fallback);
NORI_API int64_t nori_get_long(const nori_node *node, int64_t fallback);
NORI_API float nori_get_float(const nori_node *node, float fallback);
NORI_API double nori_get_double(const nori_node *node, double fallback);
NORI_API int nori_get_bool(const nori_node *node, int fallback);
NORI_API const char *nori_get_string(const nori_node *node, size_t *len);
NORI_API void *nori_get_ref(const nori_node *node, void *fallback);

NORI_API nori_node *nori_new_int(nori_doc *doc, int32_t v);
NORI_API nori_node *nori_new_long(nori_doc *doc, int64_t v);
NORI_API nori_node *nori_new_float(nori_doc *doc, float v);
NORI_API nori_node *nori_new_double(nori_doc *doc, double v);
NORI_API nori_node *nori_new_hex(nori_doc *doc, int32_t v);
NORI_API nori_node *nori_new_binary(nori_doc *doc, int32_t v);
NORI_API nori_node *nori_new_octal(nori_doc *doc, int32_t v);
NORI_API nori_node *nori_new_bool(nori_doc *doc, int v);
NORI_API nori_node *nori_new_string(nori_doc *doc, const char *s);
NORI_API nori_node *nori_new_string_n(nori_doc *doc, const char *s, size_t len);
NORI_API nori_node *nori_new_ident(nori_doc *doc, const char *name);
NORI_API nori_node *nori_new_object(nori_doc *doc, const char *name_or_null);
NORI_API nori_node *nori_new_array(nori_doc *doc);
NORI_API nori_node *nori_new_ref(nori_doc *doc, const char *name);
NORI_API nori_node *nori_new_link(nori_doc *doc, const char *path);
NORI_API nori_status nori_insert(nori_node *container, const char *name, nori_node *value);
NORI_API nori_status nori_set_default(nori_node *container, nori_node *value);

NORI_API nori_status nori_stringify(const nori_node *node, char **out, size_t *len);
NORI_API nori_status nori_stringify_doc(const nori_doc *doc, char **out, size_t *len);
NORI_API nori_status nori_print(const nori_node *node, FILE *out);

#ifdef __cplusplus
}
#endif
#endif /* NORI_H */

#ifdef NORI_IMPLEMENTATION
#ifndef NORI_IMPLEMENTATION_DONE
#define NORI_IMPLEMENTATION_DONE

#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define NORI__ALIGN _Alignof(max_align_t)
#else
typedef union {
    long long ll;
    long double ld;
    double d;
    void *p;
    void (*fp)(void);
} nori__maxalign;

#define NORI__ALIGN                                                                                \
    offsetof(                                                                                      \
        struct {                                                                                   \
            char c;                                                                                \
            nori__maxalign m;                                                                      \
        },                                                                                         \
        m                                                                                          \
    )
#endif
#define NORI__CHUNK 8192u
#define NORI__HDR ((sizeof(nori__chunk) + NORI__ALIGN - 1) / NORI__ALIGN * NORI__ALIGN)

typedef struct nori__chunk {
    struct nori__chunk *next;
    size_t used, cap;
} nori__chunk;

typedef struct nori__ref {
    char *name;
    void *ptr;
} nori__ref;

struct nori_doc {
    nori__chunk *chunks;
    nori__chunk *cur;
    nori_node *root;
    nori_node *links;
    nori__ref *refs;
    size_t nrefs, refcap;
    unsigned gen;
    nori_error err;
};

static void
nori__verr(nori_doc *d, nori_status s, size_t line, size_t col, const char *fmt, va_list ap) {
    if (d->err.status != NORI_OK)
        return;
    d->err.status = s;
    d->err.line = line;
    d->err.column = col;
    vsnprintf(d->err.message, sizeof d->err.message, fmt, ap);
}

static void nori__err(nori_doc *d, nori_status s, size_t line, size_t col, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    nori__verr(d, s, line, col, fmt, ap);
    va_end(ap);
}

static void *nori__alloc(nori_doc *d, size_t n) {
    nori__chunk *c;
    if (n == 0)
        n = 1;
    if (n > SIZE_MAX - NORI__ALIGN)
        goto oom;
    n = (n + NORI__ALIGN - 1) / NORI__ALIGN * NORI__ALIGN;
    c = d->cur;
    if (!c || c->cap - c->used < n) {
        size_t cap = n > NORI__CHUNK ? n : NORI__CHUNK;
        nori__chunk *nc;
        if (cap > SIZE_MAX - NORI__HDR)
            goto oom;
        nc = (nori__chunk *) malloc(NORI__HDR + cap);
        if (!nc)
            goto oom;
        nc->used = 0;
        nc->cap = cap;
        nc->next = d->chunks;
        d->chunks = nc;
        if (n <= NORI__CHUNK)
            d->cur = nc;
        c = nc;
    }
    {
        void *p = (unsigned char *) c + NORI__HDR + c->used;
        c->used += n;
        return p;
    }
oom:
    nori__err(d, NORI_ERR_NOMEM, 0, 0, "out of memory");
    return NULL;
}

static char *nori__strndup(nori_doc *d, const char *s, size_t n) {
    char *p;
    if (n == SIZE_MAX)
        return NULL;
    p = (char *) nori__alloc(d, n + 1);
    if (!p)
        return NULL;
    if (n)
        memcpy(p, s, n);
    p[n] = 0;
    return p;
}

static nori_node *nori__node(nori_doc *d, nori_type t) {
    nori_node *n = (nori_node *) nori__alloc(d, sizeof *n);
    if (!n)
        return NULL;
    memset(n, 0, sizeof *n);
    n->type = t;
    n->owner = d;
    return n;
}

static int nori__is_digit(int c) {
    return c >= '0' && c <= '9';
}

static int nori__is_ident_start(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c >= 0x80;
}

static int nori__is_ident_char(int c) {
    return nori__is_ident_start(c) || nori__is_digit(c);
}

static int nori__valid_ident(const char *s, size_t n) {
    size_t i;
    if (n == 0 || !nori__is_ident_start((unsigned char) s[0]))
        return 0;
    for (i = 1; i < n; i++)
        if (!nori__is_ident_char((unsigned char) s[i]))
            return 0;
    if (n == 4 && memcmp(s, "true", 4) == 0)
        return 0;
    if (n == 5 && memcmp(s, "false", 5) == 0)
        return 0;
    return 1;
}

static int nori__valid_name(const char *s, size_t n) {
    size_t i;
    if (n == 0)
        return 0;
    for (i = 0; i < n; i++)
        if (!nori__is_ident_char((unsigned char) s[i]))
            return 0;
    return 1;
}

static int nori__valid_path(const char *s, size_t n) {
    size_t i;
    if (n == 0 || s[0] == '.' || s[n - 1] == '.')
        return 0;
    for (i = 0; i < n; i++) {
        if (s[i] == '.') {
            if (s[i + 1] == '.')
                return 0;
        } else if (!nori__is_ident_char((unsigned char) s[i]))
            return 0;
    }
    return 1;
}

static int32_t nori__u32_to_i32(uint32_t u) {
    return u <= (uint32_t) INT32_MAX ? (int32_t) u : -(int32_t) (UINT32_MAX - u) - 1;
}

static int64_t nori__u64_to_i64(uint64_t u) {
    return u <= (uint64_t) INT64_MAX ? (int64_t) u : -(int64_t) (UINT64_MAX - u) - 1;
}

static int nori__finite(double x) {
    return x == x && x <= DBL_MAX && x >= -DBL_MAX;
}

static const char *nori__dp(void) {
    const struct lconv *lc = localeconv();
    return (lc && lc->decimal_point && lc->decimal_point[0]) ? lc->decimal_point : ".";
}

static int nori__strtod_c(const char *text, size_t n, double *out, int *overflow) {
    const char *dp = nori__dp();
    size_t dl = strlen(dp), i, o = 0;
    char *tmp = (char *) malloc(n + dl + 1), *end;
    double v;
    if (!tmp)
        return 0;
    for (i = 0; i < n; i++) {
        if (text[i] == '.') {
            memcpy(tmp + o, dp, dl);
            o += dl;
        } else
            tmp[o++] = text[i];
    }
    tmp[o] = 0;
    errno = 0;
    v = strtod(tmp, &end);
    *overflow = (errno == ERANGE && (v > 1.0 || v < -1.0));
    i = (size_t) (end - tmp);
    free(tmp);
    if (i != o)
        return 0;
    *out = v;
    return 1;
}

typedef enum {
    T_EOF,
    T_ERROR,
    T_IDENT,
    T_STRING,
    T_NUMBER,
    T_BOOL,
    T_LBRACE,
    T_RBRACE,
    T_LBRACKET,
    T_RBRACKET,
    T_EQUALS,
    T_REF,
    T_LINK,
    T_BANG
} nori__tt;

typedef struct {
    nori__tt type;
    const char *s;
    size_t len, line, col;
} nori__tok;

typedef struct {
    nori_doc *doc;
    const char *src;
    size_t len, pos, line, col;
    nori__tok cur, nxt;
    int has_nxt;
    unsigned depth;
} nori__parser;

static int nori__at(const nori__parser *p, size_t off) {
    return p->pos + off < p->len ? (unsigned char) p->src[p->pos + off] : -1;
}

static void nori__adv(nori__parser *p) {
    if (p->pos >= p->len)
        return;
    if (p->src[p->pos++] == '\n') {
        p->line++;
        p->col = 1;
    } else
        p->col++;
}

static void
nori__lex_fail(nori__parser *p, nori__tok *t, size_t line, size_t col, const char *msg) {
    nori__err(p->doc, NORI_ERR_SYNTAX, line, col, "%s", msg);
    t->type = T_ERROR;
}

static void nori__lex_number(nori__parser *p, nori__tok *t) {
    int c = nori__at(p, 0), c1;
    if (c == '+' || c == '-')
        nori__adv(p);
    c1 = nori__at(p, 1);
    if (nori__at(p, 0) == '0' &&
        (c1 == 'x' || c1 == 'X' || c1 == 'b' || c1 == 'B' || c1 == 'o' || c1 == 'O')) {
        nori__adv(p);
        nori__adv(p);
        while (nori__is_ident_char(nori__at(p, 0)))
            nori__adv(p);
    } else {
        while (nori__is_digit(nori__at(p, 0)))
            nori__adv(p);
        if (nori__at(p, 0) == '.') {
            nori__adv(p);
            while (nori__is_digit(nori__at(p, 0)))
                nori__adv(p);
        }
        c = nori__at(p, 0);
        if (c == 'e' || c == 'E') {
            nori__adv(p);
            c = nori__at(p, 0);
            if (c == '+' || c == '-')
                nori__adv(p);
            while (nori__is_digit(nori__at(p, 0)))
                nori__adv(p);
        }
        while (nori__is_ident_char(nori__at(p, 0)))
            nori__adv(p);
    }
    t->type = T_NUMBER;
    t->len = (size_t) (p->src + p->pos - t->s);
}

static void nori__lex(nori__parser *p, nori__tok *t) {
    int c;
    for (;;) {
        c = nori__at(p, 0);
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f')
            nori__adv(p);
        else if (c == '/' && nori__at(p, 1) == '/') {
            while (nori__at(p, 0) != -1 && nori__at(p, 0) != '\n')
                nori__adv(p);
        } else if (c == '/' && nori__at(p, 1) == '*') {
            size_t l = p->line, co = p->col;
            nori__adv(p);
            nori__adv(p);
            for (;;) {
                if (nori__at(p, 0) == -1) {
                    t->line = l;
                    t->col = co;
                    nori__lex_fail(p, t, l, co, "unterminated comment");
                    return;
                }
                if (nori__at(p, 0) == '*' && nori__at(p, 1) == '/') {
                    nori__adv(p);
                    nori__adv(p);
                    break;
                }
                nori__adv(p);
            }
        } else
            break;
    }
    t->line = p->line;
    t->col = p->col;
    t->s = p->src + p->pos;
    t->len = 0;
    c = nori__at(p, 0);
    if (c == -1) {
        t->type = T_EOF;
        return;
    }
    if (nori__is_ident_start(c)) {
        while (nori__is_ident_char(nori__at(p, 0)))
            nori__adv(p);
        t->len = (size_t) (p->src + p->pos - t->s);
        t->type = ((t->len == 4 && memcmp(t->s, "true", 4) == 0) ||
                   (t->len == 5 && memcmp(t->s, "false", 5) == 0))
                      ? T_BOOL
                      : T_IDENT;
        return;
    }
    if (c == '"') {
        nori__adv(p);
        t->s = p->src + p->pos;
        for (;;) {
            c = nori__at(p, 0);
            if (c == -1) {
                nori__lex_fail(p, t, t->line, t->col, "unterminated string literal");
                return;
            }
            if (c == '"')
                break;
            if (c == '\\') {
                nori__adv(p);
                if (nori__at(p, 0) == -1)
                    continue;
            }
            nori__adv(p);
        }
        t->len = (size_t) (p->src + p->pos - t->s);
        nori__adv(p);
        t->type = T_STRING;
        return;
    }
    if (nori__is_digit(c) || c == '+' || c == '-') {
        nori__lex_number(p, t);
        return;
    }
    if (c == '@') {
        nori__adv(p);
        t->s = p->src + p->pos;
        while (nori__is_ident_char(nori__at(p, 0)))
            nori__adv(p);
        t->len = (size_t) (p->src + p->pos - t->s);
        if (!t->len) {
            nori__lex_fail(p, t, t->line, t->col, "expected a name after '@'");
            return;
        }
        t->type = T_REF;
        return;
    }
    if (c == '$') {
        nori__adv(p);
        t->s = p->src + p->pos;
        for (;;) {
            c = nori__at(p, 0);
            if (nori__is_ident_char(c) || (c == '.' && nori__is_ident_char(nori__at(p, 1))))
                nori__adv(p);
            else
                break;
        }
        t->len = (size_t) (p->src + p->pos - t->s);
        if (!t->len) {
            nori__lex_fail(p, t, t->line, t->col, "expected a path after '$'");
            return;
        }
        t->type = T_LINK;
        return;
    }
    t->len = 1;
    switch (c) {
        case '{':
            t->type = T_LBRACE;
            break;
        case '}':
            t->type = T_RBRACE;
            break;
        case '[':
            t->type = T_LBRACKET;
            break;
        case ']':
            t->type = T_RBRACKET;
            break;
        case '=':
            t->type = T_EQUALS;
            break;
        case '!':
            t->type = T_BANG;
            break;
        default:
            {
                char m[48];
                if (c >= 0x20 && c < 0x7f)
                    snprintf(m, sizeof m, "unexpected character '%c'", c);
                else
                    snprintf(m, sizeof m, "unexpected byte 0x%02X", (unsigned) c);
                nori__lex_fail(p, t, t->line, t->col, m);
                return;
            }
    }
    nori__adv(p);
}

static nori_status nori__arr_push(nori_node *a, nori_node *v) {
    if (a->as.arr.count == a->as.arr.cap) {
        size_t nc = a->as.arr.cap ? a->as.arr.cap * 2 : 4;
        nori_node **ni;
        if (a->as.arr.cap > SIZE_MAX / 2 / sizeof *ni) {
            nori__err(a->owner, NORI_ERR_NOMEM, 0, 0, "out of memory");
            return NORI_ERR_NOMEM;
        }
        ni = (nori_node **) nori__alloc(a->owner, nc * sizeof *ni);
        if (!ni)
            return NORI_ERR_NOMEM;
        if (a->as.arr.count)
            memcpy(ni, a->as.arr.items, a->as.arr.count * sizeof *ni);
        a->as.arr.items = ni;
        a->as.arr.cap = nc;
    }
    a->as.arr.items[a->as.arr.count++] = v;
    return NORI_OK;
}

static nori_status nori__obj_push(nori_node *o, char *name, nori_node *v) {
    if (o->as.obj.count == o->as.obj.cap) {
        size_t nc = o->as.obj.cap ? o->as.obj.cap * 2 : 4;
        nori_prop *np;
        if (o->as.obj.cap > SIZE_MAX / 2 / sizeof *np) {
            nori__err(o->owner, NORI_ERR_NOMEM, 0, 0, "out of memory");
            return NORI_ERR_NOMEM;
        }
        np = (nori_prop *) nori__alloc(o->owner, nc * sizeof *np);
        if (!np)
            return NORI_ERR_NOMEM;
        if (o->as.obj.count)
            memcpy(np, o->as.obj.props, o->as.obj.count * sizeof *np);
        o->as.obj.props = np;
        o->as.obj.cap = nc;
    }
    o->as.obj.props[o->as.obj.count].name = name;
    o->as.obj.props[o->as.obj.count].value = v;
    o->as.obj.count++;
    return NORI_OK;
}

#define NORI__LINK_BUSY 1
#define NORI__LINK_DONE 2

static nori_node *nori__follow(nori_node *n, unsigned depth);

static int nori__parse_index(const char *s, size_t n, size_t *out) {
    size_t v = 0, i;
    if (!n)
        return 0;
    for (i = 0; i < n; i++) {
        size_t dg;
        if (!nori__is_digit((unsigned char) s[i]))
            return 0;
        dg = (size_t) (s[i] - '0');
        if (v > (SIZE_MAX - dg) / 10)
            return 0;
        v = v * 10 + dg;
    }
    *out = v;
    return 1;
}

static nori_node *nori__step(nori_node *cur, const char *seg, size_t n, unsigned depth) {
    size_t i, idx;
    cur = nori__follow(cur, depth);
    if (!cur)
        return NULL;
    if (cur->type == NORI_ARRAY) {
        if (!nori__parse_index(seg, n, &idx) || idx >= cur->as.arr.count)
            return NULL;
        return cur->as.arr.items[idx];
    }
    if (cur->type == NORI_OBJECT) {
        for (i = 0; i < cur->as.obj.count; i++) {
            const char *k = cur->as.obj.props[i].name;
            if (strlen(k) == n && memcmp(k, seg, n) == 0)
                return cur->as.obj.props[i].value;
        }
    }
    return NULL;
}

static nori_node *nori__path(nori_node *cur, const char *path, unsigned depth) {
    size_t start = 0, plen = strlen(path), end;
    if (plen == 0)
        return cur;
    for (;;) {
        end = start;
        while (end < plen && path[end] != '.')
            end++;
        cur = nori__step(cur, path + start, end - start, depth);
        if (!cur)
            return NULL;
        if (end >= plen)
            return cur;
        start = end + 1;
    }
}

static nori_node *nori__doc_start(const nori_doc *d) {
    nori_node *r = d->root;
    if (r && r->type == NORI_ARRAY && r->as.arr.count == 1)
        r = r->as.arr.items[0];
    return r;
}

static nori_node *nori__follow(nori_node *n, unsigned depth) {
    nori_doc *d;
    nori_node *t;
    if (!n || n->type != NORI_LINK)
        return n;
    d = n->owner;
    if (n->as.link.gen == d->gen && n->as.link.state == NORI__LINK_DONE)
        return n->as.link.target;
    if (n->as.link.gen == d->gen && n->as.link.state == NORI__LINK_BUSY)
        return NULL;
    if (depth > NORI_MAX_DEPTH)
        return NULL;
    n->as.link.gen = d->gen;
    n->as.link.state = NORI__LINK_BUSY;
    t = nori__doc_start(d);
    t = t ? nori__path(t, n->as.link.path, depth + 1) : NULL;
    t = nori__follow(t, depth + 1);
    n->as.link.state = NORI__LINK_DONE;
    n->as.link.target = t;
    return t;
}

static int nori__parse_number(nori_doc *d, const nori__tok *t, nori_number *out) {
    const char *s = t->s;
    size_t n = t->len, i = 0;
    int neg = 0, is_long = 0, is_float_suffix = 0, is_fp = 0;
    memset(out, 0, sizeof *out);
    if (i < n && (s[i] == '+' || s[i] == '-')) {
        neg = (s[i] == '-');
        i++;
    }

    if (n - i >= 2 && s[i] == '0' &&
        (s[i + 1] == 'x' || s[i + 1] == 'X' || s[i + 1] == 'b' || s[i + 1] == 'B' ||
         s[i + 1] == 'o' || s[i + 1] == 'O')) {
        char pc = s[i + 1];
        int radix = (pc == 'x' || pc == 'X') ? 16 : (pc == 'b' || pc == 'B') ? 2 : 8;
        uint64_t mag = 0;
        size_t digits = 0;
        i += 2;
        for (; i < n; i++) {
            int c = (unsigned char) s[i], v;
            if (c >= '0' && c <= '9')
                v = c - '0';
            else if (c >= 'a' && c <= 'f')
                v = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                v = c - 'A' + 10;
            else
                break;
            if (v >= radix)
                break;
            if (mag > (UINT64_MAX - (uint64_t) v) / (uint64_t) radix)
                goto range;
            mag = mag * (uint64_t) radix + (uint64_t) v;
            digits++;
        }
        if (!digits)
            goto bad;
        if (i < n && (s[i] == 'l' || s[i] == 'L')) {
            is_long = 1;
            i++;
        }
        if (i != n)
            goto bad;
        out->radix = radix;
        if (!is_long && (neg ? mag <= 0x80000000u : mag <= 0xFFFFFFFFu)) {
            out->kind = NORI_INT;
            out->v.i = nori__u32_to_i32(neg ? (uint32_t) (0u - (uint32_t) mag) : (uint32_t) mag);
        } else {
            out->kind = NORI_LONG;
            out->v.l = nori__u64_to_i64(neg ? (uint64_t) 0 - mag : mag);
        }
        return 1;
    }

    {
        size_t ds = i, intd, fracd = 0, mend;
        while (i < n && nori__is_digit((unsigned char) s[i]))
            i++;
        intd = i - ds;
        if (i < n && s[i] == '.') {
            size_t fs;
            is_fp = 1;
            i++;
            fs = i;
            while (i < n && nori__is_digit((unsigned char) s[i]))
                i++;
            fracd = i - fs;
        }
        if (intd + fracd == 0)
            goto bad;
        if (i < n && (s[i] == 'e' || s[i] == 'E')) {
            size_t es;
            is_fp = 1;
            i++;
            if (i < n && (s[i] == '+' || s[i] == '-'))
                i++;
            es = i;
            while (i < n && nori__is_digit((unsigned char) s[i]))
                i++;
            if (i == es)
                goto bad;
        }
        mend = i;
        if (i < n) {
            if (s[i] == 'f' || s[i] == 'F')
                is_float_suffix = 1;
            else if (s[i] == 'l' || s[i] == 'L')
                is_long = 1;
            else
                goto bad;
            i++;
            if (i != n)
                goto bad;
        }
        if (is_long && is_fp)
            goto bad;
        out->radix = 10;
        if (is_fp || is_float_suffix) {
            double v;
            int ovf;
            if (!nori__strtod_c(s, mend, &v, &ovf)) {
                nori__err(d, NORI_ERR_NOMEM, t->line, t->col, "out of memory");
                return 0;
            }
            if (ovf || !nori__finite(v))
                goto range;
            if (is_float_suffix) {
                if (v >= 3.40282356779733661637539395458142568448e38 ||
                    v <= -3.40282356779733661637539395458142568448e38)
                    goto range;
                out->kind = NORI_FLOAT;
                out->v.f = v > (double) FLT_MAX    ? FLT_MAX
                           : v < -(double) FLT_MAX ? -FLT_MAX
                                                   : (float) v;
            } else {
                out->kind = NORI_DOUBLE;
                out->v.d = v;
            }
            return 1;
        } else {
            uint64_t mag = 0;
            int64_t val;
            for (i = ds; i < ds + intd; i++) {
                uint64_t dg = (uint64_t) (s[i] - '0');
                if (mag > (UINT64_MAX - dg) / 10u)
                    goto range;
                mag = mag * 10u + dg;
            }
            if (neg) {
                if (mag > (uint64_t) INT64_MAX + 1u)
                    goto range;
                val = (mag == (uint64_t) INT64_MAX + 1u) ? INT64_MIN : -(int64_t) mag;
            } else {
                if (mag > (uint64_t) INT64_MAX)
                    goto range;
                val = (int64_t) mag;
            }
            if (!is_long && val >= INT32_MIN && val <= INT32_MAX) {
                out->kind = NORI_INT;
                out->v.i = (int32_t) val;
            } else {
                out->kind = NORI_LONG;
                out->v.l = val;
            }
            return 1;
        }
    }
bad:
    nori__err(
        d,
        NORI_ERR_SYNTAX,
        t->line,
        t->col,
        "invalid numeric literal '%.*s'",
        (int) (n < 32 ? n : 32),
        s
    );
    return 0;
range:
    nori__err(
        d,
        NORI_ERR_RANGE,
        t->line,
        t->col,
        "numeric literal '%.*s' is out of range",
        (int) (n < 32 ? n : 32),
        s
    );
    return 0;
}

static int nori__hex(int c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static int nori__hex4(const char *s, size_t n, size_t i, uint32_t *out) {
    uint32_t v = 0;
    size_t k;
    if (n - i < 4)
        return 0;
    for (k = 0; k < 4; k++) {
        int h = nori__hex((unsigned char) s[i + k]);
        if (h < 0)
            return 0;
        v = v * 16u + (uint32_t) h;
    }
    *out = v;
    return 1;
}

static size_t nori__utf8(char *o, uint32_t cp) {
    if (cp < 0x80) {
        o[0] = (char) cp;
        return 1;
    }
    if (cp < 0x800) {
        o[0] = (char) (0xC0 | (cp >> 6));
        o[1] = (char) (0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000) {
        o[0] = (char) (0xE0 | (cp >> 12));
        o[1] = (char) (0x80 | ((cp >> 6) & 0x3F));
        o[2] = (char) (0x80 | (cp & 0x3F));
        return 3;
    }
    o[0] = (char) (0xF0 | (cp >> 18));
    o[1] = (char) (0x80 | ((cp >> 12) & 0x3F));
    o[2] = (char) (0x80 | ((cp >> 6) & 0x3F));
    o[3] = (char) (0x80 | (cp & 0x3F));
    return 4;
}

static int nori__decode_string(nori_doc *d, const nori__tok *t, char **out, size_t *olen) {
    const char *s = t->s;
    size_t n = t->len, i = 0, o = 0;
    char *buf = (char *) nori__alloc(d, n + 1);
    if (!buf)
        return 0;
    while (i < n) {
        unsigned char c = (unsigned char) s[i++];
        if (c != '\\') {
            buf[o++] = (char) c;
            continue;
        }
        if (i >= n)
            goto bad;
        c = (unsigned char) s[i++];
        switch (c) {
            case 'n':
                buf[o++] = '\n';
                break;
            case 'r':
                buf[o++] = '\r';
                break;
            case 't':
                buf[o++] = '\t';
                break;
            case 'b':
                buf[o++] = '\b';
                break;
            case 'f':
                buf[o++] = '\f';
                break;
            case '0':
                buf[o++] = '\0';
                break;
            case '\\':
            case '"':
            case '\'':
            case '/':
                buf[o++] = (char) c;
                break;
            case 'x':
                {
                    int h1, h2;
                    if (n - i < 2)
                        goto bad;
                    h1 = nori__hex((unsigned char) s[i]);
                    h2 = nori__hex((unsigned char) s[i + 1]);
                    if (h1 < 0 || h2 < 0)
                        goto bad;
                    buf[o++] = (char) (h1 * 16 + h2);
                    i += 2;
                    break;
                }
            case 'u':
                {
                    uint32_t cp, lo;
                    if (!nori__hex4(s, n, i, &cp))
                        goto bad;
                    i += 4;
                    if (cp >= 0xD800 && cp <= 0xDBFF) {
                        if (n - i < 2 || s[i] != '\\' || s[i + 1] != 'u' ||
                            !nori__hex4(s, n, i + 2, &lo) || lo < 0xDC00 || lo > 0xDFFF)
                            goto bad;
                        i += 6;
                        cp = 0x10000u + ((cp - 0xD800u) << 10) + (lo - 0xDC00u);
                    } else if (cp >= 0xDC00 && cp <= 0xDFFF)
                        goto bad;
                    o += nori__utf8(buf + o, cp);
                    break;
                }
            default:
                goto bad;
        }
    }
    buf[o] = 0;
    *out = buf;
    *olen = o;
    return 1;
bad:
    nori__err(d, NORI_ERR_SYNTAX, t->line, t->col, "invalid escape sequence in string");
    return 0;
}

static const char *nori__tt_name(nori__tt t) {
    switch (t) {
        case T_EOF:
            return "end of input";
        case T_ERROR:
            return "invalid token";
        case T_IDENT:
            return "identifier";
        case T_STRING:
            return "string";
        case T_NUMBER:
            return "number";
        case T_BOOL:
            return "boolean";
        case T_LBRACE:
            return "'{'";
        case T_RBRACE:
            return "'}'";
        case T_LBRACKET:
            return "'['";
        case T_RBRACKET:
            return "']'";
        case T_EQUALS:
            return "'='";
        case T_REF:
            return "reference";
        case T_LINK:
            return "link";
        case T_BANG:
            return "'!'";
    }
    return "token";
}

static void nori__next(nori__parser *p) {
    if (p->has_nxt) {
        p->cur = p->nxt;
        p->has_nxt = 0;
    } else
        nori__lex(p, &p->cur);
}

static const nori__tok *nori__peek1(nori__parser *p) {
    if (!p->has_nxt) {
        nori__lex(p, &p->nxt);
        p->has_nxt = 1;
    }
    return &p->nxt;
}

static void nori__unexpected(nori__parser *p, const char *expected) {
    nori__err(
        p->doc,
        NORI_ERR_SYNTAX,
        p->cur.line,
        p->cur.col,
        "unexpected %s, expected %s",
        nori__tt_name(p->cur.type),
        expected
    );
}

static nori_node *nori__p_value(nori__parser *p);

static nori_node *nori__p_object_body(nori__parser *p, char *name) {
    nori_doc *d = p->doc;
    nori_node *o;
    nori_node *res = NULL;
    if (++p->depth > NORI_MAX_DEPTH) {
        nori__err(
            d,
            NORI_ERR_DEPTH,
            p->cur.line,
            p->cur.col,
            "nesting deeper than %d levels",
            NORI_MAX_DEPTH
        );
        p->depth--;
        return NULL;
    }
    o = nori__node(d, NORI_OBJECT);
    if (!o)
        goto out;
    o->as.obj.name = name;
    nori__next(p);
    while (p->cur.type != T_RBRACE) {
        nori__tok key;
        nori_node *v;
        char *kname;
        if (p->cur.type == T_BANG) {
            if (o->as.obj.def) {
                nori__err(
                    d, NORI_ERR_SYNTAX, p->cur.line, p->cur.col, "duplicate '!' default value"
                );
                goto out;
            }
            nori__next(p);
            if (p->cur.type != T_EQUALS) {
                nori__unexpected(p, "'='");
                goto out;
            }
            nori__next(p);
            v = nori__p_value(p);
            if (!v)
                goto out;
            o->as.obj.def = v;
            continue;
        }
        if (p->cur.type != T_IDENT) {
            nori__unexpected(p, "a property name or '}'");
            goto out;
        }
        key = p->cur;
        kname = nori__strndup(d, key.s, key.len);
        if (!kname)
            goto out;
        nori__next(p);
        if (p->cur.type == T_LBRACE)
            v = nori__p_object_body(p, kname);
        else {
            if (p->cur.type != T_EQUALS) {
                nori__unexpected(p, "'=' or '{'");
                goto out;
            }
            nori__next(p);
            v = nori__p_value(p);
        }
        if (!v || nori__obj_push(o, kname, v) != NORI_OK)
            goto out;
    }
    nori__next(p);
    res = o;
out:
    p->depth--;
    return res;
}

static nori_node *nori__p_array(nori__parser *p) {
    nori_doc *d = p->doc;
    nori_node *a, *res = NULL;
    if (++p->depth > NORI_MAX_DEPTH) {
        nori__err(
            d,
            NORI_ERR_DEPTH,
            p->cur.line,
            p->cur.col,
            "nesting deeper than %d levels",
            NORI_MAX_DEPTH
        );
        p->depth--;
        return NULL;
    }
    a = nori__node(d, NORI_ARRAY);
    if (!a)
        goto out;
    nori__next(p);
    while (p->cur.type != T_RBRACKET) {
        nori_node *v;
        if (p->cur.type == T_BANG) {
            if (a->as.arr.def) {
                nori__err(
                    d, NORI_ERR_SYNTAX, p->cur.line, p->cur.col, "duplicate '!' default value"
                );
                goto out;
            }
            nori__next(p);
            if (p->cur.type != T_EQUALS) {
                nori__unexpected(p, "'='");
                goto out;
            }
            nori__next(p);
            v = nori__p_value(p);
            if (!v)
                goto out;
            a->as.arr.def = v;
            continue;
        }
        v = nori__p_value(p);
        if (!v || nori__arr_push(a, v) != NORI_OK)
            goto out;
    }
    nori__next(p);
    res = a;
out:
    p->depth--;
    return res;
}

static nori_node *nori__p_value(nori__parser *p) {
    nori_doc *d = p->doc;
    nori__tok t = p->cur;
    nori_node *n;
    switch (t.type) {
        case T_NUMBER:
            n = nori__node(d, NORI_NUMBER);
            if (!n || !nori__parse_number(d, &t, &n->as.num))
                return NULL;
            nori__next(p);
            return n;
        case T_STRING:
            n = nori__node(d, NORI_STRING);
            if (!n || !nori__decode_string(d, &t, &n->as.str.data, &n->as.str.len))
                return NULL;
            nori__next(p);
            return n;
        case T_BOOL:
            n = nori__node(d, NORI_BOOL);
            if (!n)
                return NULL;
            n->as.boolean = (t.len == 4);
            nori__next(p);
            return n;
        case T_IDENT:
            {
                char *name = nori__strndup(d, t.s, t.len);
                if (!name)
                    return NULL;
                if (nori__peek1(p)->type == T_LBRACE) {
                    nori__next(p);
                    return nori__p_object_body(p, name);
                }
                n = nori__node(d, NORI_IDENTIFIER);
                if (!n)
                    return NULL;
                n->as.str.data = name;
                n->as.str.len = t.len;
                nori__next(p);
                return n;
            }
        case T_REF:
            {
                size_t i;
                n = nori__node(d, NORI_REF);
                if (!n)
                    return NULL;
                for (i = 0; i < d->nrefs; i++)
                    if (strlen(d->refs[i].name) == t.len &&
                        memcmp(d->refs[i].name, t.s, t.len) == 0)
                        break;
                if (i == d->nrefs) {
                    nori__err(
                        d,
                        NORI_ERR_REF,
                        t.line,
                        t.col,
                        "unresolved reference @%.*s (register it with nori_add_ref before parsing)",
                        (int) (t.len < 32 ? t.len : 32),
                        t.s
                    );
                    return NULL;
                }
                n->as.ref.name = d->refs[i].name;
                n->as.ref.ptr = d->refs[i].ptr;
                nori__next(p);
                return n;
            }
        case T_LINK:
            n = nori__node(d, NORI_LINK);
            if (!n)
                return NULL;
            n->as.link.path = nori__strndup(d, t.s, t.len);
            if (!n->as.link.path)
                return NULL;
            n->as.link.line = t.line;
            n->as.link.col = t.col;
            n->as.link.chain = d->links;
            d->links = n;
            nori__next(p);
            return n;
        case T_LBRACKET:
            return nori__p_array(p);
        case T_LBRACE:
            return nori__p_object_body(p, NULL);
        default:
            nori__unexpected(p, "a value");
            return NULL;
    }
}

NORI_API nori_doc *nori_new(void) {
    return (nori_doc *) calloc(1, sizeof(nori_doc));
}

NORI_API void nori_free(nori_doc *doc) {
    nori__chunk *c, *next;
    if (!doc)
        return;
    for (c = doc->chunks; c; c = next) {
        next = c->next;
        free(c);
    }
    free(doc);
}

NORI_API const nori_error *nori_get_error(const nori_doc *doc) {
    return doc ? &doc->err : NULL;
}

NORI_API nori_node *nori_root(const nori_doc *doc) {
    return doc ? doc->root : NULL;
}

NORI_API const char *nori_strerror(nori_status s) {
    switch (s) {
        case NORI_OK:
            return "ok";
        case NORI_ERR_NOMEM:
            return "out of memory";
        case NORI_ERR_ARG:
            return "invalid argument";
        case NORI_ERR_SYNTAX:
            return "syntax error";
        case NORI_ERR_RANGE:
            return "number out of range";
        case NORI_ERR_REF:
            return "unresolved reference";
        case NORI_ERR_LINK:
            return "unresolved or circular link";
        case NORI_ERR_DEPTH:
            return "nesting too deep";
        case NORI_ERR_IO:
            return "i/o error";
        case NORI_ERR_UNREPRESENTABLE:
            return "value cannot be written";
    }
    return "unknown error";
}

NORI_API nori_status nori_add_ref(nori_doc *d, const char *name, void *ptr) {
    size_t i, n;
    if (!d || !name || !ptr)
        return NORI_ERR_ARG;
    n = strlen(name);
    if (!nori__valid_name(name, n)) {
        nori__err(d, NORI_ERR_ARG, 0, 0, "invalid reference name");
        return NORI_ERR_ARG;
    }
    for (i = 0; i < d->nrefs; i++)
        if (strcmp(d->refs[i].name, name) == 0) {
            d->refs[i].ptr = ptr;
            return NORI_OK;
        }
    if (d->nrefs == d->refcap) {
        size_t nc = d->refcap ? d->refcap * 2 : 8;
        nori__ref *nr;
        if (d->refcap > SIZE_MAX / 2 / sizeof *nr)
            return NORI_ERR_NOMEM;
        nr = (nori__ref *) nori__alloc(d, nc * sizeof *nr);
        if (!nr)
            return NORI_ERR_NOMEM;
        if (d->nrefs)
            memcpy(nr, d->refs, d->nrefs * sizeof *nr);
        d->refs = nr;
        d->refcap = nc;
    }
    d->refs[d->nrefs].name = nori__strndup(d, name, n);
    if (!d->refs[d->nrefs].name)
        return NORI_ERR_NOMEM;
    d->refs[d->nrefs].ptr = ptr;
    d->nrefs++;
    return NORI_OK;
}

NORI_API nori_status nori_set_root(nori_doc *d, nori_node *node) {
    if (!d || (node && node->owner != d))
        return NORI_ERR_ARG;
    d->root = node;
    d->gen++;
    return NORI_OK;
}

NORI_API nori_status nori_parse(nori_doc *d, const char *src, size_t len) {
    nori__parser p;
    nori_node *root, *prev_links, *l, *bad = NULL;
    if (!d || (!src && len))
        return NORI_ERR_ARG;
    memset(&d->err, 0, sizeof d->err);
    d->root = NULL;
    d->gen++;
    prev_links = d->links;
    memset(&p, 0, sizeof p);
    p.doc = d;
    p.src = src ? src : "";
    p.len = len;
    p.line = 1;
    p.col = 1;
    if (len >= 3 && (unsigned char) src[0] == 0xEF && (unsigned char) src[1] == 0xBB &&
        (unsigned char) src[2] == 0xBF)
        p.pos = 3;
    root = nori__node(d, NORI_ARRAY);
    if (!root)
        return d->err.status;
    nori__next(&p);
    while (p.cur.type != T_EOF) {
        nori_node *v = nori__p_value(&p);
        if (!v || nori__arr_push(root, v) != NORI_OK) {
            if (d->err.status == NORI_OK)
                nori__err(d, NORI_ERR_SYNTAX, p.cur.line, p.cur.col, "parse error");
            return d->err.status;
        }
    }
    d->root = root;
    d->gen++;
    for (l = d->links; l && l != prev_links; l = l->as.link.chain)
        if (!nori__follow(l, 0))
            bad = l;
    if (bad) {
        nori__err(
            d,
            NORI_ERR_LINK,
            bad->as.link.line,
            bad->as.link.col,
            "unresolved or circular link $%.40s",
            bad->as.link.path
        );
        d->root = NULL;
        d->gen++;
        return NORI_ERR_LINK;
    }
    return NORI_OK;
}

NORI_API nori_status nori_parse_str(nori_doc *d, const char *src) {
    return src ? nori_parse(d, src, strlen(src)) : NORI_ERR_ARG;
}

NORI_API nori_status nori_parse_file(nori_doc *d, const char *path) {
    FILE *f;
    char *buf, *nb;
    size_t cap = 4096, len = 0;
    nori_status st;
    if (!d || !path)
        return NORI_ERR_ARG;
    f = fopen(path, "rb");
    if (!f) {
        memset(&d->err, 0, sizeof d->err);
        nori__err(d, NORI_ERR_IO, 0, 0, "cannot open '%.100s'", path);
        return NORI_ERR_IO;
    }
    buf = (char *) malloc(cap);
    while (buf) {
        size_t got = fread(buf + len, 1, cap - len, f);
        len += got;
        if (len < cap)
            break;
        if (cap > SIZE_MAX / 2) {
            free(buf);
            buf = NULL;
            break;
        }
        cap *= 2;
        nb = (char *) realloc(buf, cap);
        if (!nb) {
            free(buf);
            buf = NULL;
            break;
        }
        buf = nb;
    }
    if (!buf || ferror(f)) {
        int oom = !buf;
        free(buf);
        fclose(f);
        memset(&d->err, 0, sizeof d->err);
        nori__err(
            d, oom ? NORI_ERR_NOMEM : NORI_ERR_IO, 0, 0, oom ? "out of memory" : "read error"
        );
        return d->err.status;
    }
    fclose(f);
    st = nori_parse(d, buf, len);
    free(buf);
    return st;
}

NORI_API nori_node *nori_follow(const nori_node *n) {
    return nori__follow((nori_node *) n, 0);
}

NORI_API nori_node *nori_get(const nori_node *node, const char *path) {
    if (!node || !path)
        return NULL;
    return nori__follow(nori__path((nori_node *) node, path, 0), 0);
}

NORI_API nori_node *nori_doc_get(const nori_doc *doc, const char *path) {
    nori_node *s;
    if (!doc || !path)
        return NULL;
    s = nori__doc_start(doc);
    return s ? nori__follow(nori__path(s, path, 0), 0) : NULL;
}

NORI_API size_t nori_count(const nori_node *node) {
    nori_node *n = nori_follow(node);
    if (!n)
        return 0;
    if (n->type == NORI_ARRAY)
        return n->as.arr.count;
    if (n->type == NORI_OBJECT)
        return n->as.obj.count;
    return 0;
}

NORI_API nori_node *nori_at(const nori_node *node, size_t i) {
    nori_node *n = nori_follow(node);
    if (!n)
        return NULL;
    if (n->type == NORI_ARRAY && i < n->as.arr.count)
        return nori__follow(n->as.arr.items[i], 0);
    if (n->type == NORI_OBJECT && i < n->as.obj.count)
        return nori__follow(n->as.obj.props[i].value, 0);
    return NULL;
}

NORI_API const char *nori_key_at(const nori_node *node, size_t i) {
    nori_node *n = nori_follow(node);
    return (n && n->type == NORI_OBJECT && i < n->as.obj.count) ? n->as.obj.props[i].name : NULL;
}

NORI_API const char *nori_object_name(const nori_node *node) {
    nori_node *n = nori_follow(node);
    return (n && n->type == NORI_OBJECT) ? n->as.obj.name : NULL;
}

NORI_API nori_node *nori_default(const nori_node *node) {
    nori_node *n = nori_follow(node);
    if (!n)
        return NULL;
    if (n->type == NORI_OBJECT)
        return nori__follow(n->as.obj.def, 0);
    if (n->type == NORI_ARRAY)
        return nori__follow(n->as.arr.def, 0);
    return NULL;
}

static int32_t nori__d2i(double v) {
    if (v != v)
        return 0;
    if (v >= 2147483647.0)
        return INT32_MAX;
    if (v <= -2147483648.0)
        return INT32_MIN;
    return (int32_t) v;
}

static int64_t nori__d2l(double v) {
    if (v != v)
        return 0;
    if (v >= 9223372036854775808.0)
        return INT64_MAX;
    if (v <= -9223372036854775808.0)
        return INT64_MIN;
    return (int64_t) v;
}

static int32_t nori__l2i(int64_t v) {
    return v > INT32_MAX ? INT32_MAX : v < INT32_MIN ? INT32_MIN : (int32_t) v;
}

static float nori__d2f(double v) {
    if (v != v)
        return 0.0f;
    if (v > (double) FLT_MAX)
        return FLT_MAX;
    if (v < -(double) FLT_MAX)
        return -FLT_MAX;
    return (float) v;
}

static const nori_number *nori__num(const nori_node *node) {
    nori_node *n = nori_follow(node);
    return (n && n->type == NORI_NUMBER) ? &n->as.num : NULL;
}

NORI_API int32_t nori_get_int(const nori_node *node, int32_t fb) {
    const nori_number *n = nori__num(node);
    if (!n)
        return fb;
    switch (n->kind) {
        case NORI_INT:
            return n->v.i;
        case NORI_LONG:
            return nori__l2i(n->v.l);
        case NORI_FLOAT:
            return nori__d2i((double) n->v.f);
        default:
            return nori__d2i(n->v.d);
    }
}

NORI_API int64_t nori_get_long(const nori_node *node, int64_t fb) {
    const nori_number *n = nori__num(node);
    if (!n)
        return fb;
    switch (n->kind) {
        case NORI_INT:
            return n->v.i;
        case NORI_LONG:
            return n->v.l;
        case NORI_FLOAT:
            return nori__d2l((double) n->v.f);
        default:
            return nori__d2l(n->v.d);
    }
}

NORI_API double nori_get_double(const nori_node *node, double fb) {
    const nori_number *n = nori__num(node);
    if (!n)
        return fb;
    switch (n->kind) {
        case NORI_INT:
            return (double) n->v.i;
        case NORI_LONG:
            return (double) n->v.l;
        case NORI_FLOAT:
            return (double) n->v.f;
        default:
            return n->v.d;
    }
}

NORI_API float nori_get_float(const nori_node *node, float fb) {
    const nori_number *n = nori__num(node);
    if (!n)
        return fb;
    switch (n->kind) {
        case NORI_INT:
            return (float) n->v.i;
        case NORI_LONG:
            return (float) n->v.l;
        case NORI_FLOAT:
            return n->v.f;
        default:
            return nori__d2f(n->v.d);
    }
}

NORI_API int nori_get_bool(const nori_node *node, int fb) {
    nori_node *n = nori_follow(node);
    return (n && n->type == NORI_BOOL) ? n->as.boolean : fb;
}

NORI_API const char *nori_get_string(const nori_node *node, size_t *len) {
    nori_node *n = nori_follow(node);
    if (!n || (n->type != NORI_STRING && n->type != NORI_IDENTIFIER))
        return NULL;
    if (len)
        *len = n->as.str.len;
    return n->as.str.data;
}

NORI_API void *nori_get_ref(const nori_node *node, void *fb) {
    nori_node *n = nori_follow(node);
    return (n && n->type == NORI_REF) ? n->as.ref.ptr : fb;
}

static nori_node *nori__new_num(nori_doc *d, nori_num_kind k, int radix) {
    nori_node *n;
    if (!d)
        return NULL;
    n = nori__node(d, NORI_NUMBER);
    if (n) {
        n->as.num.kind = k;
        n->as.num.radix = radix;
    }
    return n;
}

NORI_API nori_node *nori_new_int(nori_doc *d, int32_t v) {
    nori_node *n = nori__new_num(d, NORI_INT, 10);
    if (n)
        n->as.num.v.i = v;
    return n;
}

NORI_API nori_node *nori_new_long(nori_doc *d, int64_t v) {
    nori_node *n = nori__new_num(d, NORI_LONG, 10);
    if (n)
        n->as.num.v.l = v;
    return n;
}

NORI_API nori_node *nori_new_hex(nori_doc *d, int32_t v) {
    nori_node *n = nori__new_num(d, NORI_INT, 16);
    if (n)
        n->as.num.v.i = v;
    return n;
}

NORI_API nori_node *nori_new_binary(nori_doc *d, int32_t v) {
    nori_node *n = nori__new_num(d, NORI_INT, 2);
    if (n)
        n->as.num.v.i = v;
    return n;
}

NORI_API nori_node *nori_new_octal(nori_doc *d, int32_t v) {
    nori_node *n = nori__new_num(d, NORI_INT, 8);
    if (n)
        n->as.num.v.i = v;
    return n;
}

NORI_API nori_node *nori_new_double(nori_doc *d, double v) {
    nori_node *n;
    if (!d)
        return NULL;
    if (!nori__finite(v)) {
        nori__err(d, NORI_ERR_RANGE, 0, 0, "NaN and infinity cannot be stored");
        return NULL;
    }
    n = nori__new_num(d, NORI_DOUBLE, 10);
    if (n)
        n->as.num.v.d = v;
    return n;
}

NORI_API nori_node *nori_new_float(nori_doc *d, float v) {
    nori_node *n;
    if (!d)
        return NULL;
    if (!nori__finite((double) v)) {
        nori__err(d, NORI_ERR_RANGE, 0, 0, "NaN and infinity cannot be stored");
        return NULL;
    }
    n = nori__new_num(d, NORI_FLOAT, 10);
    if (n)
        n->as.num.v.f = v;
    return n;
}

NORI_API nori_node *nori_new_bool(nori_doc *d, int v) {
    nori_node *n = d ? nori__node(d, NORI_BOOL) : NULL;
    if (n)
        n->as.boolean = v != 0;
    return n;
}

NORI_API nori_node *nori_new_string_n(nori_doc *d, const char *s, size_t len) {
    nori_node *n;
    if (!d || (!s && len))
        return NULL;
    n = nori__node(d, NORI_STRING);
    if (!n)
        return NULL;
    n->as.str.data = nori__strndup(d, s ? s : "", len);
    if (!n->as.str.data)
        return NULL;
    n->as.str.len = len;
    return n;
}

NORI_API nori_node *nori_new_string(nori_doc *d, const char *s) {
    return s ? nori_new_string_n(d, s, strlen(s)) : NULL;
}

NORI_API nori_node *nori_new_ident(nori_doc *d, const char *name) {
    nori_node *n;
    size_t len;
    if (!d || !name)
        return NULL;
    len = strlen(name);
    if (!nori__valid_ident(name, len)) {
        nori__err(d, NORI_ERR_ARG, 0, 0, "'%.40s' is not a valid identifier", name);
        return NULL;
    }
    n = nori__node(d, NORI_IDENTIFIER);
    if (!n)
        return NULL;
    n->as.str.data = nori__strndup(d, name, len);
    if (!n->as.str.data)
        return NULL;
    n->as.str.len = len;
    return n;
}

NORI_API nori_node *nori_new_object(nori_doc *d, const char *name) {
    nori_node *n;
    if (!d)
        return NULL;
    if (name && name[0] && !nori__valid_ident(name, strlen(name))) {
        nori__err(d, NORI_ERR_ARG, 0, 0, "'%.40s' is not a valid object name", name);
        return NULL;
    }
    n = nori__node(d, NORI_OBJECT);
    if (n && name && name[0]) {
        n->as.obj.name = nori__strndup(d, name, strlen(name));
        if (!n->as.obj.name)
            return NULL;
    }
    return n;
}

NORI_API nori_node *nori_new_array(nori_doc *d) {
    return d ? nori__node(d, NORI_ARRAY) : NULL;
}

NORI_API nori_node *nori_new_ref(nori_doc *d, const char *name) {
    size_t i;
    nori_node *n;
    if (!d || !name)
        return NULL;
    for (i = 0; i < d->nrefs; i++)
        if (strcmp(d->refs[i].name, name) == 0)
            break;
    if (i == d->nrefs) {
        nori__err(d, NORI_ERR_REF, 0, 0, "reference '%.40s' is not registered", name);
        return NULL;
    }
    n = nori__node(d, NORI_REF);
    if (n) {
        n->as.ref.name = d->refs[i].name;
        n->as.ref.ptr = d->refs[i].ptr;
    }
    return n;
}

NORI_API nori_node *nori_new_link(nori_doc *d, const char *path) {
    nori_node *n;
    if (!d || !path)
        return NULL;
    if (!nori__valid_path(path, strlen(path))) {
        nori__err(d, NORI_ERR_ARG, 0, 0, "'%.40s' is not a valid link path", path);
        return NULL;
    }
    n = nori__node(d, NORI_LINK);
    if (!n)
        return NULL;
    n->as.link.path = nori__strndup(d, path, strlen(path));
    if (!n->as.link.path)
        return NULL;
    n->as.link.chain = d->links;
    d->links = n;
    return n;
}

NORI_API nori_status nori_insert(nori_node *c, const char *name, nori_node *v) {
    nori_status st;
    nori_doc *d;
    if (!c || !v)
        return NORI_ERR_ARG;
    d = c->owner;
    if (v->owner != d || v == c) {
        nori__err(
            d, NORI_ERR_ARG, 0, 0, "nodes must belong to the same doc and cannot contain themselves"
        );
        return NORI_ERR_ARG;
    }
    if (c->type == NORI_ARRAY)
        st = nori__arr_push(c, v);
    else if (c->type == NORI_OBJECT) {
        char *k;
        if (!name || !nori__valid_ident(name, strlen(name))) {
            nori__err(d, NORI_ERR_ARG, 0, 0, "object properties need a valid identifier as name");
            return NORI_ERR_ARG;
        }
        k = nori__strndup(d, name, strlen(name));
        st = k ? nori__obj_push(c, k, v) : NORI_ERR_NOMEM;
    } else {
        nori__err(d, NORI_ERR_ARG, 0, 0, "container must be an object or an array");
        return NORI_ERR_ARG;
    }
    if (st == NORI_OK)
        d->gen++;
    return st;
}

NORI_API nori_status nori_set_default(nori_node *c, nori_node *v) {
    if (!c || !v || v->owner != c->owner || v == c)
        return NORI_ERR_ARG;
    if (c->type == NORI_OBJECT)
        c->as.obj.def = v;
    else if (c->type == NORI_ARRAY)
        c->as.arr.def = v;
    else
        return NORI_ERR_ARG;
    c->owner->gen++;
    return NORI_OK;
}

typedef struct {
    char *p;
    size_t len, cap;
    nori_status st;
} nori__sb;

static void nori__put(nori__sb *b, const char *s, size_t n) {
    if (b->st)
        return;
    if (n > SIZE_MAX - b->len - 1) {
        b->st = NORI_ERR_NOMEM;
        return;
    }
    if (b->len + n + 1 > b->cap) {
        size_t nc = b->cap ? b->cap : 256;
        char *np;
        while (nc < b->len + n + 1) {
            if (nc > SIZE_MAX / 2) {
                b->st = NORI_ERR_NOMEM;
                return;
            }
            nc *= 2;
        }
        np = (char *) realloc(b->p, nc);
        if (!np) {
            b->st = NORI_ERR_NOMEM;
            return;
        }
        b->p = np;
        b->cap = nc;
    }
    if (n)
        memcpy(b->p + b->len, s, n);
    b->len += n;
    b->p[b->len] = 0;
}

static void nori__puts(nori__sb *b, const char *s) {
    nori__put(b, s, strlen(s));
}

static void nori__indent(nori__sb *b, unsigned depth) {
    unsigned i;
    for (i = 0; i < depth; i++)
        nori__put(b, "    ", 4);
}

static void nori__w_string(nori__sb *b, const char *s, size_t n) {
    size_t i;
    nori__put(b, "\"", 1);
    for (i = 0; i < n; i++) {
        unsigned char c = (unsigned char) s[i];
        switch (c) {
            case '\\':
                nori__put(b, "\\\\", 2);
                break;
            case '"':
                nori__put(b, "\\\"", 2);
                break;
            case '\n':
                nori__put(b, "\\n", 2);
                break;
            case '\r':
                nori__put(b, "\\r", 2);
                break;
            case '\t':
                nori__put(b, "\\t", 2);
                break;
            case '\b':
                nori__put(b, "\\b", 2);
                break;
            case '\f':
                nori__put(b, "\\f", 2);
                break;
            case 0:
                nori__put(b, "\\0", 2);
                break;
            default:
                if (c < 0x20 || c == 0x7f) {
                    char e[5];
                    snprintf(e, sizeof e, "\\x%02X", (unsigned) c);
                    nori__put(b, e, 4);
                } else
                    nori__put(b, (const char *) &c, 1);
        }
    }
    nori__put(b, "\"", 1);
}

static void nori__fmt_fp(char *buf, size_t cap, double v, int is_float) {
    int prec = is_float ? 6 : 15, hi = is_float ? 9 : 17;
    const char *dp;
    for (; prec <= hi; prec++) {
        double back;
        snprintf(buf, cap, "%.*g", prec, v);
        back = strtod(buf, NULL);
        if (is_float ? (nori__d2f(back) == (float) v) : (back == v))
            break;
    }
    dp = nori__dp();
    if (dp[0] != '.' || dp[1]) {
        char *q = strstr(buf, dp);
        if (q) {
            size_t l = strlen(dp);
            q[0] = '.';
            memmove(q + 1, q + l, strlen(q + l) + 1);
        }
    }
    if (is_float)
        strcat(buf, "f");
    else if (!strchr(buf, '.') && !strchr(buf, 'e') && !strchr(buf, 'E'))
        strcat(buf, ".0");
}

static void nori__w_number(nori__sb *b, const nori_number *n) {
    char buf[96];
    uint64_t u;
    switch (n->kind) {
        case NORI_INT:
            u = (uint32_t) n->v.i;
            if (n->radix == 16)
                snprintf(buf, sizeof buf, "0x%llx", (unsigned long long) u);
            else if (n->radix == 8)
                snprintf(buf, sizeof buf, "0o%llo", (unsigned long long) u);
            else if (n->radix == 2)
                goto binary;
            else
                snprintf(buf, sizeof buf, "%ld", (long) n->v.i);
            break;
        case NORI_LONG:
            u = (uint64_t) n->v.l;
            if (n->radix == 16)
                snprintf(buf, sizeof buf, "0x%llxL", (unsigned long long) u);
            else if (n->radix == 8)
                snprintf(buf, sizeof buf, "0o%lloL", (unsigned long long) u);
            else if (n->radix == 2)
                goto binary;
            else
                snprintf(
                    buf,
                    sizeof buf,
                    (n->v.l >= INT32_MIN && n->v.l <= INT32_MAX) ? "%lldL" : "%lld",
                    (long long) n->v.l
                );
            break;
        case NORI_FLOAT:
            if (!nori__finite((double) n->v.f)) {
                b->st = NORI_ERR_UNREPRESENTABLE;
                return;
            }
            nori__fmt_fp(buf, sizeof buf, (double) n->v.f, 1);
            break;
        default:
            if (!nori__finite(n->v.d)) {
                b->st = NORI_ERR_UNREPRESENTABLE;
                return;
            }
            nori__fmt_fp(buf, sizeof buf, n->v.d, 0);
            break;
    }
    nori__puts(b, buf);
    return;
binary:
    {
        size_t o = 2;
        int bit;
        for (bit = 63; bit > 0 && !((u >> bit) & 1u); bit--) {
        }
        buf[0] = '0';
        buf[1] = 'b';
        for (; bit >= 0; bit--)
            buf[o++] = ((u >> bit) & 1u) ? '1' : '0';
        if (n->kind == NORI_LONG)
            buf[o++] = 'L';
        buf[o] = 0;
        nori__puts(b, buf);
    }
}

static void nori__w_node(nori__sb *b, const nori_node *n, unsigned depth);

static void nori__w_object(nori__sb *b, const nori_node *n, unsigned depth, int with_name) {
    size_t i;
    if (with_name && n->as.obj.name && n->as.obj.name[0]) {
        nori__puts(b, n->as.obj.name);
        nori__put(b, " ", 1);
    }
    nori__put(b, "{\n", 2);
    for (i = 0; i < n->as.obj.count; i++) {
        const nori_prop *pr = &n->as.obj.props[i];
        nori__indent(b, depth + 1);
        nori__puts(b, pr->name);
        if (pr->value && pr->value->type == NORI_OBJECT && pr->value->as.obj.name &&
            strcmp(pr->value->as.obj.name, pr->name) == 0) {
            nori__put(b, " ", 1);
            nori__w_object(b, pr->value, depth + 1, 0);
        } else {
            nori__puts(b, " = ");
            nori__w_node(b, pr->value, depth + 1);
        }
        nori__put(b, "\n", 1);
    }
    if (n->as.obj.def) {
        nori__indent(b, depth + 1);
        nori__puts(b, "! = ");
        nori__w_node(b, n->as.obj.def, depth + 1);
        nori__put(b, "\n", 1);
    }
    nori__indent(b, depth);
    nori__put(b, "}", 1);
}

static void nori__w_node(nori__sb *b, const nori_node *n, unsigned depth) {
    size_t i;
    if (b->st)
        return;
    if (!n) {
        b->st = NORI_ERR_ARG;
        return;
    }
    if (depth > NORI_MAX_DEPTH) {
        b->st = NORI_ERR_DEPTH;
        return;
    }
    switch (n->type) {
        case NORI_NUMBER:
            nori__w_number(b, &n->as.num);
            break;
        case NORI_BOOL:
            nori__puts(b, n->as.boolean ? "true" : "false");
            break;
        case NORI_STRING:
            nori__w_string(b, n->as.str.data, n->as.str.len);
            break;
        case NORI_IDENTIFIER:
            nori__put(b, n->as.str.data, n->as.str.len);
            break;
        case NORI_REF:
            nori__put(b, "@", 1);
            nori__puts(b, n->as.ref.name);
            break;
        case NORI_LINK:
            nori__put(b, "$", 1);
            nori__puts(b, n->as.link.path);
            break;
        case NORI_OBJECT:
            nori__w_object(b, n, depth, 1);
            break;
        case NORI_ARRAY:
            nori__put(b, "[\n", 2);
            for (i = 0; i < n->as.arr.count; i++) {
                nori__indent(b, depth + 1);
                nori__w_node(b, n->as.arr.items[i], depth + 1);
                nori__put(b, "\n", 1);
            }
            if (n->as.arr.def) {
                nori__indent(b, depth + 1);
                nori__puts(b, "! = ");
                nori__w_node(b, n->as.arr.def, depth + 1);
                nori__put(b, "\n", 1);
            }
            nori__indent(b, depth);
            nori__put(b, "]", 1);
            break;
    }
}

static nori_status nori__finish(nori__sb *b, char **out, size_t *len) {
    if (b->st == NORI_OK && !b->p)
        nori__put(b, "", 0);
    if (b->st == NORI_OK && !b->p)
        b->st = NORI_ERR_NOMEM;
    if (b->st != NORI_OK) {
        free(b->p);
        return b->st;
    }
    *out = b->p;
    if (len)
        *len = b->len;
    return NORI_OK;
}

NORI_API nori_status nori_stringify(const nori_node *node, char **out, size_t *len) {
    nori__sb b = { NULL, 0, 0, NORI_OK };
    if (!node || !out)
        return NORI_ERR_ARG;
    nori__w_node(&b, node, 0);
    return nori__finish(&b, out, len);
}

NORI_API nori_status nori_stringify_doc(const nori_doc *doc, char **out, size_t *len) {
    nori__sb b = { NULL, 0, 0, NORI_OK };
    if (!doc || !doc->root || !out)
        return NORI_ERR_ARG;
    if (doc->root->type == NORI_ARRAY) {
        size_t i;
        for (i = 0; i < doc->root->as.arr.count; i++) {
            nori__w_node(&b, doc->root->as.arr.items[i], 0);
            nori__put(&b, "\n", 1);
        }
    } else {
        nori__w_node(&b, doc->root, 0);
        nori__put(&b, "\n", 1);
    }
    return nori__finish(&b, out, len);
}

static void nori__pr(nori__sb *b, const nori_node *n, unsigned depth) {
    size_t i;
    if (b->st)
        return;
    if (!n)
        return;
    if (depth > 2 * NORI_MAX_DEPTH + 2) {
        b->st = NORI_ERR_DEPTH;
        return;
    }
    for (i = 0; i < depth; i++)
        nori__put(b, "  ", 2);
    switch (n->type) {
        case NORI_NUMBER:
            nori__puts(b, "Number ");
            nori__w_number(b, &n->as.num);
            break;
        case NORI_BOOL:
            nori__puts(b, n->as.boolean ? "Boolean true" : "Boolean false");
            break;
        case NORI_STRING:
            nori__puts(b, "String ");
            nori__w_string(b, n->as.str.data, n->as.str.len);
            break;
        case NORI_IDENTIFIER:
            nori__puts(b, "Identifier ");
            nori__puts(b, n->as.str.data);
            break;
        case NORI_REF:
            nori__puts(b, "Reference @");
            nori__puts(b, n->as.ref.name);
            break;
        case NORI_LINK:
            nori__puts(b, "Link $");
            nori__puts(b, n->as.link.path);
            break;
        case NORI_OBJECT:
            nori__puts(b, "Object");
            if (n->as.obj.name) {
                nori__put(b, " ", 1);
                nori__puts(b, n->as.obj.name);
            }
            nori__put(b, "\n", 1);
            for (i = 0; i < n->as.obj.count; i++) {
                size_t k;
                for (k = 0; k < depth + 1; k++)
                    nori__put(b, "  ", 2);
                nori__puts(b, "Property ");
                nori__puts(b, n->as.obj.props[i].name);
                nori__put(b, "\n", 1);
                nori__pr(b, n->as.obj.props[i].value, depth + 2);
            }
            if (n->as.obj.def) {
                size_t k;
                for (k = 0; k < depth + 1; k++)
                    nori__put(b, "  ", 2);
                nori__puts(b, "Default\n");
                nori__pr(b, n->as.obj.def, depth + 2);
            }
            return;
        case NORI_ARRAY:
            nori__puts(b, "Array\n");
            for (i = 0; i < n->as.arr.count; i++)
                nori__pr(b, n->as.arr.items[i], depth + 1);
            if (n->as.arr.def) {
                size_t k;
                for (k = 0; k < depth + 1; k++)
                    nori__put(b, "  ", 2);
                nori__puts(b, "Default\n");
                nori__pr(b, n->as.arr.def, depth + 2);
            }
            return;
    }
    nori__put(b, "\n", 1);
}

NORI_API nori_status nori_print(const nori_node *node, FILE *out) {
    nori__sb b = { NULL, 0, 0, NORI_OK };
    if (!node)
        return NORI_ERR_ARG;
    nori__pr(&b, node, 0);
    if (b.st == NORI_OK && b.p)
        fwrite(b.p, 1, b.len, out ? out : stdout);
    free(b.p);
    return b.st;
}

#endif /* NORI_IMPLEMENTATION_DONE */
#endif /* NORI_IMPLEMENTATION */
