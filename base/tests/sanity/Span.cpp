/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * Span.cpp
 *
 * Tests the memory span manipulation methods
 *
 */

#include <testrunner/TestCase.h>

#include <base/Span.h>

namespace   // prevent collisions
{

TEST_CASE("01 Sub")
{
    Span span = "0123456789ABCDEF";

    AssertEqual(span, Span("0123456789ABCDEF"));

    AssertEqual(span.Left(0), Span());
    AssertEqual(span.Left(5), Span("01234"));
    AssertEqual(span.Left(1000), span);

    AssertEqual(span.Right(0), Span());
    AssertEqual(span.Right(5), Span("BCDEF"));
    AssertEqual(span.Right(1000), span);

    AssertEqual(span.Sub(0, 1000), span);
    AssertEqual(span.Sub(1000, 1000), Span());
    AssertEqual(span.Sub(3, 5), Span("34567"));

    AssertEqual(span.RemoveLeft(0), span);
    AssertEqual(span.RemoveLeft(3), Span("3456789ABCDEF"));
    AssertEqual(span.RemoveLeft(300), Span());

    AssertEqual(span.RemoveRight(0), span);
    AssertEqual(span.RemoveRight(3), Span("0123456789ABC"));
    AssertEqual(span.RemoveRight(300), Span());

    AssertEqual(span.ConsumeLeft(3), Span("012"));
    AssertEqual(span, Span("3456789ABCDEF"));
    AssertEqual(span.ConsumeRight(3), Span("DEF"));
    AssertEqual(span, Span("3456789ABC"));
    AssertEqual(span.ConsumeLeft(300), Span("3456789ABC"));
    AssertEqual(span, Span());
}

TEST_CASE("02 Slicing")
{
    Span span = "0123456789ABCDEF";

    AssertEqual(span.SliceLeft(0), Span());
    AssertEqual(span.SliceLeft(3), Span("012"));
    AssertEqual(span.SliceLeft(100), span);
    AssertEqual(span.SliceLeft(-1), Span("0123456789ABCDE"));
    AssertEqual(span.SliceLeft(-10), Span("012345"));
    AssertEqual(span.SliceLeft(-100), Span());

    AssertEqual(span.SliceRight(0), span);
    AssertEqual(span.SliceRight(3), Span("3456789ABCDEF"));
    AssertEqual(span.SliceRight(100), Span());
    AssertEqual(span.SliceRight(-1), Span("F"));
    AssertEqual(span.SliceRight(-10), Span("6789ABCDEF"));
    AssertEqual(span.SliceRight(-100), span);

    AssertEqual(span.Slice(10, 10), Span());
    AssertEqual(span.Slice(10, 100), Span("ABCDEF"));
    AssertEqual(span.Slice(-10, 10), Span("6789"));
    AssertEqual(span.Slice(-100, 10), Span("0123456789"));
    AssertEqual(span.Slice(0, -1), Span("0123456789ABCDE"));
    AssertEqual(span.Slice(5, -3), Span("56789ABC"));
    AssertEqual(span.Slice(6, -10), Span());
    AssertEqual(span.Slice(6, 3), Span());
    AssertEqual(span.Slice(6, -100), Span());
    AssertEqual(span.Slice(-1, -100), Span());
    AssertEqual(span.Slice(-4, -6), Span());
    AssertEqual(span.Slice(-4, -1), Span("CDE"));
    AssertEqual(!!span.Slice(-4, -4), true);
    AssertEqual(!!span.Slice(-4, -5), false);
}

TEST_CASE("03 Splitting")
{
    Span span = "a=1;b=2;c=;;d=8;e=9;f";

    auto fa = span.Consume(';');
    auto fb = span.Consume(';');
    auto fc = span.Consume(';');
    auto f_ = span.Consume(';');
    auto fd = span.Consume(';');
    auto fe = span.Consume(';');
    auto ff = span.Consume(';');
    AssertEqual(!!span, false);
    AssertEqual(span.Consume(';'), Span());
    AssertEqual(!!span.Consume(';'), false);

    AssertEqual(fa, Span("a=1"));
    AssertEqual(fb, Span("b=2"));
    AssertEqual(fc, Span("c="));
    AssertEqual(f_, Span());
    AssertEqual(!!f_, true);
    AssertEqual(fd, Span("d=8"));
    AssertEqual(fe, Span("e=9"));
    AssertEqual(ff, Span("f"));
    AssertEqual(!!span, false);

    Span key, value;
    AssertEqual(fa.Split('=', key, value), true);
    AssertEqual(key, Span("a"));
    AssertEqual(value, Span("1"));
    AssertEqual(ff.Split('=', key, value), false);
    AssertEqual(key, Span());
    AssertEqual(value, Span("f"));

    AssertEqual(fa.Split('='), Span("a"));
    AssertEqual(fa, Span("1"));
    AssertEqual(fc.Split('='), Span("c"));
    AssertEqual(!!fc, true);
    AssertEqual(fc, Span());
    AssertEqual(!!f_.Split('='), false);
}

TEST_CASE("04 Integer Parsing")
{
    AssertEqual(Span("123").ParseInt(), 123);
    AssertEqual(Span("123").ParseHex(), 0x123u);
    AssertEqual(Span("12A").ParseInt(), 12);
    AssertEqual(Span("12A").ParseInt(0, false), 0);
    AssertEqual(Span("12A").ParseHex(), 0x12Au);
    AssertEqual(Span("12G").ParseHex(), 0x12u);
    AssertEqual(Span("12G ").ParseHex(0, false), 0u);
    AssertEqual(Span("42 ").ParseHex(0, false), 0x42u);
    AssertEqual(Span("123").ParseIntBase(8), 0123);
    AssertEqual(Span("0x123").ParseInt(), 0x123);
    AssertEqual(Span("0b10101").ParseInt(), 0b10101);
}

TEST_CASE("05 Integer decoding")
{
    Span abcd1234 = "\xAB\xCD\x12\x34";
    Span empty;

    AssertEqual(abcd1234.ReadUInt8(), 0xAB);
    AssertEqual(abcd1234.ReadInt8(), (int8_t)0xAB);

    AssertEqual(abcd1234.ReadUIntLE16(), 0xCDAB);
    AssertEqual(abcd1234.ReadIntLE16(), (int16_t)0xCDAB);
    AssertEqual(abcd1234.ReadUIntBE16(), 0xABCD);
    AssertEqual(abcd1234.ReadIntBE16(), (int16_t)0xABCD);

    AssertEqual(abcd1234.ReadUIntLE24(), 0x12CDABu);
    AssertEqual(abcd1234.ReadIntLE24(), 0x12CDAB);
    AssertEqual(abcd1234.ReadUIntBE24(), 0xABCD12u);
    AssertEqual(abcd1234.ReadIntBE24(), (int32_t)0xFFABCD12);

    AssertEqual(abcd1234.ReadUIntLE32(), 0x3412CDABu);
    AssertEqual(abcd1234.ReadIntLE32(), 0x3412CDAB);
    AssertEqual(abcd1234.ReadUIntBE32(), 0xABCD1234u);
    AssertEqual(abcd1234.ReadIntBE32(), (int32_t)0xABCD1234);

    AssertEqual(abcd1234.ReadInt8(42), (int8_t)0xAB);
    AssertEqual(empty.ReadInt8(42), 42);
}

}
