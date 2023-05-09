#include <chrono>
#include <iostream>
#include <thread>
#include <unordered_set>

#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"
#include "flecs.h"
#include "fmt/format.h"

struct Application {
    Application() : m_game_view{}, m_minimap_view{}, m_window{} {
        m_game_view.reset(sf::FloatRect{0.0f, 0.0f, 800.0f, 600.0f});
        m_game_view.setViewport(sf::FloatRect{0.0f, 0.0f, 1.0f, 1.0f});

        m_minimap_view.reset(sf::FloatRect{0.0f, 0.0f, 1200.0f, 900.0f});
        m_minimap_view.setViewport(sf::FloatRect{0.75f, 0.0f, 0.25f, 0.25f});

        m_window.create(sf::VideoMode{800, 600}, "TestGame");
    }

    sf::View& game_view() {
        return m_game_view;
    }
    sf::View& minimap_view() {
        return m_minimap_view;
    }
    sf::RenderWindow& window() {
        return m_window;
    }

   private:
    sf::View m_game_view;
    sf::View m_minimap_view;
    sf::RenderWindow m_window;
};

struct CharacterCommand {
    enum Type
    {
        MoveUp,
        MoveLeft,
        MoveDown,
        MoveRight,
    };

    CharacterCommand() : m_commands{} {
    }

    [[nodiscard]] bool contains(Type command) const {
        return m_commands.contains(command);
    }
    void add(Type command) {
        m_commands.insert(command);
    }
    void remove(Type command) {
        m_commands.erase(command);
    }

   private:
    std::unordered_set<Type> m_commands;
};

struct Velocity {
    float x = 0.0f;
    float y = 0.0f;
};

void cleanup_temporary_components(flecs::world& world) {
    world.remove<CharacterCommand>();
    world.remove<Velocity>();
}

void populate_character_command(flecs::entity character_entity) {
    auto* character_command = character_entity.get_mut<CharacterCommand>();
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
        character_command->add(CharacterCommand::MoveUp);
    } else {
        character_command->remove(CharacterCommand::MoveUp);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
        character_command->add(CharacterCommand::MoveLeft);
    } else {
        character_command->remove(CharacterCommand::MoveLeft);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
        character_command->add(CharacterCommand::MoveDown);
    } else {
        character_command->remove(CharacterCommand::MoveDown);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
        character_command->add(CharacterCommand::MoveRight);
    } else {
        character_command->remove(CharacterCommand::MoveRight);
    }
}

int main() {
    Application application;

    sf::CircleShape shape{100.0f};
    shape.setPosition(300, 200);
    shape.setFillColor(sf::Color::Green);

    sf::Font font;
    if (!font.loadFromFile("resources/fonts/OpenSans-Regular.ttf")) {
        return 1;
    }

    sf::Text text;
    text.setString("");
    text.setFont(font);
    text.setCharacterSize(12);
    text.setPosition(760, 0);
    text.setFillColor(sf::Color::White);
    text.setOutlineColor(sf::Color::White);

    sf::Texture texture;
    if (!texture.loadFromFile("resources/textures/colored.png")) {
        return 1;
    }

    flecs::world world;
    world.set_target_fps(60);

    auto character_entity = world.entity("Character");
    character_entity.emplace<sf::Sprite>([&]() {
        sf::Sprite sprite;
        sprite.setTexture(texture);
        sprite.setPosition(0.0f, 0.0f);
        sprite.setTextureRect(sf::IntRect(4 * 17, 2 * 17, 16, 16));
        return sprite;
    }());

    world.system<CharacterCommand>("CharacterCommand")
        .kind(flecs::OnUpdate)
        .each([](flecs::entity entity, CharacterCommand& character_command) {
            Velocity velocity;
            if (character_command.contains(CharacterCommand::MoveUp)) {
                velocity.y -= 100.0f;
            }
            if (character_command.contains(CharacterCommand::MoveLeft)) {
                velocity.x -= 100.0f;
            }
            if (character_command.contains(CharacterCommand::MoveDown)) {
                velocity.y += 100.0f;
            }
            if (character_command.contains(CharacterCommand::MoveRight)) {
                velocity.x += 100.0f;
            }

            if (velocity.x == 0.0f && velocity.y == 0.0f) {
                entity.remove<Velocity>();
            } else {
                entity.set<Velocity>(velocity);
            }
        });
    world.system<const Velocity, sf::Sprite>("Movement")
        .kind(flecs::OnUpdate)
        .each([](flecs::entity entity, const Velocity& velocity, sf::Sprite& sprite) {
            sprite.move(velocity.x * entity.delta_time(), velocity.y * entity.delta_time());
        });

    auto& window = application.window();
    auto prev_render_time = std::chrono::high_resolution_clock::now();
    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    window.close();
                    break;
                case sf::Event::Resized:
                    application.game_view().reset(
                        sf::FloatRect{0.0f,
                                      0.0f,
                                      static_cast<float>(event.size.width),
                                      static_cast<float>(event.size.height)});
                    application.minimap_view().reset(
                        sf::FloatRect{0.0f,
                                      0.0f,
                                      static_cast<float>(event.size.width) / 4.0f,
                                      static_cast<float>(event.size.height) / 4.0f});
                    break;
                case sf::Event::KeyPressed:
                case sf::Event::KeyReleased:
                    if (event.key.code == sf::Keyboard::BackSpace ||
                        event.key.code == sf::Keyboard::Delete) {
                        window.close();
                        break;
                    }
                    populate_character_command(character_entity);
                    break;
                default:
                    break;
            }
        }

        auto time_before_render = std::chrono::high_resolution_clock::now();
        auto delta_time =
            std::chrono::duration<float>(time_before_render - prev_render_time).count();

        world.progress(delta_time);
        cleanup_temporary_components(world);

        application.game_view().setCenter(character_entity.get<sf::Sprite>()->getPosition());
        application.minimap_view().setCenter(character_entity.get<sf::Sprite>()->getPosition());

        prev_render_time = time_before_render;

        double fps = 1.0 / delta_time;
        text.setString(fmt::format("{:.0f}", fps) + " FPS");

        window.clear();

        window.setView(application.game_view());
        window.draw(shape);
        world.each<const sf::Sprite>([&](const sf::Sprite& sprite) { window.draw(sprite); });

        window.setView(application.minimap_view());
        window.draw(shape);
        world.each<const sf::Sprite>([&](const sf::Sprite& sprite) { window.draw(sprite); });

        window.setView(window.getDefaultView());
        window.draw(text);
        window.display();
    }

    return 0;
}
