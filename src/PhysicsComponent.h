#pragma once
#include "raylib.h"
#include "raymath.h"

class PhysicsComponent {
public:
    Vector3 velocity = {0.0f, 0.0f, 0.0f};
    Vector3 acceleration = {0.0f, 0.0f, 0.0f};
    float mass = 1.0f;
    float gravityConstant = -9.81f; // Standard earth gravity
    bool useGravity = true;

    void Update(float deltaTime, Vector3& position) {
        // Apply gravity if enabled
        if (useGravity) {
            velocity.y += gravityConstant * deltaTime;
        }
        
        // Apply acceleration to velocity (v = v0 + a * t)
        velocity.x += acceleration.x * deltaTime;
        velocity.y += acceleration.y * deltaTime;
        velocity.z += acceleration.z * deltaTime;

        // Apply velocity to position (p = p0 + v * t)
        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;
        position.z += velocity.z * deltaTime;

        // Reset acceleration after applying
        acceleration = {0.0f, 0.0f, 0.0f};

        // Simple hardcoded primitive collision for ground (y = 0 plane).
        // Since cube size is 2.0x2.0x2.0, resting Y is 1.0
        if (position.y < 1.0f) {
            position.y = 1.0f;
            
            // Introduce a little bounce/damping if hitting hard
            if (velocity.y < -1.0f) {
                velocity.y = -velocity.y * 0.3f; // Bounce Dampening
            } else {
                velocity.y = 0.0f;
            }

            // Pseudo-friction
            velocity.x *= 0.95f;
            velocity.z *= 0.95f;
        }
    }

    void AddForce(Vector3 force) {
        // F = m * a  =>  a = F / m
        acceleration.x += force.x / mass;
        acceleration.y += force.y / mass;
        acceleration.z += force.z / mass;
    }
};
