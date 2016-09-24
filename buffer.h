#ifndef BUFFER_H
#define BUFFER_H

#include <algorithm>
#include <vector>
#include <string>

#include <assert.h>
#include <string.h>
#include <unistd.h>

using namespace std;

/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size


class Buffer
{
    public:
        static const size_t kCheapPrepend = 8;
        static const size_t kInitialSize = 16777216;
        
        explicit Buffer(size_t initialSize = kInitialSize)
            : buffer_(kCheapPrepend + initialSize),
              readerIndex_(kCheapPrepend),
              writerIndex_(kCheapPrepend)
              
        {
            assert(readableBytes() == 0);
            assert(writableBytes() == initialSize);
            assert(prependableBytes() == kCheapPrepend);
        }

        size_t readableBytes() const
        { return writerIndex_ - readerIndex_; }
        
        size_t writableBytes() const
        { return buffer_.size() - writerIndex_; }

        size_t prependableBytes() const
        { return readerIndex_; }

        const char* peek() const
        { return begin() + readerIndex_; }


        // retrieve returns void, to prevent
        // string str(retrieve(readableBytes()), readableBytes());
        // the evaluation of two functions are unspecified
        void retrieve(size_t len)
        {
            assert(len <= readableBytes());
            if (len < readableBytes())
            {
                readerIndex_ += len;
            }
            else
            {
                retrieveAll();
            }
        }

        void retrieveUntil(const char* end)
        {
            assert(peek() <= end);
            assert(end <= beginWrite());
            retrieve(end - peek());
        }

        void retrieveAll()
        {
            readerIndex_ = kCheapPrepend;
            writerIndex_ = kCheapPrepend;
        }

        string retrieveAllAsString()
        {
            return retrieveAsString(readableBytes());;
        }

        string retrieveAsString(size_t len)
        {
            assert(len <= readableBytes());
            string result(peek(), len);
            retrieve(len);
            return result;
        }

        void append(char* data, size_t len)
        {
            ensureWritableBytes(len);
            copy(data, data+len, beginWrite());
            hasWritten(len);
        }

        void ensureWritableBytes(size_t len)
        {
            if (writableBytes() < len)
            {
                makeSpace(len);
            }
            assert(writableBytes() >= len);
        }

        char* beginWrite()
        { return begin() + writerIndex_; }

        const char* beginWrite() const
        { return begin() + writerIndex_; }

        void hasWritten(size_t len)
        {
            assert(len <= writableBytes());
            writerIndex_ += len;
        }

        void unwrite(size_t len)
        {
            assert(len <= readableBytes());
            writerIndex_ -= len;
        }

        void prepend(const void* data, size_t len)
        {
            assert(len <= prependableBytes());
            readerIndex_ -= len;
            const char* d = static_cast<const char*>(data);
            std::copy(d, d+len, begin()+readerIndex_);
        }


        size_t internalCapacity() const
        {
            return buffer_.capacity();
        }

        /// Read data directly into buffer.
        ///
        /// It may implement with readv(2)
        /// @return result of read(2), @c errno is saved
        ssize_t readFd(int fd, int* savedErrno);

        char* begin()
        { return &*buffer_.begin(); }

        const char* begin() const
        { return &*buffer_.begin(); }

    private:

        void makeSpace(size_t len)
        {
            if (writableBytes() + prependableBytes() < len + kCheapPrepend)
            {
                // FIXME: move readable data
                buffer_.resize(writerIndex_+len);
            }
            else
            {
                // move readable data to the front, make space inside buffer
                assert(kCheapPrepend < readerIndex_);
                size_t readable = readableBytes();
                copy(begin()+readerIndex_, begin()+writerIndex_, begin()+kCheapPrepend);
                readerIndex_ = kCheapPrepend;
                writerIndex_ = readerIndex_ + readable;
                assert(readable == readableBytes());
            }
        }

    public:
        vector<char> buffer_;
        size_t readerIndex_;
        size_t writerIndex_;

};


#endif

