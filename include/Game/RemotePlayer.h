// RemotePlayer.h - Network-controlled player with interpolation

#pragma once

#include "Player.h"
#include <GL/gl.h>
#include <cmath>

namespace Game {

class RemotePlayer : public Player {
private:
    // Network interpolation
    PCD::Vec3 targetPosition;
    float targetYaw;
    float targetPitch;
    
    float interpolationSpeed; // How fast to interpolate to target

public:
    RemotePlayer(uint32_t id, const std::string& name)
        : Player(id, name)
        , targetPosition(0, 0, 0)
        , targetYaw(0.0f)
        , targetPitch(0.0f)
        , interpolationSpeed(10.0f) // Interpolate over ~100ms
    {
    }
    
    void Update(float deltaTime) override {
        if (!isAlive) return;
        
        // Smoothly interpolate to target position
        float t = std::min(1.0f, interpolationSpeed * deltaTime);
        
        position.x = position.x + (targetPosition.x - position.x) * t;
        position.y = position.y + (targetPosition.y - position.y) * t;
        position.z = position.z + (targetPosition.z - position.z) * t;
        
        // Interpolate rotation
        yaw = yaw + (targetYaw - yaw) * t;
        pitch = pitch + (targetPitch - pitch) * t;
    }
    
    void Render() override {
        if (!isAlive) return;
        
        glPushMatrix();
        
        // Move to player position
        glTranslatef(position.x, position.y, position.z);
        
        // Rotate to face direction (yaw only - body doesn't pitch)
        glRotatef(yaw * 180.0f / 3.14159f, 0, 1, 0);
        
        // Set player color
        glColor3f(colorR, colorG, colorB);
        
        // Draw body (simple box for now)
        DrawPlayerBody();
        
        // Draw head (separate so it can look up/down later)
        glPushMatrix();
        glTranslatef(0, bodyHeight * 0.85f, 0); // Head at top
        DrawPlayerHead();
        glPopMatrix();
        
        // Draw name tag above head
        glPushMatrix();
        glTranslatef(0, bodyHeight + 0.3f, 0);
        DrawNameTag();
        glPopMatrix();
        
        glPopMatrix();
    }
    
    // Update from network data
    void UpdateFromNetwork(float x, float y, float z, float newYaw, float newPitch, int newHealth) {
        targetPosition.x = x;
        targetPosition.y = y;
        targetPosition.z = z;
        targetYaw = newYaw;
        targetPitch = newPitch;
        health = newHealth;
    }
    
private:
    void DrawPlayerBody() {
        // Draw a simple colored box (body)
        float hw = bodyWidth / 2.0f;
        float h = bodyHeight * 0.7f; // Body is 70% of total height
        
        glBegin(GL_QUADS);
        
        // Front
        glVertex3f(-hw, 0, hw);
        glVertex3f(hw, 0, hw);
        glVertex3f(hw, h, hw);
        glVertex3f(-hw, h, hw);
        
        // Back
        glVertex3f(hw, 0, -hw);
        glVertex3f(-hw, 0, -hw);
        glVertex3f(-hw, h, -hw);
        glVertex3f(hw, h, -hw);
        
        // Left
        glVertex3f(-hw, 0, -hw);
        glVertex3f(-hw, 0, hw);
        glVertex3f(-hw, h, hw);
        glVertex3f(-hw, h, -hw);
        
        // Right
        glVertex3f(hw, 0, hw);
        glVertex3f(hw, 0, -hw);
        glVertex3f(hw, h, -hw);
        glVertex3f(hw, h, hw);
        
        // Top
        glVertex3f(-hw, h, hw);
        glVertex3f(hw, h, hw);
        glVertex3f(hw, h, -hw);
        glVertex3f(-hw, h, -hw);
        
        // Bottom
        glVertex3f(-hw, 0, -hw);
        glVertex3f(hw, 0, -hw);
        glVertex3f(hw, 0, hw);
        glVertex3f(-hw, 0, hw);
        
        glEnd();
        
        // Draw outline
        glColor3f(0, 0, 0);
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        glVertex3f(-hw, 0, hw);
        glVertex3f(hw, 0, hw);
        glVertex3f(hw, h, hw);
        glVertex3f(-hw, h, hw);
        glEnd();
        glBegin(GL_LINE_LOOP);
        glVertex3f(-hw, 0, -hw);
        glVertex3f(hw, 0, -hw);
        glVertex3f(hw, h, -hw);
        glVertex3f(-hw, h, -hw);
        glEnd();
    }
    
    void DrawPlayerHead() {
        // Draw a smaller box for head
        float hs = bodyWidth / 3.0f; // Head size
        
        // Slightly brighter color for head
        glColor3f(
            std::min(1.0f, colorR * 1.2f),
            std::min(1.0f, colorG * 1.2f),
            std::min(1.0f, colorB * 1.2f)
        );
        
        glBegin(GL_QUADS);
        
        // Simple cube for head
        glVertex3f(-hs, -hs, hs);
        glVertex3f(hs, -hs, hs);
        glVertex3f(hs, hs, hs);
        glVertex3f(-hs, hs, hs);
        
        glVertex3f(hs, -hs, -hs);
        glVertex3f(-hs, -hs, -hs);
        glVertex3f(-hs, hs, -hs);
        glVertex3f(hs, hs, -hs);
        
        glVertex3f(-hs, -hs, -hs);
        glVertex3f(-hs, -hs, hs);
        glVertex3f(-hs, hs, hs);
        glVertex3f(-hs, hs, -hs);
        
        glVertex3f(hs, -hs, hs);
        glVertex3f(hs, -hs, -hs);
        glVertex3f(hs, hs, -hs);
        glVertex3f(hs, hs, hs);
        
        glVertex3f(-hs, hs, hs);
        glVertex3f(hs, hs, hs);
        glVertex3f(hs, hs, -hs);
        glVertex3f(-hs, hs, -hs);
        
        glVertex3f(-hs, -hs, -hs);
        glVertex3f(hs, -hs, -hs);
        glVertex3f(hs, -hs, hs);
        glVertex3f(-hs, -hs, hs);
        
        glEnd();
    }
    
    void DrawNameTag() {
        // TODO: Render text with player name
        // For now, just draw a small white square above player
        glColor3f(1, 1, 1);
        glBegin(GL_QUADS);
        glVertex3f(-0.2f, 0, 0);
        glVertex3f(0.2f, 0, 0);
        glVertex3f(0.2f, 0.1f, 0);
        glVertex3f(-0.2f, 0.1f, 0);
        glEnd();
    }
};

} // namespace Game
