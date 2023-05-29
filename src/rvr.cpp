#include "rvr.hpp"

#ifdef RVR_EXTERN
extern "C" {
#endif

namespace rvr {
    vr::IVRSystem* hmd;
    vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
    glm::mat4 devicePoseMatrix[vr::k_unMaxTrackedDeviceCount];
    char deviceClassChar[vr::k_unMaxTrackedDeviceCount];
    std::string poseClasses;
    unsigned int validPoseCount;
    glm::mat4 hmdPose;
    glm::mat4 leftProjectionMatrix;
    glm::mat4 rightProjectionMatrix;
    glm::mat4 leftEyePosMatrix;
    glm::mat4 rightEyePosMatrix;
    std::string actionManfiestPath;
    std::string driver;
    std::string display;

    FramebufferDesc leftEyeDesc;
    FramebufferDesc rightEyeDesc;

    unsigned int renderModelProgram, renderModelMatrixLocation;

    RVRControllerInfo RVRControllers[2];
    std::vector <RVRCGLRenderModel*> renderModels;
    vr::VRActionSetHandle_t actionSetRVR;

    RVRTrackedDeviceDeactivateCallback_T* RVRTrackedDeviceDeactivateCallback;
    RVRTrackedDeviceUpdateCallback_T* RVRTrackedDeviceUpdateCallback;

    RVREye currentEye = RVREyeLeft;

    bool RVRCGLRenderModel::init(vr::RenderModel_t* model, vr::RenderModel_TextureMap_t* texture) {
        renderModel = model;
        textureMap = texture;

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vr::RenderModel_Vertex_t) * renderModel->unVertexCount, renderModel->rVertexData, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void*) offsetof(vr::RenderModel_Vertex_t, vPosition));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void*) offsetof(vr::RenderModel_Vertex_t, vNormal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void*) offsetof(vr::RenderModel_Vertex_t, rfTextureCoord));

        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * renderModel->unTriangleCount * 3, renderModel->rIndexData, GL_STATIC_DRAW);

        glBindVertexArray(0);
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureMap->unWidth, textureMap->unHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureMap->rubTextureMapData);

        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }
    
    bool RVRCGLRenderModel::render() {
        if (vbo) {
            glBindVertexArray(vao);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
            glDrawElements(GL_TRIANGLES, renderModel->unTriangleCount * 3, GL_UNSIGNED_SHORT, 0);
            glBindVertexArray(0);
        } return true;
    }

    bool RVRCGLRenderModel::destroy() {
        vr::VRRenderModels()->FreeRenderModel(renderModel);
        vr::VRRenderModels()->FreeTexture(textureMap);

        if (vbo) {
            glDeleteBuffers(1, &ibo);
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteTextures(1, &tex);
            ibo = 0; vao = 0; vbo = 0, tex = 0;
        } return true;
    }

    RVRCGLRenderModel::RVRCGLRenderModel(const std::string& renderModelName) : modelName(renderModelName) { }
    RVRCGLRenderModel::~RVRCGLRenderModel() { destroy(); }

    const std::string& RVRCGLRenderModel::getName() const { return modelName; }
    vr::RenderModel_t* RVRCGLRenderModel::getRenderModel() { return renderModel; }
    vr::RenderModel_TextureMap_t* RVRCGLRenderModel::getTextureMap() { return textureMap; }

    bool RVRCreateFrameBuffer(int width, int height, FramebufferDesc& framebufferDesc) {
        glGenFramebuffers(1, &framebufferDesc.renderFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.renderFramebuffer);
        glGenRenderbuffers(1, &framebufferDesc.depthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc.depthBuffer);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.depthBuffer);
        glGenTextures(1, &framebufferDesc.renderTexture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.renderTexture);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, width, height, true);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.renderTexture, 0);
        glGenFramebuffers(1, &framebufferDesc.resolveFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.resolveFramebuffer);
        glGenTextures(1, &framebufferDesc.resolveTexture);
        glBindTexture(GL_TEXTURE_2D, framebufferDesc.resolveTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.resolveTexture, 0);
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) return false;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
    }

    RVRCGLRenderModel* RVRFindOrLoadRenderModel(const char* renderModelName) {
        RVRCGLRenderModel* renderModel = NULL;

        for (std::vector <RVRCGLRenderModel*>::iterator i = renderModels.begin(); i != renderModels.end(); i++) {
            if (!_stricmp((*i)->getName().c_str(), renderModelName)) {
                renderModel = *i;
                break;
            }
        }

        if (!renderModel) {
            vr::RenderModel_t* model;
            vr::EVRRenderModelError error;

            while (true) {
                error = vr::VRRenderModels()->LoadRenderModel_Async(renderModelName, &model);
                if (error != vr::VRRenderModelError_Loading) break;
                Sleep(1);
            }

            if (error != vr::VRRenderModelError_None) {
                return NULL;
            }

            vr::RenderModel_TextureMap_t* texture;

            while (true) {
                error = vr::VRRenderModels()->LoadTexture_Async(model->diffuseTextureId, &texture);
                if (error != vr::VRRenderModelError_Loading) break;
                Sleep(1);
            }

            if (error != vr::VRRenderModelError_None) {
                vr::VRRenderModels()->FreeRenderModel(model);
                return NULL;
            }

            renderModel = new RVRCGLRenderModel(renderModelName);

            if (!renderModel->init(model, texture)) {
                delete renderModel;
                renderModel = NULL;
            } else renderModels.push_back(renderModel);
        }

        return renderModel;
    }

    std::string RVRGetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError) {
        uint32_t unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
        if (unRequiredBufferLen == 0) return "";
        char* pchBuffer = new char[unRequiredBufferLen];
        unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
        std::string sResult = pchBuffer;
        delete[] pchBuffer;
        return sResult;
    }

    void RVRGetRecommendedRenderTargetSize(unsigned int* width, unsigned int* height) {
        hmd->GetRecommendedRenderTargetSize(width, height);
    }

    bool RVRIsReady() {
        if (hmd) return true;
        else return false;
    }

    bool RVRSetupStereoRenderTargets() {
        if (!RVRIsReady()) return false;
        unsigned int renderWidth, renderHeight;
        RVRGetRecommendedRenderTargetSize(&renderWidth, &renderHeight);
        RVRCreateFrameBuffer(renderWidth, renderHeight, leftEyeDesc);
        RVRCreateFrameBuffer(renderWidth, renderHeight, rightEyeDesc);
        return true;
    }

    bool RVRBeginRendering(RVREye eye) {
        glEnable(GL_MULTISAMPLE);
        glBindFramebuffer(GL_FRAMEBUFFER, eye == RVREyeLeft ? leftEyeDesc.renderFramebuffer : rightEyeDesc.renderFramebuffer);
        unsigned int renderWidth, renderHeight;
        RVRGetRecommendedRenderTargetSize(&renderWidth, &renderHeight);
        glViewport(0, 0, renderWidth, renderHeight);
        currentEye = eye;
        return true;
    }

    bool RVREndRendering() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_MULTISAMPLE);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, currentEye == RVREyeLeft ? leftEyeDesc.renderFramebuffer : rightEyeDesc.renderFramebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, currentEye == RVREyeLeft ? leftEyeDesc.resolveFramebuffer : rightEyeDesc.resolveFramebuffer);
        unsigned int renderWidth, renderHeight;
        RVRGetRecommendedRenderTargetSize(&renderWidth, &renderHeight);
        glBlitFramebuffer(0, 0, renderWidth, renderHeight, 0, 0, renderWidth, renderHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        currentEye = RVREyeLeft;
        return true;
    }

    bool RVRSubmitFramebufferDescriptorsToCompositor() {
        vr::Texture_t leftEyeTexture = { (void*) (uintptr_t) leftEyeDesc.resolveTexture, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
        vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
        vr::Texture_t rightEyeTexture = { (void*) (uintptr_t) rightEyeDesc.resolveTexture, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
        vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
        return true;
    }

    bool RVRDeleteFramebufferDescriptors() {
        glDeleteRenderbuffers(1, &leftEyeDesc.depthBuffer);
        glDeleteTextures(1, &leftEyeDesc.renderTexture);
        glDeleteFramebuffers(1, &leftEyeDesc.renderFramebuffer);
        glDeleteTextures(1, &leftEyeDesc.resolveTexture);
        glDeleteFramebuffers(1, &leftEyeDesc.resolveFramebuffer);

        glDeleteRenderbuffers(1, &rightEyeDesc.depthBuffer);
        glDeleteTextures(1, &rightEyeDesc.renderTexture);
        glDeleteFramebuffers(1, &rightEyeDesc.renderFramebuffer);
        glDeleteTextures(1, &rightEyeDesc.resolveTexture);
        glDeleteFramebuffers(1, &rightEyeDesc.resolveFramebuffer);
        return true;
    }

    unsigned int RVRCreateShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource) {
        unsigned int program = glCreateProgram();

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

    bool RVRInitControllers() {
        renderModelProgram = RVRCreateShaderProgram(
            "#version 460 core\n"
            "\n"
            "uniform mat4 matrix;\n"
            "layout(location = 0) in vec4 position;\n"
            "layout(location = 1) in vec3 normals;\n"
            "layout(location = 2) in vec2 texCoords;\n"
            "out vec2 fragTexCoords;\n"
            "\n"
            "void main() {\n"
            "   fragTexCoords = texCoords;\n"
            "   gl_Position = matrix * vec4(position.xyz, 1);\n"
            "}\n",
            "#version 460 core\n"
            "\n"
            "uniform sampler2D diffuse;\n"
            "in vec2 fragTexCoords;\n"
            "\n"
            "void main() {\n"
            "   gl_FragColor = texture(diffuse, fragTexCoords);\n"
            "}\n"
        );

        renderModelMatrixLocation = glGetUniformLocation(renderModelProgram, "matrix");

        if (renderModelMatrixLocation == -1)
            return false;

        return renderModelProgram != 0;
    }

    RVRControllerInfo& RVRGetController(RVRController controller) {
        return RVRControllers[controller];
    }

    void RVRSetControllerShowState(RVRController controller, bool state) {
        RVRGetController(controller).showController = state;
    }

    bool RVRGetControllerShowState(RVRController controller) {
        return RVRGetController(controller).showController;
    }

    glm::mat4 RVRGetCurrentViewProjectionMatrix(RVREye eye) {
        glm::mat4 matrix;

        if (eye == RVREyeLeft)
            matrix = leftProjectionMatrix * leftEyePosMatrix * hmdPose;

        if (eye == RVREyeRight)
            matrix = rightProjectionMatrix * rightEyePosMatrix * hmdPose;

        return matrix;
    }

    float* RVRGetCurrentViewProjectionMatrixArray(RVREye eye) {
        glm::mat4 matrix = RVRGetCurrentViewProjectionMatrix(eye);
        return glm::value_ptr(matrix);
    }

    glm::mat4 RVRGetCurrentViewProjectionNoPoseMatrix(RVREye eye) {
        glm::mat4 matrix;

        if (eye == RVREyeLeft)
            matrix = leftProjectionMatrix * leftEyePosMatrix;

        if (eye == RVREyeRight)
            matrix = rightProjectionMatrix * rightEyePosMatrix;

        return matrix;
    }

    float* RVRGetCurrentViewProjectionNoPoseMatrixArray(RVREye eye) {
        glm::mat4 matrix = RVRGetCurrentViewProjectionNoPoseMatrix(eye);
        return glm::value_ptr(matrix);
    }

    glm::vec3 RVRGetHmdDirection() {
        glm::vec4 direction = glm::inverse(hmdPose) * glm::vec4(0.0, 0.0, 1.0, 0.0);
        return glm::vec3(direction.x, direction.y, direction.z);
    }

    bool RVRRenderControllers() {
        glEnable(GL_DEPTH_TEST);
        glUseProgram(renderModelProgram);

        for (RVRController controller = RVRControllerLeft; controller <= RVRControllerRight; ((int&) controller)++) {
            if (!RVRGetControllerShowState(controller) || !RVRGetController(controller).renderModel)
                continue;

            const glm::mat4& matDeviceToTracking = RVRGetController(controller).poseMatrix;
            glm::mat4 matMVP = RVRGetCurrentViewProjectionMatrix(currentEye) * matDeviceToTracking;
            glUniformMatrix4fv(renderModelMatrixLocation, 1, GL_FALSE, glm::value_ptr(matMVP));
            RVRGetController(controller).renderModel->render();
        }

        glUseProgram(0);
        glDisable(GL_DEPTH_TEST);
        return true;
    }

    void RVRSetActionManifestPath(const char* path) {
        actionManfiestPath = path;
    }

    bool RVRInit() {
        gladLoadGL();
        currentEye = RVREyeLeft;
        vr::EVRInitError error = vr::VRInitError_None;
        hmd = vr::VR_Init(&error, vr::VRApplication_Scene);
        vr::VRCompositor();

        driver = "No Driver";
        display = "No Display";

        driver = RVRGetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
        display = RVRGetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);

        vr::VRInput()->SetActionManifestPath(actionManfiestPath.c_str());
        vr::VRInput()->GetActionSetHandle("/actions/rvr", &actionSetRVR);

        vr::VRInput()->GetActionHandle("/actions/rvr/out/haptic_left", &RVRControllers[0].actionHaptic);
        vr::VRInput()->GetInputSourceHandle("/user/hand/left", &RVRControllers[0].source);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/hand_left", &RVRControllers[0].actionPose);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/left_joystick_click", &RVRControllers[0].actionJoystickClick);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/left_joystick_touch", &RVRControllers[0].actionJoystickTouch);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/left_joystick_position", &RVRControllers[0].actionJoystickPosition);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/left_grip_click", &RVRControllers[0].actionGripClick);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/left_grip_trouch", &RVRControllers[0].actionGripTouch);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/left_grip_pull", &RVRControllers[0].actionGripPull);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/left_trigger_click", &RVRControllers[0].actionTriggerClick);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/left_trigger_trouch", &RVRControllers[0].actionTriggerTouch);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/left_trigger_pull", &RVRControllers[0].actionTriggerPull);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/x_click", &RVRControllers[0].actionButtonOneClick);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/x_trouch", &RVRControllers[0].actionButtonOneTouch);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/y_click", &RVRControllers[0].actionButtonTwoClick);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/y_trouch", &RVRControllers[0].actionButtonTwoTouch);

        vr::VRInput()->GetActionHandle("/actions/rvr/out/haptic_right", &RVRControllers[1].actionHaptic);
        vr::VRInput()->GetInputSourceHandle("/user/hand/right", &RVRControllers[1].source);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/hand_right", &RVRControllers[1].actionPose);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/right_joystick_click", &RVRControllers[1].actionJoystickClick);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/right_joystick_touch", &RVRControllers[1].actionJoystickTouch);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/right_joystick_position", &RVRControllers[1].actionJoystickPosition);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/right_grip_click", &RVRControllers[1].actionGripClick);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/right_grip_trouch", &RVRControllers[1].actionGripTouch);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/right_grip_pull", &RVRControllers[1].actionGripPull);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/right_trigger_click", &RVRControllers[1].actionTriggerClick);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/right_trigger_trouch", &RVRControllers[1].actionTriggerTouch);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/right_trigger_pull", &RVRControllers[1].actionTriggerPull);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/a_click", &RVRControllers[1].actionButtonOneClick);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/a_trouch", &RVRControllers[1].actionButtonOneTouch);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/b_click", &RVRControllers[1].actionButtonTwoClick);
        vr::VRInput()->GetActionHandle("/actions/rvr/in/b_trouch", &RVRControllers[1].actionButtonTwoTouch);
        return true;
    }

    bool RVRGetDigitalActionRisingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath) {
        vr::InputDigitalActionData_t actionData;
        vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);

        if (pDevicePath) {
            *pDevicePath = vr::k_ulInvalidInputValueHandle;

            if (actionData.bActive) {
                vr::InputOriginInfo_t originInfo;

                if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
                    *pDevicePath = originInfo.devicePath;
            }
        } return actionData.bActive && actionData.bChanged && actionData.bState;
    }

    bool RVRGetDigitalActionFallingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath) {
        vr::InputDigitalActionData_t actionData;
        vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);

        if (pDevicePath) {
            *pDevicePath = vr::k_ulInvalidInputValueHandle;

            if (actionData.bActive) {
                vr::InputOriginInfo_t originInfo;

                if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
                    *pDevicePath = originInfo.devicePath;
            }
        } return actionData.bActive && actionData.bChanged && !actionData.bState;
    }

    bool RVRGetDigitalActionState(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath) {
        vr::InputDigitalActionData_t actionData;
        vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);

        if (pDevicePath) {
            *pDevicePath = vr::k_ulInvalidInputValueHandle;

            if (actionData.bActive) {
                vr::InputOriginInfo_t originInfo;

                if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
                    *pDevicePath = originInfo.devicePath;
            }
        } return actionData.bActive && actionData.bState;
    }

    bool RVRGetControllerTriggerClickState(RVRController controller) {
        return RVRGetDigitalActionState(RVRGetController(controller).actionTriggerClick);
    }

    bool RVRGetControllerTriggerClickRisingEdge(RVRController controller) {
        return RVRGetDigitalActionRisingEdge(RVRGetController(controller).actionTriggerClick);
    }

    bool RVRGetControllerTriggerClickFallingEdge(RVRController controller) {
        return RVRGetDigitalActionFallingEdge(RVRGetController(controller).actionTriggerClick);
    }

    bool RVRGetControllerGripClickState(RVRController controller) {
        return RVRGetDigitalActionState(RVRGetController(controller).actionGripClick);
    }

    bool RVRGetControllerGripClickRisingEdge(RVRController controller) {
        return RVRGetDigitalActionRisingEdge(RVRGetController(controller).actionGripClick);
    }

    bool RVRGetControllerGripClickFallingEdge(RVRController controller) {
        return RVRGetDigitalActionFallingEdge(RVRGetController(controller).actionGripClick);
    }

    bool RVRGetControllerJoystickClickState(RVRController controller) {
        return RVRGetDigitalActionState(RVRGetController(controller).actionJoystickClick);
    }

    bool RVRGetControllerJoystickClickRisingEdge(RVRController controller) {
        return RVRGetDigitalActionRisingEdge(RVRGetController(controller).actionJoystickClick);
    }

    bool RVRGetControllerJoystickClickFallingEdge(RVRController controller) {
        return RVRGetDigitalActionFallingEdge(RVRGetController(controller).actionJoystickClick);
    }

    bool RVRGetControllerTriggerTouchState(RVRController controller) {
        return RVRGetDigitalActionState(RVRGetController(controller).actionTriggerTouch);
    }

    bool RVRGetControllerTriggerTouchRisingEdge(RVRController controller) {
        return RVRGetDigitalActionRisingEdge(RVRGetController(controller).actionTriggerTouch);
    }

    bool RVRGetControllerTriggerTouchFallingEdge(RVRController controller) {
        return RVRGetDigitalActionFallingEdge(RVRGetController(controller).actionTriggerTouch);
    }

    bool RVRGetControllerGripTouchState(RVRController controller) {
        return RVRGetDigitalActionState(RVRGetController(controller).actionGripTouch);
    }

    bool RVRGetControllerGripTouchRisingEdge(RVRController controller) {
        return RVRGetDigitalActionRisingEdge(RVRGetController(controller).actionGripTouch);
    }

    bool RVRGetControllerGripTouchFallingEdge(RVRController controller) {
        return RVRGetDigitalActionFallingEdge(RVRGetController(controller).actionGripTouch);
    }

    bool RVRGetControllerJoystickTouchState(RVRController controller) {
        return RVRGetDigitalActionState(RVRGetController(controller).actionJoystickTouch);
    }

    bool RVRGetControllerJoystickTouchRisingEdge(RVRController controller) {
        return RVRGetDigitalActionRisingEdge(RVRGetController(controller).actionJoystickTouch);
    }

    bool RVRGetControllerJoystickTouchFallingEdge(RVRController controller) {
        return RVRGetDigitalActionFallingEdge(RVRGetController(controller).actionJoystickTouch);
    }

    bool RVRGetControllerButtonOneClickState(RVRController controller) {
        return RVRGetDigitalActionState(RVRGetController(controller).actionButtonOneClick);
    }

    bool RVRGetControllerButtonOneClickRisingEdge(RVRController controller) {
        return RVRGetDigitalActionRisingEdge(RVRGetController(controller).actionButtonOneClick);
    }

    bool RVRGetControllerButtonOneClickFallingEdge(RVRController controller) {
        return RVRGetDigitalActionFallingEdge(RVRGetController(controller).actionButtonOneClick);
    }

    bool RVRGetControllerButtonOneTouchState(RVRController controller) {
        return RVRGetDigitalActionState(RVRGetController(controller).actionButtonOneTouch);
    }

    bool RVRGetControllerButtonOneTouchRisingEdge(RVRController controller) {
        return RVRGetDigitalActionRisingEdge(RVRGetController(controller).actionButtonOneTouch);
    }

    bool RVRGetControllerButtonOneTouchFallingEdge(RVRController controller) {
        return RVRGetDigitalActionFallingEdge(RVRGetController(controller).actionButtonOneTouch);
    }

    bool RVRGetControllerButtonTwoClickState(RVRController controller) {
        return RVRGetDigitalActionState(RVRGetController(controller).actionButtonTwoClick);
    }

    bool RVRGetControllerButtonTwoClickRisingEdge(RVRController controller) {
        return RVRGetDigitalActionRisingEdge(RVRGetController(controller).actionButtonTwoClick);
    }

    bool RVRGetControllerButtonTwoClickFallingEdge(RVRController controller) {
        return RVRGetDigitalActionFallingEdge(RVRGetController(controller).actionButtonTwoClick);
    }

    bool RVRGetControllerButtonTwoTouchState(RVRController controller) {
        return RVRGetDigitalActionState(RVRGetController(controller).actionButtonTwoTouch);
    }

    bool RVRGetControllerButtonTwoTouchRisingEdge(RVRController controller) {
        return RVRGetDigitalActionRisingEdge(RVRGetController(controller).actionButtonTwoTouch);
    }

    bool RVRGetControllerButtonTwoTouchFallingEdge(RVRController controller) {
        return RVRGetDigitalActionFallingEdge(RVRGetController(controller).actionButtonTwoTouch);
    }

    glm::vec2 RVRGetAnalogActionData(vr::VRActionHandle_t action) {
        vr::InputAnalogActionData_t analogData;

        if (vr::VRInput()->GetAnalogActionData(action, &analogData, sizeof(analogData), vr::k_ulInvalidInputValueHandle) == vr::VRInputError_None && analogData.bActive) {
            return glm::vec2(analogData.x, analogData.y);
        } return glm::vec2(0.0f, 0.0f);
    }

    float RVRGetControllerTriggerPull(RVRController controller) {
        return RVRGetAnalogActionData(RVRGetController(controller).actionTriggerPull).x;
    }

    float RVRGetControllerGripPull(RVRController controller) {
        return RVRGetAnalogActionData(RVRGetController(controller).actionGripPull).x;
    }

    glm::vec2 RVRGetControllerJoystickPosition(RVRController controller) {
        return RVRGetAnalogActionData(RVRGetController(controller).actionJoystickPosition);
    }

    void RVRTriggerHapticVibration(RVRController controller, float duration, float frequency, float amplitude) {
        vr::VRInput()->TriggerHapticVibrationAction(RVRGetController(controller).actionHaptic, 0, duration, frequency, amplitude, vr::k_ulInvalidInputValueHandle);
    }

    bool RVRShutdown() {
        if (!RVRIsReady()) return false;

        for (std::vector <RVRCGLRenderModel*>::iterator i = renderModels.begin(); i != renderModels.end(); i++)
            delete (*i);

        for (RVRController controller = RVRControllerLeft; controller <= RVRControllerRight; ((int&) controller)++) {
            RVRGetController(controller).renderModel = nullptr;
            RVRGetController(controller).renderModelName = "";
        }

        renderModels.clear();
        vr::VR_Shutdown();
        hmd = NULL;
        return true;
    }

    bool RVRIsInputAvailable() {
        return hmd->IsInputAvailable();
    }

    glm::mat4 RVRGetProjectionMatrix(RVREye eye, float nearClip, float farClip) {
        if (!RVRIsReady()) return glm::mat4(1.0f);
        vr::HmdMatrix44_t mat = hmd->GetProjectionMatrix(eye == RVREyeLeft ? vr::Eye_Left : vr::Eye_Right, nearClip, farClip);

        return glm::mat4(
            mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
            mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
            mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
            mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
        );
    }

    float* RVRGetProjectionMatrixArray(RVREye eye, float nearClip, float farClip) {
        glm::mat4 mat = RVRGetProjectionMatrix(eye, nearClip, farClip);
        return glm::value_ptr(mat);
    }

    glm::mat4 RVRGetEyePoseMatrix(RVREye eye) {
        if (!RVRIsReady()) return glm::mat4(1.0f);
        vr::HmdMatrix34_t mat = hmd->GetEyeToHeadTransform(eye == RVREyeLeft ? vr::Eye_Left : vr::Eye_Right);

        return glm::inverse(glm::mat4(
            mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0,
            mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0,
            mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0,
            mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f
        ));
    }
    
    float* RVRGetEyePoseMatrixArray(RVREye eye) {
        glm::mat4 mat = RVRGetEyePoseMatrix(eye);
        return glm::value_ptr(mat);
    }

    void RVRSetTrackedDeviceDeactivateCallback(RVRTrackedDeviceDeactivateCallback_T* func) {
        RVRTrackedDeviceDeactivateCallback = func;
    }

    void RVRSetTrackedDeviceUpdateCallback(RVRTrackedDeviceUpdateCallback_T* func) {
        RVRTrackedDeviceUpdateCallback = func;
    }

    glm::mat4 RVRConvertOpenVRMatrixToGLMMatrix(const vr::HmdMatrix34_t& mat) {
        return glm::mat4(
            mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0,
            mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0,
            mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0,
            mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f
        );
    }

    float* RVRGetControllerPoseMatrixArray(RVRController controller) {
        return glm::value_ptr(RVRGetController(controller).poseMatrix);
    }

    glm::mat4 RVRGetControllerPoseMatrix(RVRController controller) {
        return RVRGetController(controller).poseMatrix;
    }

    glm::vec3 RVRGetControllerPosition(RVRController controller) {
        glm::mat4 mat = RVRGetController(controller).poseMatrix;

        return glm::vec3(
            mat[3][0],
            mat[3][1],
            mat[3][2]
        );
    }

    float* RVRGetHmdPoseMatrixArray() {
        return glm::value_ptr(hmdPose);
    }

    glm::mat4 RVRGetHmdPoseMatrix() {
        return hmdPose;
    }

    glm::vec3 RVRGetHmdPosition() {
        glm::mat4 mat = RVRGetCurrentViewProjectionMatrix(currentEye);

        return glm::vec3(
            -mat[3][0],
            -mat[3][1],
             mat[3][2]
        );
    }

    unsigned int RVRGetEyeResolveTexture(RVREye eye) {
        if (eye == RVREyeLeft) {
            return leftEyeDesc.resolveTexture;
        } else if (eye == RVREyeRight) {
            return rightEyeDesc.resolveTexture;
        } return 0;
    }

    void RVRCheckControllers() {
        for (RVRController controller = RVRControllerLeft; controller <= RVRControllerRight; ((int&) controller)++) {
            vr::InputPoseActionData_t poseData;

            if (vr::VRInput()->GetPoseActionDataForNextFrame(RVRGetController(controller).actionPose, vr::TrackingUniverseStanding, &poseData, sizeof(poseData), vr::k_ulInvalidInputValueHandle) != vr::VRInputError_None
                || !poseData.bActive || !poseData.pose.bPoseIsValid) {
                RVRSetControllerShowState(controller, false);
            } else {
                RVRSetControllerShowState(controller, true);
                RVRGetController(controller).poseMatrix = RVRConvertOpenVRMatrixToGLMMatrix(poseData.pose.mDeviceToAbsoluteTracking);
                vr::InputOriginInfo_t originInfo;

                if (vr::VRInput()->GetOriginTrackedDeviceInfo(poseData.activeOrigin, &originInfo, sizeof(originInfo)) == vr::VRInputError_None
                    && originInfo.trackedDeviceIndex != vr::k_unTrackedDeviceIndexInvalid) {
                    std::string sRenderModelName = RVRGetTrackedDeviceString(originInfo.trackedDeviceIndex, vr::Prop_RenderModelName_String);

                    if (sRenderModelName != RVRGetController(controller).renderModelName) {
                        RVRGetController(controller).renderModel = RVRFindOrLoadRenderModel(sRenderModelName.c_str());
                        RVRGetController(controller).renderModelName = sRenderModelName;
                    }
                }
            }
        }
    }

    void RVRPollEvents() {
        vr::VREvent_t event;

        while (hmd->PollNextEvent(&event, sizeof(event))) {
            switch (event.eventType) {
                case vr::VREvent_TrackedDeviceDeactivated: {
                    (*RVRTrackedDeviceDeactivateCallback)((unsigned int) event.trackedDeviceIndex);
                } break;

                case vr::VREvent_TrackedDeviceUpdated: {
                    (*RVRTrackedDeviceUpdateCallback)((unsigned int) event.trackedDeviceIndex);
                } break;
            }
        }

        vr::VRActiveActionSet_t actionSet = { 0 };
        actionSet.ulActionSet = actionSetRVR;
        vr::VRInput()->UpdateActionState(&actionSet, sizeof(actionSet), 1);
        RVRCheckControllers();
    }

    vr::ETrackedDeviceClass RVRGetTrackedDeviceClass(int number) {
        return hmd->GetTrackedDeviceClass(number);
    }

    void RVRCompositorWaitGetPoses(vr::TrackedDevicePose_t* renderPoseArray, unsigned int renderPoseArrayCount, vr::TrackedDevicePose_t* poseArray, unsigned int poseArrayCount) {
        vr::VRCompositor()->WaitGetPoses(renderPoseArray, renderPoseArrayCount, poseArray, poseArrayCount);
    }

    bool RVRInitEyes(float nearClip, float farClip) {
        leftProjectionMatrix = RVRGetProjectionMatrix(RVREyeLeft, nearClip, farClip);
        rightProjectionMatrix = RVRGetProjectionMatrix(RVREyeRight, nearClip, farClip);
        leftEyePosMatrix = RVRGetEyePoseMatrix(RVREyeLeft);
        rightEyePosMatrix = RVRGetEyePoseMatrix(RVREyeRight);
        return true;
    }

    bool RVRInitCompositor() {
        if (!vr::VRCompositor()) return false;
        return true;
    }

    void RVRUpdateHMDPoseMatrix() {
        if (!RVRIsReady()) return;
        RVRCompositorWaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

        validPoseCount = 0;
        poseClasses = "";

        for (int deviceNumber = 0; deviceNumber < vr::k_unMaxTrackedDeviceCount; ++deviceNumber) {
            if (trackedDevicePose[deviceNumber].bPoseIsValid) {
                validPoseCount++;
                devicePoseMatrix[deviceNumber] = RVRConvertOpenVRMatrixToGLMMatrix(trackedDevicePose[deviceNumber].mDeviceToAbsoluteTracking);
                
                if (deviceClassChar[deviceNumber] == 0) {
                    switch (RVRGetTrackedDeviceClass(deviceNumber)) {
                        case vr::TrackedDeviceClass_Controller: deviceClassChar[deviceNumber] = 'C'; break;
                        case vr::TrackedDeviceClass_HMD: deviceClassChar[deviceNumber] = 'H'; break;
                        case vr::TrackedDeviceClass_Invalid: deviceClassChar[deviceNumber] = 'I'; break;
                        case vr::TrackedDeviceClass_GenericTracker: deviceClassChar[deviceNumber] = 'G'; break;
                        case vr::TrackedDeviceClass_TrackingReference: deviceClassChar[deviceNumber] = 'T'; break;
                        default: deviceClassChar[deviceNumber] = '?'; break;
                    }
                }

                poseClasses += deviceClassChar[deviceNumber];
            }
        }

        if (trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid) {
            hmdPose = devicePoseMatrix[vr::k_unTrackedDeviceIndex_Hmd];
            hmdPose = glm::inverse(hmdPose);
        }
    }
}

#ifdef RVR_EXTERN
}
#endif