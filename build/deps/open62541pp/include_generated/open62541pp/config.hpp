#pragma once

// expose open62541 compile options defined in open62541/config.h
#include "open62541pp/detail/open62541/config.h"

#define UAPP_VERSION "0.15.0"
#define UAPP_VERSION_MAJOR 0
#define UAPP_VERSION_MINOR 15
#define UAPP_VERSION_PATCH 0

// check required open62541 compile flags
#ifndef UA_ENABLE_NODEMANAGEMENT
#error "open62541 must be compiled with UA_ENABLE_NODEMANAGEMENT"
#endif

// open62541 version checks
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define UAPP_OPEN62541_VER_EQ(MAJOR, MINOR)                                                        \
    (UA_OPEN62541_VER_MAJOR == (MAJOR)) && (UA_OPEN62541_VER_MINOR == (MINOR))
#define UAPP_OPEN62541_VER_GE(MAJOR, MINOR)                                                        \
    (UA_OPEN62541_VER_MAJOR >= (MAJOR)) && (UA_OPEN62541_VER_MINOR >= (MINOR))
#define UAPP_OPEN62541_VER_LE(MAJOR, MINOR)                                                        \
    (UA_OPEN62541_VER_MAJOR <= (MAJOR)) && (UA_OPEN62541_VER_MINOR <= (MINOR))
// NOLINTEND(cppcoreguidelines-macro-usage)

// open62541 feature checks
#if UAPP_OPEN62541_VER_GE(1, 3) &&                                                                 \
    (defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL))
#define UAPP_CREATE_CERTIFICATE
#endif
