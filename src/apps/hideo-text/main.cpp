#include <hideo-files/widgets.h>
#include <karm-io/funcs.h>
#include <karm-kira/error-page.h>
#include <karm-kira/scaffold.h>
#include <karm-sys/entry.h>
#include <karm-sys/file.h>
#include <karm-text/edit.h>
#include <karm-ui/app.h>
#include <karm-ui/dialog.h>
#include <karm-ui/layout.h>
#include <karm-ui/scroll.h>
#include <mdi/alert-decagram.h>
#include <mdi/circle-small.h>
#include <mdi/content-save-plus.h>
#include <mdi/content-save.h>
#include <mdi/file.h>
#include <mdi/folder.h>
#include <mdi/redo.h>
#include <mdi/text.h>
#include <mdi/undo.h>

namespace Hideo::Text {

struct State {
    Opt<Mime::Url> url;
    Opt<Error> error;
    Strong<Karm::Text::Model> text;
};

struct New {
};

struct Save {
    bool prompt = false;
};

using Action = Union<Karm::Text::Action, New, Save>;

void reduce(State &s, Action a) {
    a.visit(::Visitor{
        [&](Karm::Text::Action &t) {
            s.text->reduce(t);
        },
        [&](New &) {
            s.url = NONE;
            s.error = NONE;
            s.text = makeStrong<Karm::Text::Model>();
        },
        [&](Save &) {

        },
    });
}

using Model = Ui::Model<State, Action, reduce>;

Ui::Child editor(Strong<Karm::Text::Model> text) {
    return Ui::input(text, [](Ui::Node &n, Action a) {
               Model::bubble(n, a);
           }) |
           Ui::insets(16) | Ui::vscroll() | Ui::grow();
}

Ui::Child app(Opt<Mime::Url> url, Res<String> str) {
    auto text = makeStrong<Karm::Text::Model>();
    Opt<Error> error = NONE;

    if (str) {
        text->load(str.unwrap());
    } else {
        error = str.none();
    }

    return Ui::reducer<Model>(
        State{
            url,
            error,
            text,
        },
        [](State const &s) {
            return Kr::scaffold({
                .icon = Mdi::TEXT,
                .title = "Text"s,
                .startTools = slots$(
                    Ui::button(Model::bind<New>(), Ui::ButtonStyle::subtle(), Mdi::FILE),
                    Ui::button(
                        [](auto &n) {
                            Ui::showDialog(n, Files::openFileDialog());
                        },
                        Ui::ButtonStyle::subtle(), Mdi::FOLDER
                    ),
                    Ui::button(
                        Model::bindIf(s.text->dirty(), Save{}),
                        Ui::ButtonStyle::subtle(), Mdi::CONTENT_SAVE
                    ),
                    Ui::button(
                        Model::bindIf(s.text->dirty(), Save{true}), Ui::ButtonStyle::subtle(),
                        Mdi::CONTENT_SAVE_PLUS
                    )
                ),
                .endTools = slots$(
                    Ui::button(Model::bindIf<Karm::Text::Action>(s.text->canUndo(), Karm::Text::Action::UNDO), Ui::ButtonStyle::subtle(), Mdi::UNDO),
                    Ui::button(Model::bindIf<Karm::Text::Action>(s.text->canRedo(), Karm::Text::Action::REDO), Ui::ButtonStyle::subtle(), Mdi::REDO)
                ),
                .body = [=] {
                    return Ui::vflow(
                        Ui::hflow(
                            0,
                            Math::Align::CENTER,
                            Ui::labelSmall("{}{}", s.url ? s.url->basename() : "Untitled", s.text->dirty() ? "*" : ""),
                            Ui::icon(Mdi::CIRCLE_SMALL, Ui::GRAY700) | Ui::insets({0, -3}),
                            Ui::text(Ui::TextStyles::labelSmall().withColor(Ui::GRAY500), "{}", s.url)
                        ) | Ui::insets({6, 16}),
                        Ui::separator(),

                        s.error
                            ? Kr::errorPage(Mdi::ALERT_DECAGRAM, "Unable to load text"s, Io::toStr(s.error).unwrap()) | Ui::grow()
                            : editor(s.text),
                        Ui::separator(),
                        Ui::hflow(
                            6,
                            Math::Align::CENTER,
                            Ui::labelSmall("{}", s.text->dirty() ? "Edited" : ""),
                            Ui::grow(NONE),
                            Ui::labelSmall("Ln {}, Col {}", 0, 0),
                            Ui::separator(),
                            Ui::labelSmall("UTF-8"),
                            Ui::separator(),
                            Ui::labelSmall("LF")
                        ) | Ui::box({
                                .padding = {6, 12},
                                .backgroundFill = Ui::GRAY900,
                            })
                    );
                },
            });
        }
    );
}

Res<String> readAllUtf8(Mime::Url const &url) {
    auto file = try$(Sys::File::open(url));
    return Io::readAllUtf8(file);
}

} // namespace Hideo::Text

Async::Task<> entryPointAsync(Sys::Context &ctx) {
    auto &args = useArgs(ctx);
    Opt<Mime::Url> url;
    Res<String> text = Ok(""s);
    if (args.len()) {
        url = co_try$(Mime::parseUrlOrPath(args[0]));
        text = Hideo::Text::readAllUtf8(*url);
    }
    co_return Ui::runApp(ctx, Hideo::Text::app(url, text));
}
