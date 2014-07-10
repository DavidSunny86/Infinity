#define _CRT_SECURE_NO_WARNINGS

#include <framework.h>
#include <utils.h>
#include <SpectatorCamera.h>

void task(void*)
{
    PROFILER_CPU_TIMESLICE("TaskInWorker");

    size_t n=100000;
    while (n--);
}

class SchedulerTest: public ui::Stage
{
private:
    SpectatorCamera     camera;
    v128                mProj[4];

public:
    SchedulerTest()
    {
        ml::make_perspective_mat4(mProj, 30.0f * FLT_DEG_TO_RAD_SCALE, mWidth/mHeight, 0.1f, 10000.0f);
    }

    ~SchedulerTest()
    {
    }

protected:
    void onPaint()
    {
        GLenum err;

        glClearDepth(1.0);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadMatrixf((float*)mProj);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();

        v128 vm[4];
        camera.getViewMatrix(vm);
        glLoadMatrixf((float*)vm);

        glDisable(GL_CULL_FACE);

        glUseProgram(0);
        glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0);
        glVertex3f(-3, -1, -10);
        glColor3f(0, 1, 0);
        glVertex3f( 3, -1, -10);
        glColor3f(0, 0, 1);
        glVertex3f( 0, 4, -10);
        glEnd();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

        glDisable(GL_BLEND);

        ui::displayStats(10.0f, 10.0f, 300.0f, 70.0f, 0.0f, 0.0f);
    }

    void onUpdate(float dt)
    {
        mt::addAsyncTask(task, NULL);
    }
};

int main(int argc, char** argv)
{
    fwk::init(argv[0]);

    {
        SchedulerTest app;
        app.run();
    }

    fwk::fini();

    return 0;
}