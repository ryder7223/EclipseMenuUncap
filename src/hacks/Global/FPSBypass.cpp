#include <modules/config/config.hpp>
#include <modules/gui/gui.hpp>
#include <modules/gui/components/float-toggle.hpp>
#include <modules/hack/hack.hpp>

#ifdef GEODE_IS_WINDOWS
constexpr float MIN_FPS = 0.f;
constexpr float MAX_FPS = 999999999.f;

namespace eclipse::hacks::Global {
    class $hack(FPSBypass) {
    public:
        static void updateRefreshRate() {
            auto* gm = utils::get<GameManager>();

            // store settings
            bool fpsBypassEnabled = config::get<bool>("global.fpsbypass.toggle", false);
            auto fpsBypassValue = config::get<float>("global.fpsbypass", gm->m_customFPSTarget);
            float actualFPS = std::clamp(fpsBypassValue, MIN_FPS, MAX_FPS); // sometimes the value can be 0
            gm->setGameVariable("0116", fpsBypassEnabled);
            gm->m_customFPSTarget = actualFPS;

            // apply settings
            float frameTime = 1.f / (fpsBypassEnabled ? actualFPS : 60.f);
            utils::get<cocos2d::CCDirector>()->setAnimationInterval(frameTime);
        }

        void init() override {
            auto tab = gui::MenuTab::find("tab.global");
            tab->addFloatToggle("global.fpsbypass", "global.fpsbypass", 0.1f, FLT_MAX, "%.2f FPS")
               ->handleKeybinds()
               ->toggleCallback([] { updateRefreshRate(); })
               ->valueCallback([](float) { updateRefreshRate(); });
        }

        void lateInit() override {
            auto* gm = utils::get<GameManager>();
            auto fpsBypassEnabled = gm->getGameVariable("0116");
            auto fpsBypassValue = gm->m_customFPSTarget;
            if (fpsBypassValue == 0) // rare robtop bug
                fpsBypassValue = 60.f;
            config::set("global.fpsbypass", fpsBypassValue);
            config::set("global.fpsbypass.toggle", fpsBypassEnabled);

            updateRefreshRate();
        }

        [[nodiscard]] const char* getId() const override { return "FPSBypass"; }
        [[nodiscard]] int32_t getPriority() const override { return -15; }
    };

    REGISTER_HACK(FPSBypass)
}
#endif
