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

#ifndef OPENCLTEST_OPENCLWINDOW_H
#define OPENCLTEST_OPENCLWINDOW_H

#define CL_HPP_TARGET_OPENCL_VERSION 200

#ifdef __APPLE__
#include "OpenCL/opencl.hpp"
#else
#include "CL/opencl.hpp"
#endif

#include "glad/gl.h"
#include <GLFW/glfw3.h>

#include "Listeners.h"

namespace linusdev {
    class OpenClWindow {
    private:

        //CL Stuff
        cl::Platform *platform = nullptr;
        cl::Device *device = nullptr;

        cl::Context *context = nullptr;
        cl::CommandQueue *queue = nullptr;
        cl::Program *program = nullptr;
        cl::Kernel *kernel = nullptr;
        cl::BufferRenderGL *sharedRenderBuffer = nullptr;

        const std::vector <cl::Memory>* sharedGLObjects = nullptr;

        int frameBufferWidth;
        int frameBufferHeight;

        void initOpenCL();
        bool openCLInit = false;

        //GL and GLFW stuff
        GLFWwindow* window = nullptr;

        GLuint frameBufferId = 0;
        GLuint renderBufferId = 0;

        void initOpenGL();
        bool openGLInit = false;

        //Other stuff
        int width = 500;
        int height = 500;

        //listeners
        linusdev::KeyListener* keyListener = nullptr;
        linusdev::MouseListener* mouseListener = nullptr;
        linusdev::CharListener* charListener = nullptr;


    public:
        explicit OpenClWindow();

        virtual ~OpenClWindow();

        void setTitle(const char *title);

        void setSize(int width, int height);

        void setBorderlessFullscreen();

        void setProgramCode(const std::basic_string<char>& src,  const char* options);

        cl_int createSharedRenderBuffer();
        cl_int setBaseKernelArgs();

        cl_int setKernelArg(int index, size_t size, void* pointer);
        cl_int setKernelArg(int index, const cl::Buffer& value);

        void show();

        bool checkIfWindowShouldClose();
        cl_int render();
        void swapBuffer();
        void destroy();

        void setKeyListener(KeyListener *keyListener);
        void setMouseListener(MouseListener *mouseListener);
        void setCharListener(CharListener* charListener);

        //getter
        cl::Kernel* getKernel();
        cl::Context* getContext();
        cl::CommandQueue* getQueue();

        GLFWwindow* getGLFWWindow();


    public: //static
        static void onErrorStatic(int error, const char* description);
        static void onKeyStatic(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void onMouseButtonStatic(GLFWwindow* window, int button, int action, int mods);
        static void onMouseCursorStatic(GLFWwindow* window, double xpos, double ypos);
        static void onCharStatic(GLFWwindow* window, unsigned int codepoint);
    };
}

#endif //OPENCLTEST_OPENCLWINDOW_H
