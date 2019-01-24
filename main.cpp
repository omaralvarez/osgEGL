#include <osgDB/ReadFile>
#include <osgUtil/Optimizer>
#include <osg/CoordinateSystemNode>

#include <osg/Switch>
#include <osg/Types>
#include <osgText/Text>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <osgGA/TrackballManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgGA/StateSetManipulator>
#include <osgGA/AnimationPathManipulator>
#include <osgGA/TerrainManipulator>
#include <osgGA/SphericalManipulator>

#include <osgGA/Device>

#include <iostream>

#include <osg/LineWidth>
#include <osg/Point>
#include <osg/MatrixTransform>
#include <osg/io_utils>

#include <EGL/egl.h>

#include <deque>
#include <numeric>

#define SAMPLES 1000

class EGLGraphicsWindowEmbedded : public osgViewer::GraphicsWindowEmbedded
{
protected:
    EGLDisplay  eglDpy;
    EGLint      major, minor;
    EGLint      numConfigs;
    EGLConfig   eglCfg;
    EGLSurface  eglSurf;
    EGLContext  eglCtx;
public:

    EGLGraphicsWindowEmbedded()
    {
        const EGLint configAttribs[] = {
              EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
              EGL_BLUE_SIZE, 8,
              EGL_GREEN_SIZE, 8,
              EGL_RED_SIZE, 8,
              EGL_DEPTH_SIZE, 8,
              EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
              EGL_NONE
        };

        static const int pbufferWidth = 256;
        static const int pbufferHeight = 256;

        const EGLint pbufferAttribs[] = {
            EGL_WIDTH, pbufferWidth,
            EGL_HEIGHT, pbufferHeight,
            EGL_NONE,
        };

        // 1. Initialize EGL
        eglDpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        eglInitialize(eglDpy, &major, &minor);

        // 2. Select an appropriate configuration
        eglChooseConfig(eglDpy, configAttribs, &eglCfg, 1, &numConfigs);

        // 3. Create a surface
        eglSurf = eglCreatePbufferSurface(eglDpy, eglCfg, pbufferAttribs);

        // 4. Bind the API
        eglBindAPI(EGL_OPENGL_API);

        // 5. Create a context and make it current
        eglCtx = eglCreateContext(eglDpy, eglCfg, EGL_NO_CONTEXT, NULL);

        init();

    }
    virtual ~EGLGraphicsWindowEmbedded()
    {
        // 6. Terminate EGL when finished
        eglTerminate(eglDpy);
    }

    virtual bool isSameKindAs(const Object* object) const { return dynamic_cast<const EGLGraphicsWindowEmbedded*>(object)!=0; }
    virtual const char* libraryName() const { return "osgViewer"; }
    virtual const char* className() const { return "EGLGraphicsWindowEmbedded"; }

        void init()
        {
            if (valid())
            {
                setState( new osg::State );
                getState()->setGraphicsContext(this);

                getState()->setContextID( osg::GraphicsContext::createNewContextID() );
            }
        }

        // dummy implementations, assume that graphics context is *always* current and valid.
        virtual bool valid() const { return true; }
        virtual bool realizeImplementation() { return true; }
        virtual bool isRealizedImplementation() const  { return true; }
        virtual void closeImplementation() {}
        virtual bool makeCurrentImplementation() {
            eglMakeCurrent(eglDpy, eglSurf, eglSurf, eglCtx);
            return true;
        }
        virtual bool releaseContextImplementation() { return true; }
        virtual void swapBuffersImplementation() {}
        virtual void grabFocus() {}
        virtual void grabFocusIfPointerInWindow() {}
        virtual void raiseWindow() {}
};

int main(int argc, char** argv)
{
    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc,argv);

    arguments.getApplicationUsage()->setApplicationName(arguments.getApplicationName());
    arguments.getApplicationUsage()->setDescription(arguments.getApplicationName()+" is the standard OpenSceneGraph example which loads and visualises 3d models.");
    arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName()+" [options] filename ...");
    arguments.getApplicationUsage()->addCommandLineOption("--image <filename>","Load an image and render it on a quad");
    arguments.getApplicationUsage()->addCommandLineOption("--dem <filename>","Load an image/DEM and render it on a HeightField");
    arguments.getApplicationUsage()->addCommandLineOption("--login <url> <username> <password>","Provide authentication information for http file access.");
    arguments.getApplicationUsage()->addCommandLineOption("-p <filename>","Play specified camera path animation file, previously saved with 'z' key.");
    arguments.getApplicationUsage()->addCommandLineOption("--speed <factor>","Speed factor for animation playing (1 == normal speed).");
    arguments.getApplicationUsage()->addCommandLineOption("--device <device-name>","add named device to the viewer");

    osgViewer::Viewer viewer(arguments);

    unsigned int helpType = 0;
    if ((helpType = arguments.readHelpType()))
    {
        arguments.getApplicationUsage()->write(std::cout, helpType);
        return 1;
    }

    // report any errors if they have occurred when parsing the program arguments.
    if (arguments.errors())
    {
        arguments.writeErrorMessages(std::cout);
        return 1;
    }

    if (arguments.argc()<=1)
    {
        arguments.getApplicationUsage()->write(std::cout,osg::ApplicationUsage::COMMAND_LINE_OPTION);
        return 1;
    }

    std::string url, username, password;
    while(arguments.read("--login",url, username, password))
    {
        osgDB::Registry::instance()->getOrCreateAuthenticationMap()->addAuthenticationDetails(
            url,
            new osgDB::AuthenticationDetails(username, password)
        );
    }

    std::string device;
    while(arguments.read("--device", device))
    {
        osg::ref_ptr<osgGA::Device> dev = osgDB::readRefFile<osgGA::Device>(device);
        if (dev.valid())
        {
            viewer.addDevice(dev);
        }
    }

    // set up the camera manipulators.
    {
        osg::ref_ptr<osgGA::KeySwitchMatrixManipulator> keyswitchManipulator = new osgGA::KeySwitchMatrixManipulator;

        keyswitchManipulator->addMatrixManipulator( '1', "Trackball", new osgGA::TrackballManipulator() );
        keyswitchManipulator->addMatrixManipulator( '2', "Flight", new osgGA::FlightManipulator() );
        keyswitchManipulator->addMatrixManipulator( '3', "Drive", new osgGA::DriveManipulator() );
        keyswitchManipulator->addMatrixManipulator( '4', "Terrain", new osgGA::TerrainManipulator() );
        keyswitchManipulator->addMatrixManipulator( '5', "Orbit", new osgGA::OrbitManipulator() );
        keyswitchManipulator->addMatrixManipulator( '6', "FirstPerson", new osgGA::FirstPersonManipulator() );
        keyswitchManipulator->addMatrixManipulator( '7', "Spherical", new osgGA::SphericalManipulator() );

        std::string pathfile;
        double animationSpeed = 1.0;
        while(arguments.read("--speed",animationSpeed) ) {}
        char keyForAnimationPath = '8';
        while (arguments.read("-p",pathfile))
        {
            osgGA::AnimationPathManipulator* apm = new osgGA::AnimationPathManipulator(pathfile);
            if (apm || !apm->valid())
            {
                apm->setTimeScale(animationSpeed);

                unsigned int num = keyswitchManipulator->getNumMatrixManipulators();
                keyswitchManipulator->addMatrixManipulator( keyForAnimationPath, "Path", apm );
                keyswitchManipulator->selectMatrixManipulator(num);
                ++keyForAnimationPath;
            }
        }

        viewer.setCameraManipulator( keyswitchManipulator.get() );
    }

    // add the state manipulator
    viewer.addEventHandler( new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()) );

    // add the thread model handler
    viewer.addEventHandler(new osgViewer::ThreadingHandler);

    // add the window size toggle handler
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);

    // add the stats handler
    viewer.addEventHandler(new osgViewer::StatsHandler);

    // add the help handler
    viewer.addEventHandler(new osgViewer::HelpHandler(arguments.getApplicationUsage()));

    // add the record camera path handler
    viewer.addEventHandler(new osgViewer::RecordCameraPathHandler);

    // add the LOD Scale handler
    viewer.addEventHandler(new osgViewer::LODScaleHandler);

    // add the screen capture handler
    viewer.addEventHandler(new osgViewer::ScreenCaptureHandler);

    // load the data
    osg::ref_ptr<osg::Node> loadedModel =  osgDB::readRefNodeFiles(arguments);
    if (!loadedModel)
    {
        std::cout << arguments.getApplicationName() <<": No data loaded" << std::endl;
        return 1;
    }

    // any option left unread are converted into errors to write out later.
    arguments.reportRemainingOptionsAsUnrecognized();

    // report any errors if they have occurred when parsing the program arguments.
    if (arguments.errors())
    {
        arguments.writeErrorMessages(std::cout);
        return 1;
    }


    // optimize the scene graph, remove redundant nodes and state etc.
    osgUtil::Optimizer optimizer;
    optimizer.optimize(loadedModel);

    osg::ref_ptr<osg::Group> group = new osg::Group;
    group->addChild(loadedModel);

    viewer.setSceneData(group);

    viewer.getCamera()->setGraphicsContext(new EGLGraphicsWindowEmbedded);
    viewer.getCamera()->setViewport(new osg::Viewport(0, 0, 640, 480));
    viewer.realize();

    std::deque<double> timingAvg(SAMPLES, 0.);

    unsigned int current = 0;
    double time = 0.;
    while (!viewer.done())
    {
        viewer.advance();
        viewer.eventTraversal();
        viewer.updateTraversal();
        viewer.renderingTraversals();
	//std::cout << 1. / (viewer.getViewerFrameStamp()->getReferenceTime() - time) << " FPS" << std::endl;

	timingAvg.pop_back();
        timingAvg.push_front(viewer.getViewerFrameStamp()->getReferenceTime() - time);
	time = viewer.getViewerFrameStamp()->getReferenceTime();

	if(current > SAMPLES) {
            double avgTime = std::accumulate(timingAvg.begin(), timingAvg.end(), 0.) / timingAvg.size();
            OSG_NOTICE << "[Avg. Frame Time] " << avgTime << "s [FPS] " << 1. / avgTime << std::endl;
            current = 0;
        }
        else {
            ++current;
        }
    }


    return 0;

}
