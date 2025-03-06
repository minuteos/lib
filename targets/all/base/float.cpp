/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/float.cpp
 *
 * Fast FP32 to string
 *
 * Algorithm is based on Grisu2 by Florian Loitsch, adapted to use only
 * 32-bit integers and make use of full decimal to binary lookup table
 * in both directions (which is much smaller than the fp64 counterpart)
 *
 * The priorities in adjusting the algorithm were
 * - simplicity and efficiency (comparably fast to integer conversion, doesn't even use an FPU)
 * - roundtrip correctness (all possible float values tested)
 * - practicality (the output is as short and readable as possible)
 */

#include "float.h"
#include "Pow10.h"

char* fast_ftoa(float v, char* buf)
{
    uint32_t f = unsafe_cast<uint32_t>(v);

    // handle sign
    if (GETBIT(f, 31))
    {
        *buf++ = '-';
        f ^= BIT(31);
    }

    // handle zero
    if (!f)
    {
        *buf++ = '0';
        return buf;
    }

    int e = f >> 23;    // exponent (biased)
    f &= MASK(23);      // significand

    // handle subnormals
    if (e) { f |= BIT(23); } // normal numbers - hidden bit
    else { e++; } // subnormal numbers - adjust exponent

    // un-bias exponent
    e -= 150;

    // add two extra bits, get boundaries
    bool eBoundary = !(f & MASK(23));
    f <<= 2; e -= 2;
    // halve negative boundary if we are on precision boundary
    uint32_t fp = f + 2, fm = f - (eBoundary ? 1 : 2);

    // normalize everything to the same exponent
    int n = __builtin_clz(fp);
    fp <<= n; fm <<= n; f <<= n; e -= n;

    // get the closest power of 10 that fits into Q4.28
    int k = ((e + 32) * 1233 >> 12); // fast approximation of log10(2)
    auto p10 = Pow10(-k);

    // calculate Q4.28 versions of all necessary inputs
    // - p is the maximum significand to be output
    // - delta is the maximum acceptable difference before the result becomes another number
    // - err is the distance to the input number
    n = -e - p10.Exponent() - 28;
    uint32_t qfp = p10.Multiply32(fp) >> n;
    uint32_t qfm = p10.Multiply32(fm) >> n;
    uint32_t qf = p10.Multiply32(f) >> n;
    uint32_t rest = qfp;
    uint32_t delta = qfp - qfm - 3;
    uint32_t err = qfp - qf;

    // generate significant digits until rest < delta
    char* wp = buf;

    if (auto digit = rest >> 28)
    {
        // initial digits above decimal
        rest &= MASK(28);
        if (digit >= 10)
        {
            *wp++ = '1';
            digit -= 10;
            k++;
        }
        *wp++ = '0' + digit;
    }
    else
    {
        k--;
    }

    // generate digits until precision is sufficient
    while (rest >= delta)
    {
        rest *= 10;
        delta *= 10;
        err *= 10;
        *wp++ = '0' + (rest >> 28);
        rest &= MASK(28);
    }

    // grisu rounding - try to match the input more closely
    while (rest < err)
    {
        uint32_t oldDiff = err - rest;
        // increase rest (= last digit minus one) and try if it's acceptable and closer to input
        rest += BIT(28);
        if (rest < delta && (rest < err ? err - rest : rest - err) < oldDiff)
        {
            // the previous digit is always > 1 so there is no point handling edge cases
            ASSERT(wp[-1] > '1');
            wp[-1]--;
        }
        else
        {
            break;
        }
    }

    // finalize formatting
    const int len = wp - buf;   // number of significant digits generated
    const int dp = 1 + k;       // decimal point location relative to buf (can be outside)

    if (dp >= len && dp < 12)
    {
        // append zeroes up to 12 digits (plain whole numbers)
        for (int i = len; i < dp; i++)
        {
            *wp++ = '0';
        }
        return wp;
    }

    if (dp > 0 && dp < len)
    {
        // decimal point between the significant digits
        for (int i = len; i > dp; i--)
        {
            buf[i] = buf[i - 1];
        }
        buf[dp] = '.';
        return wp + 1;
    }

    if (dp > -3 && dp <= 0)
    {
        // prepend up to three zeroes before going for exponential notation
        const int extra = -dp + 2;
        for (int i = len + extra - 1; i >= extra; i--)
        {
            buf[i] = buf[i - extra];
        }
        for (int i = 0; i < extra; i++)
        {
            buf[i] = i == 1 ? '.' : '0';
        }
        return wp + extra;
    }

    // all other cases get the exponential notation,
    // start by inserting a decimal point after first digit
    if (len > 1)
    {
        for (int i = len; i > 1; i--)
        {
            buf[i] = buf[i - 1];
        }
        buf[1] = '.';
        wp++;
    }

    *wp++ = 'e';
    if (k < 0)
    {
        *wp++ = '-';
        k = -k;
    }
    if (k >= 10)
    {
        *wp++ = '0' + k / 10;
        k %= 10;
    }
    *wp++ = '0' + k;
    return wp;
}

float fast_atof(const char* s)
{
    bool minus = *s == '-';
    if (minus) s++;

    uint32_t u = 0; // first 9 parsed significant digits (as much as we can fit into U32)
    int k = 0;      // decimal exponent
    const char* s0 = s;

    while (*s >= '0' && *s <= '9')
    {
        if (u < 100000000)
        {
            u = u * 10 + *s - '0';
        }
        else
        {
            k++;
        }
        s++;
    }

    if (*s == '.')
    {
        s++;
        while (*s >= '0' && *s <= '9')
        {
            if (u < 100000000)
            {
                u = u * 10 + *s - '0';
                k--;
            }
            s++;
        }
    }

    if (s == s0)
    {
        // no digits parsed = NAN
        return unsafe_cast<float>((minus * BIT(31)) | (0xFF8 << 19));
    }

    if (*s == 'e' || *s == 'E')
    {
        s++;
        bool eminus = false;
        if (*s == '+' || *s == '-')
        {
            eminus = *s++ == '-';
        }
        int ee = 0;
        while (*s >= '0' && *s <= '9')
        {
            ee = ee * 10 + *s++ - '0';
        }
        k += eminus ? -ee : ee;
    }

    if (!u || k < Pow10::Min)
    {
        // zero or too small
        return unsafe_cast<float>(minus * BIT(31));
    }

    if (k > Pow10::Max)
    {
        // exponent out of range = infinity
        return unsafe_cast<float>((minus * BIT(31)) | (0xFF << 23));
    }


    Pow10 p10(k);
    int e = p10.Exponent();
    auto m64 = p10.Multiply64(u);

    // offset significand to normal F32 form (keep 24 significant bits in the upper word)
    int offset = 32 + 8 - __builtin_clz(m64 >> 32);
    e += 126 + offset - 8;   // final biased exponent
    if (e <= 0)
    {
        // subnormal - increase offset
        offset += -e + 1;
        e = 0;
    }

    // midpoint rounding - add the last bit to be shifted out to the result
    uint32_t m = m64 >> (offset - 1);
    m = (m >> 1) + (m & 1);

    // rounding on all-1 pattern adds an extra bit on top, transfer it to the exponent
    e += m >> (e ? 24 : 23);

    return unsafe_cast<float>((minus * BIT(31)) | (e << 23) | (m & MASK(23)));
}
