// Copyright (C) 2022  ilobilo

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// #define EPERM 1
// #define ENOENT 2
// #define ESRCH 3
// #define EINTR 4
// #define EIO 5
// #define ENXIO 6
// #define E2BIG 7
// #define ENOEXEC 8
// #define EBADF 9
// #define ECHILD 10
// #define EAGAIN 11
// #define ENOMEM 12
// #define EACCES 13
// #define EFAULT 14
// #define ENOTBLK 15
// #define EBUSY 16
// #define EEXIST 17
// #define EXDEV 18
// #define ENODEV 19
// #define ENOTDIR 20
// #define EISDIR 21
// #define EINVAL 22
// #define ENFILE 23
// #define EMFILE 24
// #define ENOTTY 25
// #define ETXTBSY 26
// #define EFBIG 27
// #define ENOSPC 28
// #define ESPIPE 29
// #define EROFS 30
// #define EMLINK 31
// #define EPIPE 32
// #define EDOM 33
// #define ERANGE 34
// #define EDEADLK 35
// #define ENAMETOOLONG 36
// #define ENOLCK 37
// #define ENOSYS 38
// #define ENOTEMPTY 39
// #define ELOOP 40
// #define EWOULDBLOCK EAGAIN
// #define ENOMSG 42
// #define EIDRM 43
// #define ECHRNG 44
// #define EL2NSYNC 45
// #define EL3HLT 46
// #define EL3RST 47
// #define ELNRNG 48
// #define EUNATCH 49
// #define ENOCSI 50
// #define EL2HLT 51
// #define EBADE 52
// #define EBADR 53
// #define EXFULL 54
// #define ENOANO 55
// #define EBADRQC 56
// #define EBADSLT 57
// #define EDEADLOCK EDEADLK
// #define EBFONT 59
// #define ENOSTR 60
// #define ENODATA 61
// #define ETIME 62
// #define ENOSR 63
// #define ENONET 64
// #define ENOPKG 65
// #define EREMOTE 66
// #define ENOLINK 67
// #define EADV 68
// #define ESRMNT 69
// #define ECOMM 70
// #define EPROTO 71
// #define EMULTIHOP 72
// #define EDOTDOT 73
// #define EBADMSG 74
// #define EOVERFLOW 75
// #define ENOTUNIQ 76
// #define EBADFD 77
// #define EREMCHG 78
// #define ELIBACC 79
// #define ELIBBAD 80
// #define ELIBSCN 81
// #define ELIBMAX 82
// #define ELIBEXEC 83
// #define EILSEQ 84
// #define ERESTART 85
// #define ESTRPIPE 86
// #define EUSERS 87
// #define ENOTSOCK 88
// #define EDESTADDRREQ 89
// #define EMSGSIZE 90
// #define EPROTOTYPE 91
// #define ENOPROTOOPT 92
// #define EPROTONOSUPPORT 93
// #define ESOCKTNOSUPPORT 94
// #define EOPNOTSUPP 95
// #define ENOTSUP EOPNOTSUPP
// #define EPFNOSUPPORT 96
// #define EAFNOSUPPORT 97
// #define EADDRINUSE 98
// #define EADDRNOTAVAIL 99
// #define ENETDOWN 100
// #define ENETUNREACH 101
// #define ENETRESET 102
// #define ECONNABORTED 103
// #define ECONNRESET 104
// #define ENOBUFS 105
// #define EISCONN 106
// #define ENOTCONN 107
// #define ESHUTDOWN 108
// #define ETOOMANYREFS 109
// #define ETIMEDOUT 110
// #define ECONNREFUSED 111
// #define EHOSTDOWN 112
// #define EHOSTUNREACH 113
// #define EALREADY 114
// #define EINPROGRESS 115
// #define ESTALE 116
// #define EUCLEAN 117
// #define ENOTNAM 118
// #define ENAVAIL 119
// #define EISNAM 120
// #define EREMOTEIO 121
// #define EDQUOT 122
// #define ENOMEDIUM 123
// #define EMEDIUMTYPE 124
// #define ECANCELED 125
// #define ENOKEY 126
// #define EKEYEXPIRED 127
// #define EKEYREVOKED 128
// #define EKEYREJECTED 129
// #define EOWNERDEAD 130
// #define ENOTRECOVERABLE 131
// #define ERFKILL 132
// #define EHWPOISON 133

enum errnos : int
{
    EPERM = 1,
    ENOENT = 2,
    ESRCH = 3,
    EINTR = 4,
    EIO = 5,
    ENXIO = 6,
    E2BIG = 7,
    ENOEXEC = 8,
    EBADF = 9,
    ECHILD = 10,
    EAGAIN = 11,
    ENOMEM = 12,
    EACCES = 13,
    EFAULT = 14,
    ENOTBLK = 15,
    EBUSY = 16,
    EEXIST = 17,
    EXDEV = 18,
    ENODEV = 19,
    ENOTDIR = 20,
    EISDIR = 21,
    EINVAL = 22,
    ENFILE = 23,
    EMFILE = 24,
    ENOTTY = 25,
    ETXTBSY = 26,
    EFBIG = 27,
    ENOSPC = 28,
    ESPIPE = 29,
    EROFS = 30,
    EMLINK = 31,
    EPIPE = 32,
    EDOM = 33,
    ERANGE = 34,
    EDEADLK = 35,
    ENAMETOOLONG = 36,
    ENOLCK = 37,
    ENOSYS = 38,
    ENOTEMPTY = 39,
    ELOOP = 40,
    EWOULDBLOCK = EAGAIN,
    ENOMSG = 42,
    EIDRM = 43,
    ECHRNG = 44,
    EL2NSYNC = 45,
    EL3HLT = 46,
    EL3RST = 47,
    ELNRNG = 48,
    EUNATCH = 49,
    ENOCSI = 50,
    EL2HLT = 51,
    EBADE = 52,
    EBADR = 53,
    EXFULL = 54,
    ENOANO = 55,
    EBADRQC = 56,
    EBADSLT = 57,
    EDEADLOCK = EDEADLK,
    EBFONT = 59,
    ENOSTR = 60,
    ENODATA = 61,
    ETIME = 62,
    ENOSR = 63,
    ENONET = 64,
    ENOPKG = 65,
    EREMOTE = 66,
    ENOLINK = 67,
    EADV = 68,
    ESRMNT = 69,
    ECOMM = 70,
    EPROTO = 71,
    EMULTIHOP = 72,
    EDOTDOT = 73,
    EBADMSG = 74,
    EOVERFLOW = 75,
    ENOTUNIQ = 76,
    EBADFD = 77,
    EREMCHG = 78,
    ELIBACC = 79,
    ELIBBAD = 80,
    ELIBSCN = 81,
    ELIBMAX = 82,
    ELIBEXEC = 83,
    EILSEQ = 84,
    ERESTART = 85,
    ESTRPIPE = 86,
    EUSERS = 87,
    ENOTSOCK = 88,
    EDESTADDRREQ = 89,
    EMSGSIZE = 90,
    EPROTOTYPE = 91,
    ENOPROTOOPT = 92,
    EPROTONOSUPPORT = 93,
    ESOCKTNOSUPPORT = 94,
    EOPNOTSUPP = 95,
    ENOTSUP = EOPNOTSUPP,
    EPFNOSUPPORT = 96,
    EAFNOSUPPORT = 97,
    EADDRINUSE = 98,
    EADDRNOTAVAIL = 99,
    ENETDOWN = 100,
    ENETUNREACH = 101,
    ENETRESET = 102,
    ECONNABORTED = 103,
    ECONNRESET = 104,
    ENOBUFS = 105,
    EISCONN = 106,
    ENOTCONN = 107,
    ESHUTDOWN = 108,
    ETOOMANYREFS = 109,
    ETIMEDOUT = 110,
    ECONNREFUSED = 111,
    EHOSTDOWN = 112,
    EHOSTUNREACH = 113,
    EALREADY = 114,
    EINPROGRESS = 115,
    ESTALE = 116,
    EUCLEAN = 117,
    ENOTNAM = 118,
    ENAVAIL = 119,
    EISNAM = 120,
    EREMOTEIO = 121,
    EDQUOT = 122,
    ENOMEDIUM = 123,
    EMEDIUMTYPE = 124,
    ECANCELED = 125,
    ENOKEY = 126,
    EKEYEXPIRED = 127,
    EKEYREVOKED = 128,
    EKEYREJECTED = 129,
    EOWNERDEAD = 130,
    ENOTRECOVERABLE = 131,
    ERFKILL = 132,
    EHWPOISON = 133
};

int *__errno_location();
// #define errno (*__errno_location())
#define errno (*__errno_location())

#ifdef __cplusplus
} // extern "C"
#endif