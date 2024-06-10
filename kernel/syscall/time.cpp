// Copyright (C) 2022-2024  ilobilo

#include <syscall/time.hpp>
#include <lib/time.hpp>
#include <lib/log.hpp>
#include <errno.h>

namespace time
{
    int sys_nanosleep(timespec *req, timespec *rem)
    {
        if (req == nullptr)
            return_err(-1, EINVAL);

        if (req->tv_sec == 0 && req->tv_nsec == 0)
            return 0;

        if (req->tv_sec < 0 || req->tv_nsec < 0 || req->tv_nsec >= 1000000000)
            return_err(-1, EINVAL);

        time::timer tmr { *req };
        if (tmr.event.await().has_value() == false)
        {
            if (rem != nullptr)
            {
                rem->tv_sec = time::monotonic.tv_sec - req->tv_sec;
                rem->tv_nsec = time::monotonic.tv_nsec - req->tv_nsec;

                if (rem->tv_nsec < 0)
                {
                    rem->tv_sec--;
                    rem->tv_nsec += 1000000000;
                }
                if (rem->tv_sec < 0)
                {
                    rem->tv_sec = 0;
                    rem->tv_nsec = 0;
                }
            }
            return_err(-1, EINTR);
        }

        return 0;
    }

    int sys_clock_gettime(clockid_t clockid, timespec *tp)
    {
        if (tp == nullptr || clockid < 0)
            return_err(-1, EINVAL);

        switch (clockid)
        {
            case clock_realtime:
                *tp = time::realtime;
                break;
            case clock_monotonic:
                *tp = time::monotonic;
                break;
            default:
                *tp = time::realtime;
        }
        return 0;
    }
} // namespace time