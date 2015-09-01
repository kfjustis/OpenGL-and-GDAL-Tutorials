
//#define GLM_FORCE_RADIANS
#include <iostream>

#include <AssetManager.h>

#include <SDL2/SDL.h>



#if defined(__APPLE__) || defined(MACOSX)
#include <OpenGL/gl3.h>
#include <OpenGL/GLU.h>

#else //linux as default
#include <GL/glew.h>
#include <GL/glu.h>

#endif

#include <renderer.h>
#include <iostream>
#include <buffer.h>
#include <triangle.h>
#include <gishandler.h>
#include <vector>
#include <camera.h>
#include <chrono>
#include <terrain.h>
#include <gbuffer.h>
#include <framerenderer.h>

#include <projector.h>
#include <plane.h>
//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
using namespace std;
using namespace chrono;


float Vertices[9] = {0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0};

string ErrorString(GLenum error)
{
	if (error == GL_INVALID_ENUM)
	{
		return "GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument.";
	}

	else if (error == GL_INVALID_VALUE)
	{
		return "GL_INVALID_VALUE: A numeric argument is out of range.";
	}

	else if (error == GL_INVALID_OPERATION)
	{
		return "GL_INVALID_OPERATION: The specified operation is not allowed in the current state.";
	}

	else if (error == GL_INVALID_FRAMEBUFFER_OPERATION)
	{
		return "GL_INVALID_FRAMEBUFFER_OPERATION: The framebuffer object is not complete.";
	}

	else if (error == GL_OUT_OF_MEMORY)
	{
		return "GL_OUT_OF_MEMORY: There is not enough memory left to execute the command.";
	}
	else
	{
		return "None";
	}
};

//Starts up SDL, creates window, and initializes OpenGL
bool init();

//Initializes matrices and clear color
bool initGL();

//Input handler
void handleKeys( unsigned char key, int x, int y )
{};

void HandleEvents(SDL_Event e, float dt = 0);

//Per frame update
void update(float dt = 0);

//Renders quad to the screen
void render();

//Frees media and shuts down SDL
void close();

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//OpenGL context
SDL_GLContext gContext;

//Render flag
bool gRenderQuad = true;

camera Camera;

framerenderer fr;
projector pr;
projector pr2;
projector pr3;
projector* currentprojector;

plane Plane;




GLint VaoId;

high_resolution_clock::time_point current;

bool quit;

int main(int argc, char** argv)
{
	GLuint test;

	renderer Renderer = renderer();

	string appPath = argv[0];
	cout << argv[0] << endl;
	appPath.erase(appPath.end() - 3, appPath.end());

	// Lets set the application path for this guy
	AssetManager::SetAppPath(appPath);
	cout << "HERE @" << endl;

	current = high_resolution_clock::now();
	high_resolution_clock::time_point past = high_resolution_clock::now();

	//Start up SDL and create window
	if ( !init() )
	{
		printf( "Failed to initialize!\n" );
	}
	else
	{
		cout << "INITIALIZED" << endl;

		
		Plane.buildPlane(-500,-500,1000,1000);
		//Main loop flag
		quit = false;

		//Event handler
		SDL_Event e;

		//Enable text input
		SDL_StartTextInput();

		//While application is running
		while ( !quit )
		{

			current = high_resolution_clock::now();
			duration<double> time_span = duration_cast<duration<double>>(current - past);
			
			//Handle events on queue
			if (time_span.count() <= 1.0 / 30.0)
			{
				continue;
			}

			while ( SDL_PollEvent( &e ) != 0 )
			{
				HandleEvents(e, time_span.count());
			}



			// Update first
			update();

			// Now render
			render();

			auto error = glGetError();
			if ( error != GL_NO_ERROR )
			{
				cout << "Error initializing OpenGL! " << gluErrorString( error )  << endl;

			}

			//Update screen
			SDL_GL_SwapWindow( gWindow );
			past = current;
		}

		//Disable text input
		SDL_StopTextInput();
	}

	Renderer.cleanup();

	//Free resources and close SDL
	close();

	return 0;
}

bool init()
{

	//Initialization flag
	bool success = true;

	//Initialize SDL
	if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
		printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
		success = false;
	}
	else
	{
		//Use OpenGL 3.3 -- Make sure you have a graphics card that supports 3.3
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );

		//Create window
		gWindow = SDL_CreateWindow( "Tutorial 4", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
		if ( gWindow == NULL )
		{
			printf( "Window could not be created! SDL Error: %s\n", SDL_GetError() );
			success = false;
		}
		else
		{
			//Create context
			gContext = SDL_GL_CreateContext( gWindow );
			if ( gContext == NULL )
			{
				printf( "OpenGL context could not be created! SDL Error: %s\n", SDL_GetError() );
				success = false;
			}
			else
			{
				auto t = glGetError();
				cout << ErrorString(t) << endl;


				//Use Vsync
				if ( SDL_GL_SetSwapInterval( 1 ) < 0 )
				{
					printf( "Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError() );
				}
				t = glGetError();
				cout << ErrorString(t) << endl;

#if !defined(__APPLE__) && !defined(MACOSX)
				cout << glewGetString(GLEW_VERSION) << endl;
				glewExperimental = GL_TRUE;

				auto status = glewInit();

				//Check for error
				if (status != GLEW_OK)
				{
					std::cerr << "GLEW Error: " << glewGetErrorString(status) << "\n";
					success = false;
				}
#endif

				t = glGetError();
				cout << ErrorString(t) << endl;
				//Initialize OpenGL
				if ( !initGL() )
				{
					printf( "OUCH Unable to initialize OpenGL!\n" );
					success = false;
				}

				//cout << glGetString(GL_VERSION) << endl;
				cout << "GUFFER SUCCESS: " << GBuffer::Init(SCREEN_WIDTH, SCREEN_HEIGHT) << endl;


				fr.setup();
				fr.setScreenDims(SCREEN_WIDTH, SCREEN_HEIGHT);
				//fr.setHasProj(-1); // we need to project something

				pr.setFile(AssetManager::GetAppPath() + "../smiley-face.png");
				pr.setup();
				pr.setScreenDims(SCREEN_WIDTH, SCREEN_HEIGHT);
				pr.SetDimensions(500,500);
				pr.SetPosition(0,0);
				pr.setTranslucency(0.0f);

				pr2.setFile(AssetManager::GetAppPath() + "../sadface.png");
				pr2.setup();
				pr2.setScreenDims(SCREEN_WIDTH, SCREEN_HEIGHT);
				pr2.SetDimensions(500,500);
				pr2.SetPosition(0,0);
				pr2.setTranslucency(0.0f);

				pr3.setFile(AssetManager::GetAppPath() + "../angryface.png");
				pr3.setup();
				pr3.setScreenDims(SCREEN_WIDTH, SCREEN_HEIGHT);
				pr3.SetDimensions(500,500);
				pr3.SetPosition(0,0);
				pr3.setTranslucency(1.0f);
				currentprojector = &pr;
			}
		}
	}

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	return success;
}

bool initGL()
{
	GLenum error = GL_NO_ERROR;
	bool success = true;

	//Initialize clear color
	glClearColor( 0.f, 0.f, 0.f, 0.f );


	error = glGetError();
	if ( error != GL_NO_ERROR )
	{
		printf( "Error initializing OpenGL! %s\n", gluErrorString( error ) );
		success = false;
	}

	return success;
}

void update(float dt)
{
	//No per frame update needed
	Camera.update();
}

void render()
{
	fr.SetCameraPos(Camera.getPos());
	glm::mat4 view = Camera.getView();
	glm::mat4 projection = Camera.getProjection();
    glClearColor( 0.f, 0.f, 0.0f, 0.f );
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	fr.render(view, projection);
    
	GBuffer::BindForWriting();
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor( 0.f, 0.f, 0.0f, 0.f );

	

    Plane.render(view,projection);
    
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
        //glBlendFunc(GL_ONE_MINUS_DST_ALPHA,GL_DST_ALPHA);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE);
	pr.render(view,projection);
	pr2.render(view,projection);
	pr3.render(view,projection);
	glDisable(GL_BLEND);
	GBuffer::DefaultBuffer();

	return;
}

void close()
{
	//Destroy window
	SDL_DestroyWindow( gWindow );
	gWindow = NULL;

	//Quit SDL subsystems
	SDL_Quit();
}

void HandleEvents(SDL_Event e, float dt)
{
	//User requests quit
	if ( e.type == SDL_QUIT )
	{
		quit = true;
	}
	else if (e.type == SDL_KEYDOWN)
	{
		// handle key down events here
		if (e.key.keysym.sym == SDLK_ESCAPE)
		{
			quit = true;
		}

		// rotate camera left
		if (e.key.keysym.sym == SDLK_q)
		{
			Camera.rotateX(1 * dt);
		}

		// rotate camera right
		if (e.key.keysym.sym == SDLK_e)
		{
			Camera.rotateX(-1 * dt);
		}

		// Move left
		if (e.key.keysym.sym == SDLK_a)
		{
			Camera.strafe(10 * dt);
		}
		// move back
		if (e.key.keysym.sym == SDLK_s)
		{
			Camera.translate(-10 * dt);
		}

		// move right
		if (e.key.keysym.sym == SDLK_d)
		{
			Camera.strafe(-10 * dt);
		}

		// move forward
		if (e.key.keysym.sym == SDLK_w)
		{
			Camera.translate(10 * dt);
		}

		if (e.key.keysym.sym == SDLK_y)
		{
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		if (e.key.keysym.sym == SDLK_u)
		{
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
		if (e.key.keysym.sym == SDLK_z)
		{
			Camera.rotateY(-1 * dt);
		}
		if (e.key.keysym.sym == SDLK_x)
		{
			Camera.rotateY(1 * dt);
		}
		if (e.key.keysym.sym == SDLK_r)
		{
			Camera.flight(1 * dt);
		}
		if (e.key.keysym.sym == SDLK_f)
		{
			Camera.flight(-1 * dt);
		}
		if(e.key.keysym.sym == SDLK_1)
		{
			currentprojector = &pr;
		}
		if(e.key.keysym.sym == SDLK_2)
		{
			currentprojector = &pr2;
		}
		if(e.key.keysym.sym == SDLK_3)
		{
			currentprojector = &pr3;
		}
		if(e.key.keysym.sym == SDLK_UP)
		{
			currentprojector->incTranslucency(.05);
		}
		if(e.key.keysym.sym == SDLK_DOWN)
		{
			currentprojector->decTranslucency(.05);
		}
	}
	else if (e.type == SDL_KEYUP)
	{
		if (e.key.keysym.sym == SDLK_w)
		{
			cout << "W RELEASED" << endl;
			Camera.resetVerticalSpeed();
		}
		else if (e.key.keysym.sym == SDLK_s)
		{
			cout << "S RELEASED" << endl;
			Camera.resetVerticalSpeed();
		}
		if (e.key.keysym.sym == SDLK_a || e.key.keysym.sym == SDLK_d)
		{
			Camera.resetHorizontalSpeed();
		}
		if (e.key.keysym.sym == SDLK_q || e.key.keysym.sym == SDLK_e)
		{
			// Reset Horizontal rotation
			Camera.resetHorizontalRotation();
		}
		if (e.key.keysym.sym == SDLK_z || e.key.keysym.sym == SDLK_x)
		{
			Camera.resetVerticalRotation();
		}
		if (e.key.keysym.sym == SDLK_r || e.key.keysym.sym == SDLK_f)
		{
			Camera.resetFlightSpeed();
		}
	}
}