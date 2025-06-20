﻿//==============================================================================

//==============================================================================

//------------------------------------------------------------------------------
#include "chai3d.h"
//------------------------------------------------------------------------------
#include <GLFW/glfw3.h>
//------------------------------------------------------------------------------
using namespace chai3d;
using namespace std;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// GENERAL SETTINGS
//------------------------------------------------------------------------------

// stereo Mode
/*
    C_STEREO_DISABLED:            Stereo is disabled 
    C_STEREO_ACTIVE:              Active stereo for OpenGL NVDIA QUADRO cards
    C_STEREO_PASSIVE_LEFT_RIGHT:  Passive stereo where L/R images are rendered next to each other
    C_STEREO_PASSIVE_TOP_BOTTOM:  Passive stereo where L/R images are rendered above each other
*/
cStereoMode stereoMode = C_STEREO_DISABLED;

// fullscreen mode
bool fullscreen = false;

// mirrored display
bool mirroredDisplay = false;


//------------------------------------------------------------------------------
// DECLARED VARIABLES
//------------------------------------------------------------------------------

// a world that contains all objects of the virtual environment
cWorld* world;

// a camera to render the world in the window display
cCamera* camera;

// a viewport to display the scene viewed by the camera
cViewport* viewport = nullptr;

// a light source to illuminate the objects in the world
cSpotLight *light;

// a haptic device handler
cHapticDeviceHandler* handler;

// a pointer to the current haptic device
cGenericHapticDevicePtr hapticDevice;

// a virtual tool representing the haptic device in the scene
cToolCursor* tool;

// a few spherical objects
cShapeSphere* object0;
cShapeSphere* object1;
cShapeSphere* object2;
cShapeSphere* object3;

// a font for rendering text
cFontPtr font;

// a label to display the rate [Hz] at which the simulation is running
cLabel* labelRates;

// a flag that indicates if the haptic simulation is currently running
bool simulationRunning = false;

// a flag that indicates if the haptic simulation has terminated
bool simulationFinished = true;

// a frequency counter to measure the simulation graphic rate
cFrequencyCounter freqCounterGraphics;

// a frequency counter to measure the simulation haptic rate
cFrequencyCounter freqCounterHaptics;

// haptic thread
cThread* hapticsThread;

// a handle to window display context
GLFWwindow* window = nullptr;

// current size of GLFW window
int windowW = 0;
int windowH = 0;

// current size of GLFW framebuffer
int framebufferW = 0;
int framebufferH = 0;

// swap interval for the display context (vertical synchronization)
int swapInterval = 1;


//------------------------------------------------------------------------------
// DECLARED FUNCTIONS
//------------------------------------------------------------------------------

// callback when the window is resized
void onWindowSizeCallback(GLFWwindow* a_window, int a_width, int a_height);

// callback when the window framebuffer is resized
void onFrameBufferSizeCallback(GLFWwindow* a_window, int a_width, int a_height);

// callback when an error GLFW occurs
void onErrorCallback(int a_error, const char* a_description);

// callback when a key is pressed
void onKeyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods);

// callback when window content scaling is modified
void onWindowContentScaleCallback(GLFWwindow* a_window, float a_xscale, float a_yscale);

// this function renders the scene
void renderGraphics(void);

// this function contains the main haptics simulation loop
void renderHaptics(void);

// this function closes the application
void close(void);


//==============================================================================

//==============================================================================

int main(int argc, char* argv[])
{
    //--------------------------------------------------------------------------
    // INITIALIZATION
    //--------------------------------------------------------------------------

    cout << endl;
    cout << "-----------------------------------" << endl;
    cout << "CHAI3D" << endl;
    cout << "Demo: 11-effects" << endl;
    cout << "Copyright 2003-2024" << endl;
    cout << "-----------------------------------" << endl << endl << endl;
    cout << "Keyboard Options:" << endl << endl;
    cout << "[f] - Enable/Disable full screen mode" << endl;
    cout << "[m] - Enable/Disable vertical mirroring" << endl;
    cout << "[q] - Exit application" << endl;
    cout << endl << endl;


    //--------------------------------------------------------------------------
    // OPEN GL - WINDOW DISPLAY
    //--------------------------------------------------------------------------

    // initialize GLFW library
    if (!glfwInit())
    {
        cout << "failed initialization" << endl;
        cSleepMs(1000);
        return 1;
    }

    // set GLFW error callback
    glfwSetErrorCallback(onErrorCallback);

    // compute desired size of window
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    windowW = 0.8 * mode->height;
    windowH = 0.5 * mode->height;
    int x = 0.5 * (mode->width - windowW);
    int y = 0.5 * (mode->height - windowH);

    // set OpenGL version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    // enable double buffering
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

    // set the desired number of samples to use for multisampling
    glfwWindowHint(GLFW_SAMPLES, 4);

    // specify that window should be resized based on monitor content scale
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    // set active stereo mode
    if (stereoMode == C_STEREO_ACTIVE)
    {
        glfwWindowHint(GLFW_STEREO, GL_TRUE);
    }
    else
    {
        glfwWindowHint(GLFW_STEREO, GL_FALSE);
    }

    // create display context
    window = glfwCreateWindow(windowW, windowH, "CHAI3D", NULL, NULL);
    if (!window)
    {
        cout << "failed to create window" << endl;
        cSleepMs(1000);
        glfwTerminate();
        return 1;
    }

    // set GLFW key callback
    glfwSetKeyCallback(window, onKeyCallback);

    // set GLFW window size callback
    glfwSetWindowSizeCallback(window, onWindowSizeCallback);

    // set GLFW framebuffer size callback
    glfwSetFramebufferSizeCallback(window, onFrameBufferSizeCallback);

    // set GLFW window content scaling callback
    glfwSetWindowContentScaleCallback(window, onWindowContentScaleCallback);

    // get width and height of window
    glfwGetFramebufferSize(window, &framebufferW, &framebufferH);

    // set position of window
    glfwSetWindowPos(window, x, y);

    // set window size
    glfwSetWindowSize(window, windowW, windowH);

    // set GLFW current display context
    glfwMakeContextCurrent(window);

    // set GLFW swap interval for the current display context
    glfwSwapInterval(swapInterval);


#ifdef GLEW_VERSION
    // initialize GLEW library
    if (glewInit() != GLEW_OK)
    {
        cout << "failed to initialize GLEW library" << endl;
        glfwTerminate();
        return 1;
    }
#endif


    //--------------------------------------------------------------------------
    // WORLD - CAMERA - LIGHTING
    //--------------------------------------------------------------------------

    // create a new world.
    world = new cWorld();

    // set the background color of the environment
    world->m_backgroundColor.setWhite();

    // create a camera and insert it into the virtual world
    camera = new cCamera(world);
    world->addChild(camera);

    // position and orient the camera
    camera->set(cVector3d(3.0, 0.0, 0.0),    // camera position (eye)
                cVector3d(0.0, 0.0, 0.0),    // lookat position (target)
                cVector3d(0.0, 0.0, 1.0));   // direction of the (up) vector

    // set the near and far clipping planes of the camera
    // anything in front or behind these clipping planes will not be rendered
    camera->setClippingPlanes(0.01, 10.0);

    // set stereo mode
    camera->setStereoMode(stereoMode);

    // set stereo eye separation and focal length (applies only if stereo is enabled)
    camera->setStereoEyeSeparation(0.03);
    camera->setStereoFocalLength(3.0);

    // set vertical mirrored display mode
    camera->setMirrorVertical(mirroredDisplay);

    // enable multi-pass rendering to handle transparent objects
    camera->setUseMultipassTransparency(true);


    // create a light source
    light = new cSpotLight(world);

    // add light to world
    world->addChild(light);

    // enable light source
    light->setEnabled(true);

    // position the light source
    light->setLocalPos(1, 1, 1);

    // define the direction of the light beam
    light->setDir(-1, -1, -1.0);

    // set light cone half angle
    light->setCutOffAngleDeg(60);

    //--------------------------------------------------------------------------
    // HAPTIC DEVICES / TOOLS
    //--------------------------------------------------------------------------

    // create a haptic device handler
    handler = new cHapticDeviceHandler();

    // get access to the first available haptic device found
    handler->getDevice(hapticDevice, 0);

    // retrieve information about the current haptic device
    cHapticDeviceInfo hapticDeviceInfo = hapticDevice->getSpecifications();

    // create a tool (cursor) and insert into the world
    tool = new cToolCursor(world);
    world->addChild(tool);

    // connect the haptic device to the virtual tool
    tool->setHapticDevice(hapticDevice);

    // define a radius for the virtual tool (sphere)
    tool->setRadius(0.03);
    // map the physical workspace of the haptic device to a larger virtual workspace.
    tool->setWorkspaceRadius(1.0);

    // haptic forces are enabled only if small forces are first sent to the device;
    // this mode avoids the force spike that occurs when the application starts when 
    // the tool is located inside an object for instance. 
    tool->setWaitForSmallForce(true);

    // start the haptic tool
    tool->start();


    //--------------------------------------------------------------------------
    // CREATING OBJECTS
    //--------------------------------------------------------------------------

    // read the scale factor between the physical workspace of the haptic
    // device and the virtual workspace defined for the tool
    double workspaceScaleFactor = tool->getWorkspaceScaleFactor();

    // get properties of haptic device
    double maxLinearForce = cMin(hapticDeviceInfo.m_maxLinearForce, 7.0);
    double maxStiffness = hapticDeviceInfo.m_maxLinearStiffness / workspaceScaleFactor;
    double maxDamping   = hapticDeviceInfo.m_maxLinearDamping / workspaceScaleFactor;


    /////////////////////////////////////////////////////////////////////////
    // OBJECT 0: "MAGNET"
    /////////////////////////////////////////////////////////////////////////

    // get current path
    bool fileload;
    string currentpath = cGetCurrentPath();

    // create a sphere and define its radius
    object0 = new cShapeSphere(0.5);

    // add object to world
    world->addChild(object0);

    // set the position of the object at the center of the world
    object0->setLocalPos(0.0, -1.2, 0.0);

    // load texture map
    object0->m_texture = cTexture2d::create();
    //fileload = object0->m_texture->loadFromFile(currentpath + "../resources/images/spheremap-3.jpg"); //metal


    // set graphic properties
    object0->m_texture->setSphericalMappingEnabled(true);
    object0->m_material->setGray();
    // set haptic properties
    //object0->m_material->setStiffness(0.4 * maxStiffness);          // % of maximum linear stiffness
    //object0->m_material->setMagnetMaxForce(0.8 * maxLinearForce);   // % of maximum linear force 
    //object0->m_material->setMagnetMaxDistance(0.15);
    //object0->m_material->setViscosity(0.1 * maxDamping);            // % of maximum linear damping

    // create a haptic surface effect
    object0->createEffectSurface();

    // create a haptic magnetic effect
    //object0->createEffectMagnetic();

    // create a haptic viscous effect
    //object0->createEffectViscosity();


    ////////////////////////////////////////////////////////////////////////
    // OBJECT 1: "FLUID"
    ////////////////////////////////////////////////////////////////////////

    // create a sphere and define its radius
    object1 = new cShapeSphere(0.3);

    // add object to world
    //world->addChild(object1);

    // set the position of the object at the center of the world
    object1->setLocalPos(0.0, 0, 0.0);

    // load texture map
    object1->m_texture = cTexture2d::create();
    //fileload = object1->m_texture->loadFromFile(currentpath + "../resources/images/spheremap-2.jpg"); //blue


    // set graphic properties
    object1->m_material->setGray();
    //object1->setTransparencyLevel(0.9);
    object1->setUseTexture(true);
    object1->m_texture->setSphericalMappingEnabled(true);

    // set haptic properties
    object1->m_material->setViscosity(0.9 * maxDamping);    // % of maximum linear damping

    // create a haptic viscous effect
    object1->createEffectViscosity();


    /////////////////////////////////////////////////////////////////////////
    // OBJECT 2: "STICK-SLIP"
    /////////////////////////////////////////////////////////////////////////

    // create a sphere by defini its radius
    object2 = new cShapeSphere(0.3);

    // add object to world
    world->addChild(object2);

    // set the position of the object at the center of the world
    object2->setLocalPos(0.0, 1, 0);

    // load texture map
    object2->m_texture = cTexture2d::create();
    //fileload = object2->m_texture->loadFromFile(currentpath + "../resources/images/spheremap-5.jpg"); //white holes


    // set graphic properties
    object2->m_texture->setSphericalMappingEnabled(true);
    object2->m_material->setGray();
    object2->setUseTexture(true);

    // set haptic properties
    object2->m_material->setStickSlipForceMax(0.2 * maxLinearForce);// % of maximum linear force
    object2->m_material->setStickSlipStiffness(0.6 * maxStiffness); // % of maximum linear stiffness
    //object2->m_material->setViscosity(-1* maxDamping); // % of maximum linear stiffness

    // create a haptic stick-slip effect
    object2->createEffectStickSlip();

    //object2->createEffectViscosity();
    // create a haptic stick-slip effect
    //object2->setuse();


    ////////////////////////////////////////////////////////////////////////
    // OBJECT 3: "VIBRATIONS"
    ////////////////////////////////////////////////////////////////////////

    // create a sphere and define its radius
    object3 = new cShapeSphere(0.5);

    // add object to world
    world->addChild(object3);

    // set the position of the object at the center of the world
    object3->setLocalPos(0, 0.0, 0);

    // load texture map
    object3->m_texture = cTexture2d::create();


    // set graphic properties
    object3->m_texture->setSphericalMappingEnabled(true);
    object3->setUseTexture(true);
    object3->m_material->setGray();
    //object3->setTransparencyLevel(0.9);

    // set haptic properties
    object3->m_material->setVibrationFrequency(60);
    object3->m_material->setVibrationAmplitude(0.5 * maxLinearForce);   // % of maximum linear force
    object3->m_material->setStiffness(0.1);   
    //object3->m_material->setViscosity(0.9 * maxDamping); // % of maximum linear stiffness
    //object3->m_material->setMagnetMaxForce(0.8 * maxLinearForce);   // % of maximum linear force 
    //object3->m_material->setMagnetMaxDistance(0.3);
    // create a haptic vibration effect
    object3->createEffectVibration();

    // create a haptic surface effect
    object3->createEffectSurface();
    object3->createEffectViscosity();
    // create a haptic magnetic effect
    //object3->createEffectMagnetic();
   
    //--------------------------------------------------------------------------
    // WIDGETS
    //--------------------------------------------------------------------------

    // create a font
    font = NEW_CFONT_CALIBRI_20();
    
    // create a label to display the haptic and graphic rate of the simulation
    labelRates = new cLabel(font);
    camera->m_frontLayer->addChild(labelRates);


    //--------------------------------------------------------------------------
    // VIEWPORT DISPLAY
    //--------------------------------------------------------------------------

    // get content scale factor
    float contentScaleW, contentScaleH;
    glfwGetWindowContentScale(window, &contentScaleW, &contentScaleH);

    // create a viewport to display the scene.
    viewport = new cViewport(camera, contentScaleW, contentScaleH);


    //--------------------------------------------------------------------------
    // START HAPTIC SIMULATION THREAD
    //--------------------------------------------------------------------------

    // create a thread which starts the main haptics rendering loop
    hapticsThread = new cThread();
    hapticsThread->start(renderHaptics, CTHREAD_PRIORITY_HAPTICS);

    // setup callback when application exits
    atexit(close);


    //--------------------------------------------------------------------------
    // MAIN GRAPHIC LOOP
    //--------------------------------------------------------------------------

    // main graphic loop
    while (!glfwWindowShouldClose(window))
    {
        // render graphics
        renderGraphics();

        // process events
        glfwPollEvents();
    }

    // close window
    glfwDestroyWindow(window);

    // terminate GLFW library
    glfwTerminate();

    // exit
    return (0);
}

//------------------------------------------------------------------------------

void onWindowSizeCallback(GLFWwindow* a_window, int a_width, int a_height)
{
    // update window size
    windowW = a_width;
    windowH = a_height;

    // render scene
    renderGraphics();
}

//------------------------------------------------------------------------------

void onFrameBufferSizeCallback(GLFWwindow* a_window, int a_width, int a_height)
{
    // update frame buffer size
    framebufferW = a_width;
    framebufferH = a_height;
}

//------------------------------------------------------------------------------

void onWindowContentScaleCallback(GLFWwindow* a_window, float a_xscale, float a_yscale)
{
    // update window content scale factor
    viewport->setContentScale(a_xscale, a_yscale);
}

//------------------------------------------------------------------------------

void onErrorCallback(int a_error, const char* a_description)
{
    cout << "Error: " << a_description << endl;
}

//------------------------------------------------------------------------------

void onKeyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods)
{
    // filter calls that only include a key press
    if ((a_action != GLFW_PRESS) && (a_action != GLFW_REPEAT))
    {
        return;
    }

    // option - exit
    else if ((a_key == GLFW_KEY_ESCAPE) || (a_key == GLFW_KEY_Q))
    {
        glfwSetWindowShouldClose(a_window, GLFW_TRUE);
    }

    // option - toggle fullscreen
    else if (a_key == GLFW_KEY_F)
    {
        // toggle state variable
        fullscreen = !fullscreen;

        // get handle to monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();

        // get information about monitor
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        // set fullscreen or window mode
        if (fullscreen)
        {
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
        else
        {
            int w = 0.8 * mode->height;
            int h = 0.5 * mode->height;
            int x = 0.5 * (mode->width - w);
            int y = 0.5 * (mode->height - h);
            glfwSetWindowMonitor(window, NULL, x, y, w, h, mode->refreshRate);
        }

        // set the desired swap interval and number of samples to use for multisampling
        glfwSwapInterval(swapInterval);
        glfwWindowHint(GLFW_SAMPLES, 4);
    }

    // option - toggle vertical mirroring
    else if (a_key == GLFW_KEY_M)
    {
        mirroredDisplay = !mirroredDisplay;
        camera->setMirrorVertical(mirroredDisplay);
    }
}

//------------------------------------------------------------------------------

void close(void)
{
    // stop the simulation
    simulationRunning = false;

    // wait for graphics and haptics loops to terminate
    while (!simulationFinished) { cSleepMs(100); }

    // close haptic device
    tool->stop();

    // delete resources
    delete hapticsThread;
    delete world;
    delete handler;
}

//------------------------------------------------------------------------------



void renderGraphics(void)
{
    // sanity check
    if (viewport == nullptr) { return; }

    /////////////////////////////////////////////////////////////////////
    // UPDATE WIDGETS
    /////////////////////////////////////////////////////////////////////

    // get width and height of CHAI3D internal rendering buffer
    int displayW = viewport->getDisplayWidth();
    int displayH = viewport->getDisplayHeight();

    // update haptic and graphic rate data
    labelRates->setText(cStr(freqCounterGraphics.getFrequency(), 0) + " Hz / " +
                        cStr(freqCounterHaptics.getFrequency(), 0) + " Hz");

    // update position of label
    labelRates->setLocalPos((int)(0.5 * (displayW - labelRates->getWidth())), 15);


    /////////////////////////////////////////////////////////////////////
    // RENDER SCENE
    /////////////////////////////////////////////////////////////////////

    // update shadow maps (if any)
    world->updateShadowMaps(false, mirroredDisplay);

    // render world
    viewport->renderView(framebufferW, framebufferH);

    // wait until all GL commands are completed
    glFinish();

    // check for any OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) cout << "Error: " << gluErrorString(error) << endl;

    // swap buffers
    glfwSwapBuffers(window);

    // signal frequency counter
    freqCounterGraphics.signal(1);
}



bool inLoop = false;

void renderHaptics(void)
{
    simulationRunning = true;
    simulationFinished = false;

    double timeStep = 0.001;
    double freq = 6.0;             // Hz
    double amp = 6.0;              // N

    double mass = 0.5;
    double damping = 0.0;

    cVector3d object2StartPos(0.0, 1.0, 0.0);
    cVector3d object2Vel(0.0, 0.0, 0.0);

    double oscTime0 = 0.0;
    double oscTime3 = 0.0;

    while (simulationRunning)
    {
        world->computeGlobalPositions(true);
        tool->updateFromDevice();
        tool->computeInteractionForces();

        cVector3d toolPos = tool->getDeviceGlobalPos();
        cVector3d baseForce = tool->getDeviceGlobalForce(); // base haptic feedback

        // --- Object0 vibration ---
        {
            cVector3d objPos = object0->getGlobalPos();
            cVector3d dir = objPos - toolPos;
            double dist = dir.length();
            double radiusSum = 0.05 + object0->getRadius();

            if (dist < radiusSum)
            {
                // read linear velocity 
                cVector3d linearVelocity = tool->getDeviceGlobalLinVel();

                // compute linear damping force
                double Kv = 0.1;
                cVector3d forceDamping = Kv * linearVelocity;
                baseForce.add(forceDamping);
                baseForce *= 4;
            }

        }

        // --- Object3 vibration ---
        {
            cVector3d objPos = object3->getGlobalPos();
            cVector3d dir = objPos - toolPos;
            double dist = dir.length();
            double radiusSum = inLoop ?
                0.05 + object3->getRadius() * 2 :
                (0.05 + object3->getRadius()) ;

            if (dist < radiusSum)
            {
                oscTime3 += timeStep;
                double f = amp * sin(2.0 * 3.14159 * freq * oscTime3);
                double f2 = amp * cos(2.0 * 3.14159 * freq * oscTime3);
                baseForce += cVector3d(-f2, f, f2);

                inLoop = true;
            }
            else
            {
                oscTime3 = 0.0;
                inLoop = false;
            }
        }

        // --- Object2 dynamics ---
        {
            cVector3d objPos = object2->getGlobalPos();
            cVector3d dir = objPos - toolPos;
            double dist = dir.length();
            double radiusSum = 0.03 + object2->getRadius();

            if (dist < radiusSum)
            {
                dir.normalize();
                cVector3d netForce = -baseForce - damping * object2Vel;
                object2Vel += (netForce / mass) * timeStep;
                baseForce *= 10;
            }

            object2->setLocalPos(object2->getLocalPos() + object2Vel * timeStep);
            object2Vel *= 0.999;

            if ((object2->getLocalPos() - object2StartPos).length() > 1.0)
            {
                object2Vel.set(0, 0, 0);
                object2->setLocalPos(object2StartPos);
            }
        }

        tool->setDeviceGlobalForce(baseForce);
        tool->applyToDevice();
        freqCounterHaptics.signal(1);
    }

    simulationFinished = true;
}





//------------------------------------------------------------------------------
