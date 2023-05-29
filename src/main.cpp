#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <windows.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include "rvr.hpp"

#define ACTION_MANIFEST_PATH "C:\\Users\\yusuf\\Desktop\\RVR\\rvr_actions.json"

std::string readFile(const char* path) {
    std::ifstream file(path);
    std::string result;
    std::string line;

    while (getline(file, line)) {
        result += line + "\n";
    } return result;
}

class RVRApp {
    public:
        RVRApp(int argc, char* argv[]);
        virtual ~RVRApp();

        bool init();
        void shutdown();
        void run();
        bool handleInput();
        void renderFrame();

        bool setupTexturemaps();
        void setupScene();
        void addCubeToScene(glm::mat4 mat, std::vector <float>& vertdata);
        void addCubeVertex(float fl0, float fl1, float fl2, float fl3, float fl4, std::vector <float>& vertdata);
        void setupWindow();

        void renderControllerAxes();
        void renderWindow();
        void renderScene();

        GLuint createProgram(const std::string& vertexShaderSourceStr, const std::string& fragmentShaderSourceStr);
        bool createAllShaders();

    private:
        GLFWwindow* window;
        unsigned int windowWidth;
        unsigned int windowHeight;

        bool showCubes;
        int sceneVolumeWidth;
        int sceneVolumeHeight;
        int sceneVolumeDepth;
        int sceneVolumeInit;
        float scaleSpacing;
        float scale;
        float nearClip;
        float farClip;

        unsigned int vertexCount;
        unsigned int cubeTexture;

        unsigned int sceneVertexBuffer;
        unsigned int sceneVertexArray;
        unsigned int windowVertexBuffer;
        unsigned int windowVertexArray;
        unsigned int windowIndexBuffer;
        unsigned int windowIndexCount;

        unsigned int controllerVertexBuffer;
        unsigned int controllerVertexArray;
        unsigned int controllerVertexCount;

        struct Vertex {
            glm::vec2 position;
            glm::vec2 texCoord;
        };

        unsigned int sceneProgram;
        unsigned int windowProgram;
        unsigned int controllerProgram;
        unsigned int renderModelProgram;

        int sceneMatrixLocation;
        int controllerMatrixLocation;
        int renderModelMatrixLocation;

        unsigned int renderWidth;
        unsigned int renderHeight;

        glm::vec3 position;
};

RVRApp::RVRApp(int argc, char* argv[])
    : window(nullptr),
      windowWidth(1200),
      windowHeight(600),
      sceneProgram(0),
      windowProgram(0),
      controllerProgram(0),
      renderModelProgram(0),
      controllerVertexBuffer(0),
      controllerVertexArray(0),
      sceneVertexArray(0),
      sceneMatrixLocation(-1),
      controllerMatrixLocation(-1),
      renderModelMatrixLocation(-1),
      sceneVolumeInit(20),
      showCubes(true) {};

RVRApp::~RVRApp() { }

bool RVRApp::init() {
    glfwInit();
    rvr::RVRSetActionManifestPath(ACTION_MANIFEST_PATH);
    rvr::RVRInit();
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(windowWidth, windowHeight, "RVR Example", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    gladLoadGL();
    glfwSwapInterval(1);

    std::string title = "RVR Example - " + rvr::driver + " " + rvr::display;
    glfwSetWindowTitle(window, title.c_str());

    sceneVolumeWidth = sceneVolumeInit;
    sceneVolumeHeight = sceneVolumeInit;
    sceneVolumeDepth = sceneVolumeInit;

    scale = 0.3f;
    scaleSpacing = 4.0f;

    nearClip = 0.1f;
    farClip = 30.0f;

    cubeTexture = 0;
    vertexCount = 0;

    position = { 0.0f, 0.0f, 0.0f };

    if (!createAllShaders())
        return false;
    
    rvr::RVRSetupStereoRenderTargets();
    rvr::RVRInitControllers();
    rvr::RVRInitEyes(nearClip, farClip);
    setupTexturemaps();
    setupScene();
    setupWindow();
    return true;
}

void RVRApp::shutdown() {
    rvr::RVRShutdown();
    rvr::RVRDeleteFramebufferDescriptors();

    glDeleteBuffers(1, &sceneVertexBuffer);

    if (sceneProgram) glDeleteProgram(sceneProgram);
    if (controllerProgram) glDeleteProgram(controllerProgram);
    if (renderModelProgram) glDeleteProgram(renderModelProgram);
    if (windowProgram) glDeleteProgram(windowProgram);

    if (windowVertexArray != 0) glDeleteVertexArrays(1, &windowVertexArray);
    if (sceneVertexArray != 0) glDeleteVertexArrays(1, &sceneVertexArray);
    if (controllerVertexArray != 0) glDeleteVertexArrays(1, &controllerVertexArray);

    if (window) {
        glfwDestroyWindow(window);
        window = NULL;
    } glfwTerminate();
}

bool RVRApp::handleInput() {
    bool destroyed = glfwWindowShouldClose(window);
    glfwPollEvents();
    if (glfwGetKey(window, GLFW_KEY_Q)) destroyed = true;

    rvr::RVRPollEvents();

    showCubes = !(rvr::RVRGetControllerTriggerClickState(rvr::RVRControllerLeft)
                || rvr::RVRGetControllerTriggerClickState(rvr::RVRControllerRight));

    for (rvr::RVRController controller = rvr::RVRControllerLeft; controller <= rvr::RVRControllerRight; ((int&) controller)++) {
        if (rvr::RVRGetControllerGripClickState(controller)) {
            rvr::RVRTriggerHapticVibration(controller, 1.0f, 4.0f, 1.0f);
        }

        if (rvr::RVRGetControllerButtonOneClickState(controller)) std::cout << (controller == rvr::RVRControllerLeft ? "left" : "right") << " button one click" << std::endl;
        if (rvr::RVRGetControllerButtonOneTouchState(controller)) std::cout << (controller == rvr::RVRControllerLeft ? "left" : "right") << " button one touch" << std::endl;
        if (rvr::RVRGetControllerButtonTwoClickState(controller)) std::cout << (controller == rvr::RVRControllerLeft ? "left" : "right") << " button two click" << std::endl;
        if (rvr::RVRGetControllerButtonTwoTouchState(controller)) std::cout << (controller == rvr::RVRControllerLeft ? "left" : "right") << " button two touch" << std::endl;
        if (rvr::RVRGetControllerGripClickState(controller)) std::cout << (controller == rvr::RVRControllerLeft ? "left" : "right") << " grip click" << std::endl;
        if (rvr::RVRGetControllerGripTouchState(controller)) std::cout << (controller == rvr::RVRControllerLeft ? "left" : "right") << " grip touch" << std::endl;
        if (rvr::RVRGetControllerTriggerClickState(controller)) std::cout << (controller == rvr::RVRControllerLeft ? "left" : "right") << " trigger click" << std::endl;
        if (rvr::RVRGetControllerTriggerTouchState(controller)) std::cout << (controller == rvr::RVRControllerLeft ? "left" : "right") << " trigger touch" << std::endl;
        if (rvr::RVRGetControllerJoystickClickState(controller)) std::cout << (controller == rvr::RVRControllerLeft ? "left" : "right") << " joystick click" << std::endl;
        if (rvr::RVRGetControllerJoystickTouchState(controller)) std::cout << (controller == rvr::RVRControllerLeft ? "left" : "right") << " joystick touch" << std::endl;
        float trigger_pull = rvr::RVRGetControllerTriggerPull(controller);
        if (trigger_pull != 0.0f) std::cout << (controller == rvr::RVRControllerLeft ? "left" : "right") << " trigger pull: " << trigger_pull << std::endl;
        float grip_pull = rvr::RVRGetControllerGripPull(controller);
        if (grip_pull != 0.0f) std::cout << (controller == rvr::RVRControllerLeft ? "left" : "right") << " grip pull: " << grip_pull << std::endl;
    }

    position.x -= rvr::RVRGetControllerJoystickPosition(rvr::RVRControllerLeft).x / 50;
    position.z += rvr::RVRGetControllerJoystickPosition(rvr::RVRControllerLeft).y / 50;
    position.y -= rvr::RVRGetControllerJoystickPosition(rvr::RVRControllerRight).y / 50;
    return destroyed;
}

void RVRApp::run() {
    bool destroyed = false;

    while (!destroyed) {
        destroyed = handleInput();
        renderFrame();
    }
}

void RVRApp::renderFrame() {
    if (rvr::RVRIsReady()) {
        renderControllerAxes();
        renderScene();
        renderWindow();

        rvr::RVRSubmitFramebufferDescriptorsToCompositor();
    }

    rvr::RVRUpdateHMDPoseMatrix();
}

unsigned int RVRApp::createProgram(const std::string& vertexShaderSourceStr, const std::string& fragmentShaderSourceStr) {
    unsigned int program = glCreateProgram();

    const char* vertexShaderSource = vertexShaderSourceStr.c_str();
    const char* fragmentShaderSource = fragmentShaderSourceStr.c_str();

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    int isVertexShaderCompiled = GL_FALSE;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isVertexShaderCompiled);

    if (isVertexShaderCompiled != GL_TRUE) {
        char log[1024];
        glGetShaderInfoLog(vertexShader, 1024, nullptr, log);
        std::cout << log << std::endl;
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        return 0;
    }

    glAttachShader(program, vertexShader);
    glDeleteShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    int isFragmentShaderCompiled = GL_FALSE;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isFragmentShaderCompiled);

    if (isFragmentShaderCompiled != GL_TRUE) {
        char log[1024];
        glGetShaderInfoLog(fragmentShader, 1024, nullptr, log);
        std::cout << log << std::endl;
        glDeleteProgram(program);
        glDeleteShader(fragmentShader);
        return 0;
    }

    glAttachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);
    glLinkProgram(program);
    int programSuccess = GL_TRUE;
    glGetProgramiv(program, GL_LINK_STATUS, &programSuccess);

    if (programSuccess != GL_TRUE) {
        char log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cout << log << std::endl;
        glDeleteProgram(program);
        return 0;
    } return program;
}

bool RVRApp::createAllShaders() {
    sceneProgram = createProgram(
        readFile("shaders/scene.vert"),
        readFile("shaders/scene.frag")
    ); sceneMatrixLocation = glGetUniformLocation(sceneProgram, "matrix");
    if (sceneMatrixLocation == -1) return false;

    controllerProgram = createProgram(
        readFile("shaders/controller.vert"),
        readFile("shaders/controller.frag")
    ); controllerMatrixLocation = glGetUniformLocation(controllerProgram, "matrix");
    if (controllerMatrixLocation == -1) return false;
    
    windowProgram = createProgram(
        readFile("shaders/window.vert"),
        readFile("shaders/window.frag")
    ); return sceneProgram != 0 && controllerProgram != 0 && windowProgram != 0;
}

bool RVRApp::setupTexturemaps() {
    int width, height, channels;
    unsigned char* bytes = stbi_load("res/cube_texture.png", &width, &height, &channels, 0);

    glGenTextures(1, &cubeTexture);
    glBindTexture(GL_TEXTURE_2D, cubeTexture);
    if (channels == 4) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
    if (channels == 3) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, bytes);
    if (channels == 2) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RG, GL_UNSIGNED_BYTE, bytes);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return (cubeTexture != 0);
}

void RVRApp::setupScene() {
    if (!rvr::RVRIsReady()) return;

    std::vector<float> vertdataarray;
    glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, scale));
    glm::mat4 matTransform = glm::translate(glm::mat4(1.0f),
        glm::vec3(
            -((float) sceneVolumeWidth * scaleSpacing) / 2.f,
            -((float) sceneVolumeHeight * scaleSpacing) / 2.f,
            -((float) sceneVolumeDepth * scaleSpacing) / 2.f)
    );

    glm::mat4 mat = scale_matrix * matTransform;

    for (int z = 0; z < sceneVolumeDepth; z++) {
        for (int y = 0; y < sceneVolumeHeight; y++) {
            for (int x = 0; x < sceneVolumeWidth; x++) {
                addCubeToScene(mat, vertdataarray);
                mat = mat * glm::translate(glm::mat4(1.0f), glm::vec3(scaleSpacing, 0, 0));
            } mat = mat * glm::translate(glm::mat4(1.0f), glm::vec3(-((float)sceneVolumeWidth) * scaleSpacing, scaleSpacing, 0));
        } mat = mat * glm::translate(glm::mat4(1.0f), glm::vec3(0, -((float)sceneVolumeHeight) * scaleSpacing, scaleSpacing));
    }

    vertexCount = vertdataarray.size() / 5;

    glGenVertexArrays(1, &sceneVertexArray);
    glBindVertexArray(sceneVertexArray);

    glGenBuffers(1, &sceneVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, sceneVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertdataarray.size(), &vertdataarray[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * (3 + 2), (const void*) 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * (3 + 2), (const void*) (3 * sizeof(float)));

    glBindVertexArray(0);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

void RVRApp::addCubeVertex(float fl0, float fl1, float fl2, float fl3, float fl4, std::vector <float>& vertdata) {
    vertdata.push_back(fl0);
    vertdata.push_back(fl1);
    vertdata.push_back(fl2);
    vertdata.push_back(fl3);
    vertdata.push_back(fl4);
}

void RVRApp::addCubeToScene(glm::mat4 mat, std::vector <float>& vertdata) {
    glm::vec4 A = mat * glm::vec4(0, 0, 0, 1);
    glm::vec4 B = mat * glm::vec4(1, 0, 0, 1);
    glm::vec4 C = mat * glm::vec4(1, 1, 0, 1);
    glm::vec4 D = mat * glm::vec4(0, 1, 0, 1);
    glm::vec4 E = mat * glm::vec4(0, 0, 1, 1);
    glm::vec4 F = mat * glm::vec4(1, 0, 1, 1);
    glm::vec4 G = mat * glm::vec4(1, 1, 1, 1);
    glm::vec4 H = mat * glm::vec4(0, 1, 1, 1);

    addCubeVertex(E.x, E.y, E.z, 0, 1, vertdata);
    addCubeVertex(F.x, F.y, F.z, 1, 1, vertdata);
    addCubeVertex(G.x, G.y, G.z, 1, 0, vertdata);
    addCubeVertex(G.x, G.y, G.z, 1, 0, vertdata);
    addCubeVertex(H.x, H.y, H.z, 0, 0, vertdata);
    addCubeVertex(E.x, E.y, E.z, 0, 1, vertdata);

    addCubeVertex(B.x, B.y, B.z, 0, 1, vertdata);
    addCubeVertex(A.x, A.y, A.z, 1, 1, vertdata);
    addCubeVertex(D.x, D.y, D.z, 1, 0, vertdata);
    addCubeVertex(D.x, D.y, D.z, 1, 0, vertdata);
    addCubeVertex(C.x, C.y, C.z, 0, 0, vertdata);
    addCubeVertex(B.x, B.y, B.z, 0, 1, vertdata);

    addCubeVertex(H.x, H.y, H.z, 0, 1, vertdata);
    addCubeVertex(G.x, G.y, G.z, 1, 1, vertdata);
    addCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
    addCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
    addCubeVertex(D.x, D.y, D.z, 0, 0, vertdata);
    addCubeVertex(H.x, H.y, H.z, 0, 1, vertdata);

    addCubeVertex(A.x, A.y, A.z, 0, 1, vertdata);
    addCubeVertex(B.x, B.y, B.z, 1, 1, vertdata);
    addCubeVertex(F.x, F.y, F.z, 1, 0, vertdata);
    addCubeVertex(F.x, F.y, F.z, 1, 0, vertdata);
    addCubeVertex(E.x, E.y, E.z, 0, 0, vertdata);
    addCubeVertex(A.x, A.y, A.z, 0, 1, vertdata);

    addCubeVertex(A.x, A.y, A.z, 0, 1, vertdata);
    addCubeVertex(E.x, E.y, E.z, 1, 1, vertdata);
    addCubeVertex(H.x, H.y, H.z, 1, 0, vertdata);
    addCubeVertex(H.x, H.y, H.z, 1, 0, vertdata);
    addCubeVertex(D.x, D.y, D.z, 0, 0, vertdata);
    addCubeVertex(A.x, A.y, A.z, 0, 1, vertdata);

    addCubeVertex(F.x, F.y, F.z, 0, 1, vertdata);
    addCubeVertex(B.x, B.y, B.z, 1, 1, vertdata);
    addCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
    addCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
    addCubeVertex(G.x, G.y, G.z, 0, 0, vertdata);
    addCubeVertex(F.x, F.y, F.z, 0, 1, vertdata);
}

void RVRApp::renderControllerAxes() {
    if (!rvr::RVRIsInputAvailable()) return;

    std::vector<float> vertdataarray;
    controllerVertexCount = 0;

    for (rvr::RVRController controller = rvr::RVRControllerLeft; controller <= rvr::RVRControllerRight; ((int&) controller)++) {
        if (!rvr::RVRGetControllerShowState(controller))
            continue;

        const glm::mat4& mat = rvr::RVRGetController(controller).poseMatrix;;
        glm::vec4 center = mat * glm::vec4(0, 0, 0, 1);

        for (int i = 0; i < 3; ++i) {
            glm::vec3 color(0, 0, 0);
            glm::vec4 point(0, 0, 0, 1);
            point[i] += 0.05f;
            color[i] = 1.0;
            point = mat * point;
            vertdataarray.push_back(center.x);
            vertdataarray.push_back(center.y);
            vertdataarray.push_back(center.z);
            vertdataarray.push_back(color.x);
            vertdataarray.push_back(color.y);
            vertdataarray.push_back(color.z);
            vertdataarray.push_back(point.x);
            vertdataarray.push_back(point.y);
            vertdataarray.push_back(point.z);
            vertdataarray.push_back(color.x);
            vertdataarray.push_back(color.y);
            vertdataarray.push_back(color.z);
            controllerVertexCount += 2;
        }

        glm::vec4 start = mat * glm::vec4(0, 0, -0.02f, 1);
        glm::vec4 end = mat * glm::vec4(0, 0, -39.f, 1);
        glm::vec3 color(0.92f, 0.92f, 0.71f);

        vertdataarray.push_back(start.x); vertdataarray.push_back(start.y); vertdataarray.push_back(start.z);
        vertdataarray.push_back(color.x); vertdataarray.push_back(color.y); vertdataarray.push_back(color.z);
        vertdataarray.push_back(end.x); vertdataarray.push_back(end.y); vertdataarray.push_back(end.z);
        vertdataarray.push_back(color.x); vertdataarray.push_back(color.y); vertdataarray.push_back(color.z);
        controllerVertexCount += 2;
    }

    if (controllerVertexArray == 0) {
        glGenVertexArrays(1, &controllerVertexArray);
        glBindVertexArray(controllerVertexArray);

        glGenBuffers(1, &controllerVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, controllerVertexBuffer);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2 * 3 * sizeof(float), (const void*) 0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 2 * 3 * sizeof(float), (const void*) (3 * sizeof(float)));
        glBindVertexArray(0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, controllerVertexBuffer);

    if (vertdataarray.size() > 0)
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertdataarray.size(), &vertdataarray[0], GL_STREAM_DRAW);
}

void RVRApp::setupWindow() {
    if (!rvr::RVRIsReady()) return;

    std::vector <Vertex> vertices = {
        { glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 1.0f) },
        { glm::vec2( 0.0f, -1.0f), glm::vec2(1.0f, 1.0f) },
        { glm::vec2(-1.0f,  1.0f), glm::vec2(0.0f, 0.0f) },
        { glm::vec2( 0.0f,  1.0f), glm::vec2(1.0f, 0.0f) },
        { glm::vec2( 0.0f, -1.0f), glm::vec2(0.0f, 1.0f) },
        { glm::vec2( 1.0f, -1.0f), glm::vec2(1.0f, 1.0f) },
        { glm::vec2( 0.0f,  1.0f), glm::vec2(0.0f, 0.0f) },
        { glm::vec2( 1.0f,  1.0f), glm::vec2(1.0f, 0.0f) }
    }; std::vector <unsigned short> indices = { 0, 1, 3, 0, 3, 2, 4, 5, 7, 4, 7, 6 };

    windowIndexCount = indices.size();
    glGenVertexArrays(1, &windowVertexArray);
    glBindVertexArray(windowVertexArray);

    glGenBuffers(1, &windowVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, windowVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &windowIndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, windowIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * indices.size(), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, texCoord));

    glBindVertexArray(0);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void RVRApp::renderScene() {
    for (rvr::RVREye eye = rvr::RVREyeLeft; eye <= rvr::RVREyeRight; ((int&) eye)++) {
        rvr::RVRBeginRendering(eye);
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        if (showCubes) {
            glUseProgram(sceneProgram);
            glUniformMatrix4fv(sceneMatrixLocation, 1, GL_FALSE, glm::value_ptr(glm::translate(rvr::RVRGetCurrentViewProjectionMatrix(eye), position)));
            glBindVertexArray(sceneVertexArray);
            glBindTexture(GL_TEXTURE_2D, cubeTexture);
            glDrawArrays(GL_TRIANGLES, 0, vertexCount);
            glBindVertexArray(0);
        }

        if (rvr::RVRIsInputAvailable()) {
            glUseProgram(controllerProgram);
            glUniformMatrix4fv(controllerMatrixLocation, 1, GL_FALSE, glm::value_ptr(rvr::RVRGetCurrentViewProjectionMatrix(eye)));
            glBindVertexArray(controllerVertexArray);
            glDrawArrays(GL_LINES, 0, controllerVertexCount);
            glBindVertexArray(0);
        }

        rvr::RVRRenderControllers();
        rvr::RVREndRendering();
    }
}

void RVRApp::renderWindow() {
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, windowWidth, windowHeight);
    glBindVertexArray(windowVertexArray);
    glUseProgram(windowProgram);
    glBindTexture(GL_TEXTURE_2D, rvr::leftEyeDesc.resolveTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glDrawElements(GL_TRIANGLES, windowIndexCount / 2, GL_UNSIGNED_SHORT, (const void*) (uintptr_t) (windowIndexCount));
    glBindTexture(GL_TEXTURE_2D, rvr::rightEyeDesc.resolveTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glDrawElements(GL_TRIANGLES, windowIndexCount / 2, GL_UNSIGNED_SHORT, (const void*) (uintptr_t) (windowIndexCount));
    glBindVertexArray(0);
    glUseProgram(0);
    glfwSwapBuffers(window);
}

int main(int argc, char* argv[]) {
    RVRApp* app = new RVRApp(argc, argv);

    if (!app->init()) {
        app->shutdown();
        return -1;
    }

    app->run();
    app->shutdown();
    return 0;
}