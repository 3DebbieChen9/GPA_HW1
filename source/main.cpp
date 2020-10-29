#include "../Include/Common.h"


//For GLUT to handle 
#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3
#define MENU_WALK 4
#define MENU_JUMP 5
#define MENU_DEFAULT 6

#define STATE_DEFAULT 1
#define STATE_WALK 2
#define STATE_JUMP 3

//GLUT timer variable
float timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

using namespace glm;
using namespace std;

mat4 view(1.0f);			// V of MVP, viewing matrix
mat4 projection(1.0f);		// P of MVP, projection matrix
mat4 model(1.0f);			// M of MVP, model matrix
vec3 temp = vec3();			// a 3 dimension vector which represents how far did the ladybug should move
float mouseAngle = -45.0f;

GLint um4p;
GLint um4mv;

GLuint program;			// shader program id

struct Shape
{
	GLuint vao;			// vertex array object
	GLuint vbo;			// vertex buffer object

	int materialId;
	int vertexCount;
	GLuint m_texture;
	Shape* parent;

	vec3 center;
	mat4 nodeModel;
};

Shape m_shapes[10];
int m_state = STATE_WALK;

// Load shader file to program
char** loadShaderSource(const char* file)
{
	FILE* fp = fopen(file, "rb");
	fseek(fp, 0, SEEK_END);
	long sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *src = new char[sz + 1];
	fread(src, sizeof(char), sz, fp);
	src[sz] = '\0';
	char **srcp = new char*[1];
	srcp[0] = src;
	return srcp;
}

// Free shader file
void freeShaderSource(char** srcp)
{
	delete srcp[0];
	delete srcp;
}

// Load .obj model
void My_LoadModels()
{
	// use for loop to load multiple .obj model

	const char* objNames[10] = { "body.obj", "rightarm.obj", "leftarm.obj", "rightleg.obj", 
		"leftleg.obj", "head.obj", "righthand.obj", "lefthand.obj", "rightfoot.obj", "leftfoot.obj" };

	for (int i = 0; i < 10; i++) {
		tinyobj::attrib_t attrib;
		vector<tinyobj::shape_t> shapes;
		vector<tinyobj::material_t> materials;
		string warn;
		string err;

		bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objNames[i]);
		if (!warn.empty()) {
			std::cout << warn << endl;
		}
		if (!err.empty()) {
			std::cout << err << endl;
		}
		if (!ret) {
			exit(1);
		}

		vector<float> vertices, texcoords, normals;  // if OBJ preserves vertex order, you can use element array buffer for memory efficiency
		for (int s = 0; s < shapes.size(); ++s) {  // for 'ladybug.obj', there is only one object
			int index_offset = 0;
			for (int f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f) {
				int fv = shapes[s].mesh.num_face_vertices[f];
				for (int v = 0; v < fv; ++v) {
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
					vertices.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
					vertices.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
					vertices.push_back(attrib.vertices[3 * idx.vertex_index + 2]);
					texcoords.push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
					texcoords.push_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
					normals.push_back(attrib.normals[3 * idx.normal_index + 0]);
					normals.push_back(attrib.normals[3 * idx.normal_index + 1]);
					normals.push_back(attrib.normals[3 * idx.normal_index + 2]);
				}
				index_offset += fv;
				m_shapes[i].vertexCount += fv;
			}
		}

		glGenVertexArrays(1, &m_shapes[i].vao);
		glBindVertexArray(m_shapes[i].vao);

		glGenBuffers(1, &m_shapes[i].vbo);
		glBindBuffer(GL_ARRAY_BUFFER, m_shapes[i].vbo);

		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float) + texcoords.size() * sizeof(float) + normals.size() * sizeof(float), NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
		glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), texcoords.size() * sizeof(float), texcoords.data());
		glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float) + texcoords.size() * sizeof(float), normals.size() * sizeof(float), normals.data());

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(vertices.size() * sizeof(float)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(vertices.size() * sizeof(float) + texcoords.size() * sizeof(float)));
		glEnableVertexAttribArray(2);

		shapes.clear();
		shapes.shrink_to_fit();
		materials.clear();
		materials.shrink_to_fit();
		vertices.clear();
		vertices.shrink_to_fit();
		texcoords.clear();
		texcoords.shrink_to_fit();
		normals.clear();
		normals.shrink_to_fit();

		std::cout << "Load " << objNames[i] << " | " << m_shapes[i].vertexCount << " vertices" << endl;

		texture_data tdata = load_img("optimus.png");

		glGenTextures(1, &m_shapes[i].m_texture);
		glBindTexture(GL_TEXTURE_2D, m_shapes[i].m_texture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		delete tdata.data;
	}
	
}

void My_framework() 
{
	m_shapes[0].parent = NULL;
	for (int i = 1; i <= 5; i++) {
		m_shapes[i].parent = &m_shapes[0];
	}
	for (int i = 6; i < 10; i++) {
		m_shapes[i].parent = &m_shapes[i - 5];
	}
	m_shapes[0].center = vec3(0, 0, 0); // body
	m_shapes[1].center = vec3(-0.096832, -4.3374, 3.40726); // rightArm
	m_shapes[2].center = vec3(-0.096832, -4.3374, 3.40726); // leftArm
	m_shapes[3].center = vec3(-0.096832, -1.43804, -5.0228); // rightLeg
	m_shapes[4].center = vec3(-0.096832,  1.30501, -5.0228); // leftLeg
	m_shapes[5].center = vec3(-0.012209, 1.29605, -8.61114); // head
	m_shapes[6].center = vec3(-0.012209, -4.34635, -0.181083); // rightHand
	m_shapes[7].center = vec3(-0.012209, -4.48602, -0.181083); // leftHand
	m_shapes[8].center = vec3(-0.012209, -1.44699, -8.61114); // rightFoot
	m_shapes[9].center = vec3(-0.012209,  1.29605, -8.61114); // leftFoot

	std::cout << "My_framework() DONE" << endl;
}

// OpenGL initialization
void My_Init()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// Create Shader Program
	program = glCreateProgram();

	// Create customize shader by tell openGL specify shader type 
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Load shader file
	char** vertexShaderSource = loadShaderSource("vertex.vs.glsl");
	char** fragmentShaderSource = loadShaderSource("fragment.fs.glsl");

	// Assign content of these shader files to those shaders we created before
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);

	// Free the shader file string(won't be used any more)
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);

	// Compile these shaders
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);

	// Logging
	shaderLog(vertexShader);
	shaderLog(fragmentShader);

	// Assign the program we created before with these shaders
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	// Get the id of inner variable 'um4p' and 'um4mv' in shader programs
	um4p = glGetUniformLocation(program, "um4p");
	um4mv = glGetUniformLocation(program, "um4mv");

	// Tell OpenGL to use this shader program now
	glUseProgram(program);
	My_framework();
	My_LoadModels();
	std::cout << "Initial Done" << endl;
}



mat4 My_Model(int index, int state)
{
	mat4 my_model(1.0f);
	mat4 translation_matrix(1.0f);
	mat4 move_matrix = translate(mat4(1.0f), temp);
	vec3 rotate_axis = vec3();
	mat4 rotation_matrix(1.0f);
	
	switch (state) {
		case STATE_WALK:
		{
			translation_matrix = translate(mat4(1.0f), m_shapes[index].center);
			rotate_axis = vec3(0.0, 0.0, 1.0);
			switch (index) {
				case 0: // body
				{
					rotate_axis = vec3(0.0, 1.0, 0.0);
					rotation_matrix = rotate(mat4(1.0f), radians(mouseAngle), rotate_axis);
				}
					break;
				case 1: // rightArm
				{
					// 0~45, 315~360
					float angle = sin(0.03 * timer_cnt) * 45;
					rotation_matrix = rotate(mat4(1.0f), radians(angle), rotate_axis);
				}
					break;
				case 2: // leftArm
				{	
					// 0~45, 315~360
					float angle = (- sin(0.03 * timer_cnt)) * 45;
					rotation_matrix = rotate(mat4(1.0f), radians(angle), rotate_axis);
				}
					break;
				case 3: // rightLeg
				{	
					// 0~45, 315~360
					float angle = (- sin(0.03 * timer_cnt)) * 30;
					rotation_matrix = rotate(mat4(1.0f), radians(angle), rotate_axis);
				}
					break;
				case 4: // leftLeg
				{	
					// 0~45, 315~360
					float angle = sin(0.03 * timer_cnt) * 30;
					rotation_matrix = rotate(mat4(1.0f), radians(angle), rotate_axis);
				}
					break;
				case 5: // head
				{
					float angle = sin(0.03 * timer_cnt) * 5;
					rotation_matrix = rotate(mat4(1.0f), radians(angle), rotate_axis);
				}
					break;
				case 6: // rightHand
				{
					// 0~45, 315~360
					float angle = sin(0.05 * timer_cnt) * 25;
					rotation_matrix = rotate(mat4(1.0f), radians(angle), rotate_axis);
				}
					break;
				case 7: // leftHand
				{
					float angle = (-sin(0.05 * timer_cnt)) * 25;
					rotation_matrix = rotate(mat4(1.0f), radians(angle), rotate_axis);
				}
					break;
				case 8: // rightFoot
				{
					float angle = (-sin(0.05 * timer_cnt)) * 25;
					rotation_matrix = rotate(mat4(1.0f), radians(angle), rotate_axis);
				}
					break;
				case 9: // leftLeg
				{
					float angle = sin(0.05 * timer_cnt) * 25;
					rotation_matrix = rotate(mat4(1.0f), radians(angle), rotate_axis);
				}
					break;
				default: 
					break;
			}

			if (m_shapes[index].parent != NULL) {
				my_model = m_shapes[index].parent->nodeModel * inverse(translation_matrix) * rotation_matrix * translation_matrix;
			}
			else {
				my_model = move_matrix * translation_matrix * rotation_matrix;
			}
			m_shapes[index].nodeModel = my_model;
		}
			break;
		case STATE_JUMP:
			rotate_axis = vec3(0.0, 1.0, 0.0);
			rotation_matrix = rotate(mat4(1.0f), radians(mouseAngle), rotate_axis);
			my_model = move_matrix * rotation_matrix;
			break;
		default: // _Default
			rotate_axis = vec3(0.0, 1.0, 0.0);
			rotation_matrix = rotate(mat4(1.0f), radians(timer_cnt), rotate_axis);
			my_model = move_matrix * rotation_matrix;
			break;
	}

	return my_model;

}

// GLUT callback. Called to draw the scene.
void My_Display()
{
	// Clear display buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Tell openGL to use the shader program we created before
	glUseProgram(program);


	for (int i = 0; i < 10; i++) {
		// Bind a vertex array for OpenGL (OpenGL will apply operation only to the vertex array objects it bind)
		glBindVertexArray(m_shapes[i].vao);

		// final result should restore in the variable 'model'
		model = My_Model(i, m_state);

		// Transfer value of (view*model) to both shader's inner variable 'um4mv'; 
		glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * model));

		// Transfer value of projection to both shader's inner variable 'um4p';
		glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));

		// Tell openGL to draw the vertex array we had binded before
		glDrawArrays(GL_TRIANGLES, 0, m_shapes[i].vertexCount);
	}

	// Change current display buffer to another one (refresh frame) 
	glutSwapBuffers();
}

// Setting up viewing matrix
void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);

	float viewportAspect = (float)width / (float)height;

	// perspective(fov, aspect_ratio, near_plane_distance, far_plane_distance)
	// ps. fov = field of view, it represent how much range(degree) is this camera could see 
	projection = perspective(radians(60.0f), viewportAspect, 0.1f, 1000.0f);

	// lookAt(camera_position, camera_viewing_vector, up_vector)
	// up_vector represent the vector which define the direction of 'up'
	view = lookAt(vec3(-10.0f, 5.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
}

void My_Timer(int val)
{
	timer_cnt += 1.0f;
	glutPostRedisplay();
	if (timer_enabled)
	{
		glutTimerFunc(timer_speed, My_Timer, val);
	}
}

void My_Keyboard(unsigned char key, int x, int y)
{
	printf("Key %c is pressed at (%d, %d)\n", key, x, y);
	if (key == 'd')
	{
		temp = temp + vec3(0, 0, 1);
	}
	else if (key == 'a')
	{
		temp = temp - vec3(0, 0, 1);
	}
	else if (key == 'w')
	{
		temp = temp + vec3(1, 0, 0);
	}
	else if (key == 's')
	{
		temp = temp - vec3(1, 0, 0);
	}
	else if (key == 'q')
	{
		temp = temp + vec3(0, 1, 0);
	}
	else if (key == 'e')
	{
		temp = temp - vec3(0, 1, 0);
	}
}

void My_Menu(int id)
{
	switch (id)
	{
	case MENU_TIMER_START:
		if (!timer_enabled)
		{
			timer_enabled = true;
			glutTimerFunc(timer_speed, My_Timer, 0);
		}
		break;
	case MENU_TIMER_STOP:
		timer_enabled = false;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	case MENU_WALK:
		// do something
		m_state = STATE_WALK;
		break;
	case MENU_JUMP:
		// do something
		m_state = STATE_JUMP;
		break;
	case MENU_DEFAULT:
		m_state = STATE_DEFAULT;
		break;
	default:
		break;
	}
}

void My_Mouse(int button, int state, int x, int y)
{
	float startAngle = 0.0f;
	if (button == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			printf("Mouse %d is pressed at (%d, %d)\n", button, x, y);
			startAngle = x;
		}
		else if (state == GLUT_UP)
		{
			printf("Mouse %d is released at (%d, %d)\n", button, x, y);
			mouseAngle = x - startAngle;
		}
	}
}


int main(int argc, char *argv[])
{
#ifdef __APPLE__
	// Change working directory to source code path
	chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(600, 600);
	glutCreateWindow("Homework 1: Robot"); // You cannot use OpenGL functions before this line;
								  // The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
	dumpInfo();
	My_Init();

	int menu_main = glutCreateMenu(My_Menu);
	int menu_timer = glutCreateMenu(My_Menu);
	int menu_new = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddSubMenu("Timer", menu_timer);
	glutAddSubMenu("Action", menu_new);
	glutAddMenuEntry("Exit", MENU_EXIT);

	glutSetMenu(menu_timer);
	glutAddMenuEntry("Start", MENU_TIMER_START);
	glutAddMenuEntry("Stop", MENU_TIMER_STOP);

	glutSetMenu(menu_new);						// Tell GLUT to design the menu which id==menu_new now
	glutAddMenuEntry("Walking", MENU_WALK);		// Add submenu "Hello" in "New"(a menu which index is equal to menu_new)
	glutAddMenuEntry("Mario Jump", MENU_JUMP);		// Add submenu "Hello" in "New"(a menu which index is equal to menu_new)
	glutAddMenuEntry("Default", MENU_DEFAULT);

	glutSetMenu(menu_main);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutKeyboardFunc(My_Keyboard);
	glutTimerFunc(timer_speed, My_Timer, 0);
	glutMouseFunc(My_Mouse);

	// Enter main event loop.
	glutMainLoop();

	return 0;
}