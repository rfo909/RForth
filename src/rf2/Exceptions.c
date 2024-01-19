#include "rfcommon.h"

// Handles error situations during execution of Forth

static char *type = (char *) null;
static char *message = (char *) null;



void PANIC (char *msg) {
    type="PANIC";
    message=msg;
}

void ERROR (char *msg) {
    type="ERROR";
    message=msg;
}

bool hasException() {
    return message != (char *) null;
}

char *getExceptionMessage() {
    return message;
}

char *getExceptionType() {
    return type;
}

void clearException () {
    message = (char *) null;
    type = (char *) null;
}