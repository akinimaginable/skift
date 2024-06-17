#include <karm-ui/layout.h>
#include <karm-ui/popover.h>

#include "context-menu.h"

namespace Karm::Kira {

struct ContextMenu : public Ui::ProxyNode<ContextMenu> {
    Ui::Slot _menu;

    ContextMenu(Ui::Child child, Ui::Slot menu)
        : Ui::ProxyNode<ContextMenu>(child),
          _menu(std::move(menu)) {
    }

    void reconcile(ContextMenu &o) override {
        Ui::ProxyNode<ContextMenu>::reconcile(o);
        _menu = std::move(o._menu);
    }

    void event(Sys::Event &e) override {
        Ui::ProxyNode<ContextMenu>::event(e);
        e.handle<Events::MouseEvent>([&](Events::MouseEvent &m) {
            if (m.type == Events::MouseEvent::PRESS and
                m.button == Events::Button::RIGHT) {
                Ui::showPopover(*this, m.pos, _menu());
                return true;
            }
            return false;
        });
    }
};

Ui::Child contextMenu(Ui::Child child, Ui::Slot menu) {
    return makeStrong<ContextMenu>(child, std::move(menu));
}

Ui::Child contextMenuList(Ui::Children children) {
    return Ui::vflow(
               children
           ) |
           Ui::minSize({200, Ui::UNCONSTRAINED}) |
           Ui::box({
               .borderRadius = 4,
               .borderWidth = 1,
               .borderPaint = Ui::GRAY800,
               .backgroundPaint = Ui::GRAY900,
               .shadowStyle = Gfx::BoxShadow::elevated(4),
           });
}

Ui::Child contextMenuItem(Ui::OnPress onPress, Opt<Mdi::Icon> i, Str t) {
    return Ui::hflow(
               12,
               Math::Align::CENTER,
               i ? Ui::icon(*i) : Ui::empty(18),
               Ui::text(t)
           ) |
           Ui::spacing({10, 6, 16, 6}) |
           Ui::minSize({Ui::UNCONSTRAINED, 36}) |
           Ui::button(
               [onPress = std::move(onPress)](auto &n) {
                   onPress(n);
                   Ui::closePopover(n);
               },
               Ui::ButtonStyle::subtle()
           ) |
           Ui::spacing(4);
}

} // namespace Karm::Kira