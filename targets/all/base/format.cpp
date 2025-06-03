/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * format.cpp
 */

#include <base/base.h>
#include <base/Span.h>

#include <base/format.h>

#if FORMAT_ENABLE_FAST_FLOAT
#include <base/float.h>
#endif

static const char l_hex[] = "0123456789ABCDEF";

void format_output_discard(void* context, char ch)
{
}

void format_output_mem(void* context, char ch)
{
    format_write_info* wi = (format_write_info*)context;
    if (wi->p < wi->end)
        *(wi->p)++ = ch;
}

void format_output_unsafe(void* pp, char ch)
{
    *(*((char**)pp))++ = ch;
}

int format(format_output output, void* context, const char* format, ...)
{
    return va_call(vformat, format, output, context, format);
}

OPTIMIZE int vformat(format_output output, void* context, const char* format, va_list va)
{
    unsigned n, base, order, maxOrder;
    int width, limit, frac;
    bool left;
    char sign;
    char fill, hexLower;
    const char* str;
    int outputCount = 0;

    if (!format)
        return 0;

    if (!output)
        output = format_output_discard;

    while (*format)
    {
        if (*format != '%')
        {
            output(context, *format++);
            outputCount++;
        }
        else
        {
            format++;	// skip the '%'
            width = 0;
            limit = -1;
            frac = -1;
            base = 10;
            fill = ' ';
            left = false;
            sign = 0;
            hexLower = 0;

            for (;;)
            {
                // this infinite loop breaks under the switch, continue in switch fetches next formatting character
                switch (*format++)
                {
                case '0'...'9':
                    n = format[-1] - '0';

                    if (limit >= 0)
                    {
                        // we're setting the limit unless it's -1
                        limit = limit * 10 + n;
                    }
                    else if (n == 0 && width == 0)
                    {
                        // zero at the first position marks the fill character
                        fill = '0';
                    }
                    else
                    {
                        width = width * 10 + n;
                    }
                    continue;

                case '*':
                    // parametric limit or width
                    if (limit >= 0)
                    {
                        limit = va_arg(va, unsigned);
                    }
                    else
                    {
                        width = va_arg(va, unsigned);
                    }
                    continue;

                case '.':
                    // limit follows
                    limit = 0;
                    continue;

                case '-':
                    // left alignment
                    left = true;
                    continue;

                case '+':
                    // force sign
                    sign = '+';
                    continue;

                case 'l':
                    // just ignore
                    continue;

                case 'c':
                    output(context, (char)va_arg(va, unsigned));
                    outputCount++;
                    break;

                case 'a':
                    fill = '.';
                    // fall through

                case 'b':
                {
                    Span data = va_arg(va, Packed<Span>);
                    str = data;
                    limit = data.Length();
                    width -= limit;

                    if (width < 0)
                        width = 0;

                    if (!left)
                    {
                        // left padding
                        while (width)
                        {
                            output(context, ' ');
                            // outputCount is already accounted for
                            width--;
                        }
                    }

                    for (unsigned i = 0; i < (unsigned)limit; i++)
                    {
                        char c = str[i];
                        if (fill != ' ' && (uint8_t)c < ' ')
                            c = fill;
                        output(context, c);
                        outputCount++;
                    }

                    // right padding
                    while (width)
                    {
                        output(context, ' ');
                        // outputCount is already accounted for
                        width--;
                    }
                    break;
                }

                case 'h':
                    hexLower = 0x20;
                    // fall through

                case 'H':
                {
                    // hexadecimal data
                    Span data = va_arg(va, Packed<Span>);
                    str = data;
                    for (int i = (left ? data.Length() - 1 : 0); i != (left ? -1 : (int)data.Length()); i += (left ? -1 : 1))
                    {
                        output(context, l_hex[data.Element<uint8_t>(i) >> 4] | hexLower);
                        output(context, l_hex[data.Element<uint8_t>(i) & 15] | hexLower);
                        outputCount += 2;
                    }
                    break;
                }

                case 's':
                    str = va_arg(va, const char*);
                    for (unsigned i = 0; str[i] && i < (unsigned)limit; i++)
                        width--;

                    if (width < 0)
                        width = 0;

                    outputCount += width;

                    if (!left)
                    {
                        // left padding
                        while (width)
                        {
                            output(context, ' ');
                            // outputCount is already accounted for
                            width--;
                        }
                    }

                    for (unsigned i = 0; str[i] && i < (unsigned)limit; i++)
                    {
                        output(context, str[i]);
                        outputCount++;
                    }

                    // right padding
                    while (width)
                    {
                        output(context, ' ');
                        // outputCount is already accounted for
                        width--;
                    }
                    break;

                case 'q':
                    // limit == fixed fractional part
                    frac = limit ? limit++ : -1;
                    goto number;

                case 'x':
                    // or output with 0x20 - digits already contain this bit, uppercase letters go lowercase
                    hexLower = 0x20;
                    // fall through

                case 'X':
                case 'p':
                    base = 16;
                    // fall through

                case 'd':
                case 'u':
                number:
                    n = va_arg(va, unsigned);

                    if ((format[-1] == 'd' || format[-1] == 'q'))
                    {
                        if ((int)n < 0)
                        {
                            n = -n;
                            sign = '-';
                        }
                    }
                    else
                    {
                        sign = 0;
                    }

                    if (frac > 0)
                    {
                        width--;
                        frac--;
                    }

                    // find the highest order, compensate for width
                    order = 1;
                    maxOrder = ~0u / base;	// maxOrder is the highest number that can still be multiplied by base without overflowing
                    while (order <= maxOrder && ((limit > 1 && limit < 10) || (order * base <= n)))
                    {
                        order *= base;
                        limit--;
                        frac--;
                        width--;
                    }

                    if (sign)
                    {
                        width--;

                        if (fill == '0')
                        {
                            // place the sign now
                            output(context, sign);
                            outputCount++;
                            sign = 0;
                        }
                    }

                    // now add the filling digits/spaces
                    while (width > 1)
                    {
                        output(context, fill);
                        outputCount++;
                        width--;
                    }

                    if (sign)
                    {
                        output(context, sign);
                        outputCount++;
                    }

                    // convert the actual value
                    while (order)
                    {
                        if (frac++ == 0)
                        {
                            output(context, '.');
                            outputCount++;
                        }
                        output(context, l_hex[n / order % base] | hexLower);
                        outputCount++;
                        order /= base;
                    }
                    break;

#if FORMAT_ENABLE_FAST_FLOAT
                case 'F':
                {
                    // 32-bit floating point number, passed using fva
                    union { uint32_t u; float f; } f32 = { .u = va_arg(va, uint32_t) };
                    char buf[16];
                    char* end = fast_ftoa(f32.f, buf);
                    for (str = buf; str < end; str++)
                    {
                        output(context, *str);
                        outputCount++;
                    }
                    break;
                }
#endif

                case '%':
                    output(context, '%');
                    outputCount++;
                    break;

                default:
                    // unknown output command
                    output(context, '?');
                    outputCount++;
                    break;
                }
                break;
            }
        }
    }

    return outputCount;
}
