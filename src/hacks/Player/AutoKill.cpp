#include <modules/config/config.hpp>
#include <modules/gui/gui.hpp>
#include <modules/gui/components/float-toggle.hpp>
#include <modules/gui/components/toggle.hpp>
#include <modules/hack/hack.hpp>

#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

namespace eclipse::hacks::Player {
    class $hack(AutoKill) {
        void init() override {
            auto tab = gui::MenuTab::find("tab.player");

            config::setIfEmpty("player.autokill.percentage.toggle", true);
            config::setIfEmpty("player.autokill.percentage", 50.0f);
            config::setIfEmpty("player.autokill.time", 90.0f);

            tab->addToggle("player.autokill")->handleKeybinds()->setDescription()
               ->addOptions([](std::shared_ptr<gui::MenuTab> options) {
                   options->addFloatToggle("player.autokill.percentage", 0.f, 100.f, "%.2f%%")
                          ->handleKeybinds()->setDescription();
                   options->addFloatToggle("player.autokill.time", 0.f, 999999999.f, "%.2f s.")
                          ->handleKeybinds()->setDescription();
               });
        }

        [[nodiscard]] const char* getId() const override { return "Auto Kill"; }
    };

    REGISTER_HACK(AutoKill)

    class $modify(AutoKillBGLHook, GJBaseGameLayer) {
        ADD_HOOKS_DELEGATE("player.autokill")

        void killPlayer() {
            auto* playLayer = utils::get<PlayLayer>();
            if (!playLayer) return;

            bool noclipEnabled = config::get<bool>("player.noclip", false);
            config::set("player.noclip", false);
            if (m_player1 && !m_player1->m_isDead)
                playLayer->PlayLayer::destroyPlayer(m_player1, m_player1);
            config::set("player.noclip", noclipEnabled);
        }

        void update(float p0) override {
            GJBaseGameLayer::update(p0);

            auto* playLayer = utils::get<PlayLayer>();
            if (!playLayer) return;

            auto percentageEnabled = config::get<bool>("player.autokill.percentage.toggle", true);
            auto percentage = config::get<float>("player.autokill.percentage", 50.0f);
            auto timeEnabled = config::get<bool>("player.autokill.time.toggle", false);
            auto time = config::get<float>("player.autokill.time", 90.0f);

            bool shouldKill = false;
            shouldKill |= percentageEnabled && playLayer->getCurrentPercent() >= percentage;
            shouldKill |= timeEnabled && m_gameState.m_levelTime >= time;

            if (shouldKill)
                killPlayer();
        }
    };
}
