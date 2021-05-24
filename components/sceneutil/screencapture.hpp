#ifndef OPENMW_COMPONENTS_SCENEUTIL_SCREENCAPTURE_H
#define OPENMW_COMPONENTS_SCENEUTIL_SCREENCAPTURE_H

#include <osg/ref_ptr>
#include <osgViewer/ViewerEventHandlers>

#include <string>

namespace osg
{
    class Image;
}

namespace SceneUtil
{
    class WorkQueue;

    std::string writeScreenshotToFile(const std::string& screenshotPath, const std::string& screenshotFormat,
                                      const osg::Image& image);

    class WriteScreenshotToFileOperation : public osgViewer::ScreenCaptureHandler::CaptureOperation
    {
        public:
            WriteScreenshotToFileOperation(const std::string& screenshotPath, const std::string& screenshotFormat,
                                           std::function<void (std::string)> callback);

            void operator()(const osg::Image& image, const unsigned int context_id) override;

        private:
            const std::string mScreenshotPath;
            const std::string mScreenshotFormat;
            const std::function<void (std::string)> mCallback;
    };

    class AsyncScreenCaptureOperation : public osgViewer::ScreenCaptureHandler::CaptureOperation
    {
        public:
            AsyncScreenCaptureOperation(osg::ref_ptr<SceneUtil::WorkQueue> queue,
                                        osg::ref_ptr<osgViewer::ScreenCaptureHandler::CaptureOperation> impl);

            void operator()(const osg::Image& image, const unsigned int context_id) override;

        private:
            const osg::ref_ptr<SceneUtil::WorkQueue> mQueue;
            const osg::ref_ptr<osgViewer::ScreenCaptureHandler::CaptureOperation> mImpl;
    };
}

#endif
