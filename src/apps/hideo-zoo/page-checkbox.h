#pragma once

#include <karm-kira/checkbox.h>
#include <karm-ui/layout.h>

#include "model.h"

namespace Hideo::Zoo {

static inline Page PAGE_CHECKBOX{
    Mdi::CHECKBOX_MARKED,
    "Checkbox",
    "A control that allows the user to toggle between checked and not checked.",
    [] {
        return Ui::vflow(
            16,
            Math::Align::CENTER,
            Kr::checkbox(
                true,
                NONE
            )
        );
    },
};

} // namespace Hideo::Zoo