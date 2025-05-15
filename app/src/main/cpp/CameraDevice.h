//
// Created by Leon on 15.05.2025.
//

#ifndef AVNC_CAMERADEVICE_H
#define AVNC_CAMERADEVICE_H

#include <GLES3/gl3.h> // OpenGL ES 3.0 Header
#include <camera/NdkCameraManager.h>      // Für ACameraManager und Kamera-Verwaltung
#include <camera/NdkCameraDevice.h>       // Für ACameraDevice und Geräteoperationen
#include <camera/NdkCameraMetadata.h>     // Für Kamera-Metadaten
#include <camera/NdkCameraError.h>        // Für Fehlercodes der Kamera-API
#include <media/NdkImageReader.h>         // Für AImageReader zum Erfassen von Bilddaten
#include <media/NdkImage.h>
#include <string>
#include <iostream>
#include <android/log.h>


using namespace std;

class CameraDeviceManager
{
private:
    ACaptureSessionOutput* output = nullptr;
    ACaptureSessionOutputContainer* outputs = nullptr;
    ACameraCaptureSession* session = nullptr;

    ACaptureRequest* request = nullptr;
    ACameraOutputTarget* outputTarget = nullptr;


    AImageReader* imageReader = nullptr;
    ACameraManager *cameraManager = nullptr;
    ACameraIdList *cameraIds = nullptr;
    string backId;
    int idIndex = 0;
    ACameraDevice *cameraDevice = nullptr;



public:
    void Init();
    void OpenDevice();
    void CloseDevice();
    GLuint getTexureID();
};

#endif //AVNC_CAMERADEVICE_H
