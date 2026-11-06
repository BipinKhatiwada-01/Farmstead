#include "farmstead/audio_system.hpp"
#include<SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace farmstead {
namespace {

constexpr int kSampleRate = 48'000;
constexpr int kChannels = 2;
constexpr int kFramesPerBlock = 512;
constexpr int kTargetQueuedFrames = 3'072;
constexpr std::size_t kMaximumVoices = 20;
constexpr double kTwoPi = 6.28318530717958647692;

constexpr float kEffectGain = 1.30f;
constexpr float kAmbienceGainBoost = 1.25f;

float clampSample(float sample) {
    return std::clamp(sample, -0.92f, 0.92f);
}

} // namespace

AudioSystem::~AudioSystem() {
    shutdown();
}

bool AudioSystem::initialize() {
    if (stream_ != nullptr) {
        return true;
    }

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        return false;
    }
    audioSubsystemInitialized_ = true;

    SDL_AudioSpec specification{};
    specification.format = SDL_AUDIO_F32;
    specification.channels = kChannels;
    specification.freq = kSampleRate;

    stream_ = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &specification, nullptr, nullptr);
    if (stream_ == nullptr) {
        shutdown();
        return false;
    }

    voices_.clear();
    ambienceFramesUntilBird_ = static_cast<std::uint64_t>(kSampleRate * 4.5);

    if (!SDL_ResumeAudioStreamDevice(stream_)) {
        shutdown();
        return false;
    }

    return true;
}

void AudioSystem::shutdown() noexcept {
    if (stream_ != nullptr) {
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
    }

    voices_.clear();

    if (audioSubsystemInitialized_) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        audioSubsystemInitialized_ = false;
    }
}

void AudioSystem::play(SoundEffect effect) noexcept {
    VoiceType type = VoiceType::MENU_MOVE;
    switch (effect) {
    case SoundEffect::MENU_MOVE:          type = VoiceType::MENU_MOVE; break;
    case SoundEffect::MENU_CONFIRM:       type = VoiceType::MENU_CONFIRM; break;
    case SoundEffect::FOOTSTEP:           type = VoiceType::FOOTSTEP; break;
    case SoundEffect::TILL:               type = VoiceType::TILL; break;
    case SoundEffect::PLANT:              type = VoiceType::PLANT; break;
    case SoundEffect::WATER:              type = VoiceType::WATER; break;
    case SoundEffect::HARVEST:            type = VoiceType::HARVEST; break;
    case SoundEffect::SELL:               type = VoiceType::SELL; break;
    case SoundEffect::SLEEP:              type = VoiceType::SLEEP; break;
    case SoundEffect::OBJECTIVE_COMPLETE: type = VoiceType::OBJECTIVE_COMPLETE; break;
    }
    addVoice(type);
}

void AudioSystem::setAmbienceGain(float gain) noexcept {
    ambienceGain_ = std::clamp(gain * kAmbienceGainBoost, 0.0f, 1.0f);
}

void AudioSystem::update() noexcept {
    if (stream_ == nullptr) {
        return;
    }

    const int queuedBytes = SDL_GetAudioStreamQueued(stream_);
    if (queuedBytes < 0) {
        return;
    }

    constexpr int bytesPerFrame = static_cast<int>(sizeof(float) * kChannels);
    int queuedFrames = queuedBytes / bytesPerFrame;
    std::array<float, kFramesPerBlock * kChannels> block{};

    while (queuedFrames < kTargetQueuedFrames) {
        generateBlock(block.data(), kFramesPerBlock);
        const int blockBytes = static_cast<int>(block.size() * sizeof(float));
        if (!SDL_PutAudioStreamData(stream_, block.data(), blockBytes)) {
            break;
        }
        queuedFrames += kFramesPerBlock;
    }
}

bool AudioSystem::isAvailable() const noexcept {
    return stream_ != nullptr;
}

void AudioSystem::addVoice(VoiceType type) noexcept {
    if (stream_ == nullptr) {
        return;
    }

    if (voices_.size() >= kMaximumVoices) {
        voices_.erase(voices_.begin());
    }

    Voice voice{};
    voice.type = type;
    switch (type) {
    case VoiceType::MENU_MOVE:          voice.durationFrames = 3'360; break;
    case VoiceType::MENU_CONFIRM:       voice.durationFrames = 9'600; break;
    case VoiceType::FOOTSTEP:           voice.durationFrames = 4'800; break;
    case VoiceType::TILL:               voice.durationFrames = 9'600; break;
    case VoiceType::PLANT:              voice.durationFrames = 6'240; break;
    case VoiceType::WATER:              voice.durationFrames = 15'360; break;
    case VoiceType::HARVEST:            voice.durationFrames = 12'000; break;
    case VoiceType::SELL:               voice.durationFrames = 15'360; break;
    case VoiceType::SLEEP:              voice.durationFrames = 28'800; break;
    case VoiceType::OBJECTIVE_COMPLETE: voice.durationFrames = 52'800; break;
    case VoiceType::BIRD:               voice.durationFrames = 36'000; break;
    }
    voice.randomState = nextRandom() | 1u;
    voice.phase = static_cast<double>(nextRandom() % 6283u) / 1000.0;
    voice.secondaryPhase = static_cast<double>(nextRandom() % 6283u) / 1000.0;
    voices_.push_back(voice);
}

void AudioSystem::generateBlock(float* output, int frameCount) noexcept {
    for (int frame = 0; frame < frameCount; ++frame) {
        if (ambienceFramesUntilBird_ == 0) {
            addVoice(VoiceType::BIRD);
            ambienceFramesUntilBird_ = static_cast<std::uint64_t>(
                kSampleRate * (8.0 + static_cast<double>(nextRandom() % 8'000u) / 1000.0));
        }
        if (ambienceFramesUntilBird_ > 0) {
            --ambienceFramesUntilBird_;
        }

        const float whiteNoise = nextRandomBipolar();
        windFilter_ += 0.018f * (whiteNoise - windFilter_);
        const float windTarget = 0.38f + 0.32f * std::abs(nextRandomBipolar());
        windEnvelope_ += 0.000035f * (windTarget - windEnvelope_);

        const float softWind = windFilter_ * windEnvelope_ * 0.17f * ambienceGain_;
        float left = softWind;
        float right = softWind;

        for (Voice& voice : voices_) {
            renderVoice(voice, left, right);
        }

        const std::size_t sampleIndex = static_cast<std::size_t>(frame * kChannels);
        output[sampleIndex] = clampSample(left);
        output[sampleIndex + 1] = clampSample(right);
    }

    std::erase_if(voices_, [](const Voice& voice) {
        return voice.ageFrames >= voice.durationFrames;
    });
}

void AudioSystem::renderVoice(Voice& voice, float& left, float& right) noexcept {
    if (voice.ageFrames >= voice.durationFrames) {
        return;
    }

    const float elapsed = static_cast<float>(voice.ageFrames) /
                          static_cast<float>(kSampleRate);
    const float normalizedAge = static_cast<float>(voice.ageFrames) /
                                static_cast<float>(voice.durationFrames);
    const float attack = std::min(1.0f, elapsed / 0.006f);
    const float envelope = attack * std::exp(-5.2f * normalizedAge);

    float signal = 0.0f;
    float pan = 0.0f;
    float frequency = 440.0f;
    float secondFrequency = 660.0f;

    const float noise = [&voice]() {
        voice.randomState ^= voice.randomState << 13;
        voice.randomState ^= voice.randomState >> 17;
        voice.randomState ^= voice.randomState << 5;
        const float unit = static_cast<float>(voice.randomState & 0x00FFFFFFu) /
                           static_cast<float>(0x01000000u);
        return unit * 2.0f - 1.0f;
    }();

    switch (voice.type) {
    case VoiceType::MENU_MOVE:
        frequency = 760.0f;
        signal = static_cast<float>(std::sin(voice.phase)) * envelope * 0.050f;
        pan = -0.08f;
        break;

    case VoiceType::MENU_CONFIRM:
        frequency = 570.0f + 330.0f * normalizedAge;
        secondFrequency = frequency * 1.5f;
        signal = (static_cast<float>(std::sin(voice.phase)) * 0.72f +
                  static_cast<float>(std::sin(voice.secondaryPhase)) * 0.28f) *
                 envelope * 0.075f;
        break;

    case VoiceType::FOOTSTEP:
        voice.noiseFilter += 0.18f * (noise - voice.noiseFilter);
        frequency = 105.0f;
        signal = (static_cast<float>(std::sin(voice.phase)) * 0.42f +
                  voice.noiseFilter * 0.58f) * envelope * 0.105f;
        break;

    case VoiceType::TILL:
        voice.noiseFilter += 0.24f * (noise - voice.noiseFilter);
        frequency = 92.0f - 24.0f * normalizedAge;
        signal = (voice.noiseFilter * 0.78f +
                  static_cast<float>(std::sin(voice.phase)) * 0.22f) *
                 envelope * 0.13f;
        pan = -0.06f;
        break;

    case VoiceType::PLANT:
        frequency = 380.0f + 180.0f * normalizedAge;
        signal = static_cast<float>(std::sin(voice.phase)) * envelope * 0.065f;
        pan = 0.05f;
        break;

    case VoiceType::WATER:
        voice.noiseFilter += 0.09f * (noise - voice.noiseFilter);
        frequency = 620.0f - 270.0f * normalizedAge;
        signal = (voice.noiseFilter * 0.72f +
                  static_cast<float>(std::sin(voice.phase)) * 0.28f) *
                 envelope * 0.075f;
        break;

    case VoiceType::HARVEST: {
        const float note = normalizedAge < 0.38f ? 720.0f :
                           (normalizedAge < 0.72f ? 960.0f : 1'200.0f);
        frequency = note;
        secondFrequency = note * 1.5f;
        signal = (static_cast<float>(std::sin(voice.phase)) * 0.78f +
                  static_cast<float>(std::sin(voice.secondaryPhase)) * 0.22f) *
                 envelope * 0.080f;
        break;
    }

    case VoiceType::SELL: {
        const float note = normalizedAge < 0.46f ? 1'046.5f : 1'568.0f;
        frequency = note;
        signal = static_cast<float>(std::sin(voice.phase)) * envelope * 0.085f;
        pan = normalizedAge < 0.46f ? -0.12f : 0.12f;
        break;
    }

    case VoiceType::SLEEP:
        frequency = 470.0f - 145.0f * normalizedAge;
        secondFrequency = frequency * 1.25f;
        signal = (static_cast<float>(std::sin(voice.phase)) * 0.82f +
                  static_cast<float>(std::sin(voice.secondaryPhase)) * 0.18f) *
                 envelope * 0.045f;
        break;

    case VoiceType::OBJECTIVE_COMPLETE: {
        constexpr std::array<float, 5> notes{
            523.25f, 659.25f, 783.99f, 1'046.5f, 1'175.0f};
        const int noteIndex = std::min(4, static_cast<int>(normalizedAge * 5.0f));
        frequency = notes[static_cast<std::size_t>(noteIndex)];
        secondFrequency = frequency * 1.5f;
        const float localAge = normalizedAge * 5.0f - static_cast<float>(noteIndex);
        const float noteEnvelope = std::min(1.0f, localAge * 8.0f) *
                                   std::exp(-3.4f * localAge);
        signal = (static_cast<float>(std::sin(voice.phase)) * 0.80f +
                  static_cast<float>(std::sin(voice.secondaryPhase)) * 0.20f) *
                 noteEnvelope * 0.090f;
        break;
    }

    case VoiceType::BIRD: {
        const float syllablePosition = std::fmod(elapsed, 0.22f);
        const float syllableEnvelope = std::max(0.0f, 1.0f - syllablePosition / 0.105f);
        const bool inSyllable = syllablePosition < 0.105f &&
                                (elapsed < 0.20f || elapsed > 0.28f);
        frequency = 1'950.0f + 520.0f * std::sin(elapsed * 19.0f) +
                    360.0f * std::sin(elapsed * 7.0f);
        signal = inSyllable
                     ? static_cast<float>(std::sin(voice.phase)) *
                           syllableEnvelope * envelope * 0.038f
                     : 0.0f;
        pan = 0.24f;
        break;
    }
    }

    voice.phase += kTwoPi * static_cast<double>(frequency) /
                   static_cast<double>(kSampleRate);
    voice.secondaryPhase += kTwoPi * static_cast<double>(secondFrequency) /
                            static_cast<double>(kSampleRate);
    if (voice.phase > kTwoPi) voice.phase = std::fmod(voice.phase, kTwoPi);
    if (voice.secondaryPhase > kTwoPi) {
        voice.secondaryPhase = std::fmod(voice.secondaryPhase, kTwoPi);
    }

    const float voiceGain = voice.type == VoiceType::BIRD ? ambienceGain_ : 1.0f;
    signal *= kEffectGain * voiceGain;
    left += signal * (1.0f - pan) * 0.5f;
    right += signal * (1.0f + pan) * 0.5f;
    ++voice.ageFrames;
}

float AudioSystem::nextRandomBipolar() noexcept {
    const float unit = static_cast<float>(nextRandom() & 0x00FFFFFFu) /
                       static_cast<float>(0x01000000u);
    return unit * 2.0f - 1.0f;
}

std::uint32_t AudioSystem::nextRandom() noexcept {
    randomState_ ^= randomState_ << 13;
    randomState_ ^= randomState_ >> 17;
    randomState_ ^= randomState_ << 5;
    return randomState_;
}

} // namespace farmstead
