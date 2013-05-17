#include <GL/glew.h>
#if defined(_WIN32)
#   include <GL/wglew.h>
#endif
#if defined(__APPLE__) || defined(MACOSX)
#   include <GLUT/glut.h>
#else
#   include <GL/glut.h>
#endif
#include <algorithm>
#include "Common.hpp"
#include <GLScreenCapturer.h>


#define WIN_WIDTH 960
#define WIN_HEIGHT 540

static GLScreenCapturer screenshot("screenshot-%04d.ppm");

using namespace vmath;
using std::sort;

static ITrackball* Trackball = 0;
static Texture Background;
static Texture Sprite;
static Texture Smoke;
static Texture Tadpole;
static Texture Billboard;

static Mesh ScreenQuad;
static Mesh ObstacleMesh;
static GLuint BlitProgram;
static GLuint LitProgram;
static GLuint ParticleProgram;
static GLuint CompositeProgram;
static Matrix4 ProjectionMatrix;
static Matrix4 ViewMatrix;
static Matrix4 ModelviewProjection;
bool ShowStreamlines = false;
bool obstacle = false;
static const float TimeStep = ShowStreamlines ? 1.0f : 2.0f;
static float Time = 0;
static ParticleList Particles;
static Surface BackgroundSurface;
static Surface ParticleSurface;

static const Vector4 Yellow(.7, 1, .5, 1);
static const Vector4 Black(0, 0, 0, 1);

void initShit()
{
    ParticleSurface = CreateSurface(WIN_WIDTH, WIN_HEIGHT);
    BackgroundSurface = CreateSurface(WIN_WIDTH, WIN_HEIGHT);
    Trackball = CreateTrackball(WIN_WIDTH * 1.0f, WIN_HEIGHT * 1.0f, WIN_WIDTH * 3.0f / 4.0f);
    ScreenQuad = CreateQuad();
    Background = LoadTexture("white.png");
    Smoke = LoadTexture("Sprite.png");
    Tadpole = LoadTexture("Tadpole.png");
    Billboard = LoadTexture("Billboard.png");
    Sprite = Smoke;
    ObstacleMesh = LoadMesh("Sphere.ctm");
    BlitProgram = loadProgram("Blit.VS", 0, "Blit.FS");
    LitProgram = loadProgram("Lit.VS", 0, "Lit.FS");
    CompositeProgram = loadProgram("Composite.VS", 0, "Composite.FS");
    ParticleProgram = loadProgram("Particle.VS", "Particle.GS", "Particle.FS");
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // http://http.developer.nvidia.com/GPUGems3/gpugems3_ch23.html
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
}

void draw()
{
    glBindFramebuffer(GL_FRAMEBUFFER, BackgroundSurface.Fbo);

    // Render the background:
    glUseProgram(BlitProgram);
    setUniform("Depth", 1.0f);
    setUniform("ScrollOffset", Time);
    glDepthFunc(GL_ALWAYS);
    glBindTexture(GL_TEXTURE_2D, Background.Handle);
    if (ShowStreamlines) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
        RenderMesh(ScreenQuad);
    }
    glDepthFunc(GL_LESS);

    // Render the sphere:
    glUseProgram(LitProgram);
    Matrix4 model( Matrix4::identity() );
    Matrix4 modelview = ViewMatrix * model;
    setUniform("ModelviewProjection", ProjectionMatrix * modelview);
    setUniform("ViewMatrix", ViewMatrix.getUpper3x3());
    setUniform("NormalMatrix", modelview.getUpper3x3());
    setUniform("LightPosition", Vector3(0.25f, 0.25f, 1));
    setUniform("DiffuseMaterial", Vector3(0.4f, 0.5f, 0.8f));
    setUniform("AmbientMaterial", Vector3(0.125f, 0.125f, 0.0f));
    setUniform("SpecularMaterial", Vector3(0.5f, 0.5f, 0.5f));
    setUniform("Shininess", 50.0f);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (!ShowStreamlines)
        ;//enderMesh(ObstacleMesh);

    // Render the particles:
    if (!Particles.empty()) {
        glBindFramebuffer(GL_FRAMEBUFFER, ParticleSurface.Fbo);
        glClearColor(0, 0, 0, 1);

        static bool first = true;
        if (!ShowStreamlines || first) {
            glClear(GL_COLOR_BUFFER_BIT);
            first = false;
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, BackgroundSurface.DepthTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, Sprite.Handle);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glUseProgram(ParticleProgram);
        setUniform("ModelviewProjection", ModelviewProjection);
        setUniform("Color", ShowStreamlines ? Yellow : Black);
        setUniform("FadeRate", TimeStep * 0.75f);
        setUniform("DepthSampler", 0);
        setUniform("SpriteSampler", 1);
        setUniform("Time", Time);
        setUniform("PointSize", ShowStreamlines ? 0.01f : 0.03f);
        setUniform("Modelview", modelview.getUpper3x3());
        setUniform("InverseSize", 1.0f / WIN_WIDTH, 1.0f / WIN_HEIGHT);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glEnableVertexAttribArray(POSITION_SLOT);
        glEnableVertexAttribArray(BIRTH_TIME_SLOT);
        glEnableVertexAttribArray(VELOCITY_SLOT);
        unsigned char* pData = (unsigned char*) &Particles[0].Px;
        glVertexAttribPointer(POSITION_SLOT, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), pData);
        glVertexAttribPointer(BIRTH_TIME_SLOT, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), 12 + pData);
        glVertexAttribPointer(VELOCITY_SLOT, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), 16 + pData);
        glDrawArrays(GL_POINTS, 0, Particles.size());
        glDisableVertexAttribArray(POSITION_SLOT);
        glDisableVertexAttribArray(BIRTH_TIME_SLOT);
        glDisableVertexAttribArray(VELOCITY_SLOT);
        glDisable(GL_BLEND);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Compose the particle layer with the obstacles layer:
    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, BackgroundSurface.ColorTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ParticleSurface.ColorTexture);
    glUseProgram(CompositeProgram);
    setUniform("BackgroundSampler", 0);
    setUniform("ParticlesSampler", 1);
    RenderMesh(ScreenQuad);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_DEPTH_TEST);
}



bool zOrderPredicate(const Particle& p0, const Particle& p1)
{
    float z0 = (ModelviewProjection * Point3(p0.Px, p0.Py, p0.Pz)).getZ();
    float z1 = (ModelviewProjection * Point3(p1.Px, p1.Py, p1.Pz)).getZ();
    return z0 > z1;
}
 

void update()
{
    int microseconds = glutGet(GLUT_ELAPSED_TIME) - Time;
    float dt = microseconds * 0.0000001f;
    Time += dt;
    Trackball->Update(microseconds);

    const float W = 0.4f;
    const float H = W * WIN_HEIGHT / WIN_WIDTH;
    ProjectionMatrix = Matrix4::frustum(-W, W, -H, H, 2, 500);

    Point3 eye(0, 0, 7);
    Vector3 up(0, 1, 0);
    Point3 target(0, 0, 0);
    Matrix3 spin = Trackball->GetRotation();
    eye = Transform3(spin, Vector3(0,0,0)) * eye;
    up = spin * up;
    ViewMatrix = Matrix4::lookAt(eye, target, up);

    ModelviewProjection = ProjectionMatrix * ViewMatrix;

    AdvanceTime(Particles, dt, dt * TimeStep);
    sort(Particles.begin(), Particles.end(), zOrderPredicate);
}

void display( void )
{
    update();
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    glPushMatrix();
    {
        
        draw();
        
    }
    glPopMatrix();

    glFlush();
    // End Drawing calls
    glutSwapBuffers();
}

void mouse( int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
        Trackball->MouseDown(x, y);
    else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)
        Trackball->MouseUp(x, y);
}

void motion(int x, int y)
{
    Trackball->MouseMove(x, y);
}

void keyboard( unsigned char key, int x, int y )
{
    switch(key)
    {
    case 27: // Escape key
        exit(0);
        break;
    case 'r':
        printf("save current screen\n");
        screenshot.capture();
        break;
    }
}

void initGlew()
{
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }
    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
}

int main (int argc, char *argv[])
{
    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize( WIN_WIDTH, WIN_HEIGHT );

    glutCreateWindow( "Opengl demo" );

    glutDisplayFunc( display );
    glutReshapeFunc( NULL );
    glutKeyboardFunc( keyboard );
    glutMouseFunc( mouse );
    glutMotionFunc( motion );
    glutIdleFunc( display );

    initGlew();

    initShit();

    if (argv[1] && !strcmp(argv[1], "--billboard")) Sprite = Billboard;
    else if (argv[1] && !strcmp(argv[1], "--tadpole")) Sprite = Tadpole;
    else if (argv[1] && !strcmp(argv[1], "--smoke")) Sprite = Smoke;
    if ((argv[1] && (!strcmp(argv[1], "--streamlines"))) || (argv[2] && !strcmp(argv[2], "--streamlines")) || (argv[3] && !strcmp(argv[3], "--streamlines"))) ShowStreamlines = true;
    if ((argv[1] && (!strcmp(argv[1], "--obstacle"))) || (argv[2] && !strcmp(argv[2], "--obstacle")) || (argv[3] && !strcmp(argv[3], "--obstacle"))) obstacle = true;


    glutMainLoop();
}