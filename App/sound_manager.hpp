#pragma once
#include <SFML/Audio.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

class SoundManager {
public:
    enum class Effect { Shoot, Cannon, Explosion, Raid, WaveStart, Coin, Click };

    SoundManager();
    void play(Effect effect);
    void toggleMute() { muted = !muted; }
    bool isMuted() const { return muted; }

private:
    std::map<Effect, sf::SoundBuffer> buffers;
    std::vector<std::unique_ptr<sf::Sound>> voices;
    std::map<Effect, float> lastPlayed;
    sf::Clock clock;
    bool muted = false;
    size_t nextVoice = 0;

    void make(Effect effect, float seconds, float startHz, float endHz, float noise, float volume);
};
