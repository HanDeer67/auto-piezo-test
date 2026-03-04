#include "RingBuffer.h"

RingBuffer::RingBuffer(size_t capacity) : m_capacity(capacity) {
    m_buffer = new uchar[capacity];
}

RingBuffer::~RingBuffer() {
    delete[] m_buffer;
}

bool RingBuffer::push(const QByteArray &data) {
    QMutexLocker locker(&m_mutex);

    const size_t dataSize = static_cast<size_t>(data.size());
    if (dataSize == 0) return false;

    // 计算连续写入空间（处理尾部回绕）
    const size_t spaceBeforeWrap = m_capacity - m_writePos;
    const size_t writeSize = qMin(dataSize, spaceBeforeWrap);

    // 分段写入数据
    memcpy(m_buffer + m_writePos, data.constData(), writeSize);
    if (writeSize < dataSize) {
        memcpy(m_buffer, data.constData() + writeSize, dataSize - writeSize);
    }

    // 更新写指针（环形回绕）
    m_writePos = (m_writePos + dataSize) % m_capacity;

    // 唤醒可能阻塞的读取线程
    m_notEmpty.wakeAll();
    return true;
}

QByteArray RingBuffer::pop(size_t maxSize) {
    QMutexLocker locker(&m_mutex);

    // 无数据时阻塞等待
    while (m_readPos == m_writePos) {
        m_notEmpty.wait(&m_mutex);
    }

    // 计算可读数据长度（处理回绕）
    size_t avail = (m_writePos >= m_readPos)
                 ? (m_writePos - m_readPos)
                 : (m_capacity - m_readPos + m_writePos);
    const size_t readSize = qMin(maxSize, avail);

    QByteArray result;
    result.resize(static_cast<int>(readSize));

    // 分段读取数据
    if (m_readPos + readSize <= m_capacity) {
        memcpy(result.data(), m_buffer + m_readPos, readSize);
        m_readPos = (m_readPos + readSize) % m_capacity;
    } else {
        const size_t part1 = m_capacity - m_readPos;
        memcpy(result.data(), m_buffer + m_readPos, part1);
        memcpy(result.data() + part1, m_buffer, readSize - part1);
        m_readPos = readSize - part1;
    }
    return result;
}

size_t RingBuffer::available() const {
    QMutexLocker locker(&m_mutex);
    return (m_writePos >= m_readPos)
        ? (m_writePos - m_readPos)
        : (m_capacity - m_readPos + m_writePos);
}

bool RingBuffer::isEmpty() const
{
    return m_writePos == m_readPos;  // 读写指针重合时为空
}
