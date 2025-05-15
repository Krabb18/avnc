#include "CameraDevice.h"
#include <GLES3/gl3.h> // OpenGL ES 3.0 Header

GLuint texture;
bool didInitTexure = false;
std::vector<uint8_t>* rgbDataPrt = nullptr;
int w = 0;
int h = 0;

static void onDisconnected(void* context, ACameraDevice* device)
{
    // ...
    __android_log_print(ANDROID_LOG_INFO, "MyTag", "Disconnext");
}

static void onError(void* context, ACameraDevice* device, int error)
{
    // ...
    __android_log_print(ANDROID_LOG_INFO, "MyTag", "Error in cam");
}

static ACameraDevice_stateCallbacks cameraDeviceCallbacks = {
        .context = nullptr,
        .onDisconnected = onDisconnected,
        .onError = onError,
};




static void onSessionActive(void* context, ACameraCaptureSession *session)
{}

static void onSessionReady(void* context, ACameraCaptureSession *session)
{}

static void onSessionClosed(void* context, ACameraCaptureSession *session)
{}



static ACameraCaptureSession_stateCallbacks sessionStateCallbacks {
        .context = nullptr,
        .onActive = onSessionActive,
        .onReady = onSessionReady,
        .onClosed = onSessionClosed
};



static void imageCallback(void* context, AImageReader* reader)
{
    __android_log_print(ANDROID_LOG_INFO, "MyTag", "Versuche bild zu bekommen");
    AImage* image = nullptr;
    media_status_t status = AImageReader_acquireNextImage(reader, &image);
    if (status != AMEDIA_OK || !image) return;

    int yLen, uLen, vLen;

    uint8_t *yData = nullptr;
    uint8_t *uData = nullptr;
    uint8_t *vData = nullptr;

    AImage_getPlaneData(image, 0, &yData, &yLen);
    AImage_getPlaneData(image, 1, &uData, &uLen);
    AImage_getPlaneData(image, 2, &vData, &vLen);


    //Konvertier yuv>rgb
    int width, height;
    AImage_getWidth(image, &width);
    AImage_getHeight(image, &height);
    std::vector<uint8_t> rgbData(width * height * 3);

    w = width;
    h = height;

    rgbDataPrt = &rgbData;

    int yStride, uStride, vStride;
    AImage_getPlaneRowStride(image, 0, &yStride);
    AImage_getPlaneRowStride(image, 1, &uStride);
    AImage_getPlaneRowStride(image, 2, &vStride);

    int pixelStrideU, pixelStrideV;
    AImage_getPlanePixelStride(image, 1, &pixelStrideU);
    AImage_getPlanePixelStride(image, 2, &pixelStrideV);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int yIndex = y * yStride + x;
            int uvIndex = (y / 2) * uStride + (x / 2) * pixelStrideU;

            int Y = yData[yIndex] & 0xFF;
            int U = uData[uvIndex] & 0xFF;
            int V = vData[uvIndex] & 0xFF;

            // YUV → RGB
            int R = Y + (int)(1.370705 * (V - 128));
            int G = Y - (int)(0.698001 * (V - 128)) - (int)(0.337633 * (U - 128));
            int B = Y + (int)(1.732446 * (U - 128));

            // Clamp
            R = std::min(255, std::max(0, R));
            G = std::min(255, std::max(0, G));
            B = std::min(255, std::max(0, B));

            int index = (y * width + x) * 3;
            rgbData[index + 0] = R;
            rgbData[index + 1] = G;
            rgbData[index + 2] = B;
        }
    }


    if (yData && yLen > 0) {
        __android_log_print(ANDROID_LOG_INFO, "MyTag", "JUHUUUU");
        __android_log_print(ANDROID_LOG_INFO, "MyTag", "Texture ID: %u", texture);
    }


    didInitTexure = true;

    AImage_delete(image);
}

GLuint CameraDeviceManager::getTexureID()
{

    if(rgbDataPrt != nullptr)
    {
        glDeleteTextures(1, &texture);
        glGenTextures(1, &texture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGB,
                w, h, 0,
                GL_RGB, GL_UNSIGNED_BYTE, rgbDataPrt
        );
        glGenerateMipmap(GL_TEXTURE_2D);

    }

    return texture;
}

void CameraDeviceManager::Init()
{
    glGenTextures(1, &texture);
    cameraManager = ACameraManager_create();
    ACameraManager_getCameraIdList(cameraManager, &cameraIds);


    for (int i = 0; i < cameraIds->numCameras; ++i)
    {
        const char* id = cameraIds->cameraIds[i];

        ACameraMetadata* metadataObj;
        ACameraManager_getCameraCharacteristics(cameraManager, id, &metadataObj);

        int32_t count = 0;
        const uint32_t* tags = nullptr;
        ACameraMetadata_getAllTags(metadataObj, &count, &tags);

        for (int tagIdx = 0; tagIdx < count; ++tagIdx)
        {
            // We are interested in entry that describes the facing of camera
            if (ACAMERA_LENS_FACING == tags[tagIdx]) {
                ACameraMetadata_const_entry lensInfo = { 0 };
                ACameraMetadata_getConstEntry(metadataObj, tags[tagIdx], &lensInfo);



                camera_status_t status = ACameraMetadata_getConstEntry(metadataObj, ACAMERA_LENS_FACING, &lensInfo);
                if (status == ACAMERA_OK) {
                    // Great, entry available

                    auto facing = static_cast<acamera_metadata_enum_android_lens_facing_t>(lensInfo.data.u8[0]);

                    // Found a back-facing camera
                    if (facing == ACAMERA_LENS_FACING_BACK)
                    {
                        backId = id;
                        idIndex = i;

                        ACameraMetadata_getConstEntry(metadataObj,ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &lensInfo);

                        for (int i = 0; i < lensInfo.count; i += 4)
                        {
                            // We are only interested in output streams, so skip input stream
                            int32_t input = lensInfo.data.i32[i + 3];
                            if (input)
                                continue;

                            int32_t format = lensInfo.data.i32[i + 0];
                            if (format == AIMAGE_FORMAT_JPEG)
                            {
                                int32_t width = lensInfo.data.i32[i + 1];
                                int32_t height = lensInfo.data.i32[i + 2];
                            }
                        }
                    }

                }


                auto facing = static_cast<acamera_metadata_enum_android_lens_facing_t>(
                        lensInfo.data.u8[0]);

                break;
            }

        }

        ACameraMetadata_free(metadataObj);
    }

}

void CameraDeviceManager::OpenDevice()
{
    ACameraManager_openCamera(cameraManager, backId.c_str(), &cameraDeviceCallbacks, &cameraDevice); //KANN VIELLEICHT WEGEN DEM INDEX ZU BUGS KOMMEN

    media_status_t  status = AImageReader_new(
            1920,
            1080,
            AIMAGE_FORMAT_YUV_420_888,
            4,       // z.B. 4
            &imageReader
            );

    AImageReader_ImageListener listener = {
            .context = nullptr,
            .onImageAvailable = imageCallback
    };

    AImageReader_setImageListener(imageReader, &listener);

    //Jetzt noch eine session ertsellen

    //erst native window holen

    ANativeWindow* imageWindow = nullptr;
    AImageReader_getWindow(imageReader, &imageWindow);

    //dannach Capture sessions erstellen wo auch callbacks mit dabei sind
    ACaptureSessionOutput_create(imageWindow, &output);
    ACaptureSessionOutputContainer_create(&outputs);
    ACaptureSessionOutputContainer_add(outputs, output);
    ACameraDevice_createCaptureSession(cameraDevice, outputs, &sessionStateCallbacks, &session);


    //jetzt noch request erstellen damit die kamera die bilder auch empfängt
    ACameraDevice_createCaptureRequest(cameraDevice, TEMPLATE_PREVIEW, &request);
    ACameraOutputTarget_create(imageWindow, &outputTarget);
    ACaptureRequest_addTarget(request, outputTarget);

    int captureSequenceId = 0;
    camera_status_t captureStatus = ACameraCaptureSession_setRepeatingRequest(
            session,
            nullptr,      // optional context
            1,            // Anzahl an Requests
            &request,
            &captureSequenceId
    );

// Log für Debugging
    __android_log_print(ANDROID_LOG_INFO, "MyTag", "CaptureRequest gestartet: status = %d", captureStatus);

}

void CameraDeviceManager::CloseDevice()
{
    ACameraManager_deleteCameraIdList(cameraIds);
}