/*
 * Copyright (c) 2020  Gaurav Ujjwal.
 *
 * SPDX-License-Identifier:  GPL-3.0-or-later
 *
 * See COPYING.txt for more details.
 */

#include <jni.h>
#include <GLES3/gl3.h> // OpenGL ES 3.0 Header
#include <rfb/rfbclient.h>
#include <EGL/egl.h>
#include <android/native_window_jni.h>
#include <thread>

#include "RenderStuff/RenderObject.h"
#include "ClientEx.h"
#include "Utility.h"
#include "glm/glm.hpp"

#include <camera/NdkCameraManager.h>      // Für ACameraManager und Kamera-Verwaltung
#include <camera/NdkCameraDevice.h>       // Für ACameraDevice und Geräteoperationen
#include <camera/NdkCameraMetadata.h>     // Für Kamera-Metadaten
#include <camera/NdkCameraError.h>        // Für Fehlercodes der Kamera-API
#include <media/NdkImageReader.h>         // Für AImageReader zum Erfassen von Bilddaten
#include <media/NdkImage.h>

#include "CameraDevice.h"

glm::mat4 model = glm::mat4(1.0f);

#define LOG_TAG "NativeDebug"

// Definiere die Log-Level-Funktion
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

const EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
};

const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
};

EGLDisplay display;
EGLSurface eglSurface;
EGLContext context;

int32_t width = 0;
int32_t height = 0;

bool started = false;
rfbClient* client;
bool initSucceded = false;

GLuint frameBufferTexture;
RenderObject renderObject;

const char *adress = "";
int portAddr = 1234;

CameraDeviceManager camManager;

//AB HIER BEGINNT DER RICHTIGE CODE

void Main();

extern "C"
JNIEXPORT jstring JNICALL


Java_com_gaurav_avnc_MainActivity_sayHelloFromJNI(JNIEnv *env, jobject thiz) {

    return env->NewStringUTF("Hello from native C++!");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_gaurav_avnc_MainActivity_nativeSetServerAddress(JNIEnv *env, jobject thiz, jstring addr)
{
    //Am anfang wird direkt nach der bip adresse gefragt
   adress = env->GetStringUTFChars(addr, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_gaurav_avnc_MainActivity_nativeSetServerPort(JNIEnv *env, jobject thiz, jint port)
{
    //Auch wird am anfang direkt nach dem prot gefragt
    portAddr = port;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_gaurav_avnc_MainActivity_createGL(JNIEnv *env, jobject thiz) {
    Main();
}


extern "C"
JNIEXPORT void JNICALL
Java_com_gaurav_avnc_MainActivity_nativeSetSurface(JNIEnv *env, jobject thiz, jobject surface) {
    //Surface für das opengl fenster
    rfbClient  client;
    ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env, surface);
    width = ANativeWindow_getWidth(nativeWindow);
    height = ANativeWindow_getHeight(nativeWindow);

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);

    EGLConfig  config;
    EGLint  numConfigs;
    eglChooseConfig(display, configAttribs, &config, 1, &numConfigs);

    EGLint  format;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(nativeWindow, 0, 0, format);
    eglSurface = eglCreateWindowSurface(display, config, nativeWindow, nullptr);
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);

    if (eglMakeCurrent(display, eglSurface, eglSurface, context) == EGL_FALSE) {
        //aout << "eglMakeCurrent failed: " << eglGetError() << std::endl;
    }

    glViewport(0, 0, width, height);

}

//rfb funktionen
rfbBool  myMallocFramebuffer(rfbClient* cl)
{
    int width = cl->width;
    int height = cl->height;
    int bpp = cl->format.bitsPerPixel / 8;

    if(cl->frameBuffer)
    {
        free(cl->frameBuffer);
    }

    cl->frameBuffer = (uint8_t*)malloc(width * height * bpp);

    if (!cl->frameBuffer) {
        return FALSE;
    }

    return TRUE;
}

void myFramebufferUpdate(rfbClient* cl, int x, int y, int w, int h)
{
    glActiveTexture(GL_TEXTURE0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, client->frameBuffer + (y * client->width + x) * 3);

    printf("Framebuffer update: x=%d y=%d w=%d h=%d\n", x, y, w, h);
}

void myFinishFrame(rfbClient* cl) {
    // Optional: z. B. FPS zählen oder fertig melden
    printf("Frame update complete.\n");
}

void connectingThread()
{
    if (rfbInitClient(client, nullptr, nullptr))
    {
        initSucceded = true;
        LOGI("VERBUNDEN");
    }
}


void Main()
{
    if(!started)
    {
        camManager.Init();
        camManager.OpenDevice();
        renderObject.Init();

        glGenTextures(1, &frameBufferTexture);
        glBindTexture(GL_TEXTURE_2D, frameBufferTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGB,
                width, height, 0,
                GL_RGB, GL_UNSIGNED_BYTE, nullptr
        );

        LOGI("VERBINDE");
        client = rfbGetClient(8, 3, 4); // RGB888
        client->MallocFrameBuffer = myMallocFramebuffer;
        client->GotFrameBufferUpdate = myFramebufferUpdate;
        client->FinishedFrameBufferUpdate = myFinishFrame;

        client->serverHost = strdup(adress);
        client->serverPort = portAddr;  // Standard VNC-Port

        std::thread connectThread(&connectingThread);
        connectThread.detach();

        started = true;
    }

    //LOGI("Main() wird aufgerufen!");
    glClearColor(1.0f, 0.1f, 0.1f, 1.0f); // Hintergrundfarbe
    glClear(GL_COLOR_BUFFER_BIT);


    if(initSucceded)
    {
        //Digitalen bildschirm anzeigen
    }
    else
    {
        //loading screen
    }

    //Render loop
    renderObject.textureID = camManager.getTexureID();
    renderObject.Render();

    eglSwapBuffers(display, eglSurface);

}

extern "C"
JNIEXPORT void JNICALL
Java_com_gaurav_avnc_MainActivity_nativeSetCameraSurface(JNIEnv *env, jobject thiz, jobject surface) {
    //CameraShit


}


extern "C"
JNIEXPORT void JNICALL
Java_com_gaurav_avnc_MainActivity_nativeCleanup(JNIEnv *env, jobject thiz) {
    if(!initSucceded)
    {
        rfbClientCleanup(client);
        initSucceded = true;
    }

    renderObject.Delete();
    glDeleteTextures(1, &frameBufferTexture);
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglDestroySurface(display, eglSurface);
    eglTerminate(display);

}
