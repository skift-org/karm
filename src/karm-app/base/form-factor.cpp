export module Karm.App.Base:formFactor;

namespace Karm::App {

export enum struct FormFactor {
    DESKTOP,
    MOBILE,

    _LEN,
};

export FormFactor formFactor = FormFactor::DESKTOP;

} // namespace Karm::App
