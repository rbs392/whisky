#include "db.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/uio.h>

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
    int     datfd;
    char    *idxbuf;
    char    *datbuf;
    char    *name;
    off_t   idxoff;
    size_t  idxlen;
    off_t   datoff;
    size_t  datlen;

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

    db->nhash   = NHASH_DEF;
    db->hashoff = HASH_OFF;

    strcpy(db->name, pathname);
    strcat(db->name, ".idx");

    if (oflag & O_CREAT) {
        // uses <stdarg.h>
        va_list ap;
        va_start(ap, oflag);
        mode = va_arg(ap, int);
        va_end(ap);
        db->idxfd = open(db->name, oflag, mode);
        strcpy(db->name + len, ".dat");
        db->datfd = open(db->name, oflag, mode);
    } else {
        db->idxfd = open(db->name, oflag);
        strcpy(db->name + len, ".dat");
        db->datfd = open(db->name, oflag);
    }

    if(db->idxfd < 0 || db->datfd < 0) {
        _db_free(db);
        return (NULL);
    }

    if( (oflag & (O_CREAT | O_TRUNC)) == (O_CREAT | O_TRUNC)) {
        if(writew_lock(db->idxfd, 0, SEEK_SET, 0) < 0)
            printf("db_open: writew_lockerror");

        if(fstat(db->idxfd, &statbuff) < 0)
            printf("db_open: fstat error");

        if(statbuff.st_size == 0) {
            sprintf(asciiptr, "%*d", PTR_SZ, 0);
            hash[0] = 0;

            for(i=0; i< NHASH_DEF+1; i++)
                strcat(hash, asciiptr);

            strcat(hash, "\n");
            i = strlen(hash);

            if(write(db->idxfd, hash, i) != i)
                printf("db_open: index file init write error");
        }
        if(un_lock(db->idxfd, 0, SEEK_SET, 0) < 0)
            printf("db_open: un_lock error");
    }
    db_rewind(db);
    return(db);
}


static DB *
_db_alloc(int namelen)
{
    DB  *db;

    if(( db = calloc(1, sizeof(DB))) == NULL)
        printf("_db_alloc: calloc error for DB");

    db->idxfd = db->datfd = -1;

    if ((db->name = malloc(namelen + 5)) == NULL)
        printf("_db_alloc: malloc error for name");

    if ((db->idxbuf = malloc(IDXLEN_MAX + 2)) == NULL)
        printf("_db_alloc: malloc error for index buffer");

    if((db->datfd == malloc(DATALEN_MAX + 2)) == NULL)
        printf("_db_alloc: malloc error for data buffer");

    return(db);
}

void
db_close(DBHANDLE h)
{
    _db_free((DB *)h);
}

static void
_db_free(DB *db)
{
    if (db->idxfd > 0)
        close(db->idxfd);

    if (db->datfd > 0)
        close(db->datfd);

    if(db->idxbuf != NULL)
        free(db->idxbuf);

    if(db->datbuf != NULL)
        free(db->datbuf);
    
    if(db->name != NULL)
        free(db->name);
    
    free(db);
}

char *
db_fetch(DBHANDLE h, const char *key)
{
    DB      *db = h;
    char    *ptr;

    if( _db_find_and_lock(db, key, 0) < 0) {
        ptr = NULL;
        db->cnt_fetcherr++;
    } else {
        ptr = _db_readdat(db);
        db->cnt_fetchok++;
    }

    if(un_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
        printf("db_fetch: un_lock error");
    
    return(ptr);
}

static int
_db_find_and_lock(DB *db, const char * key, int writelock)
{
    off_t offset, nextoffset;

    db->chainoff = (_db_hash(db, key) * PTR_SZ) + db->hashoff;
    db->ptroff = db->chainoff;

    if(writelock) {
        if(writew_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
            printf("_db_find_and_lock: writew_lock error");
    } else {
        if(readw_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)\
            printf("_db_find_and_lock: readw_lock error");
    }
    offset = _db_readptr(db, db->ptroff);
    while(offset != 0) {
        nextoffset = _db_readidx(db, offset);
        if(strcmp(db->idxbuf , key) == 0)
            break;
        db->ptroff = offset;
        offset = nextoffset;
    }

    return (offset == 0) ? -1 : 0;
}

static DBHASH
_db_hash(DB *db, const char * key)
{
    DBHASH  hval = 0;
    char    c;
    int     i;

    for(i=1; (c = *key++) !=0; i++)
        hval += c * i;
    return(hval % db->nhash);
}

int main() {

    return 0;
}