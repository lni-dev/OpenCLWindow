// Copyright (c) 2023 Linus Andera
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//
// Created by Linus on 24/03/2023.
//

#include <iostream>
#include <sstream>
#include "OpenClWindow.h"

#ifdef MAC
    //TODO:
#elifdef UNIX
    //TODO:
#else
    #include <windef.h>
    #include <wingdi.h>
#endif

namespace linusdev {
    OpenClWindow::OpenClWindow() {
        initOpenGL();
        initOpenCL();
    }

    void OpenClWindow::initOpenCL() {

        if(openCLInit) return;
        openCLInit = true;

        //Select OpenCl Platform
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        for (cl::Platform &p: platforms) {
            std::string platformVersion = p.getInfo<CL_PLATFORM_VERSION>();

            if (platformVersion.find("OpenCL 3.") != std::string::npos) {
                platform = &p;
            } else if (platform == nullptr && platformVersion.find("OpenCL 2.") != std::string::npos) {
                platform = &p;
            }
        }

        if (platform == nullptr)
            throw std::runtime_error("No OpenCL Platform with version 2 or 3 available.");


        //Select OpenCL Device
        std::vector<cl::Device> devices;
        platform->getDevices(CL_DEVICE_TYPE_GPU, &devices);


        for (cl::Device &d: devices) {
            device = &d;
        }

        if (device == nullptr)
            throw std::runtime_error("No OpenCL Device for the platform" + platform->getInfo<CL_PLATFORM_VERSION>());

        std::cout << device->getInfo<CL_DEVICE_EXTENSIONS>() << std::endl;

        programs = new std::vector<cl::Program *>();

        //Create OpenCL shared context
        #ifdef MAC
            CGLContextObj mac_context = CGLGetCurrentContext();
            CGLShareGroupObj group = CGLGetShareGroup(mac_context);
            cl_context_properties properties[] =
                    {
                        CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
                        (cl_context_properties) group,
                        0
                    };
        #elif defined(UNIX)
            cl_context_properties properties[] =
                    {
                        CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
                        CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
                        0
                    };
        #else
            cl_context_properties properties[] =
                    {
                            CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
                            CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC(),
                            0
                    };
        #endif

        int err;
        context = new cl::Context(*device, properties,NULL, NULL, &err);
        if(err) throw std::runtime_error("Error while creating Context: " + std::to_string(err));

        //Create command queue;
        queue = new cl::CommandQueue(*context, *device);
    }

    void OpenClWindow::show() {
        glfwShowWindow(window);

        glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);

        glGenFramebuffers(1, &frameBufferId);
        glBindFramebuffer(GL_FRAMEBUFFER , frameBufferId);

        glGenRenderbuffers(1, &renderBufferId );
        glBindRenderbuffer(GL_RENDERBUFFER, renderBufferId);

        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, frameBufferWidth, frameBufferHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderBufferId);
        glClearColor(0, 0, 0, 1);

        int err;
        sharedRenderBuffer = new cl::BufferRenderGL(*context, CL_MEM_WRITE_ONLY, renderBufferId, &err);
        if(err) throw std::runtime_error("Error while creating shared Render buffer. Code: " + std::to_string(err));

        err = kernel->setArg(0, *sharedRenderBuffer);
        if(err) throw std::runtime_error("Error while setting kernel arg. Code: " + std::to_string(err));


        err = kernel->setArg(1, (cl_int2){frameBufferWidth, frameBufferHeight});
        if(err) throw std::runtime_error("Error while setting kernel arg. Code: " + std::to_string(err));

        sharedGLObjects = new std::vector <cl::Memory>(1, *sharedRenderBuffer);

        render();
        swapBuffer();
    }

    bool OpenClWindow::checkIfWindowShouldClose() {
        glfwPollEvents();
        return glfwWindowShouldClose(window);
    }

    cl_int OpenClWindow::render() {
        int err;
        err = queue->enqueueAcquireGLObjects(sharedGLObjects);
        err |= queue->finish();

        cl::Event event;
        err |= queue->enqueueNDRangeKernel(*kernel, cl::NullRange, cl::NDRange(frameBufferWidth, frameBufferHeight), cl::NullRange, nullptr, &event);
        event.wait();

        err |= queue->enqueueReleaseGLObjects(sharedGLObjects);
        err |= queue->flush();
        err |= queue->finish();

        return err;
    }

    void OpenClWindow::swapBuffer() {
        glClear(GL_COLOR_BUFFER_BIT);
        glFinish();

        glBindFramebuffer (GL_DRAW_FRAMEBUFFER, 0);
        glDrawBuffer      (GL_BACK);              // Use backbuffer as color dst.

        // Read from your FBO
        glBindFramebuffer (GL_READ_FRAMEBUFFER, frameBufferId);
        glReadBuffer      (GL_COLOR_ATTACHMENT0); // Use Color Attachment 0 as color src.

        // Copy the color buffer from your FBO to the default framebuffer
        glBlitFramebuffer (0,0, frameBufferWidth, frameBufferHeight, 0,0, frameBufferWidth, frameBufferHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glfwSwapBuffers(window);
    }

    void OpenClWindow::destroy() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }



    void OpenClWindow::initOpenGL() {

        if(openGLInit) return;
        openGLInit = true;

        if (!glfwInit())
            throw std::runtime_error("Cannot init glfw");
        glfwSetErrorCallback(onErrorStatic);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
        glfwWindowHint(GLFW_DEPTH_BITS, 0);
        glfwWindowHint(GLFW_STENCIL_BITS, 0);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

        window = glfwCreateWindow(width, height, "default", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Cannot create glfw window.");
        }

        glfwMakeContextCurrent(window);
        if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
            throw std::runtime_error("Failed to initialize GLAD");

        glfwSetWindowUserPointer(window, this);
        glfwSetInputMode(window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);

        glfwSetKeyCallback(window, onKeyStatic);
        glfwSetMouseButtonCallback(window, onMouseButtonStatic);
        glfwSetCursorPosCallback(window, onMouseCursorStatic);
        glfwSetCharCallback(window, onCharStatic);
    }


    void OpenClWindow::setSize(int width, int height) {
        this->height = height;
        this->width = width;
        glfwSetWindowSize(window, width, height);
    }

    void OpenClWindow::setTitle(const char *title) {
        glfwSetWindowTitle(window, title);
    }

    void OpenClWindow::setBorderlessFullscreen() {
        glfwSetWindowAttrib(window, GLFW_DECORATED, GL_FALSE);
        glfwMaximizeWindow(window);
    }

    void OpenClWindow::setProgramCode(const std::basic_string<char>& src, const char* options) {
        auto* program = new cl::Program(*context, src);
        programs->push_back(program);

        try {
            cl_int result = program->build( *device, options);
            if (result) throw std::runtime_error("Error during compilation! (" + std::to_string(result) + ")");
        }
        catch (...) {
            // Print build info for all devices
            cl_int buildErr = CL_SUCCESS;
            auto buildInfo = program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(&buildErr);

            std::stringstream ss;
            for (auto &pair : buildInfo) {
                ss << pair.second << "\n\n";
            }

            throw std::runtime_error("Error while building:\n " + ss.str());
        }

        kernel = new cl::Kernel(*program, "render");

    }

    cl_int OpenClWindow::setKernelArg(int index, size_t size, void *pointer) {
        if(!kernel)
            throw std::runtime_error("Call setProgramCode(...) before setKernelArg(...).");


        return kernel->setArg(index, size, pointer);
    }

    cl_int OpenClWindow::setKernelArg(int index, const cl::Buffer &value) {
        if(!kernel)
            throw std::runtime_error("Call setProgramCode(...) before setKernelArg(...).");

        return kernel->setArg(index, value);
    }

    //listener


    void OpenClWindow::onErrorStatic(int error, const char *description) {
        std::cerr << "Error (" << error << "): " << description << std::endl;
    }

    void OpenClWindow::onKeyStatic(GLFWwindow *window, int key, int scancode, int action, int mods) {
        auto win = (OpenClWindow*) glfwGetWindowUserPointer(window);
        if(win->keyListener != nullptr)
            win->keyListener->onKey(key, scancode, action, mods);
    }

    void OpenClWindow::onMouseButtonStatic(GLFWwindow *window, int button, int action, int mods) {
        auto win = (OpenClWindow*) glfwGetWindowUserPointer(window);
        if(win->mouseListener != nullptr)
            win->mouseListener->onMouseButton(button, action, mods);
    }

    void OpenClWindow::onMouseCursorStatic(GLFWwindow *window, double xpos, double ypos) {
        auto win = (OpenClWindow*) glfwGetWindowUserPointer(window);
        if(win->mouseListener != nullptr)
            win->mouseListener->onMouseCursor(xpos, ypos);
    }

    void OpenClWindow::onCharStatic(GLFWwindow *window, unsigned int codepoint) {
        auto win = (OpenClWindow*) glfwGetWindowUserPointer(window);
        if(win->charListener != nullptr)
            win->charListener->onChar(codepoint);
    }

    //Destructor

    OpenClWindow::~OpenClWindow() {

        delete context;
        if(programs) {
            for(cl::Program* prg : *programs) {
                delete prg;
            }

            delete programs;
        }

        delete queue;
        delete sharedRenderBuffer;
        delete kernel;
        delete sharedGLObjects;
    }

    //Getter
    cl::Kernel* OpenClWindow::getKernel() {
        return kernel;
    }

    cl::Context* OpenClWindow::getContext() {
        return context;
    }

    cl::CommandQueue* OpenClWindow::getQueue() {
        return queue;
    }

    GLFWwindow* OpenClWindow::getGLFWWindow() {
        return window;
    }


    //Setter

    void OpenClWindow::setKeyListener(KeyListener* keyListener) {
        OpenClWindow::keyListener = keyListener;
    }

    void OpenClWindow::setMouseListener(MouseListener* mouseListener) {
        OpenClWindow::mouseListener = mouseListener;
    }

    void OpenClWindow::setCharListener(CharListener* charListener) {
        OpenClWindow::charListener = charListener;
    }


}
