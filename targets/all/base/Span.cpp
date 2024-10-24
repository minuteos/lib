/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/Span.cpp
 */

#include <base/Span.h>
#include <base/format.h>

#include <ctype.h>

Span::packed_t Span::Sub(Span s, size_t start, size_t length)
{
    if (start > s.len)
        start = s.len;
    if (length > s.len - start)
        length = s.len - start;
    return Span(s.p + start, length);
}

Span::packed_t Span::Slice(Span s, int start, int end)
{
    if (end < 0)
        end += s.len;
    if (end < 0)
        return Span();
    if ((unsigned)end > s.len)
        end = s.len;

    if (start < 0)
        start += s.len;
    if (start < 0)
        start = 0;
    if ((unsigned)start > s.len)
        return Span();

    if (end < start)
        return Span();

    return Span(s.p + start, end - start);
}

Span::packed_t Span::Split(Span& s, char sep)
{
    for (size_t i = 0; i < s.len; i++)
    {
        if (s.p[i] == sep)
        {
            auto res = Span(s.p, i);
            s = Span(s.p + i + 1, s.end());
            return res;
        }
    }

    return Span();
}

Span::packed_t Span::Consume(Span& s, char sep)
{
    size_t i;
    auto p = s.p;
    for (i = 0; i < s.len; i++)
    {
        if (p[i] == sep)
        {
            s = Span(p + i + 1, s.end());
            return Span(p, i);
        }
    }

    s = Span();
    return Span(p, i);
}

int Span::ParseInt(Span s, int baseStopOnError, int defVal)
{
    const char* data = s;
    const char* end = s.end();

    while (data < end && isspace(*data))
        data++;

    bool neg = false;

    if (data < end && (*data == '+' || (neg = *data == '-')))
        data++;

    bool stopOnError = baseStopOnError >= 0;
    unsigned base = baseStopOnError < 0 ? -baseStopOnError : baseStopOnError;

    if (baseStopOnError == 0 || baseStopOnError == -1)
    {
        // detect base by prefix (0 for octal, 0x or hex, 0b for binary)
        if (data < end && *data == '0')
        {
            data++;
            if (data < end && (*data == 'x' || *data == 'X'))
            {
                data++;
                base = 16;
            }
            else if (data < end && (*data == 'b' || *data == 'B'))
            {
                data++;
                base = 2;
            }
            else
            {
                base = 8;
            }
        }
        else
        {
            base = 10;
        }
    }

    int res = 0;
    bool hasDigit = false;
    while (data < end)
    {
        char c = *data;
        size_t digit;
        if (c >= '0' && c <= '9')
            digit = c - '0';
        else if (c >= 'a' && c <= 'z')
            digit = c + 10 - 'a';
        else if (c >= 'A' && c <= 'Z')
            digit = c + 10 - 'A';
        else
            break;

        if (digit >= base)
            break;

        res = res * base + digit;
        hasDigit = true;
        data++;
    }

    if (!hasDigit)
        return defVal;

    if (data != end && !stopOnError)
    {
        // tolerate whitespace at the end
        while (data < end && isspace(*data))
            data++;

        // if Span contains invalid characters, we return the default value
        if (data != end)
            return defVal;
    }

    return neg ? -res : res;
}

int Span::ReadInt(Span s, unsigned lenSignRev)
{
    if (!s.Length())
        return 0;

    int32_t res = *(const int32_t*)s;	// res = 44332211
    size_t len = std::min((size_t)(lenSignRev & 0xF), s.Length());
    size_t fill = 32 - (len << 3);

    // align the result with MSB - reversal does this for BE, shift for LE
#if LITTLE_ENDIAN
    if (lenSignRev & 0x20)
#else
    if (!(lenSignRev & 0x20))
#endif
    {
        res = BSWAP32(res);
    }
    else
    {
        res <<= fill;
    }

    // compensate for length by shifting
    if (lenSignRev & 0x10)
    {
        // sign extend
        res >>= fill;
    }
    else
    {
        // zero fill
        res = uint32_t(res) >> fill;
    }

    return res;
}

bool Span::IsAll(Span s, uint32_t val)
{
    while (s.len & 3)
    {
        if ((s.p[--s.len] ^ val) << 24)
        {
            return false;
        }
    }

    while (s.len)
    {
        if (*(const uint32_t*)(s.p + (s.len -= 4)) != val)
        {
            return false;
        }
    }

    return true;
}

Buffer::packed_t Buffer::FormatImpl(char* buf, size_t len, const char* format, va_list va)
{
    format_write_info fwi = { buf, buf + len };
    vformat(format_output_mem, &fwi, format, va);
    return Buffer(buf, fwi.p);
}
