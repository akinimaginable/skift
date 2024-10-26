#include <karm-sys/info.h>
#include <karm-sys/launch.h>

#include "model.h"

namespace Hideo::Files {

void reduce(State &s, Action a) {
    a.visit(Visitor{
        [&](GoRoot) {
            reduce(s, GoTo{"file:/"_url});
        },
        [&](GoBack) {
            if (s.canGoBack())
                s.currentIndex--;
        },
        [&](GoForward) {
            if (s.canGoForward())
                s.currentIndex++;
        },
        [&](GoParent p) {
            auto parent = s.currentUrl().parent(p.index);
            reduce(s, GoTo{parent});
        },
        [&](Navigate navigate) {
            auto dest = s.currentUrl();
            dest.append(navigate.item);

            auto stat = Sys::stat(dest).unwrap();
            if (stat.type == Sys::Type::FILE) {
                (void)Sys::launch(Mime::Uti::PUBLIC_PREVIEW, dest);
            } else {
                reduce(s, GoTo{dest});
            }
        },
        [&](GoTo gotTo) {
            if (s.currentUrl() == gotTo.url)
                return;

            s.history.trunc(s.currentIndex + 1);
            s.history.pushBack(gotTo.url);
            s.currentIndex++;
        },
        [&](Refresh) {
        },
        [&](AddBookmark) {
        },
    });
}

} // namespace Hideo::Files
