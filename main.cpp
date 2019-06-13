#include <stdio.h>
#include <stdlib.h>

// Xlib
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>


Window mkWindow(Display *dpy, Window root, int depth,
		Visual *vi, XColor bgColor, XColor borderColor,
		double alpha, GC *gc = NULL, Window prevWin = 0x0ul){
	Window ret;
	// attributes
	XSetWindowAttributes att;
	att.event_mask = ExposureMask | KeyPressMask;
	
	// Colors
	att.background_pixel = bgColor.pixel;
	att.border_pixel = borderColor.pixel;

	// Window creation
	if(prevWin != 0x0ul){
		int x, y;
		Window child;
		XTranslateCoordinates(dpy, prevWin, root, 0, 0, &x, &y, &child);

		XWindowAttributes winInfo;
		XGetWindowAttributes(dpy, prevWin, &winInfo);

		ret = XCreateWindow(dpy, root, 
				//winInfo.x, y - winInfo.y, winInfo.width, winInfo.height, 0,
				0, 0, winInfo.width, winInfo.height, 0,
				depth, InputOutput, vi, CWBackPixel | CWEventMask, &att);
	}else{
		ret = XCreateWindow(dpy, root, 0, 0, 10, 10, 0,
			depth, InputOutput, vi, CWBackPixel | CWEventMask, &att);
	}

	XSelectInput(dpy, ret, 
			ExposureMask | KeyPressMask | EnterWindowMask | LeaveWindowMask );

	if(gc){
		XGCValues gc_values;
		*gc = XCreateGC(dpy, ret, GCForeground, &gc_values);
	}

	// Opacity settings
	// alpha = 0 means trensparant, 1 means not
	unsigned long opacity = (unsigned long)(0xFFFFFFFFul * alpha);
	Atom XA_NET_WM_WINDOW_OPACITY = XInternAtom(dpy, "_NET_WM_WINDOW_OPACITY", false);
	XChangeProperty(dpy, ret, XA_NET_WM_WINDOW_OPACITY, XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)&opacity, 1L);

	return ret;
}

	

int main(int argc, char *argv[]){
	Display *display;
	Window root;
	Window window;
	Visual *vi;
	XEvent event;
	KeySym key;
	GC gc;
	XColor color[10];
	Colormap cmap;
	int depth;


	char defaultWinName[255] = "";

	char col[8] = "#FFFFFF";
	char *winName = defaultWinName;
	double transparency= 0;
	bool border = false;

	bool transMouse = false;
	
	for(int i = 1; i < argc; i++){
		if(argv[i][0] != '-') continue;
		switch(argv[i][1]){
			case 'c':
				if(!argv[i+1] || argv[i+1][0] == '-') break;
				for(int j = 0; j < 6; j++){
					col[j + 1] = argv[i+1][j];
				}	
				continue;
				break;
			case 'n':
				if(!argv[i+1] || argv[i+1][0] == '-') break;
				winName = argv[i+1];
				continue;
				break;
			case 't':
				transparency = (double)0;
				continue;
				break;
			case 'm':
				// Beta, Impossible to move window
				transMouse = true;
			case 'b':
				//border = true;
				continue;
				break;
			default:
					printf(" Try --help to get info");
				break;
		}
		printf(" Try --help to get info");
	}
	// Initalizing
	display = XOpenDisplay(NULL);
	root = XDefaultRootWindow(display);
	vi = DefaultVisual(display, root);
	depth = DefaultDepth(display, root);

	// Colors
	cmap = DefaultColormap(display, 0);

	XParseColor(display, cmap, col, &color[0]);
	XAllocColor(display, cmap, &color[0]);

	XParseColor(display, cmap, "#FFFFFF", &color[1]);
	XAllocColor(display, cmap, &color[1]);

	window = mkWindow(display, root, depth, vi, color[0], color[1], transparency, &gc);
	
	XStoreName(display, window, winName);
	XMapWindow(display, window);
	XFlush(display);

	bool needChange = false;
	double alphaPrev = 0;
	int lockChange = 0;
	bool resizeMode = false;
	bool isTransparent = !(bool)transparency;

	while(1){
		XNextEvent(display, &event);

		switch(event.type){
			case Expose:
				if(0){	
					XDrawRectangle(display, window, gc, 0, 0, 
						event.xexpose.width - 1, event.xexpose.height - 1);
				}
				break;

			case KeyPress:
				char text[255];
				XLookupString(&event.xkey, text, 255, &key, 0);
				switch(text[0]){
					case 'q':
						XDestroyWindow(display, window);
						XCloseDisplay(display);
						exit(0);
						break;
					case 'r':
						if(0){
							resizeMode = false;
							Window newWin = mkWindow(display, root, depth, vi, 
									color[0], color[1], (double)1, &gc, window);
							// Destroying previous window
							XDestroyWindow(display, window);

							// Creating new window
							window = newWin;
					
							XStoreName(display, window, winName);
							XMapWindow(display, window);
							lockChange = 2;
						}else if(0){
							resizeMode = true;
							Window newWin = mkWindow(display, root, depth, vi, 
									color[0], color[1], (double)1, &gc, window);
							// Destroying previous window
							XDestroyWindow(display, window);

							// Creating new window
							window = newWin;
					
							XStoreName(display, window, winName);
							XMapWindow(display, window);
							lockChange = 2;
						}
						break;
					case 't':
						Window newWin = mkWindow(display, root, depth, vi, 
								color[0], color[1], (double)0, &gc, window);
						// Destroying previous window
						XDestroyWindow(display, window);

						// Creating new window
						window = newWin;
				
						XStoreName(display, window, winName);
						XMapWindow(display, window);
						transparency = 1; 
				}	
				break;

			case EnterNotify:
				if(lockChange == 0 && transparency == 0 && !resizeMode && transMouse){
					Window newWin = mkWindow(display, root, depth, vi, 
							color[0], color[1], (double)1, &gc, window);

					 // Destroying previous window
					XDestroyWindow(display, window);

					// Creating new window
					window = newWin;

					XStoreName(display, window, winName);
					XMapWindow(display, window);

					// Locking the event when the window is closing 
					// and the new window made
					lockChange = 2;
					isTransparent = false;
					break;
				}
				if(lockChange > 0)lockChange --;
				break;

			case LeaveNotify:
				if(lockChange == 0 && transparency == 0 && !resizeMode && transMouse){
					Window newWin = mkWindow(display, root, depth, vi, 
							color[0], color[1], (double)0, &gc, window);
					 // Destroying previous window
					XDestroyWindow(display, window);

					// Creating new window
					window = newWin;
					
					XStoreName(display, window, winName);
					XMapWindow(display, window);
					isTransparent = True;
					break;
				}
				if(lockChange > 0)lockChange --;
			break;

		}
	}
		
	// Cleaning up
	XCloseDisplay(display);
}
