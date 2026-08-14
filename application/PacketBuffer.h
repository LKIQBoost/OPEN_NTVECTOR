struct BufferSlice {
    uint8_t* data;
    size_t size;

    BufferSlice() : data(nullptr), size(0) {}
    BufferSlice(uint8_t* d, size_t s) : data(d), size(s) {}

    bool empty() const { return size == 0 || data == nullptr; }
    std::vector<uint8_t> toVector() const {
        return std::vector<uint8_t>(data, data + size);
    }
};

class PacketBuffer {
public:
    // 安全常量：缓冲区最大允许扩容到 256MB
    static constexpr size_t MAX_SAFE_BUFFER_SIZE = 256ULL * 1024 * 1024;

private:
    std::vector<uint8_t> buffer;
    size_t writePos = 0;

    std::mutex mtx;
    std::condition_variable cv;
    bool isSending = false;
    bool shutdownFlag = false;

    // 在安全常量范围内动态扩容
    void ensureCapacity(size_t required) {
        size_t currentCap = buffer.capacity();
        if (required <= currentCap) return;

        size_t newCap = currentCap * 2;
        if (newCap < required) newCap = required;
        if (newCap > MAX_SAFE_BUFFER_SIZE) newCap = MAX_SAFE_BUFFER_SIZE;

        std::vector<uint8_t> newBuf(newCap);
        if (writePos > 0) {
            std::memcpy(newBuf.data(), buffer.data(), writePos);
        }
        buffer = std::move(newBuf);
    }

public:
    explicit PacketBuffer(size_t initSize = 4096) {
        if (initSize > MAX_SAFE_BUFFER_SIZE) {
            initSize = MAX_SAFE_BUFFER_SIZE;
        }
        buffer.resize(initSize);
        writePos = 0;
    }

    // ======== 写入 ========

    // 追加数据，自动添加 varint 长度头部
    //
    // 流程：
    //   1. 单包 > 256MB → return false
    //   2. 正在发送 或 已用+新包 > 256MB → 阻塞等 finishSending 清空
    //   3. 物理容量不够 → 动态扩容
    //   4. 写入 varint + data
    bool appendData(const uint8_t* data, size_t dataSize) {
        size_t varintLen = BinaryWriter::calculateVarintSizeFast(dataSize);
        size_t packetTotal = varintLen + dataSize;

        // 单包超过 256MB，不可能装下
        if (packetTotal > MAX_SAFE_BUFFER_SIZE) {
            return false;
        }

        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [&] {
            if (shutdownFlag) return true;
            // 正在发送：必须等，因为发送方持有裸指针，扩容会导致悬空
            if (isSending) return false;
            // 已用 + 新包超过 256MB：等发送方取走数据后清空
            return (writePos + packetTotal) <= MAX_SAFE_BUFFER_SIZE;
            });

        if (shutdownFlag) return false;

        size_t required = writePos + packetTotal;

        // 物理容量不够，动态扩容（不超过 256MB）
        if (required > buffer.capacity()) {
            ensureCapacity(required);
        }

        // 写入 varint 长度头部
        uint8_t* ptr = buffer.data() + writePos;
        size_t written = BinaryWriter::uint_to_varint(dataSize, ptr);
        writePos += written;

        // 写入数据体
        if (dataSize > 0) {
            std::memcpy(buffer.data() + writePos, data, dataSize);
            writePos += dataSize;
        }

        return true;
    }

    bool appendData(const std::vector<uint8_t>& data) {
        return appendData(data.data(), data.size());
    }

    bool appendData(std::vector<uint8_t>&& data) {
        return appendData(data.data(), data.size());
    }

    // 带超时的写入
    template<typename Rep, typename Period>
    bool appendDataWithTimeout(const uint8_t* data, size_t dataSize,
        const std::chrono::duration<Rep, Period>& timeout) {
        size_t varintLen = BinaryWriter::calculateVarintSizeFast(dataSize);
        size_t packetTotal = varintLen + dataSize;

        if (packetTotal > MAX_SAFE_BUFFER_SIZE) {
            return false;
        }

        std::unique_lock<std::mutex> lock(mtx);

        bool ready = cv.wait_for(lock, timeout, [&] {
            if (shutdownFlag) return true;
            if (isSending) return false;
            return (writePos + packetTotal) <= MAX_SAFE_BUFFER_SIZE;
            });

        if (!ready || shutdownFlag) return false;

        size_t required = writePos + packetTotal;
        if (required > buffer.capacity()) {
            ensureCapacity(required);
        }

        uint8_t* ptr = buffer.data() + writePos;
        size_t written = BinaryWriter::uint_to_varint(dataSize, ptr);
        writePos += written;

        if (dataSize > 0) {
            std::memcpy(buffer.data() + writePos, data, dataSize);
            writePos += dataSize;
        }

        return true;
    }
    bool hasDataInCurrentBuffer() const {
        return hasData();
    }

    BufferSlice swapAndGetBuffer() {
        return getBuffer();
    }
    // ======== 读取/发送 ========

    // 获取当前缓冲区数据用于发送
    // 调用后缓冲区进入发送状态，appendData 会阻塞
    // 发送完成后必须调用 finishSending()
    BufferSlice getBuffer() {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [this] {
            return shutdownFlag || !isSending;
            });

        if (shutdownFlag) return BufferSlice();

        isSending = true;
        return BufferSlice(buffer.data(), writePos);
    }

    // 发送完成，清空缓冲区，唤醒阻塞的写入线程
    void finishSending() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            writePos = 0;
            isSending = false;
        }
        cv.notify_all();
    }

    // ======== 控制 ========

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            shutdownFlag = true;
        }
        cv.notify_all();
    }

    bool isShutdown() const {
        return shutdownFlag;
    }

    // ======== 查询 ========

    size_t currentSize() const {
        return writePos;
    }

    size_t currentCapacity() const {
        return buffer.capacity();
    }

    bool hasData() const {
        return writePos > 0;
    }

    double usage() const {
        return static_cast<double>(writePos) / static_cast<double>(MAX_SAFE_BUFFER_SIZE);
    }

    // ======== 维护 ========

    void clear() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            writePos = 0;
        }
        cv.notify_all();
    }

    void reset() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            writePos = 0;
        }
        cv.notify_all();
    }

    struct BufferStats {
        size_t size;          // 已用字节数
        size_t capacity;      // 当前物理容量
        bool sending;         // 是否正在发送
        bool shutdown;        // 是否已关闭
    };

    BufferStats getStats() const {
        BufferStats stats;
        stats.size = writePos;
        stats.capacity = buffer.capacity();
        stats.sending = isSending;
        stats.shutdown = shutdownFlag;
        return stats;
    }

    ~PacketBuffer() {
        shutdown();
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !isSending; });
    }
};