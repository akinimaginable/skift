#include <karm-sys/entry.h>
#include <karm-sys/ipc.h>

namespace Grund::Av {

Async::Task<> serv(Sys::Context &ctx) {
    Sys::Ipc ipc = Sys::Ipc::create(ctx);

    logInfo("service started");
    while (true) {
        co_trya$(ipc.recvAsync());
        logDebug("received message from system");
    }
}

} // namespace Grund::Av

Async::Task<> entryPointAsync(Sys::Context &ctx) {
    return Grund::Av::serv(ctx);
}