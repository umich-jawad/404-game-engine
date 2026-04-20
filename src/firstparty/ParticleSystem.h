#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "Helper.h"
#include "Actor.h"
#include "ImageDB.h"

struct ParticleArray {
    std::vector<float> x;
    std::vector<float> y;
    std::vector<float> vel_x;
    std::vector<float> vel_y;
    std::vector<float> rotation;
    std::vector<float> rotation_speed;
    std::vector<float> start_scale;
    std::vector<int> start_frame;
    std::vector<uint8_t> is_active;

    size_t size() const { return x.size(); }

    void resize(size_t new_size) {
        x.resize(new_size);
        y.resize(new_size);
        vel_x.resize(new_size);
        vel_y.resize(new_size);
        rotation.resize(new_size);
        rotation_speed.resize(new_size);
        start_scale.resize(new_size);
        start_frame.resize(new_size);
        is_active.resize(new_size, 0);
    }
};

class ParticleSystem {
public:
    // Particle system properties
    bool is_emitting = true;
    ParticleArray particles;
    std::queue<int> available_particle_indices; // Queue to track available particle slots for reuse

    float x_offset = 0.0f;
    float y_offset = 0.0f;
    int local_frame_number = 0;
    int frames_between_bursts = 1; // if lower than 1, treat as 1
    int burst_quantity = 1; // if lower than 1, treat as 1

    int start_color_r = 255;
    int start_color_g = 255;
    int start_color_b = 255;
    int start_color_a = 255;
    std::string image = "";
    int sorting_order = 9999;
    int duration_frames = 300; // if lower than 1, treat as 1
    float inv_duration;

    float gravity_scale_x = 0.0f;
    float gravity_scale_y = 0.0f;
    float drag_factor = 1.0f;
    float angular_drag_factor = 1.0f;

    float end_scale = -1.0f; // if negative, end is the same as start
    float end_color_r = -1;
    float end_color_g = -1;
    float end_color_b = -1;
    int end_color_a = -1;
    bool do_scale_interp;

    float emit_angle_min = 0.0f;
    float emit_angle_max = 360.0f;
    float emit_radius_min = 0.0f;
    float emit_radius_max = 0.5f;

    float start_speed_min = 0.0f;
    float start_speed_max = 0.0f;
    float rotation_min = 0.0f;
    float rotation_max = 0.0f;
    float rotation_speed_min = 0.0f;
    float rotation_speed_max = 0.0f;
    float start_scale_min = 1.0f;
    float start_scale_max = 1.0f;

    // Distributions for random properties
    RandomEngine emit_angle_distribution;
    RandomEngine emit_radius_distribution;
    RandomEngine speed_distribution;
    RandomEngine rotation_distribution;
    RandomEngine rotation_speed_distribution;
    RandomEngine scale_distribution;

    bool enabled = true;
    std::string key = "";
    std::string type = "ParticleSystem";
    Actor* actor = nullptr;


    void OnStart() {
        emit_angle_distribution = RandomEngine(emit_angle_min, emit_angle_max, 298);
        emit_radius_distribution = RandomEngine(emit_radius_min, emit_radius_max, 404);
        rotation_distribution = RandomEngine(rotation_min, rotation_max, 440);
        scale_distribution = RandomEngine(start_scale_min, start_scale_max, 494);
        speed_distribution = RandomEngine(start_speed_min, start_speed_max, 498);
        rotation_speed_distribution = RandomEngine(rotation_speed_min, rotation_speed_max, 305);

        if (image.empty()) {
            image = "default_particle";
            ImageDB::CreateDefaultParticleTextureWithName(image);
        }
        if (end_color_r < 0) end_color_r = start_color_r;
        if (end_color_g < 0) end_color_g = start_color_g;
        if (end_color_b < 0) end_color_b = start_color_b;
        if (end_color_a < 0) end_color_a = start_color_a;
        if (duration_frames < 1) duration_frames = 1;
        if (frames_between_bursts < 1) frames_between_bursts = 1;
        if (burst_quantity < 1) burst_quantity = 1;
        inv_duration = 1.0f / static_cast<float>(duration_frames);
        do_scale_interp = end_scale >= 0.0f;
    }

    void OnUpdate() {
        if (is_emitting && local_frame_number % frames_between_bursts == 0) {
            GenerateNewParticles(burst_quantity);
        }
        IterateAllParticles();
        local_frame_number++;
    }

    void GenerateNewParticles(int quantity) {
        // Bulk-allocate any new slots needed, then pop from the free-list queue
        int available = static_cast<int>(available_particle_indices.size());
        int new_slots = quantity - available;
        if (new_slots > 0) {
            size_t old_size = particles.size();
            size_t new_size = old_size + static_cast<size_t>(new_slots);
            for (size_t i = old_size; i < new_size; i++) {
                available_particle_indices.push(static_cast<int>(i));
            }
            particles.resize(new_size);
        }

        for (int i = 0; i < quantity; i++) {
            int index = available_particle_indices.front();
            available_particle_indices.pop();

            float emit_angle = glm::radians(emit_angle_distribution.Sample());
            float emit_radius = emit_radius_distribution.Sample();
            float speed = speed_distribution.Sample();
            float cos_angle = glm::cos(emit_angle);
            float sin_angle = glm::sin(emit_angle);

            particles.x[index] = x_offset + emit_radius * cos_angle;
            particles.y[index] = y_offset + emit_radius * sin_angle;
            particles.vel_x[index] = speed * cos_angle;
            particles.vel_y[index] = speed * sin_angle;
            particles.rotation[index] = rotation_distribution.Sample();
            particles.rotation_speed[index] = rotation_speed_distribution.Sample();
            particles.start_scale[index] = scale_distribution.Sample();
            particles.start_frame[index] = local_frame_number;
            particles.is_active[index] = 1;
        }
    }

    void IterateAllParticles() {
        const size_t count = particles.size();

        for (size_t i = 0; i < count; i++) {
            if (!particles.is_active[i]) continue;

            int lifetime = local_frame_number - particles.start_frame[i];
            if (lifetime >= duration_frames) {
                particles.is_active[i] = 0;
                available_particle_indices.push(static_cast<int>(i));
                continue;
            }

            // Apply gravity
            particles.vel_x[i] += gravity_scale_x;
            particles.vel_y[i] += gravity_scale_y;

            // Apply drag and angular drag
            particles.vel_x[i] *= drag_factor;
            particles.vel_y[i] *= drag_factor;
            particles.rotation_speed[i] *= angular_drag_factor;

            // Update position based on velocity
            particles.x[i] += particles.vel_x[i];
            particles.y[i] += particles.vel_y[i];

            // Update rotation based on rotation speed
            particles.rotation[i] += particles.rotation_speed[i];

            // Process color and scale interpolation based on lifetime
            float t = static_cast<float>(lifetime) * inv_duration;

            float current_scale = do_scale_interp ?
                particles.start_scale[i] + (end_scale - particles.start_scale[i]) * t :
                particles.start_scale[i];

            ImageDB::DrawEx(image, particles.x[i], particles.y[i], particles.rotation[i],
                current_scale, current_scale, 0.5f, 0.5f,
                start_color_r + (end_color_r - start_color_r) * t,
                start_color_g + (end_color_g - start_color_g) * t,
                start_color_b + (end_color_b - start_color_b) * t,
                start_color_a + (end_color_a - start_color_a) * t,
                sorting_order);
        }
    }

    void Stop() {
        is_emitting = false;
    }

    void Play() {
        is_emitting = true;
    }

    void Burst() {
        GenerateNewParticles(burst_quantity);
    }

};

#endif // PARTICLE_SYSTEM_H