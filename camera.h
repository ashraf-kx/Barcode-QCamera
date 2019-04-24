#ifndef CAMERA_H
#define CAMERA_H

#include <QWidget>
#include "ui_camera.h"

#include <QCamera>
#include <QCameraImageCapture>
#include <QZXing>


QT_BEGIN_NAMESPACE
namespace Ui { class Camera; }
QT_END_NAMESPACE

class Camera : public QWidget,private Ui::Camera
{
    Q_OBJECT

public:
    Camera(QWidget *parent = 0);
    ~Camera();

public slots:
    void displayCodeBar();
    void getBufferFrameFromSource(int val, QVideoFrame videoFrame);
private slots:
    void setCamera(const QCameraInfo &cameraInfo);

    void startCamera();
    void stopCamera();

    void toggleLock();
    void takeImage();
    void displayCaptureError(int, QCameraImageCapture::Error, const QString &errorString);

    void configureCaptureSettings();
    void configureImageSettings();

    void displayCameraError();

    void updateCameraDevice(QAction *action);

    void updateCameraState(QCamera::State);
    void updateCaptureMode();
    void setExposureCompensation(int index);

    void processCapturedImage(int requestId, const QImage &img);
    void updateLockStatus(QCamera::LockStatus, QCamera::LockChangeReason);

    void displayViewfinder();
    void displayCapturedImage();

    void readyForCapture(bool ready);
    void imageSaved(int id, const QString &fileName);

protected:
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void closeEvent(QCloseEvent *event);

private:

    QCamera             *camera;
    QCameraImageCapture *imageCapture;
    QImageEncoderSettings imageSettings;

    QString videoContainerFormat;
    bool    isCapturingImage;
    bool    applicationExiting;

    QImage codeImage;
};

#endif
