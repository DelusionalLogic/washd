#pragma once

#define FOREACH_STATUS(F)          \
        F(STATUS_UNKNOWM, unknown) \
        F(STATUS_FREE,    free)    \
        F(STATUS_INUSE,   in use)  \
        F(STATUS_CLOSED,  closed)  \

#define GENERATE_ENUM(ENUM, STRING) ENUM,
#define GENERATE_STRING(ENUM, STRING) #STRING,

enum Status {
    FOREACH_STATUS(GENERATE_ENUM)
};

static const char *STATUS_STR[] = {
    FOREACH_STATUS(GENERATE_STRING)
};
