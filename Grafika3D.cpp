#include "pch.h"

typedef sf::Event    sfe;
typedef sf::Keyboard sfk;

// Parametry kamery
static float g_R = 2.6f;
static float g_theta = 44.0f;
static float g_phi = 25.0f;

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
    perspectiveGL(60.0, aspect, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void setupGL()
{
    // BUFOR G£ÊBOKOŒCI - prawid³owa konfiguracja
    glEnable(GL_DEPTH_TEST);     // w³¹czamy test g³êbokoœci
    glDepthFunc(GL_LEQUAL);      // piksel przechodzi jeœli jest bli¿ej lub równy
    glDepthMask(GL_TRUE);        // w³¹czamy zapis do bufora g³êbokoœci
    glClearDepth(1.0);           // wartoœæ czyszczenia bufora (najdalsza)
    glDepthRange(0.0, 1.0);      // zakres wartoœci g³êbokoœci

    // Wy³¹czamy blending aby unikn¹æ problemów z przezroczystoœci¹
    glDisable(GL_BLEND);

    // W³¹czamy face culling dla poprawnego renderowania trójk¹tów
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // AA linii + t³o
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glClearColor(0.95f, 0.97f, 0.99f, 1.0f);

    // Ustawienia dla renderowania
    glShadeModel(GL_SMOOTH);
}

static void drawAxes(float lenPos = 2.0f, float lenNeg = 2.0f)
{
    // Rysujemy osie z wy³¹czonym depth test aby by³y zawsze widoczne
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

    // Przywracamy depth test
    glEnable(GL_DEPTH_TEST);
}

// Rysowanie konturów szeœcianu
static void drawCubeWire(float s = 1.2f)
{
    const float a = s * 0.5f;

    // Ustawiamy offset dla linii aby by³y rysowane "na wierzchu" œcian
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

    // Rysujemy trójk¹t jako wype³nion¹ powierzchniê
    // Bufor g³êbokoœci automatycznie zadba o przes³anianie
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    glBegin(GL_TRIANGLES);
    glColor3f(1.f, 0.f, 0.f); glVertex3fv(v1);
    glColor3f(0.f, 1.f, 0.f); glVertex3fv(v2);
    glColor3f(0.f, 0.f, 1.f); glVertex3fv(v3);
    glEnd();
}

static void renderScene()
{
    // KLUCZOWE: Czyszczenie bufora koloru I g³êbokoœci przed ka¿d¹ klatk¹
    // To jest najwa¿niejsze - bez tego bufor g³êbokoœci pamiêta star¹ geometriê
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Upewniamy siê, ¿e bufor g³êbokoœci jest w³¹czony i zapisywalny
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Obliczenie pozycji kamery na podstawie wspó³rzêdnych sferycznych
    const float th = deg2rad(g_theta);
    const float ph = deg2rad(g_phi);
    const float x = g_R * sinf(th) * cosf(ph);
    const float y = g_R * cosf(th);
    const float z = g_R * sinf(th) * sinf(ph);

    lookAtGL(x, y, z, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f);

    // Zapisujemy stan przed transformacjami
    glPushMatrix();

    // Stosujemy transformacje sceny (w kolejnoœci: przesuniêcie -> rotacja -> skalowanie)
    glTranslatef(g_posX, g_posY, g_posZ);

    // Rotacje wokó³ ka¿dej osi
    glRotatef(g_rotX, 1.0f, 0.0f, 0.0f);  // rotacja wokó³ osi X
    glRotatef(g_rotY, 0.0f, 1.0f, 0.0f);  // rotacja wokó³ osi Y
    glRotatef(g_rotZ, 0.0f, 0.0f, 1.0f);  // rotacja wokó³ osi Z

    // Skalowanie wzglêdem ka¿dej osi
    glScalef(g_scaleX, g_scaleY, g_scaleZ);

    // Rysujemy wszystkie obiekty - dziêki buforowi g³êbokoœci
    // kolejnoœæ nie ma znaczenia dla poprawnoœci renderowania

    // 1) Trójk¹t kolorowy wewn¹trz szeœcianu
    drawInternalTriangle();

    // 2) Kontur szeœcianu
    // Czêœci konturu za trójk¹tem bêd¹ automatycznie zas³oniête
    // dziêki testowi g³êbokoœci
    drawCubeWire();

    // Przywracamy stan przed transformacjami
    glPopMatrix();

    // 3) Osie wspó³rzêdnych (rysowane bez transformacji, zawsze w centrum)
    drawAxes();
}

int main()
{
    // Kontekst OpenGL z 24-bitowym buforem g³êbokoœci
    // To jest bardzo wa¿ne - bez wystarczaj¹cej precyzji bufora g³êbokoœci
    // mog¹ wyst¹piæ artefakty z-fighting
    sf::ContextSettings ctx(24, 0, 4, 4, 5);  // depth=24, stencil=0, antialiasing=4

    sf::RenderWindow window(
        sf::VideoMode(g_width, g_height),
        "Grafika 3D - Poprawny bufor g³êbokoœci",
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
        ImGui::SliderFloat("R", &g_R, 1.0f, 6.0f, "%.3f");
        ImGui::SliderFloat("theta", &g_theta, 0.0f, 89.9f, "%.0f deg");
        ImGui::SliderFloat("phi", &g_phi, 0.0f, 360.0f, "%.0f deg");
        ImGui::End();

        // Okno transformacji sceny
        ImGui::Begin("Scene Transform");

        // Sekcja Skalowanie
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

        // Sekcja Pozycja
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

        // Sekcja Rotacja (Flip)
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

        // Renderowanie sceny - bufor g³êbokoœci jest czyszczony przy ka¿dej zmianie k¹ta
        renderScene();
        ImGui::SFML::Render(window);

        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}