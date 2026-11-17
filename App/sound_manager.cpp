#include "sound_manager.hpp"
#include <cmath>
#include <cstdint>
#include <random>

static const unsigned SAMPLE_RATE = 22050;

SoundManager::SoundManager() {
    make(Effect::Shoot,           0.06f,   900.f,   500.f,  0.5f,  0.25f);
    make(Effect::Cannon,          0.18f,   180.f,    60.f,  0.6f,  0.55f);
    make(Effect::Explosion,       0.45f,    90.f,    30.f,  0.9f,  0.6f);
    make(Effect::Raid,            0.50f,   520.f,   140.f,  0.3f,  0.45f);
    make(Effect::WaveStart,       0.45f,   330.f,   660.f,  0.0f,  0.35f);
    make(Effect::Coin,            0.10f,  1300.f,  1800.f,  0.0f,  0.20f);
    make(Effect::Click,           0.04f,   700.f,   700.f,  0.1f,  0.20f);

    for (int i = 0; i < 12; i++)
        voices.push_back(std::make_unique<sf::Sound>(buffers.at(Effect::Click)));
}

void SoundManager::make(Effect effect, float seconds, float startHz, float endHz, float noise, float volume) {
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> white(-1.f, 1.f);
    std::size_t count = static_cast<std::size_t>(seconds * SAMPLE_RATE);
    std::vector<std::int16_t> samples(count);
    float phase = 0.f;
    for (std::size_t i = 0; i < count; i++) {
        float t = static_cast<float>(i) / count;
        float hz = startHz + (endHz - startHz) * t;
        phase += 2.f * 3.14159265f * hz / SAMPLE_RATE;
        float envelope = (1.f - t) * (1.f - t);
        float attack = std::min(1.f, i / (0.005f * SAMPLE_RATE));
        float s = (1.f - noise) * std::sin(phase) + noise * white(rng);
        samples[i] = static_cast<std::int16_t>(s * envelope * attack * volume * 32767.f);
    }
    sf::SoundBuffer buffer;
    if (buffer.loadFromSamples(samples.data(), samples.size(), 1, SAMPLE_RATE, {sf::SoundChannel::Mono}))
        buffers.emplace(effect, std::move(buffer));
}

void SoundManager::play(Effect effect) {
    if (muted || !buffers.count(effect)) return;

    float now = clock.getElapsedTime().asSeconds();
    if (lastPlayed.count(effect) && now - lastPlayed[effect] < 0.06f) return;
    lastPlayed[effect] = now;

    sf::Sound& voice = *voices[nextVoice];
    nextVoice = (nextVoice + 1) % voices.size();
    voice.stop();
    voice.setBuffer(buffers.at(effect));
    voice.play();
}
