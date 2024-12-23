#include <karm-scene/page.h>
#include <karm-scene/stack.h>
#include <karm-sys/time.h>
#include <vaev-layout/builder.h>
#include <vaev-layout/layout.h>
#include <vaev-layout/paint.h>
#include <vaev-layout/positioned.h>
#include <vaev-markup/dom.h>
#include <vaev-style/computer.h>

#include "fetcher.h"
#include "render.h"

namespace Vaev::Driver {

static constexpr bool DEBUG_RENDER = false;

static void _collectStyle(Markup::Node const &node, Style::StyleBook &sb) {
    auto el = node.is<Markup::Element>();
    if (el and el->tagName == Html::STYLE) {
        auto text = el->textContent();
        Io::SScan textScan{text};
        auto sheet = Style::StyleSheet::parse(textScan);
        sb.add(std::move(sheet));
    } else if (el and el->tagName == Html::LINK) {
        auto rel = el->getAttribute(Html::REL_ATTR);
        if (rel == "stylesheet"s) {
            auto href = el->getAttribute(Html::HREF_ATTR);
            if (not href) {
                logWarn("link element missing href attribute");
                return;
            }

            auto url = Mime::parseUrlOrPath(*href);
            if (not url) {
                logWarn("link element href attribute is not a valid URL: {}", *href);
                return;
            }

            auto sheet = fetchStylesheet(url.take(), Style::Origin::AUTHOR);
            if (not sheet) {
                logWarn("failed to fetch stylesheet: {}", sheet);
                return;
            }

            sb.add(sheet.take());
        }
    } else {
        for (auto &child : node.children())
            _collectStyle(*child, sb);
    }
}

RenderResult render(Markup::Document const &dom, Style::Media const &media, Layout::Viewport viewport) {
    Style::StyleBook stylebook;
    stylebook.add(
        fetchStylesheet("bundle://vaev-driver/user-agent.css"_url, Style::Origin::USER_AGENT)
            .take("user agent stylesheet not available")
    );

    auto start = Sys::now();
    _collectStyle(dom, stylebook);
    auto elapsed = Sys::now() - start;
    logDebugIf(DEBUG_RENDER, "style collection time: {}", elapsed);

    start = Sys::now();

    Style::Computer computer{media, stylebook};
    Layout::Tree tree = {
        Layout::build(computer, dom),
        viewport,
    };

    elapsed = Sys::now() - start;

    logDebugIf(DEBUG_RENDER, "layout tree build time: {}", elapsed);

    start = Sys::now();

    Layout::layout(
        tree,
        tree.root,
        {
            .commit = Layout::Commit::YES,
            .knownSize = {viewport.small.width, NONE},
            .availableSpace = {viewport.small.width, 0_px},
            .containingBlock = {viewport.small.width, viewport.small.height},
        }
    );

    Layout::layoutPositioned(tree, tree.root, {viewport.small.width, viewport.small.height});

    auto sceneRoot = makeStrong<Scene::Stack>();

    elapsed = Sys::now() - start;
    logDebugIf(DEBUG_RENDER, "layout tree layout time: {}", elapsed);

    auto paintStart = Sys::now();

    Layout::paint(tree.root, *sceneRoot);
    sceneRoot->prepare();

    elapsed = Sys::now() - paintStart;
    logDebugIf(DEBUG_RENDER, "layout tree paint time: {}", elapsed);

    return {
        std::move(stylebook),
        makeStrong<Layout::Box>(std::move(tree.root)),
        sceneRoot,
    };
}

Vec<Strong<Scene::Page>> print(Markup::Document const &dom, Style::Media const &media) {
    Style::StyleBook stylebook;
    stylebook.add(
        fetchStylesheet("bundle://vaev-driver/user-agent.css"_url, Style::Origin::USER_AGENT)
            .take("user agent stylesheet not available")
    );

    auto start = Sys::now();
    _collectStyle(dom, stylebook);
    auto elapsed = Sys::now() - start;
    logDebugIf(DEBUG_RENDER, "style collection time: {}", elapsed);

    Style::Computer computer{media, stylebook};

    Layout::Viewport vp{
        .small = {
            Px{media.width},
            Px{media.height},
        },
    };

    Layout::Tree tree = {
        Layout::build(computer, dom),
        vp,
    };

    Layout::layout(
        tree,
        tree.root,
        {
            .commit = Layout::Commit::YES,
            .knownSize = {vp.small.width, NONE},
            .availableSpace = {vp.small.width, 0_px},
            .containingBlock = {vp.small.width, vp.small.height},
        }
    );

    Vec<Strong<Scene::Page>> pages;
    auto page = makeStrong<Scene::Page>(vp.small.size().cast<isize>());
    Layout::paint(tree.root, *page);
    page->prepare();
    pages.pushBack(page);

    return pages;
}

} // namespace Vaev::Driver
