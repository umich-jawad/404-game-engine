#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include <string>
#include "box2d/box2d.h"

class Actor; // Forward declaration

struct Collision {
    Actor* other;
    b2Vec2 point;
    b2Vec2 relative_velocity;
    b2Vec2 normal;
};

class ContactListener : public b2ContactListener {
    void ProcessContact(b2Contact* contact, bool is_begin); // defined in Actor.h after Actor is complete

    void BeginContact(b2Contact* contact) override {
        ProcessContact(contact, true);
    }

    void EndContact(b2Contact* contact) override {
        ProcessContact(contact, false);
    }
};

class Rigidbody {
public:
    const static inline float RAD_TO_DEG = 180.0f / b2_pi;
    const static inline float DEG_TO_RAD = b2_pi / 180.0f;
    static inline b2World* physics_world = nullptr;
    static inline ContactListener contact_listener; // Single instance of the contact listener

    static void PhysicsStep() {
        if (physics_world) {
            physics_world->Step(1.0f / 60.0f, 8, 3); // Step the physics simulation with a fixed time step
        }
    }

    Rigidbody() {
        if (!physics_world) {
            physics_world = new b2World(b2Vec2(0.0f, 9.8f)); // Default gravity pointing downwards
            physics_world->SetContactListener(&contact_listener); // Set the contact listener for the physics world
        }
    }

    float x = 0.0f;
    float y = 0.0f;
    std::string body_type = "dynamic";
    bool precise = true;
    float gravity_scale = 1.0f;
    float density = 1.0f;
    float angular_friction = 0.3f;
    float rotation = 0.0f;

    // temp variables to store initial values before body creation
    b2Vec2 initial_velocity = b2Vec2(0.0f, 0.0f);
    float angular_velocity = 0.0f;


    bool has_collider = true;
    bool has_trigger = true;

    std::string collider_type = "box";
    float width = 1.0f;
    float height = 1.0f;
    float radius = 0.5f;
    float friction = 0.3f;
    float bounciness = 0.3f;

    std::string trigger_type = "box";
    float trigger_width = 1.0f;
    float trigger_height = 1.0f;
    float trigger_radius = 0.5f;


    bool enabled = true;
    std::string key = "";
    std::string type = "Rigidbody";
    Actor* actor = nullptr;

    b2Body* body = nullptr;

    void OnStart() {
        b2BodyDef body_def;
        body_def.position.Set(x, y);
        body_def.angle = rotation * DEG_TO_RAD; // Convert degrees to radians
        body_def.type = (body_type == "static") ? b2_staticBody : (body_type == "kinematic") ? b2_kinematicBody : b2_dynamicBody;
        body_def.bullet = precise;
        body_def.angularDamping = angular_friction;
        body_def.gravityScale = gravity_scale;

        body = physics_world->CreateBody(&body_def);
        SetAngularVelocity(angular_velocity);
        SetVelocity(initial_velocity);

        if (has_collider) {
            if (collider_type == "box") {
                b2PolygonShape box_shape;
                box_shape.SetAsBox(width * 0.5f, height * 0.5f);

                CreateFixture(&box_shape, false);
            } else if (collider_type == "circle") {
                b2CircleShape circle_shape;
                circle_shape.m_radius = radius;

                CreateFixture(&circle_shape, false);
            }
        }

        if (has_trigger) {
            if (trigger_type == "box") {
                b2PolygonShape trigger_box_shape;
                trigger_box_shape.SetAsBox(trigger_width * 0.5f, trigger_height * 0.5f);

                CreateFixture(&trigger_box_shape, true);
            } else if (trigger_type == "circle") {
                b2CircleShape trigger_circle_shape;
                trigger_circle_shape.m_radius = trigger_radius;

                CreateFixture(&trigger_circle_shape, true);
            }
        }

        // phantom sensor to make bodies move if neither collider nor trigger is present
        if (!has_collider && !has_trigger)
        {
            b2PolygonShape phantom_shape;
            phantom_shape.SetAsBox(width * 0.5f, height * 0.5f);

            b2FixtureDef fixture_def;
            fixture_def.shape = &phantom_shape;
            fixture_def.density = density;
            fixture_def.isSensor = true;
            // userData.pointer left as 0 — marks this as a phantom fixture
            body->CreateFixture(&fixture_def);
        }

    }

    void OnDestroy() {
        if (body) {
            physics_world->DestroyBody(body);
            body = nullptr;
        }
    }

    void CreateFixture(const b2Shape* shape, bool is_trigger) {
        if (body) {
            b2FixtureDef fixture_def;
            fixture_def.shape = shape;
            fixture_def.density = density;
            fixture_def.friction = friction;
            fixture_def.restitution = bounciness;
            fixture_def.isSensor = is_trigger;
            fixture_def.userData.pointer = reinterpret_cast<uintptr_t>(actor); // Store a pointer to the actor in the fixture's user data

            body->CreateFixture(&fixture_def);
        }
    }


    void AddForce(b2Vec2 force) {
        body->ApplyForceToCenter(force, true);
    }

    void SetVelocity(b2Vec2 velocity) {
        if (!body) {
            initial_velocity = velocity;
            return; // If body is not created yet, just update the initial velocity
        }
        body->SetLinearVelocity(velocity);
    }

    void SetPosition(b2Vec2 position) {
        if (!body) {
            x = position.x;
            y = position.y;
            return; // If body is not created yet, just update the initial position
        }
        body->SetTransform(position, body->GetAngle());
    }

    void SetRotation(float rot) {
        if (!body) {
            rotation = rot;
            return; // If body is not created yet, just update the initial rotation
        }
        body->SetTransform(body->GetPosition(), rot * DEG_TO_RAD); // Convert degrees to radians
    }

    void SetAngularVelocity(float angular_velocity) {
        if (!body) {
            this->angular_velocity = angular_velocity;
            return; // If body is not created yet, just update the initial angular velocity
        }
        body->SetAngularVelocity(angular_velocity * DEG_TO_RAD); // Convert degrees to radians
    }

    void SetGravityScale(float scale) {
        if (!body) {
            gravity_scale = scale;
            return; // If body is not created yet, just update the initial gravity scale
        }
        body->SetGravityScale(scale);
    }

    void SetUpDirection(b2Vec2 up) {
        up.Normalize();
        float angle_rad = glm::atan(up.x, -up.y); // Calculate angle from up vector
        body->SetTransform(body->GetPosition(), angle_rad);
    }

    void SetRightDirection(b2Vec2 right) {
        right.Normalize();
        float angle_rad = glm::atan(right.x, -right.y) - b2_pi / 2.0f; // Calculate angle from right vector
        body->SetTransform(body->GetPosition(), angle_rad);
    }

    b2Vec2 GetPosition() {
        if (!body) return b2Vec2(x, y); // Return initial position if body is not created yet
        return body->GetPosition();
    }

    float GetRotation() {
        if (!body) return rotation; // Return initial rotation if body is not created yet
        return body->GetAngle() * RAD_TO_DEG; // Convert radians to degrees
    }

    b2Vec2 GetVelocity() {
        if (!body) return initial_velocity; // Return initial velocity if body is not created yet
        return body->GetLinearVelocity();
    }

    float GetAngularVelocity() {
        if (!body) return angular_velocity; // Return initial angular velocity if body is not created yet
        return body->GetAngularVelocity() * RAD_TO_DEG; // Convert radians to degrees
    }

    float GetGravityScale() {
        if (!body) return gravity_scale; // Return initial gravity scale if body is not created yet
        return body->GetGravityScale();
    }

    b2Vec2 GetUpDirection() {
        if (!body) return b2Vec2(0, -1); // Return default up direction if body is not created yet
        float angle_rad = body->GetAngle();
        b2Vec2 up(glm::sin(angle_rad), -glm::cos(angle_rad));
        up.Normalize();
        return up; // Up direction based on rotation
    }

    b2Vec2 GetRightDirection() {
        if (!body) return b2Vec2(1, 0); // Return default right direction if body is not created yet
        float angle_rad = body->GetAngle();
        b2Vec2 right(glm::cos(angle_rad), glm::sin(angle_rad));
        right.Normalize();
        return right; // Right direction based on rotation
    }

};

#endif // RIGIDBODY_H
