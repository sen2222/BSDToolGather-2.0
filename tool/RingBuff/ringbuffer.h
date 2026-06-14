#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <QMutex>
#include <QWaitCondition>
#include <cstdint>

class RingBuffer
{
public:
    explicit RingBuffer(uint32_t size);
    ~RingBuffer();

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    uint32_t write(uint8_t *data, uint32_t len);
    uint32_t read(uint8_t *data, uint32_t len);

    uint32_t writeWait(uint8_t *data, uint32_t len, int timeoutMs = -1);
    uint32_t readWait(uint8_t *data, uint32_t len, int timeoutMs = -1);

    uint32_t getDataSize(); 
    uint32_t getCapacity(); 
    void clear();

private:
    uint32_t availableRead();
    uint32_t availableWrite();
    int isEmpty();
    int isFull();
    uint32_t getSize(); 

private:
    uint8_t*     buffer = nullptr;
    uint32_t     size = 0;
    uint32_t     readIndex = 0;
    uint32_t     writeIndex = 0;
    QMutex       mutex;
    QWaitCondition notEmpty;
    QWaitCondition notFull;
};

#endif // RINGBUFFER_H