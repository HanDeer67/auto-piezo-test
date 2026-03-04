#pragma once
#include <QByteArray>
#include <QMutex>
#include <QWaitCondition>

class RingBuffer {
public:
    explicit RingBuffer(size_t capacity = 1024 * 1024); // 默认1MB容量
    ~RingBuffer();

    // 写入数据（串口线程调用）
    bool push(const QByteArray &data);

    // 读取指定长度数据（处理线程调用）
    QByteArray pop(size_t maxSize);

    // 获取可读数据量
    size_t available() const;

    // 添加isEmpty方法声明
    bool isEmpty() const;

private:
    uchar* m_buffer = nullptr;   // 存储空间
    size_t m_capacity = 0;       // 缓冲区总容量
    size_t m_readPos = 0;        // 读指针位置
    size_t m_writePos = 0;       // 写指针位置
    mutable QMutex m_mutex;      // 互斥锁保证线程安全
    QWaitCondition m_notEmpty;   // 数据可读条件
};
