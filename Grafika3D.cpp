#include "pch.h"

typedef sf::Event    sfe;
typedef sf::Keyboard sfk;

// Parametry kamery
static float g_R = 6.0f;
static float g_theta = 90.0f;
static float g_phi = 90.0f;
static bool g_orthographic = false;
static float g_fov = 60.0f;

// Parametry transformacji sceny
static float g_scaleX = 1.0f;
static float g_scaleY = 1.0f;
static float g_scaleZ = 1.0f;

static float g_posX = 0.0f;
static float g_posY = 0.0f;
static float g_posZ = 0.0f;

static float g_rotX = 0.0f;
static float g_rotY = 0.0f;
static float g_rotZ = 0.0f;

// Kontrolki widocznoœci obiektów
static bool g_showTriangle = true;
static bool g_showCube = true;
static bool g_showCone = true;
static bool g_showSphere = true;

static unsigned g_width = 1024;
static unsigned g_height = 768;

static inline float deg2rad(float d) { return d * 3.1415926535f / 180.0f; }

// ---------- Zamienniki GLU ----------
static void perspectiveGL(double fovY, double aspect, double zNear, double zFar)
{
    const double fH = tan(fovY * 0.5 * 3.1415926535 / 180.0) * zNear;
    const double fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}

static void lookAtGL(float eyeX, float eyeY, float eyeZ,
    float cx, float cy, float cz,
    float upX, float upY, float upZ)
{
    float f[3] = { cx - eyeX, cy - eyeY, cz - eyeZ };
    float fn = 1.0f / sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    f[0] *= fn; f[1] *= fn; f[2] *= fn;

    float up[3] = { upX, upY, upZ };
    float un = 1.0f / sqrtf(up[0] * up[0] + up[1] * up[1] + up[2] * up[2]);
    up[0] *= un; up[1] *= un; up[2] *= un;

    float s[3] = { f[1] * up[2] - f[2] * up[1],
                   f[2] * up[0] - f[0] * up[2],
                   f[0] * up[1] - f[1] * up[0] };
    float sn = 1.0f / sqrtf(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
    s[0] *= sn; s[1] *= sn; s[2] *= sn;

    float u[3] = { s[1] * f[2] - s[2] * f[1],
                   s[2] * f[0] - s[0] * f[2],
                   s[0] * f[1] - s[1] * f[0] };

    float M[16] = {
        s[0],  u[0], -f[0], 0.0f,
        s[1],  u[1], -f[1], 0.0f,
        s[2],  u[2], -f[2], 0.0f,
        0.0f,  0.0f,  0.0f, 1.0f
    };
    glMultMatrixf(M);
    glTranslatef(-eyeX, -eyeY, -eyeZ);
}
// ------------------------------------------------------------------

static void setupProjection(unsigned w, unsigned h)
{
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspect = (h == 0) ? 1.f : (float)w / (float)h;

    if (g_orthographic)
    {
        const float orthoSize = 2.5f;
        if (aspect >= 1.0f)
        {
            glOrtho(-orthoSize * aspect, orthoSize * aspect,
                -orthoSize, orthoSize,
                0.1, 100.0);
        }
        else
        {
            glOrtho(-orthoSize, orthoSize,
                -orthoSize / aspect, orthoSize / aspect,
                0.1, 100.0);
        }
    }
    else
    {
        perspectiveGL(g_fov, aspect, 0.1, 100.0);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void setupGL()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glClearDepth(1.0);
    glDepthRange(0.0, 1.0);

    glDisable(GL_BLEND);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glClearColor(0.95f, 0.97f, 0.99f, 1.0f);

    glShadeModel(GL_SMOOTH);
}

static void drawAxes(float lenPos = 2.0f, float lenNeg = 2.0f)
{
    glDisable(GL_DEPTH_TEST);

    glLineWidth(1.5f);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x00FF);
    glBegin(GL_LINES);
    glColor3f(0.7f, 0.2f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(-lenNeg, 0, 0);
    glColor3f(0.2f, 0.7f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(0, -lenNeg, 0);
    glColor3f(0.2f, 0.2f, 0.7f); glVertex3f(0, 0, 0); glVertex3f(0, 0, -lenNeg);
    glEnd();
    glDisable(GL_LINE_STIPPLE);

    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.2f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(lenPos, 0, 0);
    glColor3f(0.2f, 0.9f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(0, lenPos, 0);
    glColor3f(0.2f, 0.4f, 1.0f); glVertex3f(0, 0, 0); glVertex3f(0, 0, lenPos);
    glEnd();
    glLineWidth(1.0f);

    glEnable(GL_DEPTH_TEST);
}

static void drawCubeWire(float s = 1.2f)
{
    const float a = s * 0.5f;

    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0f, -1.0f);

    glLineWidth(2.5f);
    glColor3f(0.10f, 0.10f, 0.10f);
    glBegin(GL_LINES);
    // dolna podstawa
    glVertex3f(-a, -a, -a); glVertex3f(a, -a, -a);
    glVertex3f(a, -a, -a); glVertex3f(a, -a, a);
    glVertex3f(a, -a, a); glVertex3f(-a, -a, a);
    glVertex3f(-a, -a, a); glVertex3f(-a, -a, -a);

    // górna podstawa
    glVertex3f(-a, a, -a); glVertex3f(a, a, -a);
    glVertex3f(a, a, -a); glVertex3f(a, a, a);
    glVertex3f(a, a, a); glVertex3f(-a, a, a);
    glVertex3f(-a, a, a); glVertex3f(-a, a, -a);

    // krawêdzie pionowe
    glVertex3f(-a, -a, -a); glVertex3f(-a, a, -a);
    glVertex3f(a, -a, -a); glVertex3f(a, a, -a);
    glVertex3f(a, -a, a); glVertex3f(a, a, a);
    glVertex3f(-a, -a, a); glVertex3f(-a, a, a);
    glEnd();
    glLineWidth(1.0f);

    glDisable(GL_POLYGON_OFFSET_LINE);
}

static void drawInternalTriangle(float s = 1.2f)
{
    const float a = s * 0.5f;
    const GLfloat v1[3] = { +a, +a, +a };
    const GLfloat v2[3] = { -a, -a, +a };
    const GLfloat v3[3] = { +a, -a, -a };

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    glBegin(GL_TRIANGLES);
    glColor3f(1.f, 0.f, 0.f); glVertex3fv(v1);
    glColor3f(0.f, 1.f, 0.f); glVertex3fv(v2);
    glColor3f(0.f, 0.f, 1.f); glVertex3fv(v3);
    glEnd();
}

// Rysowanie sto¿ka
static void drawCone(float radius = 0.4f, float height = 0.8f, int segments = 32)
{
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    // Wierzcho³ek sto¿ka
    const float apex_x = 0.0f;
    const float apex_y = height / 2.0f;
    const float apex_z = 0.0f;

    // Œrodek podstawy
    const float base_y = -height / 2.0f;

    // Rysowanie œcian bocznych sto¿ka - ZIELONY gradient
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < segments; i++)
    {
        float angle1 = (float)i / segments * 2.0f * 3.14159265f;
        float angle2 = (float)(i + 1) / segments * 2.0f * 3.14159265f;

        float x1 = radius * cosf(angle1);
        float z1 = radius * sinf(angle1);
        float x2 = radius * cosf(angle2);
        float z2 = radius * sinf(angle2);

        // Kolor gradientowy - ró¿ne odcienie zieleni
        float t1 = (float)i / segments;
        float t2 = (float)(i + 1) / segments;

        // Wierzcho³ek - jasna zieleñ
        glColor3f(0.3f, 1.0f, 0.3f);
        glVertex3f(apex_x, apex_y, apex_z);

        // Podstawa - gradient zieleni
        glColor3f(0.0f, 0.7f + 0.3f * (1.0f - t1), 0.0f);
        glVertex3f(x1, base_y, z1);

        glColor3f(0.0f, 0.7f + 0.3f * (1.0f - t2), 0.0f);
        glVertex3f(x2, base_y, z2);
    }
    glEnd();

    // Rysowanie podstawy sto¿ka - ciemnozielona
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex3f(0.0f, base_y, 0.0f);  // œrodek podstawy
    for (int i = 0; i <= segments; i++)
    {
        float angle = (float)i / segments * 2.0f * 3.14159265f;
        float x = radius * cosf(angle);
        float z = radius * sinf(angle);
        glVertex3f(x, base_y, z);
    }
    glEnd();
}

// Rysowanie sfery (kuli)
static void drawSphere(float radius = 0.4f, int slices = 32, int stacks = 16)
{
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    for (int i = 0; i < stacks; i++)
    {
        float lat1 = 3.14159265f * (-0.5f + (float)i / stacks);
        float lat2 = 3.14159265f * (-0.5f + (float)(i + 1) / stacks);

        float y1 = radius * sinf(lat1);
        float y2 = radius * sinf(lat2);
        float r1 = radius * cosf(lat1);
        float r2 = radius * cosf(lat2);

        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= slices; j++)
        {
            float lng = 2.0f * 3.14159265f * (float)j / slices;
            float x1 = r1 * cosf(lng);
            float z1 = r1 * sinf(lng);
            float x2 = r2 * cosf(lng);
            float z2 = r2 * sinf(lng);

            // Kolor czerwony z gradientem jasnoœci od góry do do³u
            float t1 = (float)i / stacks;
            float t2 = (float)(i + 1) / stacks;

            // Gradient od jasnoczerwonego (góra) do ciemnoczerwonego (dó³)
            glColor3f(1.0f, 0.3f * (1.0f - t1), 0.3f * (1.0f - t1));
            glVertex3f(x1, y1, z1);

            glColor3f(1.0f, 0.3f * (1.0f - t2), 0.3f * (1.0f - t2));
            glVertex3f(x2, y2, z2);
        }
        glEnd();
    }
}

static void renderScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    const float th = deg2rad(g_theta);
    const float ph = deg2rad(g_phi);
    const float x = g_R * sinf(th) * cosf(ph);
    const float y = g_R * cosf(th);
    const float z = g_R * sinf(th) * sinf(ph);

    lookAtGL(x, y, z, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f);

    glPushMatrix();

    glTranslatef(g_posX, g_posY, g_posZ);
    glRotatef(g_rotX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_rotY, 0.0f, 1.0f, 0.0f);
    glRotatef(g_rotZ, 0.0f, 0.0f, 1.0f);
    glScalef(g_scaleX, g_scaleY, g_scaleZ);

    // Rysowanie obiektów w zale¿noœci od ustawieñ widocznoœci
    if (g_showTriangle)
    {
        drawInternalTriangle();
    }

    if (g_showCube)
    {
        drawCubeWire();
    }

    if (g_showCone)
    {
        glPushMatrix();
        glTranslatef(-0.9f, 0.0f, 0.0f); 
        drawCone();
        glPopMatrix();
    }

    if (g_showSphere)
    {
        glPushMatrix();
        glTranslatef(1.2f, 0.0f, 0.0f);  
        drawSphere();
        glPopMatrix();
    }

    glPopMatrix();

    drawAxes();
}

int main()
{
    sf::ContextSettings ctx(24, 0, 4, 4, 5);

    sf::RenderWindow window(
        sf::VideoMode(g_width, g_height),
        "Grafika 3D - Sto¿ek i Kula",
        sf::Style::Default,
        ctx
    );
    window.setVerticalSyncEnabled(true);
    window.setActive(true);

    setupGL();
    setupProjection(g_width, g_height);

    sf::Clock deltaClock;
    ImGui::SFML::Init(window);

    while (window.isOpen())
    {
        sfe event{};
        while (window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(event);
            if (event.type == sfe::Closed) window.close();
            if (event.type == sfe::KeyPressed && event.key.code == sfk::Escape) window.close();
            if (event.type == sfe::Resized)
            {
                g_width = event.size.width;
                g_height = event.size.height;
                setupProjection(g_width, g_height);
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        // Okno kontroli kamery
        ImGui::Begin("Camera");
        ImGui::SliderFloat("R", &g_R, 1.0f, 10.0f, "%.3f");
        ImGui::SliderFloat("theta", &g_theta, 0.0f, 89.9f, "%.0f deg");
        ImGui::SliderFloat("phi", &g_phi, 0.0f, 360.0f, "%.0f deg");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        bool projectionChanged = ImGui::Checkbox("Orthographic Projection", &g_orthographic);

        ImGui::Spacing();
        if (!g_orthographic)
        {
            ImGui::Text("Perspective Settings:");
            bool fovChanged = ImGui::SliderFloat("FOV (Field of View)", &g_fov, 20.0f, 120.0f, "%.0f deg");

            ImGui::Spacing();
            ImGui::Text("Quick FOV presets:");
            if (ImGui::Button("Wide (90°)"))
            {
                g_fov = 90.0f;
                fovChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Normal (60°)"))
            {
                g_fov = 60.0f;
                fovChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Narrow (45°)"))
            {
                g_fov = 45.0f;
                fovChanged = true;
            }

            if (fovChanged)
            {
                projectionChanged = true;
            }
        }
        else
        {
            ImGui::TextDisabled("FOV (disabled in orthographic mode)");
        }

        if (projectionChanged)
        {
            setupProjection(g_width, g_height);
        }

        ImGui::End();

        // Okno widocznoœci obiektów
        ImGui::Begin("Objects Visibility");
        ImGui::Checkbox("Show Triangle", &g_showTriangle);
        ImGui::Checkbox("Show Cube", &g_showCube);
        ImGui::Checkbox("Show Cone", &g_showCone);
        ImGui::Checkbox("Show Sphere", &g_showSphere);

        ImGui::Spacing();
        if (ImGui::Button("Show All"))
        {
            g_showTriangle = g_showCube = g_showCone = g_showSphere = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Hide All"))
        {
            g_showTriangle = g_showCube = g_showCone = g_showSphere = false;
        }
        ImGui::End();

        // Okno transformacji sceny
        ImGui::Begin("Scene Transform");

        if (ImGui::CollapsingHeader("Scale", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat("Scale X", &g_scaleX, 0.1f, 3.0f, "%.2f");
            ImGui::SliderFloat("Scale Y", &g_scaleY, 0.1f, 3.0f, "%.2f");
            ImGui::SliderFloat("Scale Z", &g_scaleZ, 0.1f, 3.0f, "%.2f");
            if (ImGui::Button("Reset Scale"))
            {
                g_scaleX = g_scaleY = g_scaleZ = 1.0f;
            }
        }

        if (ImGui::CollapsingHeader("Position", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat("Pos X", &g_posX, -3.0f, 3.0f, "%.2f");
            ImGui::SliderFloat("Pos Y", &g_posY, -3.0f, 3.0f, "%.2f");
            ImGui::SliderFloat("Pos Z", &g_posZ, -3.0f, 3.0f, "%.2f");
            if (ImGui::Button("Reset Position"))
            {
                g_posX = g_posY = g_posZ = 0.0f;
            }
        }

        if (ImGui::CollapsingHeader("Rotation (Flip)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat("Rot X", &g_rotX, 0.0f, 360.0f, "%.0f deg");
            ImGui::SliderFloat("Rot Y", &g_rotY, 0.0f, 360.0f, "%.0f deg");
            ImGui::SliderFloat("Rot Z", &g_rotZ, 0.0f, 360.0f, "%.0f deg");

            ImGui::Spacing();
            ImGui::Text("Quick Flips:");
            if (ImGui::Button("Flip X (180°)"))
            {
                g_rotX = fmodf(g_rotX + 180.0f, 360.0f);
            }
            ImGui::SameLine();
            if (ImGui::Button("Flip Y (180°)"))
            {
                g_rotY = fmodf(g_rotY + 180.0f, 360.0f);
            }
            ImGui::SameLine();
            if (ImGui::Button("Flip Z (180°)"))
            {
                g_rotZ = fmodf(g_rotZ + 180.0f, 360.0f);
            }

            if (ImGui::Button("Reset Rotation"))
            {
                g_rotX = g_rotY = g_rotZ = 0.0f;
            }
        }

        ImGui::Spacing();
        if (ImGui::Button("Reset All Transforms"))
        {
            g_scaleX = g_scaleY = g_scaleZ = 1.0f;
            g_posX = g_posY = g_posZ = 0.0f;
            g_rotX = g_rotY = g_rotZ = 0.0f;
        }

        ImGui::End();

        renderScene();
        ImGui::SFML::Render(window);

        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}