#include <modules/config/config.hpp>
#include <modules/gui/gui.hpp>
#include <modules/gui/components/float-toggle.hpp>
#include <modules/hack/hack.hpp>

#include <Geode/modify/CCTransitionFade.hpp>

namespace eclipse::hacks::Global {
    class $hack(TransitionSpeed) {
        void init() override {
            auto tab = gui::MenuTab::find("tab.global");

            config::setIfEmpty("global.transitionspeed.toggle", false);
            config::setIfEmpty("global.transitionspeed", 0.5f);

            tab->addFloatToggle("global.transitionspeed", 0.01f, 999999999.f, "%.2f")->setDescription()->handleKeybinds();
        }

        [[nodiscard]] const char* getId() const override { return "Transition Speed"; }
    };

    REGISTER_HACK(TransitionSpeed)

    class $modify(TransitionSpeedCCTFHook, cocos2d::CCTransitionFade) {
        ADD_HOOKS_DELEGATE("global.transitionspeed.toggle")

        #ifdef GEODE_IS_ANDROID
        static CCTransitionFade* create(float duration, CCScene* scene, cocos2d::ccColor3B const& color) {
            return CCTransitionFade::create(config::get<float>("global.transitionspeed", 0.5f), scene, color);
        }
        #else
        static CCTransitionFade* create(float duration, CCScene* scene) {
            return CCTransitionFade::create(config::get<float>("global.transitionspeed", 0.5f), scene);
        }
        #endif
    };
}
