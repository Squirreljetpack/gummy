/**
* gummy
* Copyright (C) 2023  Francesco Fusco
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdexcept>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

struct XLib
{
    class shared_image
	{
	    XImage *img;
		XShmSegmentInfo info;
	public:
		shared_image(XLib &xlib, unsigned int width, unsigned int height);
		~shared_image();
		void update(XLib &xlib);

		char* data();
		size_t length();
	};

	Display *dsp;
	XLib();
	~XLib();
};


inline XLib::XLib()
{
	if (XInitThreads() == 0) {
		throw std::runtime_error("XInitThreads fail");
	}

	if (!(dsp = XOpenDisplay(nullptr))) {
		throw std::runtime_error("XOpenDisplay fail");
	}
};

inline XLib::~XLib()
{
	XCloseDisplay(dsp);
}

inline XLib::shared_image::shared_image(XLib &xlib, unsigned int width, unsigned int height)
{
	info.shmid    = shmget(IPC_PRIVATE, width * height * 4, IPC_CREAT | 0600);
	info.shmaddr  = reinterpret_cast<char*>(shmat(info.shmid, nullptr, 0));
	info.readOnly = False;
	XShmAttach(xlib.dsp, &info);

	img = XShmCreateImage(
	       xlib.dsp,
	       DefaultVisual(xlib.dsp, DefaultScreen(xlib.dsp)),
	       DefaultDepth(xlib.dsp, DefaultScreen(xlib.dsp)),
	       ZPixmap,
	       info.shmaddr,
	       &info,
	       width,
	       height);
}

inline void XLib::shared_image::update(XLib &xlib)
{
	XShmGetImage(
	    xlib.dsp,
	    DefaultRootWindow(xlib.dsp),
	    img,
	    0,
	    0,
	    AllPlanes);
}

inline char* XLib::shared_image::data()
{
	return img->data;
}

inline size_t XLib::shared_image::length()
{
	return img->bytes_per_line * img->height;
}

inline XLib::shared_image::~shared_image()
{
	shmdt(info.shmaddr);
	shmctl(info.shmid, IPC_RMID, 0);
	XDestroyImage(img);
}


