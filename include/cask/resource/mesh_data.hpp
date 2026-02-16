#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>

#include <cask/resource/resource_handle.hpp>

using MeshHandle = ResourceHandle<struct MeshData>;

struct MeshData {
private:
    std::vector<float> positions_;
    std::vector<uint32_t> indices_;
    std::vector<float> normals_;
    std::vector<float> uvs_;

    static void validate_positions(const std::vector<float>& positions) {
        if (positions.empty()) {
            throw std::runtime_error("positions must not be empty");
        }
        if (positions.size() % 3 != 0) {
            throw std::runtime_error("positions size must be divisible by 3");
        }
    }

    static void validate_indices(const std::vector<uint32_t>& indices) {
        if (indices.empty()) {
            throw std::runtime_error("indices must not be empty");
        }
    }

    static void validate_normals(const std::vector<float>& normals, const std::vector<float>& positions) {
        if (normals.empty()) {
            return;
        }
        if (normals.size() != positions.size()) {
            throw std::runtime_error("normals size must match positions size");
        }
    }

    static void validate_uvs(const std::vector<float>& uvs, const std::vector<float>& positions) {
        if (uvs.empty()) {
            return;
        }
        if (uvs.size() != (positions.size() / 3) * 2) {
            throw std::runtime_error("uvs size must equal 2 per vertex");
        }
    }

public:
    MeshData(std::vector<float> positions, std::vector<uint32_t> indices, std::vector<float> normals = {}, std::vector<float> uvs = {})
        : positions_(std::move(positions)), indices_(std::move(indices)), normals_(std::move(normals)), uvs_(std::move(uvs)) {
        validate_positions(positions_);
        validate_indices(indices_);
        validate_normals(normals_, positions_);
        validate_uvs(uvs_, positions_);
    }

    bool has_normals() const { return !normals_.empty(); }
    bool has_uvs() const { return !uvs_.empty(); }
    const std::vector<float>& positions() const { return positions_; }
    const std::vector<uint32_t>& indices() const { return indices_; }
    const std::vector<float>& normals() const { return normals_; }
    const std::vector<float>& uvs() const { return uvs_; }
};
