#include "camera.h"

#include "imagesettings.h"
#include <QCameraViewfinder>
#include <QCameraInfo>
#include <QMediaMetaData>
#include <QtMultimedia/QSound>

#include <QMessageBox>
#include <QPalette>
#include <QDebug>

#include <QtWidgets>

Q_DECLARE_METATYPE(QCameraInfo)

Camera::Camera(QWidget *parent) :
    QWidget(parent),
    camera(0),
    imageCapture(0),
    isCapturingImage(false),
    applicationExiting(false)
{
    setupUi(this);
    L_displayTextCode->setAttribute(Qt::WA_TranslucentBackground);
    takeImageButton->setAttribute(Qt::WA_TranslucentBackground);

    QActionGroup *videoDevicesGroup = new QActionGroup(this);
    videoDevicesGroup->setExclusive(true);
    // Detect Available camera, list them in the MenuBar.
    foreach (const QCameraInfo &cameraInfo, QCameraInfo::availableCameras()) {
        QAction *videoDeviceAction = new QAction(cameraInfo.description(), videoDevicesGroup);
        videoDeviceAction->setCheckable(true);
        videoDeviceAction->setData(QVariant::fromValue(cameraInfo));
        if (cameraInfo == QCameraInfo::defaultCamera())
            videoDeviceAction->setChecked(true);

        menuDevices->addAction(videoDeviceAction);
    }

    setCamera(QCameraInfo::defaultCamera());

    qDebug()<<imageCapture->captureDestination();
    imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
     qDebug()<<imageCapture->captureDestination();
 if(imageCapture->isCaptureDestinationSupported(QCameraImageCapture::CaptureToBuffer))
//    {
        qDebug()<<"Yeap, supported.";
        imageCapture->setBufferFormat(QVideoFrame::Format_RGB24);
        QImageEncoderSettings imageSettings;
        imageSettings.setCodec("image/png");
        imageSettings.setResolution(640, 480);
        imageCapture->setEncodingSettings(imageSettings);
//    }else
//    {
//        qDebug()<<"Nope, supported.";
//    }

    connect(imageCapture,SIGNAL(imageAvailable(int,QVideoFrame)),
            this,SLOT(getBufferFrameFromSource(int,QVideoFrame)));
}

Camera::~Camera()
{
    delete imageCapture;
    delete camera;
}

void Camera::getBufferFrameFromSource(int val,QVideoFrame videoFrame)
{
    Q_UNUSED(val);
    //QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(videoFrame.pixelFormat());
    QImage img(videoFrame.bits(),
               videoFrame.width(),
               videoFrame.height(),
               QImage::Format_RGB888);

    L_displayCode->setPixmap(QPixmap::fromImage(img));

//    disconnect(imageCapture,SIGNAL(imageAvailable(int,QVideoFrame)),
//            this,SLOT(getBufferFrameFromSource()));
}

void Camera::setCamera(const QCameraInfo &cameraInfo)
{
    delete imageCapture;
    delete camera;

    camera = new QCamera(cameraInfo);

    connect(camera, SIGNAL(stateChanged(QCamera::State)), this, SLOT(updateCameraState(QCamera::State)));
    connect(camera, SIGNAL(error(QCamera::Error)), this, SLOT(displayCameraError()));

    imageCapture = new QCameraImageCapture(camera);

    connect(exposureCompensation, SIGNAL(valueChanged(int)), SLOT(setExposureCompensation(int)));

    camera->setViewfinder(viewfinder);

    updateCameraState(camera->state());
    updateLockStatus(camera->lockStatus(), QCamera::UserRequest);

    connect(imageCapture, SIGNAL(readyForCaptureChanged(bool)), this, SLOT(readyForCapture(bool)));
    connect(imageCapture, SIGNAL(imageCaptured(int,QImage)), this, SLOT(processCapturedImage(int,QImage)));
    connect(imageCapture, SIGNAL(imageSaved(int,QString)), this, SLOT(imageSaved(int,QString)));
    connect(imageCapture, SIGNAL(error(int,QCameraImageCapture::Error,QString)), this,
            SLOT(displayCaptureError(int,QCameraImageCapture::Error,QString)));

    connect(camera, SIGNAL(lockStatusChanged(QCamera::LockStatus,QCamera::LockChangeReason)),
            this, SLOT(updateLockStatus(QCamera::LockStatus,QCamera::LockChangeReason)));

    // captureWidget->setTabEnabled(0, (camera->isCaptureModeSupported(QCamera::CaptureStillImage)));

    updateCaptureMode();
    camera->start();
}

void Camera::keyPressEvent(QKeyEvent * event)
{
    if (event->isAutoRepeat())
        return;

    switch (event->key()) {
    case Qt::Key_CameraFocus:
        displayViewfinder();
        camera->searchAndLock();
        event->accept();
        break;
    case Qt::Key_Camera:
        if (camera->captureMode() == QCamera::CaptureStillImage) {
            takeImage();
        }
        event->accept();
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

void Camera::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat())
        return;

    switch (event->key()) {
    case Qt::Key_CameraFocus:
        camera->unlock();
        break;
    default:
        QWidget::keyReleaseEvent(event);
    }
}

void Camera::processCapturedImage(int requestId, const QImage& img)
{
    Q_UNUSED(requestId);
    QImage scaledImage = img.scaled(viewfinder->size(),
                                    Qt::KeepAspectRatio,
                                    Qt::FastTransformation);

    codeImage = scaledImage;

    displayCodeBar();
    // Display captured image for 4  seconds. (200 ms) haha
//    displayCapturedImage();
//    QTimer::singleShot(200, this, SLOT(displayViewfinder()));
}

void Camera::displayCodeBar()
{
    //L_displayCode->setPixmap(QPixmap::fromImage(codeImage));
    QTime t;
    t.start();

    QImage imageToDecode(codeImage);
    QZXing decoder;
    /*decoder.setDecoder( QZXing::DecoderFormat_QR_CODE | QZXing::DecoderFormat_EAN_13 |
                        QZXing::DecoderFormat_CODE_128 );
    // Use all CodeBar exists*/
    QString result = decoder.decodeImage(imageToDecode);
    if(!result.isEmpty())
    {
        QSound::play("://BEEP");
        QString outputStr = "Code :\n"+result+" \n "+QString::number(t.elapsed())+" ms.";
//        L_displayTextCode->setText(outputStr);
        L_displayCode->setText(outputStr);
    }
}

void Camera::configureCaptureSettings()
{
    switch (camera->captureMode()) {
    case QCamera::CaptureStillImage:
        configureImageSettings();
        break;
    default:
        break;
    }
}

void Camera::configureImageSettings()
{
    ImageSettings settingsDialog(imageCapture);

    settingsDialog.setImageSettings(imageSettings);

    if (settingsDialog.exec()) {
        imageSettings = settingsDialog.imageSettings();
        imageCapture->setEncodingSettings(imageSettings);
    }
}

void Camera::toggleLock()
{
    switch (camera->lockStatus()) {
    case QCamera::Searching:
    case QCamera::Locked:
        camera->unlock();
        break;
    case QCamera::Unlocked:
        camera->searchAndLock();
    }
}

void Camera::updateLockStatus(QCamera::LockStatus status, QCamera::LockChangeReason reason)
{
    QColor indicationColor = Qt::black;

    switch (status) {
    case QCamera::Searching:
        indicationColor = Qt::yellow;
        lockButton->setText(tr("Focusing..."));
        break;
    case QCamera::Locked:
        indicationColor = Qt::darkGreen;
        lockButton->setText(tr("Unlock"));
        break;
    case QCamera::Unlocked:
        indicationColor = reason == QCamera::LockFailed ? Qt::red : Qt::black;
        lockButton->setText(tr("Focus"));
        if (reason == QCamera::LockFailed)
            QString x =0; // Just to skip this, new message .
    }

    QPalette palette = lockButton->palette();
    palette.setColor(QPalette::ButtonText, indicationColor);
    lockButton->setPalette(palette);
}

void Camera::takeImage()
{
    isCapturingImage = true;
    imageCapture->capture();
}

void Camera::displayCaptureError(int id, const QCameraImageCapture::Error error, const QString &errorString)
{
    Q_UNUSED(id);
    Q_UNUSED(error);
    QMessageBox::warning(this, tr("Image Capture Error"), errorString);
    isCapturingImage = false;
}

void Camera::startCamera()
{
    camera->start();
}

void Camera::stopCamera()
{
    camera->stop();
}

void Camera::updateCaptureMode()
{
    if (camera->isCaptureModeSupported(QCamera::CaptureStillImage))
        camera->setCaptureMode(QCamera::CaptureStillImage);
}

// enable/disable item on Camera menu List [setting, Stop, Start ]
void Camera::updateCameraState(QCamera::State state)
{
    switch (state) {
    case QCamera::ActiveState:
        actionStartCamera->setEnabled(false);
        actionStopCamera->setEnabled(true);
        actionSettings->setEnabled(true);
        break;
    case QCamera::UnloadedState:
    case QCamera::LoadedState:
        actionStartCamera->setEnabled(true);
        actionStopCamera->setEnabled(false);
        actionSettings->setEnabled(false);
    }
}

void Camera::setExposureCompensation(int index)
{
    camera->exposure()->setExposureCompensation(index*0.5);
}


void Camera::displayCameraError()
{
    QMessageBox::warning(this, tr("Camera error"), camera->errorString());
}

void Camera::updateCameraDevice(QAction *action)
{
    setCamera(qvariant_cast<QCameraInfo>(action->data()));
}

void Camera::displayViewfinder()
{
    stackedWidget->setCurrentIndex(0);
}

void Camera::displayCapturedImage()
{
    stackedWidget->setCurrentIndex(1);
}

void Camera::readyForCapture(bool ready)
{
    takeImageButton->setEnabled(ready);
}

void Camera::imageSaved(int id, const QString &fileName)
{
    Q_UNUSED(id);
    Q_UNUSED(fileName);

    isCapturingImage = false;
    if (applicationExiting)
        close();
}

void Camera::closeEvent(QCloseEvent *event)
{
    if (isCapturingImage) {
        setEnabled(false);
        applicationExiting = true;
        event->ignore();
    } else {
        event->accept();
    }
}
