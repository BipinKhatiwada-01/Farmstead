#pragma once

#include <SDL3/SDL_audio.h>

#include <cstdint>
#include <vector>

namespace farmstead {

enum class SoundEffect {
    MENU_MOVE,
    MENU_CONFIRM,
    FOOTSTEP,
    TILL,
    PLANT,
    WATER,
    HARVEST,
    SELL,
    SLEEP,
    OBJECTIVE_COMPLETE
};

// Small SDL3 audio layer. Effects and a quiet countryside ambience are
// synthesized in memory, so the game does not need an external audio pack.
class AudioSystem {
public:
    AudioSystem() = default;
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;
    AudioSystem(AudioSystem&&) = delete;
    AudioSystem& operator=(AudioSystem&&) = delete;

    // Returns false when audio is unavailable. The game can continue silently.
    [[nodiscard]] bool initialize();
    void shutdown() noexcept;

    void play(SoundEffect effect) noexcept;
    void setAmbienceGain(float gain) noexcept;

    // Keep the device queue supplied with a short buffer of mixed audio.
    // Call once per application-loop iteration.
    void update() noexcept;

    [[nodiscard]] bool isAvailable() const noexcept;

private:
    enum class VoiceType {
        MENU_MOVE,
        MENU_CONFIRM,
        FOOTSTEP,
        TILL,
        PLANT,
        WATER,
        HARVEST,
        SELL,
        SLEEP,
        OBJECTIVE_COMPLETE,
        BIRD
    };

    struct Voice {
        VoiceType type{};
        std::uint64_t ageFrames{};
        std::uint64_t durationFrames{};
        double phase{};
        double secondaryPhase{};
        float noiseFilter{};
        std::uint32_t randomState{1};
    };

    void addVoice(VoiceType type) noexcept;
    void generateBlock(float* output, int frameCount) noexcept;
    void renderVoice(Voice& voice, float& left, float& right) noexcept;
    [[nodiscard]] float nextRandomBipolar() noexcept;
    [[nodiscard]] std::uint32_t nextRandom() noexcept;

    SDL_AudioStream* stream_{};
    bool audioSubsystemInitialized_{};
    float ambienceGain_{0.55f};
    float windFilter_{};
    float windEnvelope_{0.55f};
    std::uint64_t ambienceFramesUntilBird_{};
    std::uint32_t randomState_{0xC001D00Du};
    std::vector<Voice> voices_;
};

} // namespace farmstead
