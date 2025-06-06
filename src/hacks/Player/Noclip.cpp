#include <modules/config/config.hpp>
#include <modules/gui/color.hpp>
#include <modules/gui/gui.hpp>
#include <modules/gui/components/float-toggle.hpp>
#include <modules/gui/components/input-float.hpp>
#include <modules/gui/components/int-toggle.hpp>
#include <modules/gui/components/toggle.hpp>
#include <modules/hack/hack.hpp>
#include <modules/labels/variables.hpp>

#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

namespace eclipse::hacks::Player {
    class $hack(Noclip) {
        void init() override {
            auto tab = gui::MenuTab::find("tab.player");

            config::setIfEmpty("player.noclip.p1", true);
            config::setIfEmpty("player.noclip.p2", true);
            config::setIfEmpty("player.noclip.opacity", 90.f);
            config::setIfEmpty("player.noclip.time", 0.f);
            config::setIfEmpty("player.noclip.color", gui::Color::RED);
            config::setIfEmpty("player.noclip.acclimit", 95.f);
            config::setIfEmpty("player.noclip.deathlimit", 2);

            tab->addToggle("player.noclip")
               ->setDescription()
               ->handleKeybinds()
               ->addOptions([](std::shared_ptr<gui::MenuTab> options) {
                   options->addToggle("player.noclip.p1");
                   options->addToggle("player.noclip.p2");
                   options->addFloatToggle("player.noclip.acclimit", 0.000000001f, 999999999.f, "%.2f")->handleKeybinds();
                   options->addIntToggle("player.noclip.deathlimit", 1, 999999999)->handleKeybinds();
                   options->addToggle("player.noclip.tint");
                   options->addColorComponent("player.noclip.color");
                   options->addInputFloat("player.noclip.opacity", 0.f, 100.f, "%.0f%");
                   options->addInputFloat("player.noclip.time", 0.f, 999999999.f, "%.2fs")->setDescription();
               });
        }

        [[nodiscard]] bool isCheating() const override {
            return config::get<"player.noclip", bool>() &&
                   config::getTemp<int>("noclipDeaths", 0) > 0;
        }
        [[nodiscard]] const char* getId() const override { return "Noclip"; }
    };

    REGISTER_HACK(Noclip)

    class $modify(NoClipPLHook, PlayLayer) {
        struct Fields {
            cocos2d::CCLayerColor* m_noclipTint = nullptr;
            bool m_wouldDie = false;
            bool m_wouldDieFrame = false;
            bool m_deadLastFrame = false;
            float m_tintTimer = 0.f;
            float m_tintOpacity = 0.f;
            size_t m_deadFrames = 0;
        };

        ENABLE_SAFE_HOOKS_ALL()

        void destroyPlayer(PlayerObject* player, GameObject* object) override {
            if (object == m_anticheatSpike)
                return PlayLayer::destroyPlayer(player, object);

            if (config::get<bool>("player.noclip.acclimit.toggle", false)) {
                auto acc = config::getTemp<float>("noclipAccuracy", 100.f);
                auto limit = config::get<float>("player.noclip.acclimit", 95.f);
                if (acc < limit)
                    return PlayLayer::destroyPlayer(player, object);
            }
            if (config::get<bool>("player.noclip.deathlimit.toggle", false)) {
                auto deaths = config::getTemp<int>("noclipDeaths", 0);
                auto limit = config::get<int>("player.noclip.deathlimit", 2);
                if (deaths >= limit)
                    return PlayLayer::destroyPlayer(player, object);
            }

            auto fields = m_fields.self();

            if (!fields->m_noclipTint && config::get<bool>("player.noclip.tint", false)) {
                auto color = config::get<gui::Color>("player.noclip.color", gui::Color::RED).toCCColor3B();
                fields->m_noclipTint = cocos2d::CCLayerColor::create(
                    cocos2d::_ccColor4B{color.r, color.g, color.b, 0}
                );
                fields->m_noclipTint->setZOrder(1000);
                fields->m_noclipTint->setID("nocliptint"_spr);
                if (auto uiMenu = utils::getEclipseUILayer()) {
                    uiMenu->addChild(fields->m_noclipTint);
                } else { // fallback
                    m_uiLayer->addChild(fields->m_noclipTint);
                }
            }

            bool noclipActive = config::get<bool>("player.noclip", false);
            if (!noclipActive) return PlayLayer::destroyPlayer(player, object);

            bool player1 = config::get<bool>("player.noclip.p1", true) && player == m_player1;
            bool player2 = config::get<bool>("player.noclip.p2", true) && player == m_player2;
            if (player1 || player2) {
                fields->m_wouldDieFrame = true;
                fields->m_wouldDie = true;
                fields->m_tintTimer = 0.f;
            } else {
                PlayLayer::destroyPlayer(player, object);
            }
        }

        void postUpdate(float dt) override {
            auto fields = m_fields.self();
            config::setTemp<bool>("noclipDying", fields->m_wouldDieFrame || fields->m_deadLastFrame);

            if (config::get<bool>("player.noclip.tint", false) && fields->m_noclipTint && !m_hasCompletedLevel && !m_player1->m_isDead) {
                float time = config::get<float>("player.noclip.time", 0.f);
                if (time == 0.f) { // this doesnt really work but ok ninx
                    if (fields->m_wouldDie) {
                        if (fields->m_tintOpacity < 1.f) {
                            fields->m_tintOpacity += 0.25f;
                            fields->m_noclipTint->setOpacity(
                                fields->m_tintOpacity * config::get<float>("player.noclip.opacity", 90.f)
                            );
                        }
                        fields->m_wouldDie = false;
                    } else {
                        if (fields->m_tintOpacity > 0.f) {
                            fields->m_tintOpacity -= 0.25f;
                            fields->m_noclipTint->setOpacity(
                                fields->m_tintOpacity * config::get<float>("player.noclip.opacity", 90.f)
                            );
                        }
                    }
                } else {
                    if (fields->m_wouldDie) {
                        fields->m_tintTimer += dt;
                        float progress = fields->m_tintTimer / time;
                        if (progress >= 1.0f) {
                            fields->m_noclipTint->setOpacity(0);
                            fields->m_wouldDie = false;
                            fields->m_tintTimer = 0.F;
                            fields->m_tintOpacity = 0.f;
                        } else {
                            auto startOpacity = config::get<float>("player.noclip.opacity", 90.f);
                            fields->m_tintOpacity = (255.F * (startOpacity / 100.F)) * (1.f - progress);
                        }
                        if (fields->m_tintOpacity <= 0.0F)
                            fields->m_tintOpacity = 0.0F;
                        fields->m_noclipTint->setOpacity(m_fields->m_tintOpacity);
                    }
                }
            }

            PlayLayer::postUpdate(dt);
        }

        void resetLevel() {
            PlayLayer::resetLevel();
            auto fields = m_fields.self();
            fields->m_wouldDie = false;
            fields->m_wouldDieFrame = false;
            fields->m_deadFrames = 0;
            if (fields->m_noclipTint != nullptr) {
                fields->m_noclipTint->setOpacity(0);
                fields->m_tintTimer = 0.F;
                fields->m_tintOpacity = 0.f;
            }
            config::setTemp<int>("noclipDeaths", 0);
        }
    };

    class $modify(NoClipGJBGLHook, GJBaseGameLayer) {
        void processCommands(float dt) {
            GJBaseGameLayer::processCommands(dt);

            if (!utils::get<PlayLayer>()) {
                config::setTemp<int>("noclipDeaths", 0);
                config::setTemp<float>("noclipAccuracy", 100.f);
                return;
            }

            auto pl = reinterpret_cast<NoClipPLHook*>(this);
            if (pl->m_hasCompletedLevel || pl->m_levelEndAnimationStarted) return;

            auto fields = pl->m_fields.self();
            if (fields->m_wouldDieFrame) {
                fields->m_deadFrames++;
                if (!fields->m_deadLastFrame) {
                    auto deaths = config::getTemp<int>("noclipDeaths", 0);
                    config::setTemp<int>("noclipDeaths", deaths + 1);
                    if (config::get<bool>("player.noclip.deathlimit.toggle", false) && deaths + 1 >= config::get<int>("player.noclip.deathlimit", 0))
                        utils::get<PlayLayer>()->destroyPlayer(m_player1, nullptr);
                }
            }

            fields->m_deadLastFrame = fields->m_wouldDieFrame;
            fields->m_wouldDieFrame = false;

            auto frame = m_gameState.m_currentProgress;
            if (frame > 0) {
                float acc = static_cast<float>(frame - fields->m_deadFrames) / static_cast<float>(frame) * 100.f;
                config::setTemp("noclipAccuracy", acc);
                bool dead = (m_player1 && m_player1->m_isDead) || (m_player2 && m_player2->m_isDead);
                //if (config::get<bool>("player.noclip.acclimit.toggle", false) && acc <= config::get<float>("player.noclip.acclimit", 95.f) && !dead)
                //utils::get<PlayLayer>()->destroyPlayer(m_player1, (GameObject*)((int*)1));
            }
        }
    };
}

/*
if (config::get<bool>("player.noclip.acclimit.toggle", false) && acc < config::get<float>("player.noclip.acclimit", 95.f)) {
                    utils::get<PlayLayer>()->destroyPlayer(m_player1, nullptr);
                    config::setTemp("noclipAccuracy", config::get<float>("player.noclip.acclimit", 95.f));
                }*/
