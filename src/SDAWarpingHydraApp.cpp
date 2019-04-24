#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"

// Settings
#include "SDASettings.h"
// Session
#include "SDASession.h"
// Log
#include "SDALog.h"
// warping
#include "Warp.h"

using namespace ci;
using namespace ci::app;
using namespace ph::warping;
using namespace std;
using namespace SophiaDigitalArt;

class SDAWarpingHydraApp : public App {

public:
	SDAWarpingHydraApp();
	void mouseMove(MouseEvent event) override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void mouseUp(MouseEvent event) override;
	void keyDown(KeyEvent event) override;
	void keyUp(KeyEvent event) override;
	void fileDrop(FileDropEvent event) override;
	void update() override;
	void draw() override;
	void cleanup() override;
	void resize() override;
	void setUIVisibility(bool visible);

private:
	// Settings
	SDASettingsRef					mSDASettings;
	// Session
	SDASessionRef					mSDASession;
	// Log
	SDALogRef						mSDALog;
	// fbo
	bool							mIsShutDown;
	Anim<float>						mRenderWindowTimer;
	void							positionRenderWindow();
	bool							mFadeInDelay;
	int								xLeft, xRight, yLeft, yRight;
	int								margin, tWidth, tHeight;
	//! fbos
	//void							renderToFbo();
	//gl::FboRef						mFbo;
	//! shaders
	//gl::GlslProgRef					mGlsl;

	ci::SurfaceRef 					mSurface;
	// warping
	fs::path		mSettings;
	gl::TextureRef	mImage;
	WarpList		mWarps;
	Area			mSrcArea;
};


SDAWarpingHydraApp::SDAWarpingHydraApp()
{
	// Settings
	mSDASettings = SDASettings::create("Hydra");
	// Session
	mSDASession = SDASession::create(mSDASettings);
	mSDASession->getWindowsResolution();
	// warping
	mSettings = getAssetPath("") / "warps.xml";
	if (fs::exists(mSettings)) {
		// load warp settings from file if one exists
		mWarps = Warp::readSettings(loadFile(mSettings));
	}
	else {
		// otherwise create a warp from scratch
		mWarps.push_back(WarpPerspectiveBilinear::create());
	}
	// load test image
	try {
		mImage = gl::Texture::create(loadImage(loadAsset("splash.jpg")), gl::Texture2d::Format().loadTopDown().mipmap(true).minFilter(GL_LINEAR_MIPMAP_LINEAR));

		mSrcArea = mImage->getBounds();

		// adjust the content size of the warps
		Warp::setSize(mWarps, mImage->getSize());
	}
	catch (const std::exception &e) {
		console() << e.what() << std::endl;
	}

	mFadeInDelay = true;
	xLeft = 0;
	xRight = mSDASettings->mRenderWidth;
	yLeft = 0;
	yRight = mSDASettings->mRenderHeight;
	margin = 20;
	tWidth = mSDASettings->mFboWidth / 2;
	tHeight = mSDASettings->mFboHeight / 2;
	// windows
	mIsShutDown = false;
	// fbo
	gl::Fbo::Format format;
	//format.setSamples( 4 ); // uncomment this to enable 4x antialiasing
	// shader
	/*mGlsl = gl::GlslProg::create(gl::GlslProg::Format().vertex(loadAsset("passthrough.vs")).fragment(loadAsset("post.glsl")));
	mFbo = gl::Fbo::create(mSDASettings->mFboWidth, mSDASettings->mFboHeight, format.depthTexture());*/
	// adjust the content size of the warps
	Warp::setSize(mWarps, vec2(mSDASettings->mFboWidth, mSDASettings->mFboHeight));
	mSDASession->setFloatUniformValueByIndex(mSDASettings->IRESX, mSDASettings->mFboWidth);
	mSDASession->setFloatUniformValueByIndex(mSDASettings->IRESY, mSDASettings->mFboHeight);
	mSrcArea = Area(0, 0, mSDASettings->mFboWidth, mSDASettings->mFboHeight);

	gl::enableDepthRead();
	gl::enableDepthWrite();
#ifdef _DEBUG

#else
	mRenderWindowTimer = 0.0f;
	timeline().apply(&mRenderWindowTimer, 1.0f, 2.0f).finishFn([&] { positionRenderWindow(); });
	positionRenderWindow();

#endif  // _DEBUG
}
void SDAWarpingHydraApp::positionRenderWindow() {
	setUIVisibility(mSDASettings->mCursorVisible);
	mSDASettings->mRenderPosXY = ivec2(mSDASettings->mRenderX, mSDASettings->mRenderY);//20141214 was 0
	setWindowPos(mSDASettings->mRenderX, mSDASettings->mRenderY);
	setWindowSize(mSDASettings->mRenderWidth, mSDASettings->mRenderHeight);
}
void SDAWarpingHydraApp::resize()
{
	// tell the warps our window has been resized, so they properly scale up or down
	Warp::handleResize(mWarps);
}
// Render into the FBO
/*
void SDAWarpingHydraApp::renderToFbo()
{

	// this will restore the old framebuffer binding when we leave this function
	// on non-OpenGL ES platforms, you can just call mFbo->unbindFramebuffer() at the end of the function
	// but this will restore the "screen" FBO on OpenGL ES, and does the right thing on both platforms
	gl::ScopedFramebuffer fbScp(mFbo);
	// clear out the FBO with black
	gl::clear(Color::black());

	// setup the viewport to match the dimensions of the FBO
	gl::ScopedViewport scpVp(ivec2(0), mFbo->getSize());

	// render

	// texture binding must be before ScopedGlslProg

	mImage->bind(0);

	gl::ScopedGlslProg prog(mGlsl);

	mGlsl->uniform("iTime", (float)getElapsedSeconds());
	mGlsl->uniform("iResolution", vec3(mSDASettings->mRenderWidth, mSDASettings->mRenderHeight, 1.0));
	mGlsl->uniform("iChannel0", 0); // texture 0
	mGlsl->uniform("iExposure", 1.0f);
	mGlsl->uniform("iSobel", 1.0f);
	mGlsl->uniform("iChromatic", 1.0f);

	gl::drawSolidRect(getWindowBounds());

} */
void SDAWarpingHydraApp::setUIVisibility(bool visible)
{
	if (visible)
	{
		showCursor();
	}
	else
	{
		hideCursor();
	}
}
void SDAWarpingHydraApp::fileDrop(FileDropEvent event)
{
	mSDASession->fileDrop(event);
}
void SDAWarpingHydraApp::update()
{
	if (mFadeInDelay == false) {
		mSDASession->setFloatUniformValueByIndex(mSDASettings->IFPS, getAverageFps());
		mSDASession->update();
		// render into our FBO
		//renderToFbo();
	}
}
void SDAWarpingHydraApp::cleanup()
{
	if (!mIsShutDown)
	{
		mIsShutDown = true;
		CI_LOG_V("shutdown");
		// save settings
		// save warp settings
		Warp::writeSettings(mWarps, writeFile(mSettings));
		mSDASettings->save();
		mSDASession->save();
		quit();
	}
}
void SDAWarpingHydraApp::mouseMove(MouseEvent event)
{
	mSDASession->setFloatUniformValueByIndex(mSDASettings->IMOUSEX, event.getX());
	mSDASession->setFloatUniformValueByIndex(mSDASettings->IMOUSEY, event.getY());
	// hm mSDAAnimation->setVec4UniformValueByIndex(70, vec4(event.getX(), event.getY(), event.isLeftDown(), event.isRightDown()));

	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseMove(mWarps, event)) {
		// let your application perform its mouseMove handling here
		if (!mSDASession->handleMouseMove(event)) {
		}
	}

}
void SDAWarpingHydraApp::mouseDown(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseDown(mWarps, event)) {
		// let your application perform its mouseDown handling here
		if (!mSDASession->handleMouseDown(event)) {
			if (event.isRightDown()) {


			}
		}
	}
}
void SDAWarpingHydraApp::mouseDrag(MouseEvent event)
{
	if (!Warp::handleMouseDrag(mWarps, event)) {
		if (!mSDASession->handleMouseDrag(event)) {
		}
	}
}
void SDAWarpingHydraApp::mouseUp(MouseEvent event)
{
	if (!Warp::handleMouseUp(mWarps, event)) {
		if (!mSDASession->handleMouseUp(event)) {
			// let your application perform its mouseUp handling here
		}
	}
}

void SDAWarpingHydraApp::keyDown(KeyEvent event)
{
#if defined( CINDER_COCOA )
	bool isModDown = event.isMetaDown();
#else // windows
	bool isModDown = event.isControlDown();
#endif
	bool isShiftDown = event.isShiftDown();
	bool isAltDown = event.isAltDown();
	CI_LOG_V("main keydown: " + toString(event.getCode()) + " ctrl: " + toString(isModDown) + " shift: " + toString(isShiftDown) + " alt: " + toString(isAltDown));

	// pass this key event to the warp editor first
	if (!Warp::handleKeyDown(mWarps, event)) {
		if (!mSDASession->handleKeyDown(event)) {
			switch (event.getCode()) {
			case KeyEvent::KEY_KP_PLUS:
			case KeyEvent::KEY_TAB:
			case KeyEvent::KEY_f:
				positionRenderWindow();
				break;
			case KeyEvent::KEY_w:
				CI_LOG_V("warp edit mode");
				if (!isModDown) {
					// toggle warp edit mode
					Warp::enableEditMode(!Warp::isEditModeEnabled());
				}
				if (isAltDown) {
					// toggle drawing a random region of the image
				if (mSrcArea.getWidth() != mImage->getWidth() || mSrcArea.getHeight() != mImage->getHeight())
					mSrcArea = mImage->getBounds();
				else {
					int x1 = Rand::randInt(0, mImage->getWidth() - 150);
					int y1 = Rand::randInt(0, mImage->getHeight() - 150);
					int x2 = Rand::randInt(x1 + 150, mImage->getWidth());
					int y2 = Rand::randInt(y1 + 150, mImage->getHeight());
					mSrcArea = Area(x1, y1, x2, y2);
				}
				}
				break;

			case KeyEvent::KEY_c:
				// mouse cursor and ui visibility
				mSDASettings->mCursorVisible = !mSDASettings->mCursorVisible;
				setUIVisibility(mSDASettings->mCursorVisible);
				break;
			}
		}
	}
}
void SDAWarpingHydraApp::keyUp(KeyEvent event)
{
	if (!Warp::handleKeyUp(mWarps, event)) {
		if (!mSDASession->handleKeyUp(event)) {
		}
	}
}

void SDAWarpingHydraApp::draw()
{
	gl::clear(Color::black());
	if (mFadeInDelay) {
		mSDASettings->iAlpha = 0.0f;
		if (getElapsedFrames() > mSDASession->getFadeInDelay()) {
			mFadeInDelay = false;
			timeline().apply(&mSDASettings->iAlpha, 0.0f, 1.0f, 1.5f, EaseInCubic());
		}
	}

	xLeft = mSDASession->getFloatUniformValueByName("iResolutionX") / 3.0f;
	xRight = mSDASettings->mRenderWidth;
	yLeft = 0;
	yRight = mSDASettings->mRenderHeight;
	/*if (mSDASettings->mFlipV) {
		yLeft = yRight;
		yRight = 0;
	}
	if (mSDASettings->mFlipH) {
		xLeft = xRight;
		xRight = 0;
	}
	Rectf rectangle = Rectf(xLeft, yLeft, xRight, yRight);*/
	gl::setMatricesWindow(toPixels(getWindowSize()));
	// iterate over the warps and draw their content
	for (auto &warp : mWarps) {
		//warp->draw(mSDASession->getHydraTexture(), mSrcArea);	
		//warp->draw(mSDASession->getMixetteTexture(), mSrcArea);
		warp->draw(mSDASession->getRenderTexture(), mSrcArea);
	}

	if (mSDASettings->mCursorVisible) {
		gl::ScopedBlendAlpha alpha;
		gl::enableAlphaBlending();
		// original MODE_HYDRA = 5
		gl::draw(mSDASession->getMixetteTexture(), Rectf(0, 0, tWidth, tHeight));
		gl::drawString("Mixette", vec2(toPixels(xLeft), toPixels(tHeight)), Color(1, 1, 1), Font("Verdana", toPixels(16)));
		// flipH MODE_IMAGE = 1
		gl::draw(mSDASession->getRenderTexture(), Rectf(tWidth * 2 + margin, 0, tWidth + margin, tHeight));
		gl::drawString("Render", vec2(toPixels(xLeft + tWidth + margin), toPixels(tHeight)), Color(1, 1, 1), Font("Verdana", toPixels(16)));
		// flipV MODE_MIX = 0
		gl::draw(mSDASession->getMixTexture(), Rectf(0, tHeight * 2 + margin, tWidth, tHeight + margin));
		gl::drawString("Mix", vec2(toPixels(xLeft), toPixels(tHeight * 2 + margin)), Color(1, 1, 1), Font("Verdana", toPixels(16)));
		// show the FBO color texture 
		gl::draw(mSDASession->getHydraTexture(), Rectf(tWidth + margin, tHeight + margin, tWidth * 2 + margin, tHeight * 2 + margin));
		gl::drawString("Hydra", vec2(toPixels(xLeft + tWidth + margin), toPixels(tHeight * 2 + margin)), Color(1, 1, 1), Font("Verdana", toPixels(16)));


		gl::drawString("irx: " + std::to_string(mSDASession->getFloatUniformValueByName("iResolutionX"))
			+ " iry: "+ std::to_string(mSDASession->getFloatUniformValueByName("iResolutionY")) 
			+ " fw: " + std::to_string(mSDASettings->mFboWidth)
			+ " fh: " + std::to_string(mSDASettings->mFboHeight),
			vec2(xLeft, getWindowHeight() - toPixels(30)), Color(1, 1, 1),
			Font("Verdana", toPixels(24)));

	}




	getWindow()->setTitle(mSDASettings->sFps + " fps Hydra");
}

void prepareSettings(App::Settings *settings)
{
	settings->setWindowSize(1280, 720);
	settings->setConsoleWindowEnabled();
}

CINDER_APP(SDAWarpingHydraApp, RendererGl, prepareSettings)

