#ifndef CAMERA_H
#define CAMERA_H

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 cross(const Vec3& v) const {
        return Vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }
    Vec3 normalized() const {
        float len = sqrt(x*x + y*y + z*z);
        return len > 0 ? Vec3(x/len, y/len, z/len) : Vec3(0, 0, 0);
    }
};

class Camera {
public:
    Vec3 position;
    float yaw;      // horizontal rotation (radians)
    float pitch;    // vertical rotation (radians)
    float fov;
    float sensitivity;
    
    Camera() : position(0, 1.6f, 3), yaw(0), pitch(0), fov(75.0f), sensitivity(0.002f) {}
    
    void ProcessMouse(float dx, float dy) {
        yaw += dx * sensitivity;
        pitch -= dy * sensitivity;
        
        // Clamp pitch
        const float maxPitch = M_PI * 0.49f;
        if (pitch > maxPitch) pitch = maxPitch;
        if (pitch < -maxPitch) pitch = -maxPitch;
    }
    
    Vec3 GetForward() const {
        return Vec3(
            sin(yaw) * cos(pitch),
            sin(pitch),
            -cos(yaw) * cos(pitch)
        );
    }
    
    Vec3 GetRight() const {
        Vec3 forward = GetForward();
        Vec3 up(0, 1, 0);
        return forward.cross(up).normalized();
    }
    
    void Move(float forward, float right, float dt) {
        Vec3 fwd = GetForward();
        Vec3 rgt = GetRight();
        
        // Remove vertical component for ground movement
        fwd.y = 0;
        fwd = fwd.normalized();
        
        position = position + fwd * forward * dt * 5.0f;  // 5 units/sec
        position = position + rgt * right * dt * 5.0f;
    }
    
    // Get view matrix (simplified - returns vectors for OpenGL)
    void GetViewMatrix(float* mat) const {
        Vec3 forward = GetForward();
        Vec3 right = GetRight();
        Vec3 up = right.cross(forward).normalized();
        
        Vec3 target = position + forward;
        
        // LookAt matrix (column-major for OpenGL)
        Vec3 f = (target - position).normalized();
        Vec3 r = f.cross(Vec3(0, 1, 0)).normalized();
        Vec3 u = r.cross(f);
        
        mat[0] = r.x;  mat[4] = r.y;  mat[8] = r.z;   mat[12] = -r.x*position.x - r.y*position.y - r.z*position.z;
        mat[1] = u.x;  mat[5] = u.y;  mat[9] = u.z;   mat[13] = -u.x*position.x - u.y*position.y - u.z*position.z;
        mat[2] = -f.x; mat[6] = -f.y; mat[10] = -f.z; mat[14] = f.x*position.x + f.y*position.y + f.z*position.z;
        mat[3] = 0;    mat[7] = 0;    mat[11] = 0;    mat[15] = 1;
    }
    
    void GetProjectionMatrix(float* mat, float aspect, float near = 0.1f, float far = 100.0f) const {
        float f = 1.0f / tan(fov * M_PI / 360.0f);
        
        mat[0] = f/aspect; mat[4] = 0;  mat[8] = 0;                          mat[12] = 0;
        mat[1] = 0;        mat[5] = f;  mat[9] = 0;                          mat[13] = 0;
        mat[2] = 0;        mat[6] = 0;  mat[10] = (far+near)/(near-far);    mat[14] = (2*far*near)/(near-far);
        mat[3] = 0;        mat[7] = 0;  mat[11] = -1;                        mat[15] = 0;
    }
};

#endif
