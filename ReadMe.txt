To begin, you must have GLEW, GLFW, OpenGL, GLM, SOIL, and Assimp downloaded. To do so you can run any of these in your command line interpreter:
Update your package list:
sudo apt update
sudo apt upgrade

GLEW:
sudo apt install libglew-dev
GLFW:
sudo apt install libglfw3-dev
OpenGL:
sudo apt install mesa-common-dev
GLM:
sudo apt install libglm-dev
SOIL:
sudo apt install libsoil-dev
Assimp:
sudo apt install libassimp-dev

To run this code you must download the Shader.h, Camera.h, Project5.cpp, Project5.vs, and Project5.frag. Then download the Textures folder and the towel.obj and ensure they are in the same directory as the rest of the code. Once each of these are downloaded you should be able to run it.

Once it runs, a string of textures and objects loading should appear in the terminal and our scene should appear. You will see a television stand, a Wii game console, two stacks of Wii games, a Wii sensor bar, a green towel, a television, and reflections within that television.

Line to Run: 
g++ Project5.cpp -o Main -lGL -lGLEW -lGLU -lglfw -lSOIL -lassimp

Then upon compilation:
./Main