#include <Skirnir/Async.hpp>
#include <Skirnir/Common/Arc.hpp>
#include <Skirnir/DependencyInjection/ApplicationBuilder.hpp>
#include <Skirnir/Logging/Logger.hpp>
#include <Skirnir/Logging/LoggingExtension.hpp>

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/steady_timer.hpp>
#include <asio/write.hpp>

#include <chrono>
#include <cstdio>
#include <memory>
#include <string>

class AsioEchoApp : public skr::IAsyncApplication
{
  public:
    explicit AsioEchoApp(const skr::Arc<skr::ServiceProvider>& root) :
        IAsyncApplication(root)
    {
    }

    skr::Task<> RunAsync() override
    {
        auto logger = co_await mRootServiceProvider
                          ->GetServiceAsync<skr::Logger<AsioEchoApp>>();
        asio::io_context   ctx(1);
        skr::AsioScheduler sched(ctx.get_executor());

        asio::ip::tcp::acceptor acceptor(ctx, { asio::ip::tcp::v4(), 0 });
        const auto              port = acceptor.local_endpoint().port();
        logger->LogInformation("listening on 127.0.0.1:{}", port);

        acceptor.async_accept([this, &sched, &ctx, logger](
                                  std::error_code       ec,
                                  asio::ip::tcp::socket sock) {
            if (!ec)
            {
                auto session =
                    std::make_shared<asio::ip::tcp::socket>(std::move(sock));
                (void) EchoOneSession(sched, ctx, session, logger);
            }
        });

        asio::steady_timer timer(ctx, std::chrono::milliseconds(200));
        timer.async_wait([&](std::error_code) { ctx.stop(); });
        ctx.run();
    }

  private:
    skr::Task<> EchoOneSession(skr::AsioScheduler& sched,
                               asio::io_context&   ctx,
                               std::shared_ptr<asio::ip::tcp::socket>
                                                                         sock,
                               const skr::Arc<skr::Logger<AsioEchoApp>>& logger)
    {
        char            buf[128];
        std::error_code ec;
        std::size_t     n = sock->read_some(asio::buffer(buf), ec);
        if (ec)
            co_return;
        std::string msg(buf, n);
        logger->LogInformation("recv: {}", msg);
        asio::write(*sock, asio::buffer(msg), ec);
        (void) ctx;
        (void) sched;
        co_return;
    }
};

int main()
{
    auto app = skr::ApplicationBuilder()
                   .WithExtension<skr::LoggingExtension>()
                   .BuildAsync<AsioEchoApp>();
    return skr::AsyncApplicationHost::Run(*app);
}
