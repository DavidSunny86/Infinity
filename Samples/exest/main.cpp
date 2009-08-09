#define GLEW_STATIC
#include <framework.h>
#include <glm\glmext.h>

#ifdef _DEBUG
#	include <string>
#	define START_PROFILE(section) sec=section; time_ = glfwGetTime()
#	define END_PROFILE() print("Section %s in %f sec\n", sec.c_str(), glfwGetTime()-time_)
static double time_=0;
static std::string sec;
#else
#	define START_PROFILE(section)
#	define END_PROFILE()
#endif

class FirstPersonCamera1
{
	public:
		glm::mat4	viewMatrix;
		glm::vec3	pos;
	public:
		FirstPersonCamera1()
		{
			viewMatrix = glm::mat4(1,0,0,0,
								   0,1,0,0,
								   0,0,1,0,
								   0,0,0,1);
		}
		void rotateLR(float angle)
		{
			viewMatrix = glm::mat4GTX(glm::angleAxisGTX(-angle, 0.0f, 1.0f, 0.0f)) * viewMatrix;
		}
		void rotateUD(float angle)
		{
			viewMatrix = glm::mat4GTX(glm::angleAxisGTX(angle, /*1.0f, 0.0f, 0.0f)*/glm::vec3(viewMatrix[0]))) * viewMatrix;
		}
		glm::mat4 getViewMatrix()
		{
			return /*glm::inverseGTX(glm::translate3DGTX(viewMatrix, -1.0f*pos));//*/glm::inverseGTX(viewMatrix);
			//return viewMatrix;
		}
		void moveFB(float dist)
		{
			pos+=glm::vec3(viewMatrix[2])*(-dist);
		}
		void moveLR(float dist)
		{
			pos+=glm::vec3(viewMatrix[0])*(-dist);
		}
		void moveUD(float dist)
		{
			pos+=glm::vec3(viewMatrix[1])*(-dist);
		}

		//void move(glm::vec3 vec)
		//{
		//}

		void rotate(float psi, float phi)
		{
			rotateLR(psi);
			rotateUD(phi);
		}
};

#include <DataPtr.h>

#define MESH_INDICES_MAGIC '10im'

VertexDecl SubMesh::decl_[3]=
{
	{USAGE_POSITION, 3, GL_FLOAT, 0, 8*4},
	{USAGE_NORMAL, 3, GL_FLOAT, 3*4, 8*4},
	{USAGE_TEXCOORD0, 2, GL_FLOAT, 6*4, 8*4}
};



MeshPtr loadMesh1(const char* path)
{
	int magic;
	MeshPtr mesh;
	DataStreamPtr stream = vfsMapFile(path);
	if(!stream)
		return mesh;
	
	DataPtr ptr(stream->MapStream(), stream->GetSize());

	ptr.read(magic);
	
	if(magic == MESH_INDICES_MAGIC) 
	{
		int num_surfaces;
		ptr.read(num_surfaces);
		mesh = MeshPtr(new Mesh);
		for(int i = 0; i < num_surfaces; i++) {
			SubMesh *s = new SubMesh();
			
			ptr.move(128);

			ptr.read(s->numVert_);
			s->vert_.setData(8*4*s->numVert_, ptr.move(8*4*s->numVert_));

			ptr.read(s->numInd_);
			s->numInd_*=3;
			s->ind_.setData(s->numInd_*4, ptr.move(s->numInd_*4));
			mesh->push_back(SubMeshPtr(s));
		}
	}
	return mesh;
}

VertexDecl decl = {USAGE_POSITION, 3, GL_FLOAT, 0, 0};

class Exest: public UI::Stage
{
	private:
		//TextureManager	mTexMgr;
		//ShaderManager	mShaderMgr;
		//ProgramManager	mProgMgr;
		
		GLint timeLoc, tex1Loc, tex0Loc;
		glRenderer	renderer_;
		TextureRef	mImage[2];
		ProgramRef	mProg;
		MeshPtr		mesh;
		FirstPersonCamera1	camera;
	public:
		void OnCreate()
		{
			mesh = loadMesh1("sky.mesh");

			mImage[0].create("HorizontTex", "horizon.png");
			mImage[0]->setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
			mImage[1].create("CloudsTex", "clouds.png");

			mProg.create("SkyProg", "sky.vert", "sky.frag");

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluPerspective(90.0, (float)mWidth/mHeight,1,2048);
			glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
			glClearColor(0.0f, 0.0f, 0.0f, 0.5f);				// Black Background
			glClearDepth(1.0f);									// Depth Buffer Setup
			glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
			glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
			glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
			timeLoc = mProg->bindUniform("time");
			tex0Loc = mProg->bindUniform("tex0");
			tex1Loc = mProg->bindUniform("tex1");
			renderer_.useProgram(mProg.get());
			renderer_.setUniform1f(timeLoc, (float)glfwGetTime());
			renderer_.setUniform1i(tex0Loc, 0);
			renderer_.setUniform1i(tex1Loc, 1);
			renderer_.useProgram(0);
			//camera.rotate(180, 0);
			//camera.move(glm::vec3(0, 1, -4)); 
		}
	
	protected:
		void paint()
		{
			glClearDepth(1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			glMatrixMode(GL_MODELVIEW);
			//glLoadIdentity();
			glLoadMatrixf(camera.getViewMatrix());
			glTranslatef(-camera.pos[0], -camera.pos[1], -camera.pos[2]);
			//glTranslatef(0.0f, -1.0f, -4.0f);
			renderer_.begin();
				glDisable(GL_DEPTH_TEST);
				glColor3f(1.0f, 1.0f, 1.0f);
				renderer_.setTexture(GL_TEXTURE0, mImage[0].get());
				renderer_.setTexture(GL_TEXTURE1, mImage[1].get());
				renderer_.useProgram(mProg.get());
				renderer_.setUniform1f(timeLoc, (float)glfwGetTime());
				glPushMatrix();
					//glLoadIdentity();
					glTranslatef(camera.pos[0], camera.pos[1], camera.pos[2]);
					glMultMatrixf(glm::mat4(1,0,0,0,
											0,0,1,0,
											0,1,0,0,
											0,0,0,1));
					glTranslatef(0,0,-400);
					renderer_.addAttribBuffer((*mesh)[0]->vert_, 1, SubMesh::decl_);
					renderer_.setIndexBuffer((*mesh)[0]->ind_, GL_UNSIGNED_INT);
					renderer_.drawPrimitives(GL_TRIANGLES, (*mesh)[0]->numInd_);
				glPopMatrix();
				renderer_.useProgram(0);
				glEnable(GL_DEPTH_TEST);
				glDisable(GL_TEXTURE_2D);
				for(float i = -10; i <= 10; i += 1)
				{
					glBegin(GL_LINES);
						glColor3ub(68, 193, 181);						
						glVertex3f(-10, 0, i);					
						glVertex3f(10, 0, i);
						glVertex3f(i, 0,-10);							
						glVertex3f(i, 0, 10);
					glEnd();
				}
				const float ww=1;
				glBegin(GL_LINES);
					glColor3f(1,0,0);
					glVertex3f(0,0,0);
					glVertex3f(ww,0,0);

					glColor3f(0,1,0);
					glVertex3f(0,0,0);
					glVertex3f(0,ww,0);

					glColor3f(0,0,1);
					glVertex3f(0,0,0);
					glVertex3f(0,0,ww);
				glEnd();
			renderer_.end();
			glFlush();
		}

		void onUpdate(float frame_time)
		{
			if( glfwGetKey(GLFW_KEY_ESC) )
				close();
			if( glfwGetKey(GLFW_KEY_KP_8) )
				camera.rotateUD(0.5);
			if( glfwGetKey(GLFW_KEY_KP_2) )
				camera.rotateUD(-0.5);
			if( glfwGetKey(GLFW_KEY_KP_4) )
				camera.rotateLR(-0.5);
			if( glfwGetKey(GLFW_KEY_KP_6) )
				camera.rotateLR(0.5);
			if( glfwGetKey('W') )
				camera.moveFB(0.1f);
			if( glfwGetKey('S') )
				camera.moveFB(-0.1f);
			if( glfwGetKey('A') )
				camera.moveLR(0.1f);
			if( glfwGetKey('D') )
				camera.moveLR(-0.1f);
			
			static int look = 0;
			
			if(!look && glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT)) 
			{
				glfwDisable(GLFW_MOUSE_CURSOR);
				glfwSetMousePos(mWidth / 2, mHeight/ 2);
				look = 1;
			}
			
			if(glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT)) 
			{
				look = 0;
				glfwEnable(GLFW_MOUSE_CURSOR);
			}

			if(look)
			{
				int mouseX=0, mouseY=0;
				glfwGetMousePos(&mouseX, &mouseY);
				float psi = (mouseX - mWidth / 2) * 0.2f;
				float phi = (mouseY - mHeight / 2) * 0.2f;
				if(phi < -89) phi = -89;
				if(phi > 89) phi = 89;
				camera.rotate(psi, -phi);
				//camera.rotateLR(psi);
				//camera.rotateUD(-phi);
				glfwSetMousePos(mWidth / 2, mHeight / 2);
			} 
		}
};

int main()
{
	Exest app;
	app.run();
	return 0;
}