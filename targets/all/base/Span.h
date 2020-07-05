/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/Span.h
 *
 * Helper for working with a contiguous block of memory
 */

#pragma once

#include <base/base.h>

#include <base/ResultPair.h>

#include <algorithm>

class Buffer;

/*!
 * A class representing a contiguous range of bytes in memory
 *
 * \attention
 * ARM calling convention returns all structures on stack. This causes quite a
 * lot of unnecessary code to be generated, hence the return values of all
 * non-inline functions actually return a res_pair_t (a platform-optimized
 * way of returning two register-size values) and have inline wrappers that
 * call them.
 */
class Span
{
    union
    {
        struct
        {
            const char* p;
            size_t len;
        };
        res_pair_t pair;
    };

public:
    //! Constructs an empty span
    constexpr Span() : p(NULL), len(0) {}
    //! Constructs a Span covering a range of memory determined by start and length
    constexpr Span(const void* p, size_t len) : p((const char*)p), len(len) {}
    //! Constructs a Span covering a range of memory determined by start (inclusive) and end (exclusive)
    constexpr Span(const void* start, const void* end) : p((const char*)start), len((const char*)end - (const char*)start) {}
    //! Constructs a Span covering a single object
    template<class T> constexpr Span(const T& value) : p((const char*)&value), len(sizeof(T)) {}
    //! Constructs a Span covering a single object
    template<class T> static Span Of(const T& value) { return Span((const char*)&value, sizeof(T)); }

    //! Constructs a Span from a Null-terminated string literal, excluding the terminator
    template<size_t n> constexpr Span(const char (&literal)[n]) : p(literal), len(n - 1)
    {
        ASSERT(literal[n - 1] == 0);
    }

    //! Constructs a Span from a @ref res_pair_t - used to force passing of return values via registers
    ALWAYS_INLINE constexpr Span(res_pair_t pair) : pair(pair) {}
    //! Converts a Span to a @ref res_pair_t - used to force passing of return values via registers
    ALWAYS_INLINE constexpr operator res_pair_t() const { return pair; }

    //! Gets the pointer to the beginning of the Span
    ALWAYS_INLINE const char* Pointer() const { return p; }
    //! Gets the pointer to the beginning of the Span
    template<class T> ALWAYS_INLINE const T* Pointer() const { return (const T*)p; }
    //! Gets the reference to the element of the Span with the specified index
    template<class T> ALWAYS_INLINE const T& Element(size_t index = 0) const { return ((const T*)p)[index]; }
    //! Gets the length of the Span
    ALWAYS_INLINE size_t Length() const { return len; }

    //! Gets the pointer to the beginning of the Span
    ALWAYS_INLINE constexpr operator const char*() const { return p; }
    //! Span is convertible to any constant pointer
    template<typename T> ALWAYS_INLINE constexpr operator const T*() const { return (const T*)p; }

    //! C++ iterator interface
    ALWAYS_INLINE constexpr const char* begin() const { return p; }
    //! C++ iterator interface
    ALWAYS_INLINE constexpr const char* end() const { return p + len; }

    //! Checks if the content of the Span is equal to another Span
    ALWAYS_INLINE bool operator ==(Span other) const { return len == other.len && (p == other.p || !memcmp(p, other.p, len)); }
    //! Checks if the content of the Span differs from another Span
    ALWAYS_INLINE bool operator !=(Span other) const { return len != other.len || (p != other.p && memcmp(p, other.p, len)); }

    //! Checks if the other span is contained entirely in the current Span
    ALWAYS_INLINE bool Contains(Span other) const { return p <= other.p && p + len >= other.p + other.len; }

    //! Explicit indexer implementation to resolve possible ambiguity
    ALWAYS_INLINE constexpr const char operator[](int index) const { return p[index]; }
    //! Explicit indexer implementation to resolve possible ambiguity
    ALWAYS_INLINE constexpr const char operator[](unsigned index) const { return p[index]; }
    //! Explicit indexer implementation to resolve possible ambiguity
    ALWAYS_INLINE constexpr const char operator[](long index) const { return p[index]; }
    //! Explicit indexer implementation to resolve possible ambiguity
    ALWAYS_INLINE constexpr const char operator[](unsigned long index) const { return p[index]; }

    //! Copies the content of the Span to another memory location, returns a Span representing the copy
    Span CopyTo(void* p) const { memmove(p, this->p, len); return Span(p, len); }
    //! Copies the content of the Span to another memory location, limiting the maximum size. Returns a Span representing the copy
    Span CopyTo(void* p, size_t max) const { auto len = std::min(max, this->len); memcpy(p, this->p, len); return Span(p, len); }
    //! Copies the content of the Span to an array of arbitrary type. Returns a Span representing the copy
    template<typename T, size_t n> ALWAYS_INLINE Span CopyTo(T (&buffer)[n]) const { return CopyTo(buffer, sizeof(T) * n); }
    //! Copies the content of the Span to a Buffer. Returns the part of the Buffer representing the copy
    ALWAYS_INLINE Buffer CopyTo(Buffer buf) const;

    //! Compares the span byte-by-byte with another memory location
    int CompareTo(const void* p) const { return memcmp(this->p, p, len); }
    //! Compares the span with byte-by-byte with another Span
    int CompareTo(Span other) const { auto res = memcmp(this->p, p, std::min(len, other.len)); return res ? res : len - other.len; }

    //! Returns a new Span consisting of up to n bytes from the start of the original Span
    ALWAYS_INLINE Span Left(size_t n) const { return Span(p, n < len ? n : len); }
    //! Returns a new Span consisting of up to n bytes from the end of the original Span
    ALWAYS_INLINE Span Right(size_t n) const { return n < len ? Span(p + len - n, n) : *this; }
    //! Returns a new Span consisting of up to length bytes, starting at the specified position in the original Span
    ALWAYS_INLINE Span Sub(size_t start, size_t length) const { return Sub(*this, start, length); }
    //! Returns a new Span consisting of the original Span with up to n bytes removed from the start
    ALWAYS_INLINE Span RemoveLeft(size_t n) const { return n < len ? Span(p + n, len - n) : Span(); }
    //! Returns a new Span consisting of the original Span with up to n bytes removed from the end
    ALWAYS_INLINE Span RemoveRight(size_t n) const { return n < len ? Span(p, len - n) : Span(); }

    //! Returns a new Span consisting of the part of the orignal Span to the left of the slicing point
    /*! @p n can be negative, in which case it is counted from the end of the original Span */
    ALWAYS_INLINE Span SliceLeft(int n) const { return Slice(*this, 0, n); }
    //! Returns a new Span consisting of the part of the orignal Span to the left of the slicing point
    /*! @p n can be negative, in which case it is counted from the end of the original Span */
    ALWAYS_INLINE Span SliceRight(int n) const { return Slice(*this, n, std::numeric_limits<int>::max()); }
    //! Returns a new Span consisting of the part of the orignal Span between the two slicing points
    /*! Both @p start and @p end can be negative, in which case they are counted
     *  from the end of the original Span */
    ALWAYS_INLINE Span Slice(int start, int end) const { return Slice(*this, start, end); }

    //! Returns a new Span consisting of up to n bytes from the start of the original Span. The bytes are removed from the target Span
    ALWAYS_INLINE Span ConsumeLeft(size_t n) { auto res = Left(n); p += res.len; len -= res.len; return res; }
    //! Returns a new Span consisting of up to n bytes from the end of the original Span. The bytes are removed from the target Span
    ALWAYS_INLINE Span ConsumeRight(size_t n) { auto res = Right(n); len -= res.len; return res; }

    //! Splits off the part of the span up to the specified @p separator
    /*!
     * The target Span is modified in place and will contain the part remaining
     * after the separator. If the separator is not found, the original Span
     * is left unmodified.
     *
     * @return The part of the original Span to the left of the separator.
     * An empty span, if the separator is not found in the original Span.
     */
    ALWAYS_INLINE Span Split(char separator) { return Split(*this, separator); }

    //! Splits the span into a part before and after the separator
    /*!
     * If @p separator is found in the Span, @p key will be set to the part to the
     * left of the separator and @p value will be the part to the right.
     * If @p separator is not found, @p value will contain the entire original
     * Span and @p key will be an empty span.
     *
     * @return @c true if the separator was found in the Span, @c false otherwise
     */
    ALWAYS_INLINE bool Split(char separator, Span& key, Span& value) const { value = *this; return (key = Split(value, separator)).p != NULL; }

    //! Consumes part of the Span up to the specified @p separator or the end of the Span
    /*!
     * The target Span is modified in place and will contain the part remaining
     * after the separator. If the separator is not found, the result is the entire
     * Span and the target Span is reset (set to empty).
     *
     * This function is useful for processing spans containing multiple parts
     * separated by a given character, such as configuration strings or paths
     *
     * @code
     * Span config = "a=3;b=4;c=5";
     * Span part;
     * while ((part = config.Consume(';'))
     * {
     *   Span key, value;
     *   if (part.Split('=', key, value))
     *   {
     *   }
     * }
     * @endcode
     *
     * @return The part of the original Span to the left of the separator.
     * The entire original target Span is returned if the separator is not found.
     */
    ALWAYS_INLINE Span Consume(char separator) { return Consume(*this, separator); }

    //! Parses the content of the span as an ASCII integer
    /*!
     * Whitespace at the start and end of the Span is skipped automatically. If there are
     * no valid digits found, or if @p stopAtInvalid is @c false and an invalid
     * character is encountered in the number, @defVal is returned.
     *
     * @return The parsed integer, or @defVal when parsing fails
     */
    ALWAYS_INLINE int ParseInt(int defVal = 0, bool stopAtInvalid = true) const { return ParseInt(0, defVal, stopAtInvalid); }
    //! Parses the content of the span as an ASCII integer with the specified @base
    /*!
     * Whitespace at the start and end of the Span is skipped automatically. If there are
     * no valid digits found, or if @p stopAtInvalid is @c false and an invalid
     * character is encountered in the number, @defVal is returned.
     *
     * @return The parsed integer, or @defVal when parsing fails
     */
    ALWAYS_INLINE int ParseIntBase(unsigned base, int defVal = 0, bool stopAtInvalid = true) const { return ParseInt(base, defVal, stopAtInvalid); }
    //! Parses the content of the span as an ASCII hexadecimal integer (Base 16)
    /*!
     * Whitespace at the start and end of the Span is skipped automatically. If there are
     * no valid digits found, or if @p stopAtInvalid is @c false and an invalid
     * character is encountered in the number, @defVal is returned.
     *
     * @return The parsed integer, or @defVal when parsing fails
     */
    ALWAYS_INLINE unsigned ParseHex(unsigned defVal = 0, bool stopAtInvalid = true) const { return ParseInt(16, defVal, stopAtInvalid); }

    //! Decodes the first byte of the Span as a single byte unsigned integer
    ALWAYS_INLINE uint8_t ReadUInt8(uint8_t defVal = 0) const { return ReadInt(1, defVal); }
    //! Decodes the first byte of the Span as a single byte signed integer
    ALWAYS_INLINE int8_t ReadInt8(int8_t defVal = 0) const { return ReadInt(0x11, defVal); }

    //! Decodes the content of the Span (up to 2 bytes) as a little-endian unsigned integer
    ALWAYS_INLINE uint16_t ReadUIntLE16(uint16_t defVal = 0) const { return ReadInt(2, defVal); }
    //! Decodes the content of the Span (up to 2 bytes) as a little-endian signed integer
    ALWAYS_INLINE int16_t ReadIntLE16(int16_t defVal = 0) const { return ReadInt(0x12, defVal); }
    //! Decodes the content of the Span (up to 2 bytes) as a big-endian unsigned integer
    ALWAYS_INLINE uint16_t ReadUIntBE16(uint16_t defVal = 0) const { return ReadInt(0x22, defVal); }
    //! Decodes the content of the Span (up to 2 bytes) as a big-endian signed integer
    ALWAYS_INLINE int16_t ReadIntBE16(int16_t defVal = 0) const { return ReadInt(0x32, defVal); }

    //! Decodes the content of the Span (up to 3 bytes) as a little-endian unsigned integer
    ALWAYS_INLINE uint32_t ReadUIntLE24(uint32_t defVal = 0) const { return ReadInt(3, defVal); }
    //! Decodes the content of the Span (up to 3 bytes) as a little-endian signed integer
    ALWAYS_INLINE int32_t ReadIntLE24(int32_t defVal = 0) const { return ReadInt(0x13, defVal); }
    //! Decodes the content of the Span (up to 3 bytes) as a big-endian unsigned integer
    ALWAYS_INLINE uint32_t ReadUIntBE24(uint32_t defVal = 0) const { return ReadInt(0x23, defVal); }
    //! Decodes the content of the Span (up to 3 bytes) as a big-endian signed integer
    ALWAYS_INLINE int32_t ReadIntBE24(int32_t defVal = 0) const { return ReadInt(0x33, defVal); }

    //! Decodes the content of the Span (up to 4 bytes) as a little-endian unsigned integer
    ALWAYS_INLINE uint32_t ReadUIntLE32(uint32_t defVal = 0) const { return ReadInt(4, defVal); }
    //! Decodes the content of the Span (up to 4 bytes) as a little-endian signed integer
    ALWAYS_INLINE int32_t ReadIntLE32(int32_t defVal = 0) const { return ReadInt(0x14, defVal); }
    //! Decodes the content of the Span (up to 4 bytes) as a big-endian unsigned integer
    ALWAYS_INLINE uint32_t ReadUIntBE32(uint32_t defVal = 0) const { return ReadInt(0x24, defVal); }
    //! Decodes the content of the Span (up to 4 bytes) as a big-endian signed integer
    ALWAYS_INLINE int32_t ReadIntBE32(int32_t defVal = 0) const { return ReadInt(0x34, defVal); }

private:
    static RES_PAIR_DECL(Sub, Span s, size_t start, size_t len);
    static RES_PAIR_DECL(Slice, Span s, int start, int end);

    static RES_PAIR_DECL(Split, Span& s, char separator);
    static RES_PAIR_DECL(Consume, Span& s, char separator);

    ALWAYS_INLINE int ParseInt(int base, int defVal, bool stopAtInvalid) const { return ParseInt(*this, stopAtInvalid ? base : base ? -base : -1, defVal); }
    static int ParseInt(Span s, int baseStopAtInvalid, int defVal);
    ALWAYS_INLINE int ReadInt(unsigned lenSignRev, uint32_t defVal) const { return defVal && !Length() ? defVal : ReadInt(*this, lenSignRev); }
    static int ReadInt(Span s, unsigned lenSignRev);

    friend class Buffer;
};

class Buffer : public Span
{
public:
    //! Constructs an empty (invalid) buffer
    constexpr Buffer() {}
    //! Constructs a Buffer covering a range of memory determined by start and length
    constexpr Buffer(void* p, size_t len) : Span(p, len) {}
    //! Constructs a Buffer covering a range of memory determined by start (inclusive) and end (exclusive)
    constexpr Buffer(void* start, void* end) : Span(start, end) {}
    //! Constructs a Buffer covering a single object
    template<class T> constexpr Buffer(T& value) : Span((char*)&value, sizeof(T)) {}

    //! Constructs a Buffer from a @ref res_pair_t - used to force passing of return values via registers
    ALWAYS_INLINE constexpr Buffer(res_pair_t pair) : Span(pair) {}

    //! Gets the pointer to the beginning of the Buffer
    constexpr char* Pointer() const { return (char*)p; }
    //! Gets the pointer to the beginning of the Buffer
    template<class T> ALWAYS_INLINE T* Pointer() const { return (T*)p; }
    //! Gets the reference to the element of the Buffer with the specified index
    template<class T> ALWAYS_INLINE T& Element(size_t index = 0) const { return ((T*)p)[index]; }

    //! Explicit indexer implementation to resolve possible ambiguity
    ALWAYS_INLINE constexpr char& operator[](size_t index) const { return ((char*)p)[index]; }

    //! Returns a new Buffer consisting of up to n bytes from the start of the original Buffer
    ALWAYS_INLINE Buffer Left(size_t n) const { return _FromSpan(Span::Left(n)); }
    //! Returns a new Buffer consisting of up to n bytes from the end of the original Buffer
    ALWAYS_INLINE Buffer Right(size_t n) const { return _FromSpan(Span::Right(n)); }
    //! Returns a new Buffer consisting of up to length bytes, starting at the specified position in the original Buffer
    ALWAYS_INLINE Buffer Sub(size_t start, size_t length) const { return _FromSpan(Span::Sub(start, length)); }
    //! Returns a new Buffer consisting of the original Buffer with up to n bytes removed from the start
    ALWAYS_INLINE Buffer RemoveLeft(size_t n) const { return _FromSpan(Span::RemoveLeft(n)); }
    //! Returns a new Buffer consisting of the original Buffer with up to n bytes removed from the end
    ALWAYS_INLINE Buffer RemoveRight(size_t n) const { return _FromSpan(Span::RemoveRight(n)); }

    //! Fills the entire bufer with the specified value
    ALWAYS_INLINE Buffer Fill(int value) const { memset(Pointer(), value, len); return *this; }

private:
    //! This is a strictly internal function for reinterpreting a Span as a writable Buffer
    ALWAYS_INLINE static Buffer _FromSpan(Span span) { return Buffer((char*)span.p, span.len); }
};

ALWAYS_INLINE Buffer Span::CopyTo(Buffer buf) const { auto len = std::min(buf.len, this->len); memcpy(buf.Pointer(), this->p, len); return Buffer(buf.Pointer(), len); }

template<> constexpr Span::Span(const Span& value) : Span(value.pair) {}
template<> constexpr Buffer::Buffer(Buffer& value) : Span(value.pair) {}
