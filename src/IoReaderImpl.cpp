#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Connection.h"
#include "restc-cpp/Socket.h"
#include "restc-cpp/DataReader.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/IoTimer.h"

using namespace std;

namespace restc_cpp {


class IoReaderImpl : public DataReader {
public:
    using buffer_t = std::array<char, RESTC_CPP_IO_BUFFER_SIZE>;

    IoReaderImpl(const Connection::ptr_t& conn, Context& ctx,
                 const ReadConfig& cfg)
    : ctx_{ctx}, connection_{conn}, cfg_{cfg}
    {
    }

    void Finish() override {
    }


    boost::asio::const_buffers_1 ReadSome() override {
        if (auto conn = connection_.lock()) {
            auto timer = IoTimer::Create("IoReaderImpl",
                                        cfg_.msReadTimeout,
                                        conn);
            size_t bytes = 0;
            try {
                bytes = conn->GetSocket().AsyncReadSome(
                    {buffer_.data(), buffer_.size()}, ctx_.GetYield());
            } catch (const exception& ex) {
                RESTC_CPP_LOG_DEBUG_("IoReaderImpl::ReadSome: Caught exception: " << ex.what());
                throw;
            }

            RESTC_CPP_LOG_TRACE_("Read #" << bytes
                << " bytes from " << conn);

            timer->Cancel();
            return {buffer_.data(), bytes};
        }

        throw ObjectExpiredException("Connection expired");
    }

    bool IsEof() const override {
        if (auto conn = connection_.lock()) {
            return !conn->GetSocket().IsOpen();
        }
        return true;
    }

private:
    Context& ctx_;
    const std::weak_ptr<Connection> connection_;
    const ReadConfig cfg_;
    buffer_t buffer_ = {};
};



DataReader::ptr_t
DataReader::CreateIoReader(const Connection::ptr_t& conn, Context& ctx,
                           const DataReader::ReadConfig& cfg) {
    return make_unique<IoReaderImpl>(conn, ctx, cfg);
}

} // namespace


