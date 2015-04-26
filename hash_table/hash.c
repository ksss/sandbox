#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct item {
  char *key;
  int value;
  struct item *next;
};

struct hash {
  struct item **ary;
  int len;
  int capa;
};

#define FALSE 0
#define TRUE 1
#define MAX_CHAIN 3

static int malloc_count = 0;
static int hash_count = 0;
static int item_count = 0;

void *
mmalloc(size_t size)
{
  malloc_count++;
  return malloc(size);
}

void
ffree(void *ptr)
{
  malloc_count--;
  free(ptr);
}

struct item *
item_new(const char *key, int value)
{
  struct item *item = (struct item *) mmalloc(sizeof(struct item));
  item->key = (char *)mmalloc(strlen(key)+1);
  memcpy(item->key, key, strlen(key)+1);
  item->value = value;
  item->next = NULL;
  item_count++;
  return item;
}

void
item_free(struct item *item)
{
  ffree(item->key);
  ffree(item);
  item_count--;
}

struct hash *
hash_new(int capa)
{
  struct hash *h = (struct hash *) mmalloc(sizeof(struct hash));
  h->ary = (struct item **) mmalloc(sizeof(struct item *) * capa);
  memset(h->ary, 0, sizeof(struct item *) * capa);
  h->len = 0;
  h->capa = capa;
  hash_count++;
  return h;
}

void
hash_free(struct hash *h)
{
  struct item *item;
  struct item *next;
  int i;

  for (i = 0; i < h->capa; i++) {
    item = h->ary[i];
    while (item != NULL) {
      next = item->next;
      item_free(item);
      item = next;
    }
  }
  ffree(h->ary);
  ffree(h);
  hash_count--;
}

void
hash_inspect(struct hash *h)
{
  struct item *item;
  struct item *next;
  int i, j, indent = 1;

  printf("===hash_inspect begin===\n");
  for (i = 0; i < h->capa; i++) {
    item = h->ary[i];
    indent = 0;
    while (item != NULL) {
      for (j = 0; j < indent; j++) {
        printf("  ");
      }
      printf("%d:%p = %s : %d\n", i, item, item->key, item->value);
      item = item->next;
      indent++;
    }
  }
  printf("capa: %d, len: %d\n", h->capa, h->len);
  printf("===hash_inspect end===\n");
}

void
fatal(const char *message)
{
  printf("fatal: %s\n", message);
  exit(1);
}

unsigned int
hashing(const char *key)
{
  unsigned int ret = 0;
  int i, len = strlen(key);

  for(i = 0; i < len; i++) {
    ret += (int)*key++;
    ret *= 31;
  }
  return ret;
}

static void hash_set_internal(struct hash *, const char *, int, int);

static
void
hash_rehash(struct hash *h)
{
  struct hash *new_h;
  struct item *item;
  struct item *next;
  int i;
  unsigned int key_index;

  new_h = hash_new(h->capa * 2);
  printf("rehash! capa=%d\n", h->capa * 2);

  for (i = 0; i < h->capa; i++) {
    item = h->ary[i];
    while (item != NULL) {
      next = item->next;
      hash_set_internal(new_h, item->key, item->value, FALSE);
      item_free(item);
      item = next;
    }
  }

  ffree(h->ary);
  *h = *new_h;
  ffree(new_h);
  hash_count--;
}

static
void
hash_set_internal(struct hash *h, const char *key, int value, int can_rehash)
{
  struct item *item;
  struct item *top;
  unsigned int key_index;
  int chain = 0, rehash = FALSE;

  key_index = hashing(key) % h->capa;
  top = h->ary[key_index];

  if (h->ary[key_index] == NULL) {
    item = item_new(key, value);
    h->ary[key_index] = item;
    h->len++;
    return;
  }

  while (top->next) {
    if (strcmp(top->key, key) == 0) {
      top->value = value;
      return;
    }

    if (MAX_CHAIN <= chain++) {
      rehash = TRUE;
    }
    top = top->next;
  }

  item = item_new(key, value);
  top->next = item;
  h->len++;

  if (can_rehash && rehash) {
    hash_rehash(h);
  }
}

void
hash_set(struct hash *h, char *key, int value)
{
  hash_set_internal(h, key, value, TRUE);
}

int
hash_get(struct hash *h, char *key)
{
  struct item *item = h->ary[hashing(key) % h->capa];

  if (item) {
    do {
      if (strcmp(item->key, key) == 0) {
        return item->value;
      }
      item = item->next;
    } while (item);
    fatal("key not found");
  } else {
    fatal("item not found");
  }
  return 0; // none reach
}

#define A 65
int main(int argc, char **argv)
{
  struct hash *h = hash_new(8);
  int i;
  char ch[4];

  for (i = 1; i < 60; i++) {
    ch[0] = ch[1] = ch[2] = i + A;
    ch[3] = '\0';
    hash_set(h, ch, i);
  }

  for (i = 1; i < 60; i++) {
    ch[0] = ch[1] = ch[2] = i + A;
    ch[3] = '\0';
    printf("get %s = %d\n", (const char *)ch, hash_get(h, ch));
  }
  hash_inspect(h);
  printf("malloc_count=%d\n", malloc_count);
  printf("item_count=%d\n", item_count);
  printf("hash_count=%d\n", hash_count);
  hash_free(h);
}

