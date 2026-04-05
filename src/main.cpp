#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <array>
#include <string>
#include <iostream>

#include "functionHelpers/funcGen.h"
#include "data.h"
 
// ─────────────────────────────────────────────
// Minimal math types
// ─────────────────────────────────────────────
struct Vec3 { float x, y, z; };
struct Vec4 { float x, y, z, w; };
struct Projected { float px, py, ndcZ, w; };
 
// Column-major 4×4 matrix  m[col][row]
struct Mat4 {
    float m[4][4] = {};
 
    float& at(int row, int col)       { return m[col][row]; }
    float  at(int row, int col) const { return m[col][row]; }
 
    static Mat4 identity() {
        Mat4 r;
        r.at(0,0) = r.at(1,1) = r.at(2,2) = r.at(3,3) = 1.f;
        return r;
    }
};

 
Mat4 mul(const Mat4& a, const Mat4& b) {
    Mat4 r;
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            for (int k = 0; k < 4; ++k)
                r.at(row,col) += a.at(row,k) * b.at(k,col);
    return r;
}
 
Vec4 mul(const Mat4& m, Vec4 v) {
    return {
        m.at(0,0)*v.x + m.at(0,1)*v.y + m.at(0,2)*v.z + m.at(0,3)*v.w,
        m.at(1,0)*v.x + m.at(1,1)*v.y + m.at(1,2)*v.z + m.at(1,3)*v.w,
        m.at(2,0)*v.x + m.at(2,1)*v.y + m.at(2,2)*v.z + m.at(2,3)*v.w,
        m.at(3,0)*v.x + m.at(3,1)*v.y + m.at(3,2)*v.z + m.at(3,3)*v.w,
    };
}
 
float dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
float len(Vec3 v) { return std::sqrt(dot(v, v)); }
Vec3 norm(Vec3 v) { float l = len(v); return {v.x/l, v.y/l, v.z/l}; }
Vec3 cross(Vec3 a, Vec3 b) {
    return { a.y*b.z - a.z*b.y,
             a.z*b.x - a.x*b.z,
             a.x*b.y - a.y*b.x };
}
Vec3 sub(Vec3 a, Vec3 b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }

// View matrix: camera at `eye` looking at `center`, with `up` hint
// Builds an orthonormal basis (right, up, forward) then inverts it
//   as a rigid-body transform (transpose of rotation + negated translation).
Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 upHint) {
    Vec3 f = norm(sub(center, eye));  // forward (into scene)
    Vec3 r = norm(cross(f, upHint)); // right
    Vec3 u = cross(r, f);            // true up (re-orthogonalised)
 
    // The view matrix transforms world coords into camera space.
    // Camera axes become the new basis rows; eye position is negated.
    Mat4 v = Mat4::identity();
    v.at(0,0) =  r.x;  v.at(0,1) =  r.y;  v.at(0,2) =  r.z;
    v.at(1,0) =  u.x;  v.at(1,1) =  u.y;  v.at(1,2) =  u.z;
    v.at(2,0) = -f.x;  v.at(2,1) = -f.y;  v.at(2,2) = -f.z;
    // Translation part: dot product accounts for camera position offset
    v.at(0,3) = -dot(r, eye);
    v.at(1,3) = -dot(u, eye);
    v.at(2,3) =  dot(f, eye);
    return v;
}
 
// Perspective projection matrix (right-handed, maps z into [-1,1] NDC)
// Derived from similar-triangles scaling (x' = near*x/z, y' = near*y/z)
// packed into a matrix so the perspective divide (÷w) happens after mul.
//
//  fovY   — vertical field of view in radians
//  aspect — width / height
//  near, far — clipping planes
Mat4 perspective(float fovY, float aspect, float near, float far) {
    float tanHalf = std::tan(fovY * 0.5f);
    Mat4 p;
    // x scaling: 1 / (aspect * tan(fov/2))
    p.at(0,0) = 1.f / (aspect * tanHalf);
    // y scaling: 1 / tan(fov/2)
    p.at(1,1) = 1.f / tanHalf;
    // z remapping into [-1, 1]:  -(far+near)/(far-near)
    p.at(2,2) = -(far + near) / (far - near);
    // z translation term:        -2*far*near/(far-near)
    p.at(2,3) = -2.f * far * near / (far - near);
    // Perspective divide trigger: stores z into w so dividing by w = dividing by z
    p.at(3,2) = -1.f;
    return p;
}

// Returns {pixel_x, pixel_y, depth_ndc, w}.  w > 0 means in front of camera.
//
// Pipeline:
//   1. Multiply by View  → camera/eye space
//   2. Multiply by Proj  → clip space (homogeneous)
//   3. Perspective divide (÷w) → NDC  [-1,1]³
//   4. Viewport transform → window pixels
//      px = (x_ndc + 1) / 2 * width
//      py = (1 - y_ndc) / 2 * height   ← y flipped (screen y grows down)
 
Projected project(Vec3 worldPos, const Mat4& view, const Mat4& proj,
                  float winWidth, float winHeight)
{
    // Step 1 & 2: world → clip space
    Vec4 worldView = {worldPos.x, worldPos.y, worldPos.z, 1.f};
    Vec4 clip = mul(proj, mul(view, worldView));
 
    // Step 3: perspective divide → NDC
    float ndcX = clip.x / clip.w;
    float ndcY = clip.y / clip.w;
    float ndcZ = clip.z / clip.w;
 
    // Step 4: viewport transform
    float px = (ndcX + 1.f) * 0.5f * winWidth;
    float py = (1.f - ndcY) * 0.5f * winHeight;  // flip y
 
    return { px, py, ndcZ, clip.w };
}
 

void drawLine(sf::RenderWindow& win,
              sf::Vector2f a, sf::Vector2f b,
              sf::Color color, float thickness = 2.f)
{
    sf::Vector2f dir = b - a;
    float length = std::hypot(dir.x, dir.y);
    if (length < 0.001f) return;
 
    sf::RectangleShape line({length, thickness});
    line.setOrigin({0.f, thickness * 0.5f});
    line.setPosition(a);
    line.setRotation(sf::radians(std::atan2(dir.y, dir.x)));
    line.setFillColor(color);
    win.draw(line);
}
 
void drawLabel(sf::RenderWindow& win, const sf::Font& font,
               const std::string& text, sf::Vector2f pos, sf::Color color)
{
    sf::Text label(font, text, 20);
    label.setFillColor(color);
    label.setPosition(pos + sf::Vector2f(6.f, -10.f));
    win.draw(label);
}

/*
    So, here we have world coordinates system with Y axe going up and XZ surface.
    We will need to set a function (non linear, of course) and draw it using triangles.
    Then we will run a training process and visualize how it converges to the original function y=f(x, z).
*/
int main()
{
    constexpr unsigned W = 900, H = 700;
    const float draggingSpeed = 0.007f;
    sf::RenderWindow window(sf::VideoMode({W, H}), "Adam optimizer work visualization");
    window.setFramerateLimit(60);

 
    sf::Font font;
    // Try to load a system font; fall back gracefully if not found
    bool fontLoaded = font.openFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    if (!fontLoaded)
        fontLoaded = font.openFromFile("C:/Windows/Fonts/arial.ttf");
 
    // ── Camera state ──────────────────────────────
    float yaw    =  0.4f;   // horizontal rotation around Y (parallel to the 0-surface)
    float pitch  =  0.35f;  // vertical
    float radius =  3.5f;   // distance from origin
    const float pitchMin =  0.05f;
    const float pitchMax =  1.55f;  // just under π/2, can't look under the 0-surface
 
    bool dragging = false;
    sf::Vector2i lastMouse;
 
    // Each axis: origin → positive tip → negative tip
    const Vec3 origin  = {0, 0, 0};
    const Vec3 xPos    = {1.2f,  0,     0};
    const Vec3 yPos    = {0,     1.2f,  0};
    const Vec3 zPos    = {0,     0,     1.2f};
    const Vec3 xNeg    = {-0.4f, 0,     0};
    const Vec3 yNeg    = {0,    -0.4f,  0};
    const Vec3 zNeg    = {0,     0,    -0.4f};
 
    const sf::Color colX(220, 60,  60);   // red
    const sf::Color colY(60,  200, 80);   // green
    const sf::Color colZ(60,  130, 220);  // blue
    const sf::Color colGrid(60, 60, 60);

    Smooth3DFunction originalFunction = getRandomFunction(10, 10);
    std::vector<Projected> functionProjectedPoints(originalFunction.nx * originalFunction.nz);
    for (auto vertex: originalFunction.vertices) {
        std::cout << vertex.x << " " << vertex.y << " " << vertex.z << "\n";
    }
 
    while (window.isOpen())
    {
        // ── Events ───────────────────────────────────
        while (auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
 
            if (auto* mb = event->getIf<sf::Event::MouseButtonPressed>())
                if (mb->button == sf::Mouse::Button::Left) {
                    dragging = true;
                    lastMouse = mb->position;
                }
            if (auto* mb = event->getIf<sf::Event::MouseButtonReleased>())
                if (mb->button == sf::Mouse::Button::Left)
                    dragging = false;
 
            if (auto* mm = event->getIf<sf::Event::MouseMoved>()) {
                if (dragging) {
                    sf::Vector2i delta = mm->position - lastMouse;
                    yaw -= delta.x * draggingSpeed;
                    pitch += delta.y * draggingSpeed;
                    pitch = std::clamp(pitch, pitchMin, pitchMax);
                    lastMouse = mm->position;
                }
            }
 
            if (auto* sw = event->getIf<sf::Event::MouseWheelScrolled>())
                radius = std::clamp(radius - sw->delta * 0.25f, 1.0f, 12.f);
        }
 
        // ── Build MVP matrices ────────────────────────
        // Convert spherical (yaw, pitch, radius) → Cartesian eye position
        Vec3 eye = {
            radius * std::cos(pitch) * std::sin(yaw),
            radius * std::sin(pitch),
            radius * std::cos(pitch) * std::cos(yaw)
        };
 
        Mat4 view = lookAt(eye, {0,0,0}, {0,1,0});
        Mat4 proj = perspective(
            0.8f,                          // ~46° vertical FOV
            float(W) / float(H),           // aspect ratio
            0.1f, 100.f                    // near / far planes
        );
 
        // ── Lambda: project + draw a line segment ────
        auto drawSeg = [&](Vec3 a, Vec3 b, sf::Color col) {
            auto pa = project(a, view, proj, W, H);
            auto pb = project(b, view, proj, W, H);
            // Clip: only draw if both endpoints are in front of camera
            if (pa.w > 0 && pb.w > 0)
                drawLine(window,
                         {pa.px, pa.py},
                         {pb.px, pb.py},
                         col, 2.5f);
        };
 
        // ── Render ───────────────────────────────────
        window.clear(sf::Color(18, 18, 22));
 
        // Optional: small XZ grid
        for (int i = -4; i <= 4; ++i) {
            float f = float(i) * 0.3f;
            drawSeg({f,  0.f, -1.2f}, {f,  0.f,  1.2f}, colGrid);
            drawSeg({-1.2f, 0.f, f},  { 1.2f, 0.f, f},  colGrid);
        }
 
        // Axes (negative part dashed-style: drawn shorter + dimmer)
        auto dim = [](sf::Color c) { return sf::Color(c.r/3, c.g/3, c.b/3); };
        drawSeg(xNeg, origin, dim(colX));
        drawSeg(yNeg, origin, dim(colY));
        drawSeg(zNeg, origin, dim(colZ));
        drawSeg(origin, xPos, colX);
        drawSeg(origin, yPos, colY);
        drawSeg(origin, zPos, colZ);
 
        // Origin dot
        {
            auto po = project(origin, view, proj, W, H);
            if (po.w > 0) {
                sf::CircleShape dot(4.f);
                dot.setFillColor(sf::Color(200, 200, 200));
                dot.setOrigin({4.f, 4.f});
                dot.setPosition({po.px, po.py});
                window.draw(dot);
            }
        }
 
        // Axis tip dots + labels
        if (fontLoaded) {
            auto drawTip = [&](Vec3 p, sf::Color col, const std::string& label) {
                auto pp = project(p, view, proj, W, H);
                if (pp.w > 0) {
                    sf::CircleShape dot(5.f);
                    dot.setFillColor(col);
                    dot.setOrigin({5.f, 5.f});
                    dot.setPosition({pp.px, pp.py});
                    window.draw(dot);
                    drawLabel(window, font, label, {pp.px, pp.py}, col);
                }
            };
            drawTip(xPos, colX, "X");
            drawTip(yPos, colY, "Y");
            drawTip(zPos, colZ, "Z");
        }
 
        // HUD
        if (fontLoaded) {
            sf::Text hud(font,
                "Drag: orbit    Scroll: zoom\n"
                "Eye: (" + std::to_string(int(eye.x*10)/10.f).substr(0,4) + ", "
                         + std::to_string(int(eye.y*10)/10.f).substr(0,4) + ", "
                         + std::to_string(int(eye.z*10)/10.f).substr(0,4) + ")",
                14);
            hud.setFillColor(sf::Color(140, 140, 150));
            hud.setPosition({12.f, 12.f});
            window.draw(hud);
        }

        for (__uint32_t i = 0; i < originalFunction.indices.size(); i += 3) {
            Vertex3D vertex1 = originalFunction.vertices[originalFunction.indices[i]];
            Vertex3D vertex2 = originalFunction.vertices[originalFunction.indices[i + 1]];
            Vertex3D vertex3 = originalFunction.vertices[originalFunction.indices[i + 2]];
            
            Projected vp1 = project({vertex1.x, vertex1.y, vertex1.z}, view, proj, W, H);
            Projected vp2 = project({vertex2.x, vertex2.y, vertex2.z}, view, proj, W, H);
            Projected vp3 = project({vertex3.x, vertex3.y, vertex3.z}, view, proj, W, H);
            
            sf::VertexArray triangle(sf::PrimitiveType::Triangles, 3);
            triangle[0].position = sf::Vector2f(vp1.px, vp1.py);
            triangle[1].position = sf::Vector2f(vp3.px, vp2.py);
            triangle[2].position = sf::Vector2f(vp2.px, vp3.py);

            triangle[0].color = sf::Color::Red;
            triangle[1].color = sf::Color::Blue;
            triangle[2].color = sf::Color::Green;

            window.draw(triangle);
        }
 
        window.display();
    }
    return 0;
}