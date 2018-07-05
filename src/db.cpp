#include "db.h"
#include <string.h>
#include <stdio.h>

#define IDXLEN_SZ   4;
#define SEP         ':';
#define SPACE       ' ';
#define NEWLINE     '\n'


#define PTR_SZ      7;
#define PTR_MAX     999999;
#define NHASH_DEF   137;
#define FREE_OFF    0;
#define HASH_OFF    PTR_SZ;


typedef unsigned long DBHASH;
typedef unsigned long COUNT;


typedef struct {
    int     idxfd;
    int     datafd;
    char    *idxbuf;
    char    *databuf;
    char    *name;
    off_t   idxoff;
    size_t  idxlen;
    off_t   dataoff;
    size_t  datalen;

    off_t   ptrval;
    off_t   ptroff;
    off_t   chainoff;
    off_t   hashoff;

    DBHASH  nhash;
    COUNT   cnt_delok;
    COUNT   cnt_delerr;
    COUNT   cnt_fetchok;
    COUNT   cnt_fetcherr;
    COUNT   cnt_nextrec;
    COUNT   cnt_stor1;
    COUNT   cnt_stor2;
    COUNT   cnt_stor3;
    COUNT   cnt_stor4;
    COUNT   cnt_storerr;
} DB;


static DB       *_db_alloc(int);
static void     _db_dodelete(DB *);
static int      _db_find_and_lock(DB *, const char *, int);
static int      _db_findfree(DB *, const char *);
static void     _db_free(DB *);
static DBHASH   _db_hash(DB *, const char *);
static char     *_db_readdat(DB *);
static off_t    _db_readidx(DB *, off_t);
static off_t    _db_readptr(DB *, off_t);
static void     _db_writedat(DB *, const char *, off_t, int);
static void     _db_writeidx(DB *, const char *, off_t, int, off_t);
static void     _db_writeptr(DB *, off_t, off_t);


DBHANDLE
db_open(const char *pathname, int oflag, ...)
{
    DB      *db;
    int     len, mode;
    size_t  i;
    char    asciiptr[PTR_SZ + 1],
            hash[(NHASH_DEF + 1) * PTR_SZ + 2];
    
    struct stat statbuff;

    len = strlen(pathname);

    if((db == _db_alloc(len)) == NULL)
        printf("db_open: _db_alloc error for DB");

}


int main() {

    return 0;
}