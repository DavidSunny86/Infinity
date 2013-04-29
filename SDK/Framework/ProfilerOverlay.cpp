#include <windows.h>
#include <opengl.h>
#include <vg/vg.h>

#include "ProfilerOverlay.h"
#include "profiler.h"

//HACK: already defined in shader overlay!!!!
void SetAStyle(Editor& ed, int style, Colour fore, Colour back=0xFFFFFFFF, int size=-1, const char *face=0);
//{
//	ed.Command(SCI_STYLESETFORE, style, fore);
//	ed.Command(SCI_STYLESETBACK, style, back);
//	if (size >= 1)
//		ed.Command(SCI_STYLESETSIZE, style, size);
//	if (face) 
//		ed.Command(SCI_STYLESETFONT, style, reinterpret_cast<sptr_t>(face));
//}

void ProfilerOverlay::initialiseView() {
    mProfilerView.Command(SCI_SETSTYLEBITS, 7);

    // Set up the global default style. These attributes are used wherever no explicit choices are made.
    SetAStyle(mProfilerView, STYLE_DEFAULT, 0xFF00FF00, 0xD0000000, 16, "c:/windows/fonts/cour.ttf");
    mProfilerView.Command(SCI_STYLECLEARALL);	// Copies global style to all others
    mProfilerView.Command(SCI_SETMARGINWIDTHN, 1, 0);//Calculate correct width

    mProfilerView.Command(SCI_SETUSETABS, 1);
    mProfilerView.Command(SCI_SETTABWIDTH, 4);

    mProfilerView.Command(SCI_SETSELBACK,            1, 0xD0CC9966);
    mProfilerView.Command(SCI_SETCARETFORE,          0x00FFFFFF, 0);
    mProfilerView.Command(SCI_SETCARETLINEVISIBLE,   1);
    mProfilerView.Command(SCI_SETCARETLINEBACK,      0xFFFFFFFF);
    mProfilerView.Command(SCI_SETCARETLINEBACKALPHA, 0x20);

    mProfilerView.Command(SCI_SETFOCUS, true);
}

void ProfilerOverlay::initialise(int w, int h)
{
    initialiseView();

    mWidth = (float)w; mHeight=(float)h;

    float w1=mWidth-60.0f, h1=mHeight-60.0f;/*60=30+30*/

    mProfilerView.SetSize(w1, h1);
}

void ProfilerOverlay::update()
{
    static const size_t MAX_STACK_DEPTH = 8;

    size_t                  numEvents;
    const profiler_event_t* events;

    profilerGetData(&numEvents, &events);

    char* buf = (char*) malloc(numEvents*100);
    char* line=buf;
    memset(buf, ' ', numEvents*100);

    int     stackTop = -1;
    struct
    {
        size_t  id;
        __int64 sliceBegin;
    } stack[MAX_STACK_DEPTH];

    for (size_t i=0; i<numEvents; ++i)
    {
        profiler_event_t event = events[i];

        if (event.phase==PROF_EVENT_PHASE_BEGIN)
        {
            //Add event to stack
            ++stackTop;
            stack[stackTop].id         = event.id;
            stack[stackTop].sliceBegin = event.timestamp;
        }
        else if (event.phase==PROF_EVENT_PHASE_END)
        {
            //remove item from stack and add entry
            assert(stackTop>=0);
            assert(stackTop<MAX_STACK_DEPTH);

            assert (stack[stackTop].id == event.id); //implement backtracking end fixing later
           
            size_t offset = 44-stackTop*4;
            memcpy(line+stackTop*4, (char*)event.id, min(offset, strlen((char*)event.id)));
            line += 44;
            line += _snprintf(line, 15, "%10.3f\n", (event.timestamp - stack[stackTop].sliceBegin) / 1000.0f);
            *line = ' ';

            --stackTop;
        }
    }

    assert(stackTop==-1);

    *(line!=buf?line-1:line) = 0;

    GLint len = strlen(buf);

    int pos = (int)mProfilerView.Command(SCI_GETCURRENTPOS);
    int lin = (int)mProfilerView.Command(SCI_LINEFROMPOSITION, (WPARAM)pos);

    mProfilerView.Command(SCI_CANCEL);
    mProfilerView.Command(SCI_CLEARALL);
    mProfilerView.Command(SCI_SETUNDOCOLLECTION, 0);
    mProfilerView.Command(SCI_ADDTEXT, len, reinterpret_cast<LPARAM>(buf));
    mProfilerView.Command(SCI_SETUNDOCOLLECTION, 1);
    mProfilerView.Command(SCI_EMPTYUNDOBUFFER);
    mProfilerView.Command(SCI_SETSAVEPOINT);
    mProfilerView.Command(SCI_GOTOLINE, (WPARAM)lin);

    free(buf);
}

void ProfilerOverlay::renderFullscreen()
{
    glUseProgram(0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0, mWidth, 0, mHeight, 0, 500);
    glTranslatef(0, mHeight, 0);
    glScalef(1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CLIP_PLANE0);
    glEnable(GL_CLIP_PLANE1);
    glEnable(GL_CLIP_PLANE2);
    glEnable(GL_CLIP_PLANE3);

    float w1=mWidth-80.0f, h1=mHeight-80.0f;

    glTranslatef(30, 30, 0);
    mProfilerView.Paint();

    glPopAttrib();
}

void ProfilerOverlay::handleKeyDown(SDL_KeyboardEvent& event)
{
    switch (event.keysym.sym)
    {
    case SDLK_UP:
        mProfilerView.Command(SCI_LINEUP);
        break;
    case SDLK_DOWN:
        mProfilerView.Command(SCI_LINEDOWN);
        break;
    }
}
