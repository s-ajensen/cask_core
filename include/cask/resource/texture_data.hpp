#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>

#include <cask/resource/resource_handle.hpp>

using TextureHandle = ResourceHandle<struct TextureData>;

struct TextureData {
private:
    uint32_t width_;
    uint32_t height_;
    uint32_t channels_;
    std::vector<uint8_t> pixels_;

    static void validate_width(uint32_t width) {
        if (width == 0) {
            throw std::runtime_error("width must not be zero");
        }
    }

    static void validate_height(uint32_t height) {
        if (height == 0) {
            throw std::runtime_error("height must not be zero");
        }
    }

    static void validate_channels(uint32_t channels) {
        if (channels != 1 && channels != 3 && channels != 4) {
            throw std::runtime_error("channels must be 1, 3, or 4");
        }
    }

    static void validate_pixels(const std::vector<uint8_t>& pixels, uint32_t width, uint32_t height, uint32_t channels) {
        if (pixels.size() != width * height * channels) {
            throw std::runtime_error("pixels size must equal width * height * channels");
        }
    }

public:
    TextureData(uint32_t width, uint32_t height, uint32_t channels, std::vector<uint8_t> pixels)
        : width_(width), height_(height), channels_(channels), pixels_(std::move(pixels)) {
        validate_width(width_);
        validate_height(height_);
        validate_channels(channels_);
        validate_pixels(pixels_, width_, height_, channels_);
    }

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    uint32_t channels() const { return channels_; }
    const std::vector<uint8_t>& pixels() const { return pixels_; }
};
