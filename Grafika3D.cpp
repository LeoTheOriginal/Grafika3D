// Grafika3D.cpp : entry point

#include "pch.h"

typedef sf::Event    sfe;
typedef sf::Keyboard sfk;

static float g_R = 2.6f;   // zoom (odleg³oœæ kamery)
static float g_theta = 44.0f;  // elewacja [deg]
static float g_phi = 25.0f;  // azymut [deg]

static unsigned g_width = 1024;
static unsigned g_height = 768;

static inline float deg2rad(float d) { return d * 3.1415926535f / 180.0f; }

// ----------------- ZAMIENNIKI GLU (bez potrzeby linkowania glu32.lib) ------
static void perspectiveGL(double fovY, double aspect, double zNear, double zFar)
{
    const double fH = tan(fovY * 0.5 * 3.1415926535 / 180.0) * zNear;
    const double fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}

static void lookAtGL(float eyeX, float eyeY, float eyeZ,
    float centerX, float centerY, float centerZ,
    float upX, float upY, float upZ)
{
    float f[3] = { centerX - eyeX, centerY - eyeY, centerZ - eyeZ };
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
// ---------------------------------------------------------------------------

static void setupProjection(unsigned w, unsigned h)
{
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspect = (h == 0) ? 1.f : (float)w / (float)h;
    perspectiveGL(60.0, aspect, 0.1, 100.0); // zamiast gluPerspective

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void setupGL()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glClearColor(0.95f, 0.97f, 0.99f, 1.0f); // jasne t³o jak na zdjêciu
}

static void drawAxes(float lenPos = 2.0f, float lenNeg = 2.0f)
{
    glLineWidth(1.5f);

    // ujemne - przerywane
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x00FF);
    glBegin(GL_LINES);
    glColor3f(0.7f, 0.2f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(-lenNeg, 0, 0);
    glColor3f(0.2f, 0.7f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(0, -lenNeg, 0);
    glColor3f(0.2f, 0.2f, 0.7f); glVertex3f(0, 0, 0); glVertex3f(0, 0, -lenNeg);
    glEnd();
    glDisable(GL_LINE_STIPPLE);

    // dodatnie - pe³ne
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.2f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(lenPos, 0, 0); // X
    glColor3f(0.2f, 0.9f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(0, lenPos, 0); // Y
    glColor3f(0.2f, 0.4f, 1.0f); glVertex3f(0, 0, 0); glVertex3f(0, 0, lenPos); // Z
    glEnd();

    glLineWidth(1.0f);
}

static void drawCubeWire(float s = 1.2f)
{
    const float a = s * 0.5f;
    glColor3f(0.10f, 0.10f, 0.10f);
    glLineWidth(2.5f);
    glBegin(GL_LINES);
    // dó³
    glVertex3f(-a, -a, -a); glVertex3f(a, -a, -a);
    glVertex3f(a, -a, -a); glVertex3f(a, -a, a);
    glVertex3f(a, -a, a); glVertex3f(-a, -a, a);
    glVertex3f(-a, -a, a); glVertex3f(-a, -a, -a);
    // góra
    glVertex3f(-a, a, -a); glVertex3f(a, a, -a);
    glVertex3f(a, a, -a); glVertex3f(a, a, a);
    glVertex3f(a, a, a); glVertex3f(-a, a, a);
    glVertex3f(-a, a, a); glVertex3f(-a, a, -a);
    // s³upki
    glVertex3f(-a, -a, -a); glVertex3f(-a, a, -a);
    glVertex3f(a, -a, -a); glVertex3f(a, a, -a);
    glVertex3f(a, -a, a); glVertex3f(a, a, a);
    glVertex3f(-a, -a, a); glVertex3f(-a, a, a);
    glEnd();
    glLineWidth(1.0f);
}

static void drawInternalTriangle(float s = 1.2f)
{
    const float a = s * 0.5f;
    const GLfloat v1[3] = { +a, +a, +a }; // r
    const GLfloat v2[3] = { -a, -a, +a }; // g
    const GLfloat v3[3] = { +a, -a, -a }; // b

    glBegin(GL_TRIANGLES);
    glColor3f(1.f, 0.f, 0.f); glVertex3fv(v1);
    glColor3f(0.f, 1.f, 0.f); glVertex3fv(v2);
    glColor3f(0.f, 0.f, 1.f); glVertex3fv(v3);
    glEnd();
}

static void renderScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // kamera sferyczna (Y-up)
    const float th = deg2rad(g_theta);
    const float ph = deg2rad(g_phi);
    const float x = g_R * sinf(th) * cosf(ph);
    const float y = g_R * cosf(th);
    const float z = g_R * sinf(th) * sinf(ph);

    lookAtGL(x, y, z, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f); // zamiast gluLookAt

    drawAxes();
    drawCubeWire();
    drawInternalTriangle();
}

int main()
{
    sf::ContextSettings ctx(24); // 24-bit depth buffer
    sf::RenderWindow window(sf::VideoMode(g_width, g_height), "Grafika 3D", sf::Style::Default, ctx);
    window.setVerticalSyncEnabled(true);

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

        ImGui::Begin("Camera");
        ImGui::SliderFloat("R", &g_R, 1.0f, 6.0f, "%.3f");
        ImGui::SliderFloat("theta", &g_theta, 0.0f, 89.9f, "%.0f deg");
        ImGui::SliderFloat("phi", &g_phi, 0.0f, 360.0f, "%.0f deg");
        ImGui::End();

        renderScene();
        ImGui::SFML::Render(window);

        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
