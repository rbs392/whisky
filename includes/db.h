#pragma once

typedef void *DBHANDLE;

DBHANDLE    db_open(const char*, int, ...);
void        db_close(DBHANDLE);
char        *db_fetch(DBHANDLE, char *);
int         db_store(DBHANDLE, char *, char *, int);
int         db_delete(DBHANDLE, char *);
void        db_rewind(DBHANDLE);
char        *db_nextrec(DBHANDLE, char *);


#define DB_INSERT   1
#define DB_REPLACE  2
#define DB_STORE    3


#define IDXLEN_MIN  6
#define IDXLEN_MAX  1024
#define DATALEN_MIN 2
#define DATALEN_MAX 1024