#ifndef QTSAFEQUEUE_H
#define QTSAFEQUEUE_H

#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QList>


template<typename T>
class QtSafeQueue
{
public:
    /**
     * @brief 构造函数
     * @param maxSize 队列最大容量，必须 > 0
     */
    explicit QtSafeQueue(int maxSize)
        : m_maxSize(maxSize)
    {}

    /**
     * @brief 阻塞式入队
     * @param item 待存入数据
     * @details 队列已满时，线程一直阻塞，直到队列腾出空间
     */
    void enqueue(const T& item)
    {
        QMutexLocker locker(&m_mutex);
        // 队列满则等待
        while (m_queue.size() >= m_maxSize)
        {
            m_notFullCond.wait(&m_mutex);
        }
        m_queue.enqueue(item);
        // 有新数据，唤醒等待的出队线程
        m_notEmptyCond.wakeOne();
    }

    /**
     * @brief 带超时阻塞入队
     * @param item 待存入数据
     * @param timeout 超时时间，单位：毫秒
     * @return true=入队成功  false=队列满且等待超时，入队失败
     */
    bool enqueue(const T& item, unsigned long timeout)
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.size() >= m_maxSize)
        {
            // 等待超时直接返回失败
            if (!m_notFullCond.wait(&m_mutex, timeout))
            {
                return false;
            }
        }
        m_queue.enqueue(item);
        m_notEmptyCond.wakeOne();
        return true;
    }

    /**
     * @brief 阻塞式出队
     * @param item 接收取出的数据
     * @details 队列为空时，线程一直阻塞，直到有新数据
     * @return 固定返回 true
     */
    bool dequeue(T& item)
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.isEmpty())
        {
            m_notEmptyCond.wait(&m_mutex);
        }
        item = m_queue.dequeue();
        // 取出数据，队列有空位，唤醒等待的入队线程
        m_notFullCond.wakeOne();
        return true;
    }

    /**
     * @brief 带超时阻塞出队
     * @param item 接收取出的数据
     * @param timeout 超时时间，单位：毫秒
     * @return true=取数成功  false=队列空且等待超时
     */
    bool dequeue(T& item, unsigned long timeout)
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.isEmpty())
        {
            if (!m_notEmptyCond.wait(&m_mutex, timeout))
            {
                return false;
            }
        }
        item = m_queue.dequeue();
        m_notFullCond.wakeOne();
        return true;
    }

    /**
     * @brief 批量出队（高性能接口，高频数据优先使用）
     * @param list 接收批量数据的列表
     * @param maxCount 单次最大读取条数
     * @return 本次实际读取到的数据数量
     */
    int dequeueBatch(QList<T>& list, int maxCount)
    {
        list.clear();
        QMutexLocker locker(&m_mutex);

        while (!m_queue.isEmpty() && list.size() < maxCount)
        {
            list.append(m_queue.dequeue());
        }

        // 批量取出后释放大量空间，唤醒所有阻塞的入队线程
        if (!list.isEmpty())
        {
            m_notFullCond.wakeAll();
        }
        return list.size();
    }
    /**
     * @brief 获取当前元素数量，并一次性取出所有数据
     * @param outList 接收所有数据
     * @return 实际取出的元素个数
     */
    int takeAll(QList<T> &outList)
    {
        outList.clear();
        QMutexLocker locker(&m_mutex);

        int total = m_queue.size();
        while (!m_queue.isEmpty())
        {
            outList.append(m_queue.dequeue());
        }
        m_notFullCond.wakeAll();
        return total;
    }

    /**
     * @brief 清空队列所有数据
     */
    void clear()
    {
        QMutexLocker locker(&m_mutex);
        m_queue.clear();
        // 清空后队列全空，唤醒所有等待入队的线程
        m_notFullCond.wakeAll();
    }

    /**
     * @brief 判断队列是否为空
     * @return true=空队列  false=队列有数据
     */
    bool isEmpty() const
    {
        QMutexLocker locker(&m_mutex);
        return m_queue.isEmpty();
    }

    /**
     * @brief 获取当前队列元素个数
     * @return 当前数据条数
     */
    int size() const
    {
        QMutexLocker locker(&m_mutex);
        return m_queue.size();
    }

    /**
     * @brief 获取队列最大容量
     * @return 队列容量上限
     */
    int maxSize() const
    {
        return m_maxSize;
    }

private:
    QQueue<T>         m_queue;         ///< 底层存储容器
    mutable QMutex    m_mutex;         ///< 读写互斥锁
    QWaitCondition    m_notFullCond;   ///< 队列未满条件变量：唤醒阻塞的入队线程
    QWaitCondition    m_notEmptyCond;  ///< 队列非空条件变量：唤醒阻塞的出队线程
    int               m_maxSize;       ///< 队列最大容量
};

#endif // QTSAFEQUEUE_H