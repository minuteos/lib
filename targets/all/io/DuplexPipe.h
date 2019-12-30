/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/DuplexPipe.h
 */

#pragma once

#include <base/base.h>

#include <io/PipeReader.h>
#include <io/PipeWriter.h>

namespace io
{

class DuplexPipe
{
public:
    DuplexPipe(PipeReader reader, PipeWriter writer)
        : reader(reader), writer(writer)
    {
    }

private:
    PipeReader reader;
    PipeWriter writer;

    friend class PipeReader;
    friend class PipeWriter;
};

inline constexpr PipeReader::PipeReader(const DuplexPipe& pipe)
    : pipe(pipe.reader.pipe) {}
inline constexpr PipeWriter::PipeWriter(const DuplexPipe& pipe)
    : pipe(pipe.writer.pipe) {}

}
