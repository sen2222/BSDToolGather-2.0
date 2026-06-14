#include "ringbuffer.h"
#include <cstring>

#define RING_MIN(a, b) ((a) < (b) ? (a) : (b))
#define RING_MAX(a, b) ((a) > (b) ? (a) : (b))

RingBuffer::RingBuffer(uint32_t size)
{
    this->size = RING_MAX(size, 2);
    this->buffer = new uint8_t[this->size]();
    this->readIndex = 0;
    this->writeIndex = 0;
}

RingBuffer::~RingBuffer()
{
    QMutexLocker locker(&mutex);
    if (buffer)
    {
        delete[] buffer;
        buffer = nullptr;
    }
    size = 0;
    readIndex = 0;
    writeIndex = 0;
    notEmpty.wakeAll();
    notFull.wakeAll();
}

uint32_t RingBuffer::write(uint8_t *data, uint32_t len)
{
    if (data == nullptr || len == 0)
        return 0;

    QMutexLocker locker(&mutex);
    uint32_t writable = availableWrite();
    if (writable == 0)
        return 0;

    uint32_t realWrite = RING_MIN(len, writable);
    if (writeIndex + realWrite <= size)
    {
        memcpy(buffer + writeIndex, data, realWrite);
        writeIndex += realWrite;
    }
    else
    {
        uint32_t firstLen = size - writeIndex;
        memcpy(buffer + writeIndex, data, firstLen);
        uint32_t secondLen = realWrite - firstLen;
        memcpy(buffer, data + firstLen, secondLen);
        writeIndex = secondLen;
    }

    notEmpty.wakeOne();
    return realWrite;
}

uint32_t RingBuffer::read(uint8_t *data, uint32_t len)
{
    if (data == nullptr || len == 0)
        return 0;

    QMutexLocker locker(&mutex);
    uint32_t readable = availableRead();
    if (readable == 0)
        return 0;

    uint32_t realRead = RING_MIN(len, readable);
    if (readIndex + realRead <= size)
    {
        memcpy(data, buffer + readIndex, realRead);
        readIndex += realRead;
    }
    else
    {
        uint32_t firstLen = size - readIndex;
        memcpy(data, buffer + readIndex, firstLen);
        uint32_t secondLen = realRead - firstLen;
        memcpy(data + firstLen, buffer, secondLen);
        readIndex = secondLen;
    }

    notFull.wakeOne();
    return realRead;
}

uint32_t RingBuffer::writeWait(uint8_t *data, uint32_t len, int timeoutMs)
{
    if (data == nullptr || len == 0 || len > size)
        return 0;

    QMutexLocker locker(&mutex);
    while (availableWrite() < len)
    {
        if (!notFull.wait(&mutex, timeoutMs))
            return 0;
    }

    if (writeIndex + len <= size)
    {
        memcpy(buffer + writeIndex, data, len);
        writeIndex += len;
    }
    else
    {
        uint32_t firstLen = size - writeIndex;
        memcpy(buffer + writeIndex, data, firstLen);
        uint32_t secondLen = len - firstLen;
        memcpy(buffer, data + firstLen, secondLen);
        writeIndex = secondLen;
    }

    notEmpty.wakeOne();
    return len;
}

uint32_t RingBuffer::readWait(uint8_t *data, uint32_t len, int timeoutMs)
{
    if (data == nullptr || len == 0 || len > size)
        return 0;

    QMutexLocker locker(&mutex);
    while (availableRead() < len)
    {
        if (!notEmpty.wait(&mutex, timeoutMs))
            return 0;
    }

    if (readIndex + len <= size)
    {
        memcpy(data, buffer + readIndex, len);
        readIndex += len;
    }
    else
    {
        uint32_t firstLen = size - readIndex;
        memcpy(data, buffer + readIndex, firstLen);
        uint32_t secondLen = len - firstLen;
        memcpy(data + firstLen, buffer, secondLen);
        readIndex = secondLen;
    }

    notFull.wakeOne();
    return len;
}

// ==============================
// 内部使用 → 已私有化
// ==============================
uint32_t RingBuffer::availableRead()
{
    return (writeIndex - readIndex + size) % size;
}

uint32_t RingBuffer::availableWrite()
{
    return (readIndex - writeIndex - 1 + size) % size;
}

int RingBuffer::isEmpty()
{
    return readIndex == writeIndex;
}

int RingBuffer::isFull()
{
    return (writeIndex + 1) % size == readIndex;
}

uint32_t RingBuffer::getSize()
{
    QMutexLocker locker(&mutex);
    return size;
}

uint32_t RingBuffer::getDataSize()
{
    QMutexLocker locker(&mutex);
    return availableRead();
}

uint32_t RingBuffer::getCapacity()
{
    QMutexLocker locker(&mutex);
    return size;
}

void RingBuffer::clear()
{
    QMutexLocker locker(&mutex);
    readIndex = 0;
    writeIndex = 0;
    notFull.wakeAll();
    notEmpty.wakeAll();
}