// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <cstdint>
#include <cstddef>

// GCC and Clang folding hack (C++11)
#define fold(x) __builtin_constant_p(x) ? x : x

#define SYMLOOP_MAX 40

using off_t = long;

using dev_t = unsigned long;
using ino_t = unsigned long;

using nlink_t = unsigned long;
using mode_t = unsigned int;

using blksize_t = long;
using blkcnt_t = long;

using uid_t = unsigned int;
using gid_t = unsigned int;

using pid_t = int;
using tid_t = int;

using time_t = long;
using clockid_t = int;

using suseconds_t = long;

using cc_t = unsigned char;
using speed_t = unsigned int;
using tcflag_t = unsigned int;

enum prctls
{
    arch_set_gs = 0x1001,
    arch_set_fs = 0x1002,
    arch_get_fs = 0x1003,
    arch_get_gs = 0x1004
};

enum seeks
{
    seek_set = 0,
    seek_cur = 1,
    seek_end = 2
};

enum fcntls
{
    f_dupfd_cloexec = 1030,
    f_dupfd = 0,
    f_getfd = 1,
    f_setfd = 2,
    f_getfl = 3,
    f_setfl = 4,

    f_setown = 8,
    f_getown = 9,
    f_setsig = 10,
    f_getsig = 11,

    fd_cloexec = 1
};

enum types
{
    s_ifmt = 0170000,
    s_ifsock = 0140000,
    s_iflnk = 0120000,
    s_ifreg = 0100000,
    s_ifblk = 0060000,
    s_ifdir = 0040000,
    s_ifchr = 0020000,
    s_ififo = 0010000,
};
inline constexpr types mode2type(mode_t mode)
{
    return types(mode & s_ifmt ?: s_ifreg);
}

enum dtypes
{
    dt_unknown = 0,
    dt_fifo = 1,
    dt_chr = 2,
    dt_dir = 4,
    dt_blk = 6,
    dt_reg = 8,
    dt_lnk = 10,
    dt_sock = 12,
    dt_wht = 14
};

enum modes
{
    s_irwxu = 00700, // user rwx
    s_irusr = 00400, // user r
    s_iwusr = 00200, // user w
    s_ixusr = 00100, // user x
    s_irwxg = 00070, // group rwx
    s_irgrp = 00040, // group r
    s_iwgrp = 00020, // group w
    s_ixgrp = 00010, // group x
    s_irwxo = 00007, // others rwx
    s_iroth = 00004, // others r
    s_iwoth = 00002, // others w
    s_ixoth = 00001, // others x
    s_isuid = 04000, // set-user-id
    s_isgid = 02000, // set-group-id
    s_isvtx = 01000, // set-sticky
};

enum oflags
{
    o_async = 020000,
    o_direct = 040000,
    o_largefile = 0100000,
    o_noatime = 01000000,
    o_path = 010000000,
    o_tmpfile = 020000000,
    o_exec = o_path,
    o_search = o_path,

    o_accmode = (03 | o_path),
    o_rdonly = 00,
    o_wronly = 01,
    o_rdwr = 02,

    o_creat = 0100,
    o_excl = 0200,
    o_noctty = 0400,
    o_trunc = 01000,
    o_append = 02000,
    o_nonblock = 04000,
    o_dsync = 010000,
    o_sync = 04010000,
    o_rsync = 04010000,
    o_directory = 0200000,
    o_nofollow = 0400000,
    o_cloexec = 02000000,
};

enum atflags
{
    at_fdcwd = -100,
    at_symlink_follow = 0x400,
    at_symlink_nofollow = 0x100,
    at_removedir = 0x200,
    at_no_automount = 0x800,
    at_empty_path = 0x1000,
    at_eaccess = 0x200
};

enum wflags
{
    wnohang = 1,
    wuntraced = 2,
    wstopped = 2,
    wexited = 4,
    wcontinued = 8,
    wnowait = 0x01000000,
    wcoreflag = 0x80
};

inline constexpr auto wexitstatus(auto x) { return (x & 0xff00) >> 8; }
inline constexpr auto wtermsig(auto x) { return x & 0x7F; }
inline constexpr auto wstopsig(auto x) { return wexitstatus(x); }
inline constexpr auto wifexited(auto x) { return wtermsig(x) == 0; }
inline constexpr auto wifsignaled(auto x) { return (static_cast<int8_t>((x & 0x7F) + 1) >> 1) > 0; }
inline constexpr auto wifstopped(auto x) { return (x & 0xFF) == 0x7F; }
inline constexpr auto wifcontinued(auto x) { return x == 0xFFFF; }
inline constexpr auto wcoredump(auto x) { return (x) & wcoreflag; }
inline constexpr auto w_exitcode(auto ret, auto sig) { return (ret << 8) | sig; }

enum clone_flags
{
    csignal = 0x000000FF,              // signal mask to be sent at exit
    clone_vm = 0x00000100,             // set if VM shared between processes
    clone_fs = 0x00000200,             // set if fs info shared between processes
    clone_files = 0x00000400,          // set if open files shared between processes
    clone_sighand = 0x00000800,        // set if signal handlers and blocked signals shared
    clone_pidfd = 0x00001000,          // set if a pidfd should be placed in parent
    clone_ptrace = 0x00002000,         // set if we want to let tracing continue on the child too
    clone_vfork = 0x00004000,          // set if the parent wants the child to wake it up on mm_release
    clone_parent = 0x00008000,         // set if we want to have the same parent as the cloner
    clone_thread = 0x00010000,         // same thread group?
    clone_newns = 0x00020000,          // new mount namespace group
    clone_sysvsem = 0x00040000,        // share system V SEM_UNDO semantics
    clone_settls = 0x00080000,         // create a new TLS for the child
    clone_parent_settid = 0x00100000,  // set the TID in the parent
    clone_child_cleartid = 0x00200000, // clear the TID in the child
    clone_detached = 0x00400000,       // unused, ignored
    clone_untraced = 0x00800000,       // set if the tracing process can't force clone_PTRACE on this clone
    clone_child_settid = 0x01000000,   // set the TID in the child
    clone_newcgroup = 0x02000000,      // new cgroup namespace
    clone_newuts = 0x04000000,         // new utsname namespace
    clone_newipc = 0x08000000,         // new ipc namespace
    clone_newuser = 0x10000000,        // new user namespace
    clone_newpid = 0x20000000,         // new pid namespace
    clone_newnet = 0x40000000,         // new network namespace
    clone_io  = 0x80000000             // clone io context
};

enum c_iflags
{
    brkint = 0000002,
    icrnl = 0000400,
    ignbrk = 0000001,
    igncr = 0000200,
    ignpar = 0000004,
    inlcr = 0000100,
    inpck = 0000020,
    istrip = 0000040,
    ixany = 0004000,
    ixoff = 0010000,
    ixon = 0002000,
    parmrk = 0000010
};

enum c_oflags
{
    opost = 0000001,
    onlcr = 0000004,
    ocrnl = 0000010,
    onocr = 0000020,
    onlret = 0000040,
    ofdel = 0000200,
    ofill = 0000100,

    nldly = 0000400,
    nl0 = 0000000,
    nl1 = 0000400,

    crdly = 0003000,
    cr0 = 0000000,
    cr1 = 0001000,
    cr2 = 0002000,
    cr3 = 0003000,

    tabdly = 0014000,
    tab0 = 0000000,
    tab1 = 0004000,
    tab2 = 0010000,
    tab3 = 0014000,

    xtabs = 0014000,
    bsdly = 0020000,
    bs0 = 0000000,
    bs1 = 0020000,

    vtdly = 0040000,
    vt0 = 0000000,
    vt1 = 0040000,

    ffdly = 0100000,
    ff0 = 0000000,
    ff1 = 0100000
};

enum c_cflags
{
    csize = 0000060,
    cs5 = 0000000,
    cs6 = 0000020,
    cs7 = 0000040,
    cs8 = 0000060,

    cstopb = 0000100,
    cread = 0000200,
    parenb = 0000400,
    parodd = 0001000,
    hupcl = 0002000,
    clocal = 0004000
};

enum c_lflags
{
    echo = 0000010,
    echoe = 0000020,
    echok = 0000040,
    echonl = 0000100,
    icanon = 0000002,
    iexten = 0100000,
    isig = 0000001,
    noflsh = 0000200,
    tostop = 0000400,
    echoprt = 0002000,

    echoctl = 0001000,
    flusho  = 0010000,
    imaxbel = 0020000,
    echoke  = 0040000
};

enum ccs
{
    vintr = 0,
    vquit = 1,
    verase = 2,
    vkill = 3,
    veof = 4,
    vtime = 5,
    vmin = 6,
    vswtc = 7,
    vstart = 8,
    vstop = 9,
    vsusp = 10,
    veol = 11,
    vreprint = 12,
    vdiscard = 13,
    vwerase = 14,
    vlnext = 15,
    veol2 = 16
};

enum bauds
{
    b0 = 0,
    b50 = 1,
    b75 = 2,
    b110 = 3,
    b134 = 4,
    b150 = 5,
    b200 = 6,
    b300 = 7,
    b600 = 8,
    b1200 = 9,
    b1800 = 10,
    b2400 = 11,
    b4800 = 12,
    b9600 = 13,
    b19200 = 14,
    b38400 = 15,
    b57600 = 16,
    b115200 = 17,
    b230400 = 18,
};

enum ioctls
{
    tcgets = 0x5401,
    tcsets = 0x5402,
    tcsetsw = 0x5403,
    tcsetsf = 0x5404,
    tcsbrk = 0x5409,
    tcxonc = 0x540A,
    tiocsctty = 0x540E,
    tiocsti = 0x5412,
    tiocgwinsz = 0x5413,
    tiocmget = 0x5415,
    tiocmset = 0x5418,
    tiocinq = 0x541B,
    tiocnotty = 0x5422
};

enum fbtypes
{
    fb_type_packed_pixels = 0,
    fb_type_planes = 1,
    fb_type_interleaved_planes = 2,
    fb_type_text = 3,
    fb_type_vga_planes = 4,
    fb_type_fourcc = 5,
};

enum fbvisuals
{
    fb_visual_mono01 = 0,
    fb_visual_mono10 = 1,
    fb_visual_truecolor = 2,
    fb_visual_pseudocolor = 3,
    fb_visual_directcolor = 4,
    fb_visual_static_pseudocolor = 5,
    fb_visual_fourcc = 6,
};

enum fbaccels
{
    fb_accel_none = 0,
    /* ... */
};

enum fbioctls
{
    fbioget_vscreeninfo = 0x4600,
    fbioput_vscreeninfo = 0x4601,
    fbioget_fscreeninfo = 0x4602,
    fbiogetcmap = 0x4604,
    fbioputcmap = 0x4605,
    fbiopan_display = 0x4606,
    fbioblank = 0x4611
};

struct kernel_clone_args
{
    uint64_t flags;
    int *pidfd;
    int *child_tid;
    int *parent_tid;
    uint32_t exit_signal;
    uintptr_t stack;
    size_t stack_size;
    uintptr_t tls;
    pid_t *set_tid;
    size_t set_tid_size;
    int cgroup;
};

struct clone_args
{
    uint64_t flags;        // flags bit mask
    uint64_t pidfd;        // where to store PID file descriptor (int *)
    uint64_t child_tid;    // where to store child TID, in child's memory (pid_t *)
    uint64_t parent_tid;   // where to store child TID, in parent's memory (pid_t *)
    uint64_t exit_signal;  // signal to deliver to parent on child termination
    uint64_t stack;        // pointer to lowest byte of stack
    uint64_t stack_size;   // size of stack
    uint64_t tls;          // location of new TLS
    uint64_t set_tid;      // pointer to a pid_t array (since Linux 5.5)
    uint64_t set_tid_size; // number of elements in set_tid (since Linux 5.5)
    uint64_t cgroup;       // file descriptor for target cgroup of child (since Linux 5.7)
};

struct timespec
{
    time_t tv_sec;
    long tv_nsec;

    constexpr timespec() : tv_sec(0), tv_nsec(0) { }
    constexpr timespec(time_t s, long ns) : tv_sec(s), tv_nsec(ns) { }
    constexpr timespec(time_t ms) : tv_sec(ms / 1'000), tv_nsec((ms % 1'000) * 1'000'000) { }

    constexpr long to_ns()
    {
        return this->tv_sec * 1'000'000'000 + this->tv_nsec;
    }

    constexpr time_t to_ms()
    {
        return this->to_ns() / 1'000'000;
    }

    constexpr timespec &operator+=(const timespec &rhs)
    {
        this->tv_sec += rhs.tv_sec;
        this->tv_nsec += rhs.tv_nsec;

        while (this->tv_nsec >= 1'000'000'000)
        {
            this->tv_sec++;
            this->tv_nsec -= 1'000'000'000;
        }

        return *this;
    }

    friend constexpr timespec operator+(const timespec &lhs, const timespec &rhs)
    {
        timespec ret = lhs;
        return ret += rhs;
    }

    friend constexpr timespec operator+(const timespec &lhs, long ns)
    {
        return lhs + timespec(0, ns);
    }

    constexpr timespec &operator-=(const timespec &rhs)
    {
        this->tv_sec -= rhs.tv_sec;
        this->tv_nsec -= rhs.tv_nsec;

        while (this->tv_nsec < 0)
        {
            this->tv_sec--;
            this->tv_nsec += 1'000'000'000;
        }
        if (this->tv_sec < 0)
        {
            this->tv_sec = 0;
            this->tv_nsec = 0;
        }

        return *this;
    }

    friend constexpr timespec operator-(const timespec &lhs, const timespec &rhs)
    {
        timespec ret = lhs;
        return ret -= rhs;
    }

    friend constexpr timespec operator-(const timespec &lhs, long ns)
    {
        return lhs - timespec(0, ns);
    }
};

struct timeval
{
    time_t tv_sec;
    suseconds_t tv_usec;
};

inline constexpr unsigned char if2dt(mode_t mode)
{
    return (mode & s_ifmt) >> 12;
}

inline constexpr mode_t dt2if(unsigned char dt)
{
    return dt << 12;
}

struct stat_t
{
    dev_t st_dev;
    ino_t st_ino;
    nlink_t st_nlink;
    mode_t st_mode;
    uid_t st_uid;
    gid_t st_gid;
    unsigned int __unused0;
    dev_t st_rdev;
    off_t st_size;

    blksize_t st_blksize;
    blkcnt_t st_blocks;

    timespec st_atim;
    timespec st_mtim;
    timespec st_ctim;

    #define st_atime st_atim.tv_sec
    #define st_mtime st_mtim.tv_sec
    #define st_ctime st_ctim.tv_sec

    long __unused1[3];

    constexpr types type()
    {
        return types(this->st_mode & s_ifmt);
    }

    constexpr mode_t mode()
    {
        return this->st_mode & ~s_ifmt;
    }

    enum which { access = (1 << 0), modify = (1 << 1), status = (1 << 2) };
    void update_time(size_t flags);

    enum accmode { read = (1 << 0), write = (1 << 1), exec = (1 << 2) };
    bool has_access(uid_t uid, gid_t gid, size_t rwe)
    {
        if (uid == 0)
            return true;

        mode_t mask_uid = 0, mask_gid = 0, mask_oth = 0;
        if (rwe & read) { mask_uid |= s_irusr; mask_gid |= s_irgrp; mask_oth |= s_iwoth; }
        if (rwe & write) { mask_uid |= s_iwusr; mask_gid |= s_iwgrp; mask_oth |= s_iwoth; }
        if (rwe & exec) { mask_uid |= s_ixusr; mask_gid |= s_ixgrp; mask_oth |= s_ixoth; }

        if (this->st_uid == uid)
            return (this->st_mode & mask_uid) == mask_uid;
        else if (this->st_gid == gid)
            return (this->st_mode & mask_gid) == mask_gid;

        return (this->st_mode & mask_oth) == mask_oth;
    }
};

inline constexpr unsigned int major(dev_t dev)
{
    unsigned int ret = ((dev & dev_t(0x00000000000FFF00U)) >> 8);
    return ret |= ((dev & dev_t(0xFFFFF00000000000U)) >> 32);
}

inline constexpr unsigned int minor(dev_t dev)
{
    unsigned int ret = (dev & dev_t(0x00000000000000FFU));
    return ret |= ((dev & dev_t(0x00000FFFFFF00000U)) >> 12);
}

inline constexpr dev_t makedev(unsigned int maj, unsigned int min)
{
    dev_t ret  = dev_t(maj & 0x00000FFFU) << 8;
    ret |= dev_t(maj & 0xFFFFF000U) << 32;
    ret |= dev_t(min & 0x000000FFU);
    ret |= dev_t(min & 0xFFFFFF00U) << 12;
    return ret;
}

struct dirent
{
    ino_t d_ino;
    off_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[1024];
};
constexpr auto dirent_len = sizeof(dirent) - 1024;

struct utsname
{
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};

#define NCCS 32
struct termios
{
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_line;
    cc_t c_cc[NCCS];
    speed_t ispeed;
    speed_t ospeed;
};

struct winsize
{
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

struct fb_cmap
{
    uint32_t start;
    uint32_t len;
    uint16_t *red;
    uint16_t *green;
    uint16_t *blue;
    uint16_t *transp;
};

struct fb_bitfield
{
    uint32_t offset;
    uint32_t length;
    uint32_t msb_right;
};

struct fb_fix_screeninfo
{
    char id[16];
    unsigned long smem_start;
    uint32_t smem_len;
    uint32_t type;
    uint32_t type_aux;
    uint32_t visual;
    uint16_t xpanstep;
    uint16_t ypanstep;
    uint16_t ywrapstep;
    uint32_t line_length;
    unsigned long mmio_start;
    uint32_t mmio_len;
    uint32_t accel;
    uint16_t capabilities;
    uint16_t reserved[2];
};

struct fb_var_screeninfo
{
    uint32_t xres;
    uint32_t yres;
    uint32_t xres_virtual;
    uint32_t yres_virtual;
    uint32_t xoffset;
    uint32_t yoffset;

    uint32_t bits_per_pixel;
    uint32_t grayscale;
    fb_bitfield red;
    fb_bitfield green;
    fb_bitfield blue;
    fb_bitfield transp;

    uint32_t nonstd;

    uint32_t activate;

    uint32_t height;
    uint32_t width;

    uint32_t accel_flags;

    uint32_t pixclock;
    uint32_t left_margin;
    uint32_t right_margin;
    uint32_t upper_margin;
    uint32_t lower_margin;
    uint32_t hsync_len;
    uint32_t vsync_len;
    uint32_t sync;
    uint32_t vmode;
    uint32_t rotate;
    uint32_t colorspace;
    uint32_t reserved[4];
};

struct rusage
{
    timeval ru_utime;
    timeval ru_stime;
    long ru_maxrss;
    long ru_ixrss;
    long ru_idrss;
    long ru_isrss;
    long ru_minflt;
    long ru_majflt;
    long ru_nswap;
    long ru_inblock;
    long ru_oublock;
    long ru_msgsnd;
    long ru_msgrcv;
    long ru_nsignals;
    long ru_nvcsw;
    long ru_nivcsw;
};