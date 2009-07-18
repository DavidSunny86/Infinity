#define GLEW_STATIC
#include <windows.h>
#include <iniparser.h>
#include <cstdlib>
#include <gl\glee.h>
#include "UI.h"
#include "Framework.h"
#include <SOIL.h> // remove later

namespace UI
{
	void Stage::loadConfig(const char* cfgFile)
	{
		dictionary * dict;
		
		dict = iniparser_load(cfgFile);
		
		mWidth  = (float)iniparser_getint(dict, "Video:Width", 800);
		mHeight = (float)iniparser_getint(dict, "Video:Height", 600);
		
		mBitsPerPixel = iniparser_getint(dict, "Video:BitsPerPixel", 32);
		mDepthBits    = iniparser_getint(dict, "Video:DepthBits", 24);
		mStencilBits  = iniparser_getint(dict, "Video:StencilBits", 8);
		
		mFullscreen = iniparser_getboolean(dict, "Video:Fullscreen", false) != 0;
		mVSync      = iniparser_getboolean(dict, "Video:VSync", false) != 0;
		
		strcpy(mPaths, iniparser_getstring(dict, "General:Paths", ""));
		
		iniparser_freedict(dict);
	}

	Stage::Stage():
		mFocused(0), mDoAllocate(true),
		mLastVisited(0), mPhase(PHASE_DEFAULT),
		mInputTicker(50)
	{
		mRunning = false;

		AllocConsole();

		//vfsInit();
		glfwInit();

		loadConfig("infinity.cfg");

		logMessage("Resources paths initializing...");
		char* path=strtok(mPaths, ";");
		while( path )
		{
			if( strstr(path, ".zip" ) )
				//vfsAddZip(path);
				VFS::mount(path);
			else
				VFS::mount(path);
				//vfsAddRoot(path);
			path = strtok(0, ";");
		}

		if (!mVSync) glfwOpenWindowHint(GLFW_REFRESH_RATE, 0);

		if (!glfwOpenWindow((int)mWidth, (int)mHeight, 
							8,8,8,8,//0, 0, 0, 0, 
							mDepthBits, mStencilBits, 
							mFullscreen ? GLFW_FULLSCREEN : GLFW_WINDOW))
		{
			glfwTerminate();
			return;
		}
		
		glfwSetWindowCloseCallback(closeCallback);
		glfwSetKeyCallback(keyCallback);
		glfwSetCharCallback(charCallback);

//		glewInit();

		mVG.create();
		mVG.setSize((GLuint)mWidth, (GLuint)mHeight);

		mTimer.start();
		mInputTicker.onFrame.connect(this, &Stage::handleInput);
		mInputTicker.start();

		//setPos(-mWidth/2, -mHeight/2); //Fix if center of coords is in center of screen
	}

	Stage::~Stage()
	{
		mTimer.stop();
		glfwTerminate();
		//vfsTerminate();
	}

	int GLFWCALL Stage::closeCallback()
	{
		Stage::getRef().close();
		return GL_FALSE;
	}

	Stage& Stage::queueRelayout()
	{
		if (mPhase != PHASE_ALLOCATE) {mDoAllocate = true;}	return *this;
	}
	
	Stage& Stage::captureFocus(Actor* focused)
	{
		if (mFocused != focused)
		{
			if (mFocused) mFocused->onFocusOut();
			mFocused = focused;
			if (mFocused) mFocused->onFocusIn();
		}
		return *this;
	}

	Stage& Stage::releaseFocus()
	{
		if (mFocused) mFocused->onFocusOut();
		mFocused = 0; return *this;
	}

	void GLFWCALL Stage::charCallback(int unicode, int action)
	{
		assert(action == InputState::Pressed || action == InputState::Released);
		assert(!Stage::getRef().mKeyEvents.empty());
		assert(Stage::getRef().mKeyEvents.back().action == action);

		Stage::getRef().mKeyEvents.back().event.unicode = unicode;
	}

	void GLFWCALL Stage::keyCallback(int key, int action)
	{
		assert(action == InputState::Pressed || action == InputState::Released);

		KeyEvent		event  = {key, Stage::getRef().mModifiers, 0};
		KeyQueuedEvent	qevent = {event, action};
		
		//qevent.event  = event;
		//qevent.action = action;
		
		Stage::getRef().mKeyEvents.push(qevent);

		//if (Stage::getRef().mFocused)
		//{
		//	if (action == GLFW_PRESS)
		//		Stage::getRef().mFocused->onKeyDown(event/*(u32)key*/);
		//	else if (action == GLFW_RELEASE)
		//		Stage::getRef().mFocused->onKeyUp(event/*(u32)key*/);
		//}
	}

	void Stage::handleKeybdEvents()
	{
		static bool firstTime = true;
		if (firstTime)
		{
			mAutoRepeat = false;
			firstTime = false;
		}

		while (!mKeyEvents.empty())
		{
			KeyEvent&	event = mKeyEvents.front().event;
			if (mFocused)
			{
				if (mKeyEvents.front().action == InputState::Pressed)
				{
					mAutoRepeat = true;
					mAutoRepeatTime = mTimer.getTime() + 500;
					mEvtKeyAutoRepeat = event;
				}
				else if (mKeyEvents.front().action == InputState::Released)
				{
					if (mAutoRepeat && mEvtKeyAutoRepeat.key == mKeyEvents.front().event.key)
						mAutoRepeat = false;
				}

				if (mKeyEvents.front().action == InputState::Pressed)
					mFocused->onKeyDown(event);
				else if (mKeyEvents.front().action == InputState::Released)
					mFocused->onKeyUp(event);
			}

			mKeyEvents.pop();
		}

		if (mAutoRepeat && mFocused && mTimer.getTime() >= mAutoRepeatTime)
		{
			//mFocused->onKeyUp(mEvtKeyAutoRepeat);
			mFocused->onKeyDown(mEvtKeyAutoRepeat);
			mAutoRepeatTime += 70;
		}
	}

	void Stage::handleMouseEvents()
	{
		int tx, ty;

		glfwGetMousePos(&tx, &ty);
		
		//Quick fix for strange behaviour of GLFW
		tx = glm::clamp<int>(tx, 0, (int)mWidth-1);
		ty = glm::clamp<int>(ty, 0, (int)mHeight-1);

		Actor* actor = doPick(tx, ty);

		int modState;
		float x = (float)tx, y = (float)ty;

		static bool firstTime = true;
		if (firstTime)
		{
			memset(mPrevButtonState, 0, sizeof(mPrevButtonState));
			mPrevX = x;
			mPrevY = y;

			firstTime = false;
		}

		//perform picking rendering

		mModifiers = 0;

		modState = glfwGetKey(KeySyms::KeyLCtrl)  | glfwGetKey(KeySyms::KeyRCtrl);
		if (modState == InputState::Pressed)
			mModifiers |= Modifiers::Ctrl;

		modState = glfwGetKey(KeySyms::KeyLAlt)   | glfwGetKey(KeySyms::KeyRAlt);
		if (modState == InputState::Pressed)
			mModifiers |= Modifiers::Alt;

		modState = glfwGetKey(KeySyms::KeyLShift) | glfwGetKey(KeySyms::KeyRShift);
		if (modState == InputState::Pressed)
			mModifiers |= Modifiers::Shift;

		for (int button = 0; button < MouseButtons::Count; ++button)
		{
			int& prevState = mPrevButtonState[button];
			int  curState  = glfwGetMouseButton(button);
			
			if (prevState != curState)
			{
				ButtonEvent event = {button, mModifiers, x, y};
				
				if (actor)
				{
					if (curState == InputState::Pressed)
						actor->onTouch(event);
					else
						actor->onUntouch(event);
				}
			}

			prevState = curState;
		}

		if (x!=mPrevX || y!=mPrevY)
		{
			MotionEvent evtMotion = {mModifiers, x, y};

			if (mLastVisited != actor)
			{
				if (mLastVisited)
					mLastVisited->onLeave();

				if (actor)
				{
					actor->onEnter();
					for (int button = 0; button < MouseButtons::Count; ++button)
					{
						int	curState = mPrevButtonState[button];

						ButtonEvent evtTouch = {button, mModifiers, x, y};

						if (curState == InputState::Pressed)
							actor->onTouch(evtTouch);
					}
				}
			}

			if (actor)
				actor->onMotion(evtMotion);

			mLastVisited = actor;
		}
	}

	void Stage::handleInput(u32	frame)
	{
		handleKeybdEvents();
		handleMouseEvents();
	}

	void Stage::close()
	{
		mRunning = false;
	}

	static const u32 rMaskUsed = 8;
	static const u32 gMaskUsed = 8;
	static const u32 bMaskUsed = 8;

	static const u32 rMask = 7;
	static const u32 gMask = 7;
	static const u32 bMask = 7;

	/*code from clutter!!!!!!!!!!!!!!!!!!!!LGPL????????????????????????*/
	Color idToColor(u32 id)
	{
		Color color;
		u32 red, green, blue;

		/* compute the numbers we'll store in the components */
		red   = (id >> (gMaskUsed+bMaskUsed)) & (0xff >> (8-rMaskUsed));
		green = (id >> bMaskUsed) & (0xff >> (8-gMaskUsed));
		blue  = (id) & (0xff >> (8-bMaskUsed));

		/* shift left bits a bit and add one, this circumvents
		* at least some potential rounding errors in GL/GLES
		* driver / hw implementation. 
		*/
		if (rMaskUsed != rMask)
		red = red * 2 + 1;
		if (gMaskUsed != gMask)
		green = green * 2 + 1;
		if (bMaskUsed != bMask)
		blue  = blue  * 2 + 1;

		/* shift up to be full 8bit values */ 
		red   = red   << (8 - rMask);
		green = green << (8 - gMask);
		blue  = blue  << (8 - bMask);

		color.red   = (u8)red;
		color.green = (u8)green;
		color.blue  = (u8)blue;
		color.alpha = 0xff;

		return color;
	}

	u32 pixelToId (u8 pixel[4])                 
	{
		u8  red, green, blue;

		/* reduce the pixel components to the number of bits actually used of the
		* 8bits.
		*/
		red   = pixel[0] >> (8 - rMask);
		green = pixel[1] >> (8 - gMask);
		blue  = pixel[2] >> (8 - bMask);

		/* divide potentially by two if 'fuzzy' */
		red   = red   >> (rMaskUsed - rMask);
		green = green >> (gMaskUsed - gMask);
		blue  = blue  >> (bMaskUsed - bMask);  

		/* combine the correct per component values into the final id */
		u32 id =  blue + (green << bMaskUsed) + (red << (bMaskUsed + gMaskUsed));

		return id;
	} 
	
	Actor* Stage::doPick(u32 x, u32 y)
	{
		mVG.begin();

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClearDepth(1.0f);
		glClearStencil(0);
		glDepthMask(GL_TRUE);
		glStencilMask( 0xFF );
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glUseProgram(0);
		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);

		for(u32 i = 0; i < mRenderQueue.size(); ++i)
		{
			glPushMatrix();
			glMultMatrixf(mRenderQueue[i].transform);
			mRenderQueue[i].actor->onPick(idToColor(i));
			glPopMatrix();
		}

		mVG.end();
		glFinish();

		GLint	viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		
		/* Read the color of the screen co-ords pixel */
		u8	pixel[4];
		glReadPixels (x, viewport[3]-y-1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
		
		//u8* p = new u8[mWidth*mHeight*4];
		//glReadPixels (0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, p);

		//SOIL_save_image("pick.bmp", SOIL_SAVE_TYPE_BMP, mWidth, mHeight, 4, p);		

		//delete [] p;

		if (pixel[0] == 0xff && pixel[1] == 0xff && pixel[2] == 0xff)
			return this;

		u32 id = pixelToId(pixel);
		assert(id < mRenderQueue.size());
		return mRenderQueue[id].actor;
	}

	void Stage::renderActors()
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		//glClearDepth(1.0);
		glClear(GL_COLOR_BUFFER_BIT/* | GL_DEPTH_BUFFER_BIT*/);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);
	
		mVG.begin();
	
		onPaint(mVG);

		std::vector<RenderItem>::iterator	it  = mRenderQueue.begin(),
											end = mRenderQueue.end();

		for (; it != end; ++it)
		{
			glPushMatrix();
			glMultMatrixf((*it).transform);
			(*it).actor->onPaint(mVG);
			//(*it).actor->onPick(idToColor(it - mRenderQueue.begin()));
			glPopMatrix();
		}

		mVG.end();

		glFlush();
	}

	void Stage::updateRenderQueue(Container* container, const glm::mat4& parentTransform)
	{
		ChildsVector::iterator	it  = container->mChilds.begin(),
								end = container->mChilds.end();
		for (; it != end; ++it)
		{
			Actor* child = *it;

			if (child->isVisible())
			{
				glm::mat4	actorTransform = parentTransform*child->getTransformMatrix();
				RenderItem	item = {actorTransform, child};

				mRenderQueue.push_back(item);

				Container* childAsContainer = dynamic_cast<Container*>(child);
				if (childAsContainer)
				{
					updateRenderQueue(childAsContainer, actorTransform);
				}
			}
		}
	}

	void Stage::run()
	{
		mRunning = true;

		mTimer.resetFrame();
		while (mRunning && glfwGetWindowParam(GLFW_OPENED))
		{
			mPhase = PHASE_EVENT_HANDLING;
			mScheduler.mainLoop();

			mTimer.resetFrame();
			
			if (mDoAllocate)
			{
				mPhase = PHASE_ALLOCATE;
				mDoAllocate = false;
				onAllocate();

				mRenderQueue.clear();
				updateRenderQueue(this, getTransformMatrix());
			}
			
			mPhase = PHASE_RENDERING;

			renderActors();

			glFlush();
			glfwSwapBuffers();
		}
	}
}