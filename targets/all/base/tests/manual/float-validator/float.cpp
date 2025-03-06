/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/tests/manual/float.cpp
 *
 * Long-running test testing roundtrip conversion of all possible FP32 values
 */

#include <testrunner/TestCase.h>
#include <testrunner/ParallelRunner.h>

#include <base/float.h>

namespace
{

TEST_CASE("FtoA-AtoF roundtrip")
{
    struct Range { uint32_t rng; uint32_t len; };
    struct MaxLen { float f; size_t len; };

    ParallelRunner<Range, MaxLen> pr([](Range rng) {
        char buf[16];
        MaxLen res = {};
        for (uint32_t i = 0; i < rng.len; i++)
        {
            uint32_t u = rng.rng + i;
            float f = unsafe_cast<float>(u);
            if (isnan(f))
            {
                continue;
            }
            bool log = i == 0;
            char* end = fast_ftoa(f, buf);
            *end = 0;
            size_t len = end - buf;
            if (len > res.len)
            {
                res.len = len;
                res.f = f;
            }
            float f2 = fast_atof(buf);
            if (f2 != f || log)
            {
                char buf2[16];
                *fast_ftoa(f2, buf2) = 0;
                printf("\r%08X   %s  >  %s%s\033[K", unsigned(u), buf, buf2, f != f2 ? " !\n" : "");
            }
        }
        return res;
    });

    for (uint32_t rng = 0, i = 0; i < BIT(12); i++, rng += BIT(20))
    {
        pr.Enqueue({ rng, BIT(20) });
    }

#if TESTRUNNER_PARALLEL_SUPPORTED
    pr.Close();

    int max = 0;
    MaxLen m;
    while (pr.WaitAndDequeue(m))
    {
        if (max < m.len)
        {
            max = m.len;
            char buf[30];
            *fast_ftoa(m.f, buf) = 0;
            fprintf(stderr, "\rMax len: %d str: %s\033[K\n", max, buf);
        }
    }
#endif
}

}
